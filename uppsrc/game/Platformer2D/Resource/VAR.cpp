#include "VAR.h"

NAMESPACE_UPP

static const int CK_VAR_MAXVARS = 8192;
static const int CK_VAR_MAXACTIONS = 512;
static const int CK_VAR_MAX_ARRAY_LEN = 96;

ArrayMap<String, VAR_Value> VAR_Engine::s_varTable;
Vector<Action> VAR_Engine::s_actionData;
int VAR_Engine::s_actionsUsed = 0;
bool VAR_Engine::s_varsLocked = false;
ArrayMap<String, ThinkFunc> VAR_Engine::s_thinkFuncs;
ArrayMap<String, CollideFunc> VAR_Engine::s_collideFuncs;
ArrayMap<String, DrawFunc> VAR_Engine::s_drawFuncs;
String VAR_Engine::s_currentFile;

void VAR_Engine::Startup()
{
    s_varTable.Clear();
    s_actionData.SetCount(CK_VAR_MAXACTIONS);
    s_actionsUsed = 0;
    s_varsLocked = false;
    s_thinkFuncs.Clear();
    s_collideFuncs.Clear();
    s_drawFuncs.Clear();
}

void VAR_Engine::Shutdown()
{
    for (int i = 0; i < s_varTable.GetCount(); i++)
    {
        VAR_Value& val = s_varTable[i];
        val.Clear();
    }
    s_varTable.Clear();
    s_actionData.Clear();
    s_actionsUsed = 0;
    s_thinkFuncs.Clear();
    s_collideFuncs.Clear();
    s_drawFuncs.Clear();
}

void VAR_Engine::SetVarsLocked(bool locked)
{
    s_varsLocked = locked;
}

bool VAR_Engine::AreVarsLocked()
{
    return s_varsLocked;
}

VAR_Value* VAR_Engine::FindVar(const char* name)
{
    int idx = s_varTable.Find(name);
    if (idx >= 0)
        return &s_varTable[idx];
    return nullptr;
}

int32_t VAR_Engine::GetInt(const char* name, int32_t def)
{
    VAR_Value* var = FindVar(name);
    if (!var || var->type != VAR_VarType::Int)
        return def;
    return var->intVal;
}

const char* VAR_Engine::GetString(const char* name, const char* def)
{
    VAR_Value* var = FindVar(name);
    if (!var || var->type != VAR_VarType::String)
        return def;
    return var->stringVal;
}

const Vector<int32_t>* VAR_Engine::GetIntArray(const char* name)
{
    VAR_Value* var = FindVar(name);
    if (!var || var->type != VAR_VarType::IntArray)
        return nullptr;
    return var->intArrayVal;
}

int32_t VAR_Engine::GetIntArrayElement(const char* name, int index)
{
    const Vector<int32_t>* arr = GetIntArray(name);
    if (!arr || index < 0 || index >= arr->GetCount())
        return 0;
    return (*arr)[index];
}

const Vector<String>* VAR_Engine::GetStringArray(const char* name)
{
    VAR_Value* var = FindVar(name);
    if (!var || var->type != VAR_VarType::StringArray)
        return nullptr;
    return var->stringArrayVal;
}

const char* VAR_Engine::GetStringArrayElement(const char* name, int index)
{
    const Vector<String>* arr = GetStringArray(name);
    if (!arr || index < 0 || index >= arr->GetCount())
        return "";
    return (*arr)[index];
}

Action* VAR_Engine::GetAction(const char* name)
{
    VAR_Value* var = FindVar(name);
    if (!var || var->type != VAR_VarType::Action)
        return nullptr;
    return var->actionVal;
}

Action* VAR_Engine::GetOrCreateAction(const char* name)
{
    VAR_Value* var = FindVar(name);
    if (var)
    {
        if (var->type != VAR_VarType::Action)
            return nullptr;
        return var->actionVal;
    }
    
    if (s_actionsUsed >= CK_VAR_MAXACTIONS)
        return nullptr;
    
    Action* act = &s_actionData[s_actionsUsed++];
    new(act) Action();
    act->compatDosPointer = 0;
    act->chunkLeft = 0;
    act->chunkRight = 0;
    act->type = (int16_t)ActionType::UnscaledOnce;
    act->protectAnimation = 0;
    act->stickToGround = 0;
    act->timer = 0;
    act->velX = 0;
    act->velY = 0;
    act->think = nullptr;
    act->collide = nullptr;
    act->draw = nullptr;
    act->next = nullptr;
    
    VAR_Value& val = s_varTable.GetAdd(name);
    val.type = VAR_VarType::Action;
    val.actionVal = act;
    
    return act;
}

Action* VAR_Engine::LookupActionFromDosPointer(uint16_t dosPtr)
{
    for (int i = 0; i < s_actionsUsed; ++i)
        if (s_actionData[i].compatDosPointer == dosPtr)
            return &s_actionData[i];
    return nullptr;
}

int VAR_Engine::GetVarCount()
{
    return s_varTable.GetCount();
}

const char* VAR_Engine::GetVarNameAt(int index)
{
    if (index < 0 || index >= s_varTable.GetCount())
        return nullptr;
    return s_varTable.GetKey(index);
}

VAR_VarType VAR_Engine::GetVarTypeAt(int index)
{
    if (index < 0 || index >= s_varTable.GetCount())
        return VAR_VarType::Invalid;
    return s_varTable[index].type;
}

int32_t VAR_Engine::GetIntAt(int index)
{
    if (index < 0 || index >= s_varTable.GetCount())
        return 0;
    const VAR_Value& v = s_varTable[index];
    return v.type == VAR_VarType::Int ? v.intVal : 0;
}

const char* VAR_Engine::GetStringAt(int index)
{
    if (index < 0 || index >= s_varTable.GetCount())
        return nullptr;
    const VAR_Value& v = s_varTable[index];
    return v.type == VAR_VarType::String ? v.stringVal.Begin() : nullptr;
}

const Vector<int32_t>* VAR_Engine::GetIntArrayAt(int index)
{
    if (index < 0 || index >= s_varTable.GetCount())
        return nullptr;
    const VAR_Value& v = s_varTable[index];
    return v.type == VAR_VarType::IntArray ? v.intArrayVal : nullptr;
}

const Vector<String>* VAR_Engine::GetStringArrayAt(int index)
{
    if (index < 0 || index >= s_varTable.GetCount())
        return nullptr;
    const VAR_Value& v = s_varTable[index];
    return v.type == VAR_VarType::StringArray ? v.stringArrayVal : nullptr;
}

void VAR_Engine::SetInt(const char* name, int32_t val)
{
    if (s_varsLocked && FindVar(name))
        return;
    VAR_Value& v = s_varTable.GetAdd(name);
    v.Clear();
    v.type = VAR_VarType::Int;
    v.intVal = val;
}

void VAR_Engine::SetString(const char* name, const char* val)
{
    if (s_varsLocked && FindVar(name))
        return;
    VAR_Value& v = s_varTable.GetAdd(name);
    v.Clear();
    v.type = VAR_VarType::String;
    new(&v.stringVal) String(val);
}

void VAR_Engine::SetIntArray(const char* name, const Vector<int32_t>& arr)
{
    if (s_varsLocked && FindVar(name))
        return;
    VAR_Value& v = s_varTable.GetAdd(name);
    v.Clear();
    v.type = VAR_VarType::IntArray;
    v.intArrayVal = new Vector<int32_t>;
    for (int i = 0; i < arr.GetCount(); i++)
        v.intArrayVal->Add(arr[i]);
}

void VAR_Engine::SetStringArray(const char* name, const Vector<String>& arr)
{
    if (s_varsLocked && FindVar(name))
        return;
    VAR_Value& v = s_varTable.GetAdd(name);
    v.Clear();
    v.type = VAR_VarType::StringArray;
    v.stringArrayVal = new Vector<String>;
    for (int i = 0; i < arr.GetCount(); i++)
        v.stringArrayVal->Add(arr[i]);
}

void VAR_Engine::SetFunction(const char* name, ThinkFunc fn)
{
    s_thinkFuncs.GetAdd(name) = fn;
}

void VAR_Engine::SetColFunction(const char* name, CollideFunc fn)
{
    s_collideFuncs.GetAdd(name) = fn;
}

void VAR_Engine::SetDrawFunction(const char* name, DrawFunc fn)
{
    s_drawFuncs.GetAdd(name) = fn;
}

Token VAR_Engine::GetToken(VAR_ParserState& ps)
{
    if (ps.haveBufferedToken)
    {
        ps.haveBufferedToken = false;
        Token tok;
        tok.type = ps.bufferedTokenType;
        tok.valuePtr = ps.bufferedToken;
        tok.valueLength = strlen(ps.bufferedToken);
        return tok;
    }
    
    while (ps.dataIndex < ps.dataSize)
    {
        char c = ps.data[ps.dataIndex];
        if (c == '\r' || c == '\n' || c == ' ' || c == '\t')
        {
            if (c == '\n')
                ps.lineCount++;
            ps.dataIndex++;
            continue;
        }
        
        if (c == '/' && ps.dataIndex + 1 < ps.dataSize && ps.data[ps.dataIndex + 1] == '/')
        {
            ps.dataIndex += 2;
            while (ps.dataIndex < ps.dataSize && ps.data[ps.dataIndex] != '\n')
                ps.dataIndex++;
            continue;
        }
        
        if (c == '#')
        {
            while (ps.dataIndex < ps.dataSize && ps.data[ps.dataIndex] != '\n')
                ps.dataIndex++;
            continue;
        }
        
        if (c == '"')
        {
            ps.dataIndex++;
            const char* start = ps.data + ps.dataIndex;
            size_t len = 0;
            while (ps.dataIndex < ps.dataSize && ps.data[ps.dataIndex] != '"')
            {
                ps.dataIndex++;
                len++;
            }
            if (ps.dataIndex < ps.dataSize)
                ps.dataIndex++;
            
            Token tok;
            tok.type = TokenType::String;
            tok.valuePtr = start;
            tok.valueLength = len;
            return tok;
        }
        
        if (c == '%' || c == '@' || isdigit(c) || c == '-' || c == '+' || isalpha(c) || c == '_')
        {
            const char* start = ps.data + ps.dataIndex;
            size_t len = 0;
            
            if (c == '%')
            {
                ps.dataIndex++;
                len++;
                while (ps.dataIndex < ps.dataSize && (isalnum(ps.data[ps.dataIndex]) || ps.data[ps.dataIndex] == '_'))
                {
                    ps.dataIndex++;
                    len++;
                }
            }
            else if (c == '@')
            {
                ps.dataIndex++;
                len++;
                while (ps.dataIndex < ps.dataSize && (isalnum(ps.data[ps.dataIndex]) || ps.data[ps.dataIndex] == '_'))
                {
                    ps.dataIndex++;
                    len++;
                }
            }
            else if (c == '-' || c == '+' || isdigit(c))
            {
                size_t i = ps.dataIndex;
                if (ps.data[i] == '-' || ps.data[i] == '+')
                    i++;
                // Handle hex numbers with optional sign (e.g. 0x100, -0x200)
                bool isHex = (i + 1 < ps.dataSize) && ps.data[i] == '0'
                    && (ps.data[i + 1] == 'x' || ps.data[i + 1] == 'X');
                if (isHex)
                {
                    i += 2; // Skip "0x"
                    while (i < ps.dataSize && isxdigit((unsigned char)ps.data[i]))
                        i++;
                }
                else
                {
                    while (i < ps.dataSize && (isdigit(ps.data[i]) || ps.data[i] == '-' || ps.data[i] == '+'))
                        i++;
                }
                len = i - ps.dataIndex;
                ps.dataIndex = i;
            }
            else
            {
                while (ps.dataIndex < ps.dataSize && (isalnum(ps.data[ps.dataIndex]) || ps.data[ps.dataIndex] == '_'))
                {
                    ps.dataIndex++;
                    len++;
                }
            }
            
            Token tok;
            tok.type = TokenType::Ident;
            tok.valuePtr = start;
            tok.valueLength = len;
            return tok;
        }
        
        if (c == '=' || c == ',' || c == ';' || c == '{' || c == '}' || c == '(' || c == ')')
        {
            ps.dataIndex++;
            Token tok;
            tok.type = TokenType::Symbol;
            tok.valuePtr = ps.data + ps.dataIndex - 1;
            tok.valueLength = 1;
            return tok;
        }
        
        ps.dataIndex++;
    }
    
    Token tok;
    tok.type = TokenType::End;
    tok.valuePtr = nullptr;
    tok.valueLength = 0;
    return tok;
}

Token VAR_Engine::PeekToken(VAR_ParserState& ps)
{
    Token tok = GetToken(ps);
    if (tok.type != TokenType::End)
    {
        memcpy(ps.bufferedToken, tok.valuePtr, (tok.valueLength < 255 ? tok.valueLength : 255));
        ps.bufferedToken[tok.valueLength < 255 ? tok.valueLength : 255] = 0;
        ps.bufferedTokenType = tok.type;
        ps.haveBufferedToken = true;
    }
    return tok;
}

void VAR_Engine::GetIdent(VAR_ParserState& ps, char* out, size_t maxLen)
{
    Token tok = GetToken(ps);
    if (tok.type == TokenType::Ident && tok.valueLength > 0)
    {
        size_t len = tok.valueLength < maxLen - 1 ? tok.valueLength : maxLen - 1;
        memcpy(out, tok.valuePtr, len);
        out[len] = 0;
    }
    else
    {
        out[0] = 0;
    }
}

void VAR_Engine::GetString(VAR_ParserState& ps, char* out, size_t maxLen)
{
    Token tok = GetToken(ps);
    if (tok.type == TokenType::String && tok.valueLength > 0)
    {
        size_t len = tok.valueLength < maxLen - 1 ? tok.valueLength : maxLen - 1;
        memcpy(out, tok.valuePtr, len);
        out[len] = 0;
    }
    else
    {
        out[0] = 0;
    }
}

int32_t VAR_Engine::GetIntegerValue(const Token& tok)
{
    if (tok.valueLength == 0)
        return 0;
    char buf[64];
    size_t len = tok.valueLength < 63 ? tok.valueLength : 63;
    memcpy(buf, tok.valuePtr, len);
    buf[len] = 0;
    return (int32_t)strtol(buf, nullptr, 0);
}

size_t VAR_Engine::GetStringValue(const Token& tok, char* out, size_t maxLen)
{
    if (tok.valueLength == 0)
    {
        out[0] = 0;
        return 0;
    }
    size_t len = tok.valueLength < maxLen - 1 ? tok.valueLength : maxLen - 1;
    memcpy(out, tok.valuePtr, len);
    out[len] = 0;
    return len;
}

bool VAR_Engine::IsTokenIdent(const Token& tok, const char* ident)
{
    if (tok.type != TokenType::Ident)
        return false;
    size_t len = strlen(ident);
    return tok.valueLength == len && memcmp(tok.valuePtr, ident, len) == 0;
}

VAR_VarType VAR_Engine::ParseVarType(VAR_ParserState& ps)
{
    Token tok = GetToken(ps);
    if (tok.type == TokenType::End)
        return VAR_VarType::EndOfFile;
    if (tok.type != TokenType::Ident)
    {
        LOG("Got a non-ident token reading var type (line " << ps.lineCount << ")");
        return VAR_VarType::Invalid;
    }
    
    if (IsTokenIdent(tok, "%bool"))
        return VAR_VarType::Bool;
    if (IsTokenIdent(tok, "%int"))
        return VAR_VarType::Int;
    if (IsTokenIdent(tok, "%string"))
        return VAR_VarType::String;
    if (IsTokenIdent(tok, "%intarray"))
        return VAR_VarType::IntArray;
    if (IsTokenIdent(tok, "%stringarray"))
        return VAR_VarType::StringArray;
    if (IsTokenIdent(tok, "%function"))
        return VAR_VarType::Function;
    if (IsTokenIdent(tok, "%action"))
        return VAR_VarType::Action;
    if (IsTokenIdent(tok, "%include"))
        return VAR_VarType::TOK_Include;
    if (IsTokenIdent(tok, "%override"))
        return VAR_VarType::TOK_Override;
    
    LOG("Unknown var type (" << s_currentFile << ") on line " << ps.lineCount << " (token: \"" << String(tok.valuePtr, tok.valueLength) << "\")");
    return VAR_VarType::Invalid;
}

ActionType VAR_Engine::ParseActionType(VAR_ParserState& ps)
{
    Token tok = GetToken(ps);
    if (IsTokenIdent(tok, "UnscaledOnce"))
        return ActionType::UnscaledOnce;
    if (IsTokenIdent(tok, "ScaledOnce"))
        return ActionType::ScaledOnce;
    if (IsTokenIdent(tok, "Frame"))
        return ActionType::Frame;
    if (IsTokenIdent(tok, "UnscaledFrame"))
        return ActionType::UnscaledFrame;
    if (IsTokenIdent(tok, "ScaledFrame"))
        return ActionType::ScaledFrame;
    
    LOG("Got a bad action type on line " << ps.lineCount);
    return ActionType::UnscaledOnce;
}

int32_t VAR_Engine::ParseIntOrVar(VAR_ParserState& ps)
{
    Token tok = GetToken(ps);
    if (tok.valueLength > 0 && tok.valuePtr[0] == '@')
    {
        char varName[256];
        size_t len = tok.valueLength - 1 < 255 ? tok.valueLength - 1 : 255;
        memcpy(varName, tok.valuePtr + 1, len);
        varName[len] = 0;
        
        VAR_Value* var = FindVar(varName);
        if (!var)
            LOG("Couldn't resolve integer variable reference " << varName << " on line " << ps.lineCount);
        
        return var ? var->intVal : 0;
    }
    else
    {
        return GetIntegerValue(tok);
    }
}

void VAR_Engine::ParseFunction(VAR_ParserState& ps)
{
    char varName[256];
    char valName[256];
    GetIdent(ps, varName, 256);
    GetIdent(ps, valName, 256);
    
    int idx = s_thinkFuncs.Find(valName);
    ThinkFunc fn = idx >= 0 ? s_thinkFuncs[idx] : nullptr;
    
    if (!fn)
        LOG("Function \"" << valName << "\" (referred to by \"" << varName << "\") doesn't seem to exist on line " << ps.lineCount);
    
    SetFunction(varName, fn);
}

bool VAR_Engine::ParseAction(VAR_ParserState& ps)
{
    char actName[256];
    GetIdent(ps, actName, 256);
    
    Token next = PeekToken(ps);
    if (next.type == TokenType::Symbol && next.valueLength == 1 && next.valuePtr[0] == '=')
    {
        GetToken(ps);
        char targetName[256];
        GetIdent(ps, targetName, 256);
        Action* target = GetAction(targetName);
        if (!target)
            LOG("Couldn't resolve action variable reference " << targetName << " on line " << ps.lineCount);
        
        VAR_Value& v = s_varTable.GetAdd(actName);
        v.Clear();
        v.type = VAR_VarType::Action;
        v.actionVal = target;
        return true;
    }
    
    Action* act = GetOrCreateAction(actName);
    if (!act)
        return false;
    
    act->compatDosPointer = ParseIntOrVar(ps);
    act->chunkLeft = ParseIntOrVar(ps);
    act->chunkRight = ParseIntOrVar(ps);
    act->type = (int16_t)ParseActionType(ps);
    act->protectAnimation = ParseIntOrVar(ps);
    act->stickToGround = ParseIntOrVar(ps);
    act->timer = ParseIntOrVar(ps);
    act->velX = ParseIntOrVar(ps);
    act->velY = ParseIntOrVar(ps);
    
    char cThink[256];
    char cCollide[256];
    char cDraw[256];
    GetIdent(ps, cThink, 256);
    GetIdent(ps, cCollide, 256);
    GetIdent(ps, cDraw, 256);
    
    int idx = s_thinkFuncs.Find(cThink);
    act->think = idx >= 0 ? s_thinkFuncs[idx] : nullptr;
    if (!act->think && strcmp(cThink, "NULL") != 0)
        LOG("Action \"" << actName << "\" (" << s_currentFile << " line " << ps.lineCount << ") references unknown think function \"" << cThink << "\"");
    
    idx = s_collideFuncs.Find(cCollide);
    act->collide = idx >= 0 ? s_collideFuncs[idx] : nullptr;
    if (!act->collide && strcmp(cCollide, "NULL") != 0)
        LOG("Action \"" << actName << "\" (" << s_currentFile << " line " << ps.lineCount << ") references unknown collide function \"" << cCollide << "\"");
    
    idx = s_drawFuncs.Find(cDraw);
    act->draw = idx >= 0 ? s_drawFuncs[idx] : nullptr;
    if (!act->draw && strcmp(cDraw, "NULL") != 0)
        LOG("Action \"" << actName << "\" (" << s_currentFile << " line " << ps.lineCount << ") references unknown draw function \"" << cDraw << "\"");
    
    char nextActionName[256];
    GetIdent(ps, nextActionName, 256);
    
    if (strcmp(nextActionName, "NULL") != 0)
        act->next = GetOrCreateAction(nextActionName);
    
    return true;
}

void VAR_Engine::ParseInt(VAR_ParserState& ps)
{
    char varName[256];
    GetIdent(ps, varName, 256);
    int32_t val = ParseIntOrVar(ps);
    SetInt(varName, val);
}

void VAR_Engine::ParseIntArray(VAR_ParserState& ps)
{
    char varName[256];
    GetIdent(ps, varName, 256);
    
    Vector<int32_t> tempArray;
    while (true)
    {
        tempArray.Add(ParseIntOrVar(ps));
        Token next = PeekToken(ps);
        if (!(next.type == TokenType::Symbol && next.valueLength == 1 && next.valuePtr[0] == ','))
            break;
        GetToken(ps);
        if (tempArray.GetCount() >= CK_VAR_MAX_ARRAY_LEN)
        {
            LOG("Array exceeded max size!");
            break;
        }
    }
    SetIntArray(varName, tempArray);
}

void VAR_Engine::ParseString(VAR_ParserState& ps)
{
    char varName[256];
    GetIdent(ps, varName, 256);
    
    String result;
    do
    {
        Token tok = GetToken(ps);
        if (tok.type == TokenType::String && tok.valueLength > 0)
        {
            char buf[4096];
            size_t len = tok.valueLength < 4095 ? tok.valueLength : 4095;
            memcpy(buf, tok.valuePtr, len);
            buf[len] = 0;
            result.Cat(buf);
        }
    } while (PeekToken(ps).type == TokenType::String);
    
    SetString(varName, result);
}

void VAR_Engine::ParseStringArray(VAR_ParserState& ps)
{
    char varName[256];
    GetIdent(ps, varName, 256);
    
    Vector<String> tempArray;
    while (true)
    {
        String str;
        do
        {
            Token tok = GetToken(ps);
            if (tok.type == TokenType::String && tok.valueLength > 0)
            {
                char buf[4096];
                size_t len = tok.valueLength < 4095 ? tok.valueLength : 4095;
                memcpy(buf, tok.valuePtr, len);
                buf[len] = 0;
                str.Cat(buf);
            }
        } while (PeekToken(ps).type == TokenType::String);
        
        tempArray.Add(str);
        
        Token next = PeekToken(ps);
        if (!(next.type == TokenType::Symbol && next.valueLength == 1 && next.valuePtr[0] == ','))
            break;
        GetToken(ps);
        
        if (tempArray.GetCount() >= CK_VAR_MAX_ARRAY_LEN)
        {
            LOG("Array exceeded max size!");
            break;
        }
    }
    SetStringArray(varName, tempArray);
}

void VAR_Engine::ParseInclude(VAR_ParserState& ps)
{
    char filename[256];
    GetString(ps, filename, 256);
    LoadVars(filename);
}

void VAR_Engine::ParseOverride(VAR_ParserState& ps)
{
    char filename[256];
    GetString(ps, filename, 256);
    bool wereLocked = s_varsLocked;
    s_varsLocked = true;
    LoadVars(filename);
    s_varsLocked = wereLocked;
}

bool VAR_Engine::ParseVar(VAR_ParserState& ps)
{
    VAR_VarType varType = ParseVarType(ps);
    
    if (varType == VAR_VarType::EndOfFile)
        return false;
    
    switch (varType)
    {
    case VAR_VarType::Int:
        ParseInt(ps);
        break;
    case VAR_VarType::String:
        ParseString(ps);
        break;
    case VAR_VarType::IntArray:
        ParseIntArray(ps);
        break;
    case VAR_VarType::StringArray:
        ParseStringArray(ps);
        break;
    case VAR_VarType::Function:
        ParseFunction(ps);
        break;
    case VAR_VarType::Action:
        ParseAction(ps);
        break;
    case VAR_VarType::TOK_Include:
        ParseInclude(ps);
        break;
    case VAR_VarType::TOK_Override:
        ParseOverride(ps);
        break;
    default:
        LOG("Unsupported var type: " << (int)varType);
        break;
    }
    
    return true;
}

bool VAR_Engine::LoadVars(const char* filename)
{
    int numVarsParsed = 0;
    VAR_ParserState parserstate;
    
    String fullPath;
    if (!FS_PathManager::FileExistsInPaths(filename, &fullPath))
    {
        LOG("Couldn't load variable file " << filename);
        return false;
    }
    
    String content = LoadFile(fullPath);
    if (content.IsEmpty())
    {
        LOG("Couldn't load variable file " << filename);
        return false;
    }
    
    parserstate.Init(content.Begin(), content.GetLength());
    
    int totalLines = 0;
    for (int i = 0; i < content.GetLength(); i++)
        if (content[i] == '\n')
            totalLines++;
    if (content.GetLength() > 0 && content[content.GetLength() - 1] != '\n')
        totalLines++;
    
    String prevFile = s_currentFile;
    s_currentFile = filename;
    while (ParseVar(parserstate))
        numVarsParsed++;
    s_currentFile = prevFile;
    
    LOG("Parsed " << numVarsParsed << " vars from \"" << filename << "\" over " << totalLines << " lines (" << s_actionsUsed << " actions created).");
    
    return true;
}

END_UPP_NAMESPACE