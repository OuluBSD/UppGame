#ifndef _Platformer2D_Core_Core_h_
#define _Platformer2D_Core_Core_h_

#include <game/Platformer2D/Platformer2D.h>
#include <game/Platformer2D/Core/ObjectSystem.h>

NAMESPACE_UPP

// ============================================================================
// Episode System
// ============================================================================

class EpisodeDef
{
public:
	using SetupFunc = void(*)();
	using PresenceFunc = bool(*)();
	using LogicFunc = void(*)();
	
	const char* episodeFile;
	const char* name;
	SetupFunc setupFunctions;
	PresenceFunc isPresent;
	LogicFunc episodeLogic;
	
	// Resource file extensions and names (for Resource package)
	const char* mapHeadExt;      // "CK4", "CK5", "CK6"
	const char* graphHeadExt;    // "CK4", "CK5", "CK6"
	const char* audioHeadExt;    // "CK4", "CK5", "CK6"
	const char* tileInfoExt;     // "CK4", "CK5", "CK6"
	const char* mapHeadFile;     // "MAPHEAD.CK4" etc.
	const char* mapFile;         // "GAMEMAPS.CK4" etc.
	const char* graphDictFile;   // "EGADICT.CK4" etc.
	const char* graphHeadFile;   // "EGAHEAD.CK4" etc.
	const char* graphFile;       // "EGAGRAPH.CK4" etc.
	const char* gfxInfoFile;     // "GFXINFOE.CK4" etc.
	const char* audioDictFile;   // "AUDIODCT.CK4" etc.
	const char* audioHeadFile;   // "AUDIOHHD.CK4" etc.
	const char* audioFile;       // "AUDIO.CK4" etc.
	const char* audioInfoFile;   // "AUDINFOE.CK4" etc.
	const char* tileInfoFile;    // "TILEINFO.CK4" etc.
	
	// GFXINFOE offsets (from TED5)
	uint16_t numTiles8 = 0, numTiles8m = 0, numTiles16 = 0, numTiles16m = 0, numTiles32 = 0, numTiles32m = 0;
	uint16_t offTiles8 = 0, offTiles8m = 0, offTiles16 = 0, offTiles16m = 0, offTiles32 = 0, offTiles32m = 0;
	uint16_t numBitmaps = 0, numMasked = 0, numSprites = 0;
	uint16_t offBitmaps = 0, offMasked = 0, offSprites = 0;
	uint16_t hdrBitmaps = 0, hdrMasked = 0, hdrSprites = 0;
	uint16_t numBinaries = 0, offBinaries = 0;
	
	EpisodeDef() : episodeFile(nullptr), name(nullptr), setupFunctions(nullptr), isPresent(nullptr), episodeLogic(nullptr),
		mapHeadExt(nullptr), graphHeadExt(nullptr), audioHeadExt(nullptr), tileInfoExt(nullptr),
		mapHeadFile(nullptr), mapFile(nullptr), graphDictFile(nullptr), graphHeadFile(nullptr), graphFile(nullptr),
		gfxInfoFile(nullptr), audioDictFile(nullptr), audioHeadFile(nullptr), audioFile(nullptr),
		audioInfoFile(nullptr), tileInfoFile(nullptr) {}
	
	bool IsValid() const { return episodeFile && name && setupFunctions && isPresent; }
};

// Episode registry - populated at startup
extern EpisodeDef* g_episodes[4];
extern EpisodeDef* g_currentEpisode;
extern Episode g_currentEpisodeEnum;

void RegisterEpisode(int index, EpisodeDef* ep);

// ============================================================================
// Game State (matches OmniSpeak dumpprinter schema exactly)
// ============================================================================

struct GameState
{
	// Common fields
	uint16_t mapPosX;
	uint16_t mapPosY;
	uint16_t levelsDone[25];
	int32_t keenScore;
	int32_t nextKeenAt;
	int16_t numShots;
	int16_t numCentilife;
	
	// Episode-specific union
	union
	{
		struct { int16_t wetsuit; int16_t membersRescued; } ck4;
		struct { int16_t securityCard; int16_t word_4729C; int16_t fusesRemaining; } ck5;
		struct { int16_t sandwich; int16_t rope; int16_t passcard; int16_t inRocket; } ck6;
	} ep;
	
	int16_t keyGems[4];
	int16_t currentLevel;
	int16_t numLives;
	int16_t difficulty;
	uint16_t platformOffset; // Backwards compatibility
	
	GameState() { Zero(*this); }
	
	void Clear() { Zero(*this); }
	
	// Serialization for save/dump compatibility
	void Serialize(Stream& s); // Little-endian
};

enum class Difficulty : int16_t
{
	Easy = 0,
	Normal = 1,
	Hard = 2
};

enum class LevelState : int16_t
{
	Playing = 0,
	ResetGame = 1,
	LoadedGame = 2,
	LevelDone = 3,
	GameOver = 4
};

extern GameState g_gameState;
extern LevelState g_levelState;

// ============================================================================
// Action System
// ============================================================================

// Action function registration
void ACT_SetupFunctions();
void KeenSetupFunctions();
void OBJ_SetupFunctions();
void Map_SetupFunctions();
void Misc_SetupFunctions();

// Episode-specific setup
void CK4_SetupFunctions();
void CK5_SetupFunctions();
void CK6_SetupFunctions();

// ============================================================================
// Variable/Action Compiler (from EPISODE.CKx) - defined in Resource/VAR.h
// ============================================================================

// Forward declarations - full definitions in <game/Platformer2D/Resource/VAR.h>
class VAR_Engine;
enum class VAR_VarType : uint8_t;

// ============================================================================
// Simulation Clock Interface (for headless testing)
// ============================================================================

class ISimulationClock
{
public:
	virtual ~ISimulationClock() {}
	virtual uint32_t GetTickCount() const = 0;
	virtual void SetTickCount(uint32_t ticks) = 0;
	virtual void WaitTick() = 0;
	virtual int GetTicksPerSecond() const { return PLATFORMER2D_TICKS_PER_SECOND; }
};

// Global clock instance (set by platform layer)
extern ISimulationClock* g_simClock;

// ============================================================================
// RNG Interface (for deterministic replay)
// ============================================================================

class IRandomGenerator
{
public:
	virtual ~IRandomGenerator() {}
	virtual uint16_t Random() = 0;
	virtual void SetSeed(uint32_t seed) = 0;
	virtual uint32_t GetSeed() const = 0;
};

extern IRandomGenerator* g_rng;

// ============================================================================
// Core Functions
// ============================================================================

void InitGame();
void ShutdownGame();
void DemoLoop();
void GameLoop();
void NewGame();
void LoadGame(int slot);
void SaveGame(int slot);
void PlayDemo(int demoNum);
void PlayDemoFile(const char* filename);

// Input handling (injected)
// Scancodes matching OmniSpeak IN_SC_* constants
enum ScanCode : uint16_t
{
	SC_None = 0,
	SC_Escape = 1,
	SC_F1 = 2,
	SC_F2 = 3,
	SC_F3 = 4,
	SC_F4 = 5,
	SC_F5 = 6,
	SC_F6 = 7,
	SC_F7 = 8,
	SC_F8 = 9,
	SC_F9 = 10,
	SC_F10 = 11,
	SC_F11 = 12,
	SC_F12 = 13,
	SC_UpArrow = 14,
	SC_DownArrow = 15,
	SC_LeftArrow = 16,
	SC_RightArrow = 17,
	SC_Space = 18,
	SC_Control = 19,
	SC_Alt = 20,
	SC_LeftShift = 21,
	SC_Enter = 22,
	SC_Tab = 23,
	SC_Backspace = 24,
	SC_A = 25,
	SC_B = 26,
	SC_C = 27,
	SC_D = 28,
	SC_E = 29,
	SC_F = 30,
	SC_G = 31,
	SC_H = 32,
	SC_I = 33,
	SC_J = 34,
	SC_K = 35,
	SC_L = 36,
	SC_M = 37,
	SC_N = 38,
	SC_O = 39,
	SC_P = 40,
	SC_Q = 41,
	SC_R = 42,
	SC_S = 43,
	SC_T = 44,
	SC_U = 45,
	SC_V = 46,
	SC_W = 47,
	SC_X = 48,
	SC_Y = 49,
	SC_Z = 50,
	SC_Zero = 51,
	SC_One = 52,
	SC_Two = 53,
	SC_Three = 54,
	SC_Four = 55,
	SC_Five = 56,
	SC_Six = 57,
	SC_Seven = 58,
	SC_Eight = 59,
	SC_Nine = 60,
	SC_Home = 61,
	SC_PgUp = 62,
	SC_End = 63,
	SC_PgDown = 64,
};

// Motion values for control frame
enum Motion : int8_t
{
	Motion_None = 0,
	Motion_Left = -1,
	Motion_Right = 1,
	Motion_Up = -2,
	Motion_Down = 2
};

// Direction values for control frame
enum InputDirection : uint8_t
{
	Dir_North = 0,
	Dir_NorthEast = 1,
	Dir_East = 2,
	Dir_SouthEast = 3,
	Dir_South = 4,
	Dir_SouthWest = 5,
	Dir_West = 6,
	Dir_NorthWest = 7,
	Dir_None = 255
};

struct ControlFrame
{
	bool jump, pogo, fire, button2, button3;
	int x, y;
	Motion xDirection, yDirection;
	InputDirection dir;
	
	void Clear()
	{
		jump = pogo = fire = button2 = button3 = false;
		x = y = 0;
		xDirection = Motion::Motion_None;
		yDirection = Motion::Motion_None;
		dir = InputDirection::Dir_None;
	}
};

void ReadControls(int player, ControlFrame* controls);

// Episode detection
bool DetectEpisodes();
const char* GetEpisodeFile(Episode ep);

END_UPP_NAMESPACE

#endif // _Platformer2D_Core_Core_h_