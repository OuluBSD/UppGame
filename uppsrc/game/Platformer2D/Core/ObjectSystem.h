#ifndef _Platformer2D_Core_ObjectSystem_h_
#define _Platformer2D_Core_ObjectSystem_h_

#include "Core.h"

NAMESPACE_UPP

// ============================================================================
// Object System (100 objects × 38 fields matching dumpprinter)
// ============================================================================

struct ClipRects
{
	int16_t unitX1, unitY1, unitX2, unitY2;
	int16_t unitXmid;
	int16_t tileX1, tileY1, tileX2, tileY2;
	int16_t tileXmid;
};

struct Object
{
	// Fields matching OmniSpeak dumpprinter objFields[38]
	enum FieldIndex : uint8_t
	{
		FLD_TYPE = 0,           // Object type
		FLD_ACTIVE = 1,         // Active state (0=inactive, 1=active, 2=always active, 3=exists onscreen)
		FLD_VISIBLE = 2,        // Visible flag
		FLD_CLIPPED = 3,        // Clipping type
		FLD_TIME_UNTIL_THINK = 4, // Think timer
		FLD_POS_X = 5,          // X position (fixed point)
		FLD_POS_Y = 6,          // Y position (fixed point)
		FLD_X_DIRECTION = 7,    // X direction (-1, 0, 1)
		FLD_Y_DIRECTION = 8,    // Y direction (-1, 0, 1)
		FLD_DELTA_POS_X = 9,    // Delta X position
		FLD_DELTA_POS_Y = 10,   // Delta Y position
		FLD_VEL_X = 11,         // X velocity
		FLD_VEL_Y = 12,         // Y velocity
		FLD_ACTION_TIMER = 13,  // Action timer
		FLD_ACTION_PTR = 14,    // Current action pointer (16-bit offset)
		FLD_GFX_CHUNK = 15,     // Graphics chunk ID
		FLD_Z_LAYER = 16,       // Z layer for drawing
		FLD_CLIP_UNIT_X1 = 17,  // Clip rect unit X1
		FLD_CLIP_UNIT_Y1 = 18,  // Clip rect unit Y1
		FLD_CLIP_UNIT_X2 = 19,  // Clip rect unit X2
		FLD_CLIP_UNIT_Y2 = 20,  // Clip rect unit Y2
		FLD_CLIP_UNIT_XMID = 21, // Clip rect unit X mid
		FLD_CLIP_TILE_X1 = 22,  // Clip rect tile X1
		FLD_CLIP_TILE_Y1 = 23,  // Clip rect tile Y1
		FLD_CLIP_TILE_X2 = 24,  // Clip rect tile X2
		FLD_CLIP_TILE_Y2 = 25,  // Clip rect tile Y2
		FLD_CLIP_TILE_XMID = 26, // Clip rect tile X mid
		FLD_TOP_TI = 27,        // Top tile info
		FLD_RIGHT_TI = 28,      // Right tile info
		FLD_BOTTOM_TI = 29,     // Bottom tile info
		FLD_LEFT_TI = 30,       // Left tile info
		FLD_USER1 = 31,         // User variable 1
		FLD_USER2 = 32,         // User variable 2
		FLD_USER3 = 33,         // User variable 3
		FLD_USER4 = 34,         // User variable 4
		FLD_SDE = 35,           // Sprite draw entry
		FLD_NEXT = 36,          // Next object pointer
		FLD_PREV = 37,          // Previous object pointer
	};

	uint16_t fields[38];
	
	Object() { Zero(fields); }
	
	// Accessors for commonly used fields
	uint16_t GetField(uint8_t idx) const { return idx < 38 ? fields[idx] : 0; }
	void SetField(uint8_t idx, uint16_t val) { if (idx < 38) fields[idx] = val; }
	
	// Typed accessors
	uint16_t GetType() const { return GetField(FLD_TYPE); }
	void SetType(uint16_t v) { SetField(FLD_TYPE, v); }
	
	uint16_t GetActive() const { return GetField(FLD_ACTIVE); }
	void SetActive(uint16_t v) { SetField(FLD_ACTIVE, v); }
	
	uint16_t GetVisible() const { return GetField(FLD_VISIBLE); }
	void SetVisible(uint16_t v) { SetField(FLD_VISIBLE, v); }
	
	uint16_t GetPosX() const { return GetField(FLD_POS_X); }
	void SetPosX(uint16_t v) { SetField(FLD_POS_X, v); }
	
	uint16_t GetPosY() const { return GetField(FLD_POS_Y); }
	void SetPosY(uint16_t v) { SetField(FLD_POS_Y, v); }
	
	int16_t GetXDirection() const { return (int16_t)GetField(FLD_X_DIRECTION); }
	void SetXDirection(int16_t v) { SetField(FLD_X_DIRECTION, (uint16_t)v); }
	
	int16_t GetYDirection() const { return (int16_t)GetField(FLD_Y_DIRECTION); }
	void SetYDirection(int16_t v) { SetField(FLD_Y_DIRECTION, (uint16_t)v); }
	
	int16_t GetVelX() const { return (int16_t)GetField(FLD_VEL_X); }
	void SetVelX(int16_t v) { SetField(FLD_VEL_X, (uint16_t)v); }
	
	int16_t GetVelY() const { return (int16_t)GetField(FLD_VEL_Y); }
	void SetVelY(int16_t v) { SetField(FLD_VEL_Y, (uint16_t)v); }
	
	uint16_t GetGfxChunk() const { return GetField(FLD_GFX_CHUNK); }
	void SetGfxChunk(uint16_t v) { SetField(FLD_GFX_CHUNK, v); }
	
	uint16_t GetZLayer() const { return GetField(FLD_Z_LAYER); }
	void SetZLayer(uint16_t v) { SetField(FLD_Z_LAYER, v); }
	
	uint16_t GetUser1() const { return GetField(FLD_USER1); }
	void SetUser1(uint16_t v) { SetField(FLD_USER1, v); }
	
	uint16_t GetUser2() const { return GetField(FLD_USER2); }
	void SetUser2(uint16_t v) { SetField(FLD_USER2, v); }
	
	uint16_t GetUser3() const { return GetField(FLD_USER3); }
	void SetUser3(uint16_t v) { SetField(FLD_USER3, v); }
	
	uint16_t GetUser4() const { return GetField(FLD_USER4); }
	void SetUser4(uint16_t v) { SetField(FLD_USER4, v); }
	
	uint16_t GetActionTimer() const { return GetField(FLD_ACTION_TIMER); }
	void SetActionTimer(uint16_t v) { SetField(FLD_ACTION_TIMER, v); }
	
	uint16_t GetTimeUntilThink() const { return GetField(FLD_TIME_UNTIL_THINK); }
	void SetTimeUntilThink(uint16_t v) { SetField(FLD_TIME_UNTIL_THINK, v); }
};

// Object active states
enum ObjActive : uint16_t
{
	OBJ_INACTIVE = 0,
	OBJ_ACTIVE = 1,
	OBJ_ALWAYS_ACTIVE = 2,
	OBJ_EXISTS_ONSCREEN = 3
};

// Clipping types
enum ClipType : uint16_t
{
	CLIP_not = 0,
	CLIP_sides = 1,
	CLIP_all = 2,
	CLIP_try = 3
};

// ============================================================================
// Action System
// ============================================================================

// Action function types
using ThinkFunc = void(*)(int objIdx);
using CollideFunc = void(*)(int objIdx, int otherIdx);
using DrawFunc = void(*)(int objIdx);

struct Action
{
	int16_t chunkLeft;
	int16_t chunkRight;
	int16_t type;
	int16_t protectAnimation;
	int16_t stickToGround;
	int16_t timer;
	int16_t velX;
	int16_t velY;
	ThinkFunc think;
	CollideFunc collide;
	DrawFunc draw;
	Action* next;
	uint16_t compatDosPointer; // For validator compatibility
	
	Action() : chunkLeft(0), chunkRight(0), type(0), protectAnimation(0), stickToGround(0), timer(0), velX(0), velY(0), think(nullptr), collide(nullptr), draw(nullptr), next(nullptr), compatDosPointer(0) {}
};

// Action function tables (populated by episode setup)
extern Action* g_actionTable;
extern int g_numActions;

// Action lookup
Action* LookupAction(uint16_t actionIdx);
Action* LookupActionFromDosPointer(uint16_t dosPtr);

extern Object g_objects[100];
extern int g_numObjects;

// Object management
Object* GetFreeObject();
void SetupObjArray();
void RemoveObj(Object* obj);
Object* GetNewObj(bool isPersistent);
void RunAction(Object* obj);

// Keen state (separate from object array)
struct KeenState
{
	Object* platform;       // Platform Keen is standing on
	int16_t pogoTimer;         // Pogo stick timer
	bool jumpWasPressed;       // Jump was pressed last frame
	bool pogoWasPressed;       // Pogo was pressed last frame
	bool shootWasPressed;      // Shoot was pressed last frame
	// ... other Keen-specific state
	
	KeenState() : platform(nullptr), pogoTimer(0), jumpWasPressed(false), pogoWasPressed(false), shootWasPressed(false) {}
};

extern KeenState g_keenState;
extern Object* g_keenObj;
extern Object* g_scoreBoxObj;
extern int g_gameInProgress;
extern bool g_godMode;
extern int16_t g_invincibilityTimer;

// Camera/scroll
extern int g_scrollXUnit, g_scrollYUnit;
extern bool g_scrollDisabled;

// Active object rectangle
extern int g_activeX0Tile, g_activeY0Tile, g_activeX1Tile, g_activeY1Tile;

// Level state
extern int g_mapOn;
extern int g_currentLevel;

END_UPP_NAMESPACE

#endif // _Platformer2D_Core_ObjectSystem_h_