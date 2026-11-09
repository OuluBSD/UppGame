#ifndef _Platformer2D_Core_Physics_h_
#define _Platformer2D_Core_Physics_h_

#include "Core.h"
#include "ObjectSystem.h"

NAMESPACE_UPP

// ============================================================================
// Physics System
// ============================================================================

struct ObjPhysData
{
	int16_t unitX1, unitX2, unitY1, unitY2;
	int16_t unitXmid;
	int16_t tileX1, tileX2, tileY1, tileY2;
	int16_t tileXmid;
};

struct ObjPhysDataDelta
{
	int16_t unitX1, unitY1, unitX2, unitY2;
	int16_t unitXmid;
};

extern ObjPhysData g_oldRects;
extern ObjPhysDataDelta g_deltaRects;
extern int16_t g_nextX, g_nextY;
extern bool g_keenIgnoreVertClip;

// Physics functions
void ResetClipRects(Object* obj);
void SetOldClipRects(Object* obj);
void SetDeltaClipRects(Object* obj);
void PhysUpdateX(Object* obj, int16_t deltaUnitX);
void PhysUpdateY(Object* obj, int16_t deltaUnitY);

// Object-vs-map clipping (OmniSpeak ck_phys.c)
bool NotStuckInWall(Object* obj);
void PhysClipHorz(Object* obj);
void PhysClipVert(Object* obj);

// Object update routines
void PhysUpdateNormalObj(Object* obj);
void PhysUpdateSimpleObj(Object* obj);

// Push/interaction routines
void PhysPushX(Object* pushee, Object* pusher);
void PhysPushY(Object* pushee, Object* pusher);
void PhysPushXY(Object* passenger, Object* platform, bool squish);

// Action setting
void SetAction(Object* obj, Action* act);
void SetAction2(Object* obj, Action* act);

// Keen-specific physics
void PhysKeenClipDown(Object* obj);
void PhysKeenClipUp(Object* obj);
void PhysGravity(Object* obj);
void PhysGravityHigh(Object* obj);
void PhysGravityMid(Object* obj);
void PhysGravityLow(Object* obj);
void PhysDampHorz(Object* obj);
void PhysAccelHorz(Object* obj, int16_t accX, int16_t velLimit);
void PhysAccelHorz2(Object* obj, int16_t accX, int16_t velLimit);
void PhysAccelVert1(Object* obj, int16_t accY, int16_t velLimit);

// Platform physics
void PhysPlatform(Object* obj);
void KeenRidePlatform(Object* keen);

// Full clip
void PhysFullClipToWalls(Object* obj);

// Global state (OmniSpeak ck_phys.c)
extern int16_t g_nextX, g_nextY;
extern ObjPhysData g_oldRects;
extern ObjPhysDataDelta g_deltaRects;
extern bool g_keenIgnoreVertClip;

// Slope height table (from OmniSpeak ck_phys.c)
extern const int16_t g_physSlopeHeight[8][16];

END_UPP_NAMESPACE

#endif // _Platformer2D_Core_Physics_h_