#include "LuaScripting.h"

NAMESPACE_UPP

// Template specializations for type conversion
template<>
void LuaBinder::PushValue<int>(lua_State* L, const int& value) {
    lua_pushinteger(L, value);
}

template<>
void LuaBinder::PushValue<double>(lua_State* L, const double& value) {
    lua_pushnumber(L, value);
}

template<>
void LuaBinder::PushValue<bool>(lua_State* L, const bool& value) {
    lua_pushboolean(L, value);
}

template<>
void LuaBinder::PushValue<String>(lua_State* L, const String& value) {
    lua_pushstring(L, value);
}

template<>
int LuaBinder::GetValue<int>(lua_State* L, int index) {
    return lua_tointeger(L, index);
}

template<>
double LuaBinder::GetValue<double>(lua_State* L, int index) {
    return lua_tonumber(L, index);
}

template<>
bool LuaBinder::GetValue<bool>(lua_State* L, int index) {
    return lua_toboolean(L, index) != 0;
}

template<>
String LuaBinder::GetValue<String>(lua_State* L, int index) {
    const char* str = lua_tostring(L, index);
    return str ? String(str) : String();
}

int LuaBinder::MakeRef(lua_State* L, int index) {
    return luaL_ref(L, LUA_REGISTRYINDEX);
}

void LuaBinder::UnRef(lua_State* L, int ref) {
    luaL_unref(L, LUA_REGISTRYINDEX, ref);
}

void LuaBinder::GetFromRef(lua_State* L, int ref) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
}

// LuaScriptState implementation
LuaScriptState::LuaScriptState() : L(nullptr), initialized(false), sandboxed(false) {
}

LuaScriptState::~LuaScriptState() {
    if (L) {
        lua_close(L);
    }
}

bool LuaScriptState::Initialize() {
    L = luaL_newstate();
    if (!L) {
        return false;
    }
    
    luaL_openlibs(L);
    
    if (sandboxed) {
        ApplySandbox();
    }
    
    initialized = true;
    return true;
}

bool LuaScriptState::ExecuteString(const String& code) {
    if (!initialized) return false;
    
    int result = luaL_loadstring(L, ~code) || lua_pcall(L, 0, 0, 0);
    if (result) {
        const char* error = lua_tostring(L, -1);
        LOG("Lua error: " << error);
        lua_pop(L, 1);
        return false;
    }
    return true;
}

bool LuaScriptState::ExecuteFile(const String& filename) {
    if (!initialized) return false;
    
    int result = luaL_loadfile(L, ~filename) || lua_pcall(L, 0, 0, 0);
    if (result) {
        const char* error = lua_tostring(L, -1);
        LOG("Lua error: " << error);
        lua_pop(L, 1);
        return false;
    }
    return true;
}

void LuaScriptState::Reset() {
    if (L) {
        lua_close(L);
    }
    L = luaL_newstate();
    luaL_openlibs(L);
    initialized = true;
    
    if (sandboxed) {
        ApplySandbox();
    }
}

void LuaScriptState::ApplySandbox() {
    // Limit potentially dangerous functions
    lua_pushnil(L);
    lua_setglobal(L, "dofile");
    lua_pushnil(L);
    lua_setglobal(L, "loadfile");
    lua_pushnil(L);
    lua_setglobal(L, "require");
}

// LuaScriptManager implementation
LuaScriptManager::LuaScriptManager() : initialized(false), hotReloadEnabled(false) {
}

LuaScriptManager::~LuaScriptManager() {
}

bool LuaScriptManager::Initialize() {
    defaultState = CreateScriptState();
    initialized = defaultState != nullptr;
    return initialized;
}

std::shared_ptr<LuaScriptState> LuaScriptManager::CreateScriptState() {
    auto state = std::make_shared<LuaScriptState>();
    if (state->Initialize()) {
        scriptStates.Add(state);
        return state;
    }
    return nullptr;
}

std::shared_ptr<LuaScriptState> LuaScriptManager::GetDefaultState() {
    return defaultState;
}

bool LuaScriptManager::ExecuteScript(const String& code) {
    return defaultState ? defaultState->ExecuteString(code) : false;
}

bool LuaScriptManager::ExecuteScriptFile(const String& filename) {
    return defaultState ? defaultState->ExecuteFile(filename) : false;
}

void LuaScriptManager::RegisterCommonBindings() {
    if (!defaultState) return;
    
    lua_State* L = defaultState->GetState();
    
    // Register some basic functions
    lua_register(L, "log", [](lua_State* L) -> int {
        const char* msg = lua_tostring(L, 1);
        LOG(msg);
        return 0;
    });
    
    // Register print function
    lua_register(L, "print", [](lua_State* L) -> int {
        int n = lua_gettop(L);
        String result;
        for (int i = 1; i <= n; i++) {
            size_t len;
            const char* s = lua_tolstring(L, i, &len);
            if (i > 1) result += "\t";
            result += String(s, s + len);
        }
        LOG(result);
        return 0;
    });
}

void LuaScriptManager::Update() {
    if (hotReloadEnabled) {
        // Check for script file changes and reload if necessary
        // This is a simplified version - in practice, you'd want to check actual file timestamps
    }
}

// LuaScriptableObject implementation
LuaScriptableObject::LuaScriptableObject() {
}

void LuaScriptableObject::AttachScript(const String& scriptName) {
    attachedScriptName = scriptName;
    
    // Create a new script state for this object, or use a shared one
    scriptState = std::make_shared<LuaScriptState>();
    scriptState->Initialize();
    
    // Execute the script
    scriptState->ExecuteFile(scriptName);
}

void LuaScriptableObject::Update(float deltaTime) {
    if (scriptState) {
        // Call update function in the script
        lua_State* L = scriptState->GetState();
        
        lua_getglobal(L, "update");
        if (lua_isfunction(L, -1)) {
            lua_pushnumber(L, deltaTime);
            if (lua_pcall(L, 1, 0, 0) != 0) {
                const char* error = lua_tostring(L, -1);
                LOG("Lua update error: " << error);
                lua_pop(L, 1);
            }
        } else {
            lua_pop(L, 1); // Remove non-function value
        }
    }
}

bool LuaScriptableObject::ExecuteFunction(const String& funcName) {
    if (!scriptState) return false;
    
    lua_State* L = scriptState->GetState();
    
    lua_getglobal(L, ~funcName);
    if (lua_isfunction(L, -1)) {
        if (lua_pcall(L, 0, 0, 0) != 0) {
            const char* error = lua_tostring(L, -1);
            LOG("Lua function error: " << error);
            lua_pop(L, 1);
            return false;
        }
        return true;
    } else {
        lua_pop(L, 1); // Remove non-function value
        return false;
    }
}

END_UPP_NAMESPACE