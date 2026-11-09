#include "ObjectSystem.h"
#include <game/Platformer2D/Resource/Resource.h>
#include <game/Platformer2D/Render/Render.h>

NAMESPACE_UPP

// ============================================================================
// Global instances
// ============================================================================

Action* g_actionTable = nullptr;
int g_numActions = 0;

Object g_objects[100];
int g_numObjects = 0;

KeenState g_keenState;
Object* g_keenObj = nullptr;
Object* g_scoreBoxObj = nullptr;
int g_gameInProgress = 0;
bool g_godMode = false;
int16_t g_invincibilityTimer = 0;

int g_scrollXUnit = 0, g_scrollYUnit = 0;
bool g_scrollDisabled = false;

int g_activeX0Tile = 0, g_activeY0Tile = 0, g_activeX1Tile = 0, g_activeY1Tile = 0;

extern int g_mapOn;
extern int g_currentLevel;

// ============================================================================
// Action Lookup
// ============================================================================

Action* LookupAction(uint16_t actionIdx)
{
	if (actionIdx < (uint16_t)g_numActions)
		return &g_actionTable[actionIdx];
	return nullptr;
}

Action* LookupActionFromDosPointer(uint16_t dosPtr)
{
	// For backward compatibility with save games
	// In original, this was a direct pointer, now we map it
	for (int i = 0; i < g_numActions; ++i)
	{
		if (g_actionTable[i].compatDosPointer == dosPtr)
			return &g_actionTable[i];
	}
	return nullptr;
}

// ============================================================================
// Object Management
// ============================================================================

Object* GetFreeObject()
{
	for (int i = 0; i < 100; ++i)
	{
		if (g_objects[i].GetActive() == OBJ_INACTIVE)
			return &g_objects[i];
	}
	return nullptr; // No free objects
}

void SetupObjArray()
{
	// Initialize object array
	for (int i = 0; i < 100; ++i)
	{
		g_objects[i] = Object();
	}
	g_numObjects = 0;
}

void RemoveObj(Object* obj)
{
	if (!obj) return;
	obj->SetActive(OBJ_INACTIVE);
	obj->SetVisible(0);
	// Remove from linked list if needed
	// Note: In original, objects are in a linked list via next/prev pointers
}

Object* GetNewObj(bool isPersistent)
{
	Object* obj = GetFreeObject();
	if (!obj) return nullptr;
	
	obj->SetActive(isPersistent ? OBJ_ALWAYS_ACTIVE : OBJ_ACTIVE);
	obj->SetVisible(1);
	g_numObjects++;
	return obj;
}

void RunAction(Object* obj)
{
	if (!obj || obj->GetActive() == OBJ_INACTIVE) return;
	
	uint16_t actionPtr = obj->GetField(Object::FLD_ACTION_PTR);
	Action* action = LookupActionFromDosPointer(actionPtr);
	if (!action) return;
	
	// Decrement timer
	if (obj->GetActionTimer() > 0)
		obj->SetActionTimer(obj->GetActionTimer() - 1);
	
	// Run think function
	if (action->think)
	{
		// Find object index
		int objIdx = -1;
		for (int i = 0; i < 100; ++i)
		{
			if (&g_objects[i] == obj) { objIdx = i; break; }
		}
		if (objIdx >= 0)
			action->think(objIdx);
	}
}

END_UPP_NAMESPACE