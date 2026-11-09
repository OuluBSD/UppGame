#ifndef _Platformer2D_Resource_VAR_h_
#define _Platformer2D_Resource_VAR_h_

#include <game/Platformer2D/Resource/Resource.h>
#include <game/Platformer2D/Core/ObjectSystem.h>

NAMESPACE_UPP

// ============================================================================
// Variable Engine (parses EPISODE.CKx and ACTION.CKx files)
// Based on OmniSpeak ck_var.c / ck_act.c
// ============================================================================

// Variable types
enum class VAR_VarType : uint8_t
{
	Invalid = 0,
	EndOfFile = 1,
	Bool = 2,
	Int = 3,
	String = 4,
	IntArray = 5,
	StringArray = 6,
	Function = 7,
	Action = 8,
	TOK_Include = 9,
	TOK_Override = 10,
};

// Action types
enum class ActionType : uint8_t
{
	UnscaledOnce = 0,
	ScaledOnce = 1,
	Frame = 2,
	UnscaledFrame = 3,
	ScaledFrame = 4,
};

// Variable value (type-erased storage)
struct VAR_Value
{
	VAR_VarType type = VAR_VarType::Invalid;
	
	union
	{
		bool boolVal;
		int32_t intVal;
		String stringVal;
		Vector<int32_t>* intArrayVal;
		Vector<String>* stringArrayVal;
		Action* actionVal;
		void* funcVal;
	};
	
	VAR_Value() {}
	~VAR_Value() { Clear(); }
	
	void Clear()
	{
		if (type == VAR_VarType::String)
		{
			stringVal.~String();
		}
		else if (type == VAR_VarType::IntArray && intArrayVal)
		{
			delete intArrayVal;
			intArrayVal = nullptr;
		}
		else if (type == VAR_VarType::StringArray && stringArrayVal)
		{
			delete stringArrayVal;
			stringArrayVal = nullptr;
		}
		type = VAR_VarType::Invalid;
	}
};

// Token types
enum class TokenType : uint8_t
{
	End = 0,
	Ident = 1,
	String = 2,
	Number = 3,
	Symbol = 4,
};

struct Token
{
	TokenType type;
	const char* valuePtr;
	size_t valueLength;
	
	Token() : type(TokenType::End), valuePtr(nullptr), valueLength(0) {}
};

// Parser state for text files
struct VAR_ParserState
{
	const char* data;
	size_t dataSize;
	size_t dataIndex;
	int lineCount;
	bool haveBufferedToken;
	char bufferedToken[256];
	TokenType bufferedTokenType;
	
	VAR_ParserState() : data(nullptr), dataSize(0), dataIndex(0), lineCount(1), haveBufferedToken(false), bufferedTokenType(TokenType::End) {}
	
	void Init(const char* d, size_t sz) 
	{ 
		data = d; 
		dataSize = sz; 
		dataIndex = 0; 
		lineCount = 1; 
		haveBufferedToken = false; 
		bufferedTokenType = TokenType::End;
	}
};

class VAR_Engine
{
public:
	static void Startup();
	static void Shutdown();
	static bool LoadVars(const char* filename); // Parses EPISODE.CKx or ACTION.CKx
	
	// Variable access
	static int32_t GetInt(const char* name, int32_t def = 0);
	static const char* GetString(const char* name, const char* def = "");
	static const Vector<int32_t>* GetIntArray(const char* name);
	static int32_t GetIntArrayElement(const char* name, int index);
	static const Vector<String>* GetStringArray(const char* name);
	static const char* GetStringArrayElement(const char* name, int index);
	
	// Function/Action access
	static Action* GetAction(const char* name);
	static Action* GetOrCreateAction(const char* name);
	static Action* LookupActionFromDosPointer(uint16_t dosPtr);
	
	// Iteration (for tooling: declarations, dumps)
	static int GetVarCount();
	static const char* GetVarNameAt(int index);
	static VAR_VarType GetVarTypeAt(int index);
	static int32_t GetIntAt(int index);
	static const char* GetStringAt(int index);
	static const Vector<int32_t>* GetIntArrayAt(int index);
	static const Vector<String>* GetStringArrayAt(int index);
	
	// Test access
	static bool ParseVar(VAR_ParserState& ps);
	
	// Setters (for parser)
	static void SetInt(const char* name, int32_t val);
	static void SetString(const char* name, const char* val);
	static void SetIntArray(const char* name, const Vector<int32_t>& arr);
	static void SetStringArray(const char* name, const Vector<String>& arr);
	static void SetFunction(const char* name, ThinkFunc fn);
	static void SetColFunction(const char* name, CollideFunc fn);
	static void SetDrawFunction(const char* name, DrawFunc fn);
	
	// Variable locking (for %override)
	static void SetVarsLocked(bool locked);
	static bool AreVarsLocked();

private:
	// Internal parser
	static VAR_VarType ParseVarType(VAR_ParserState& ps);
	static ActionType ParseActionType(VAR_ParserState& ps);
	static int32_t ParseIntOrVar(VAR_ParserState& ps);
	static void ParseFunction(VAR_ParserState& ps);
	static bool ParseAction(VAR_ParserState& ps);
	static void ParseInt(VAR_ParserState& ps);
	static void ParseIntArray(VAR_ParserState& ps);
	static void ParseString(VAR_ParserState& ps);
	static void ParseStringArray(VAR_ParserState& ps);
	static void ParseInclude(VAR_ParserState& ps);
	static void ParseOverride(VAR_ParserState& ps);
	
	// Tokenizer
	static Token GetToken(VAR_ParserState& ps);
	static Token PeekToken(VAR_ParserState& ps);
	static void GetIdent(VAR_ParserState& ps, char* out, size_t maxLen);
	static void GetString(VAR_ParserState& ps, char* out, size_t maxLen);
	static int32_t GetIntegerValue(const Token& tok);
	static size_t GetStringValue(const Token& tok, char* out, size_t maxLen);
	static bool IsTokenIdent(const Token& tok, const char* ident);
	
	// Internal helpers
	static VAR_Value* FindVar(const char* name);
	
	// Static data
	static ArrayMap<String, VAR_Value> s_varTable;
	static Vector<Action> s_actionData;
	static int s_actionsUsed;
	static bool s_varsLocked;
	static ArrayMap<String, ThinkFunc> s_thinkFuncs;
	static ArrayMap<String, CollideFunc> s_collideFuncs;
	static ArrayMap<String, DrawFunc> s_drawFuncs;
	static String s_currentFile;
};

END_UPP_NAMESPACE

#endif // _Platformer2D_Resource_VAR_h_