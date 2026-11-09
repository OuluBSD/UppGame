#ifndef UPP_LUA_SCRIPTING_H
#define UPP_LUA_SCRIPTING_H

extern "C" {
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

#include <Core/Core.h>
#include <GameLib/GameLib.h>
#include <GameEngine/GameEngine.h>
#include <Vector/Vector.h>
#include <functional>

NAMESPACE_UPP

// Lua script binding utilities
class LuaBinder {
public:
    // Register a C++ function with Lua
    template<typename Func>
    static void RegisterFunction(lua_State* L, const char* name, Func&& func);
    
    // Register a C++ class with Lua
    template<typename Class, typename... Args>
    static void RegisterClass(lua_State* L, const char* className, Args&&... args);
    
    // Register a C++ object instance with Lua
    template<typename Class>
    static void RegisterObject(lua_State* L, const char* name, Class* obj);
    
    // Push a C++ value to the Lua stack
    template<typename T>
    static void PushValue(lua_State* L, const T& value);
    
    // Get a value from the Lua stack
    template<typename T>
    static T GetValue(lua_State* L, int index);
    
    // Helper to get a safe reference to a Lua object
    static int MakeRef(lua_State* L, int index);
    static void UnRef(lua_State* L, int ref);
    static void GetFromRef(lua_State* L, int ref);
};

// Lua script state manager
class LuaScriptState {
public:
    LuaScriptState();
    ~LuaScriptState();
    
    // Initialize the Lua state
    bool Initialize();
    
    // Load and execute a Lua script from a string
    bool ExecuteString(const String& code);
    
    // Load and execute a Lua script from a file
    bool ExecuteFile(const String& filename);
    
    // Call a Lua function by name with arguments
    template<typename... Args>
    bool CallFunction(const String& funcName, Args&&... args);
    
    // Get a value from Lua global space
    template<typename T>
    T GetGlobalValue(const String& name);
    
    // Set a value in Lua global space
    template<typename T>
    void SetGlobalValue(const String& name, const T& value);
    
    // Register a function in the Lua global space
    template<typename Func>
    void RegisterFunction(const String& name, Func&& func);
    
    // Get the raw Lua state (use with caution)
    lua_State* GetState() { return L; }
    
    // Reset the Lua state
    void Reset();
    
    // Enable/disable sandboxing
    void SetSandboxed(bool sandboxed) { this->sandboxed = sandboxed; }
    bool IsSandboxed() const { return sandboxed; }
    
private:
    lua_State* L;
    bool initialized;
    bool sandboxed;
    
    // Apply sandbox restrictions
    void ApplySandbox();
};

// LuaScriptManager - manages multiple script states
class LuaScriptManager {
public:
    LuaScriptManager();
    ~LuaScriptManager();
    
    // Initialize the script manager
    bool Initialize();
    
    // Create a new script state
    std::shared_ptr<LuaScriptState> CreateScriptState();
    
    // Get the default/global script state
    std::shared_ptr<LuaScriptState> GetDefaultState();
    
    // Execute a script directly
    bool ExecuteScript(const String& code);
    bool ExecuteScriptFile(const String& filename);
    
    // Register common game engine bindings
    void RegisterCommonBindings();
    
    // Update scripts (for hot-reloading, etc.)
    void Update();
    
    // Enable hot-reloading of scripts
    void SetHotReloadEnabled(bool enabled) { hotReloadEnabled = enabled; }
    bool IsHotReloadEnabled() const { return hotReloadEnabled; }
    
private:
    Vector<std::shared_ptr<LuaScriptState>> scriptStates;
    std::shared_ptr<LuaScriptState> defaultState;
    bool initialized;
    bool hotReloadEnabled;
    
    // Hot reload tracking
    HashMap<String, Time> scriptLastModified;
};

// Base class for Lua scriptable objects in the game engine
class LuaScriptableObject {
public:
    LuaScriptableObject();
    virtual ~LuaScriptableObject() = default;
    
    // Attach a Lua script to this object
    virtual void AttachScript(const String& scriptName);
    
    // Execute an update function in the attached script
    virtual void Update(float deltaTime);
    
    // Execute a custom function in the attached script
    virtual bool ExecuteFunction(const String& funcName);
    
    // Get the script state associated with this object
    std::shared_ptr<LuaScriptState> GetScriptState() const { return scriptState; }
    
protected:
    String attachedScriptName;
    std::shared_ptr<LuaScriptState> scriptState;
};

END_UPP_NAMESPACE

#endif