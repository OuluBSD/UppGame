#ifndef _Platformer2D_Core_EpisodeActions_h_
#define _Platformer2D_Core_EpisodeActions_h_

#include "Core.h"
#include "ObjectSystem.h"

NAMESPACE_UPP

// ============================================================================
// Episode-specific Action Functions
// ============================================================================

// Common/Shared actions
void Act_Null(int objIdx);
void Act_DoNothing(int objIdx);
void Act_RemoveObj(int objIdx);

// Keen 4 actions
void CK4_Act_Keen(int objIdx);
void CK4_Act_KeenWalk(int objIdx);
void CK4_Act_KeenJump(int objIdx);
void CK4_Act_KeenPogo(int objIdx);
void CK4_Act_KeenShoot(int objIdx);
void CK4_Act_KeenClimb(int objIdx);
void CK4_Act_KeenLookUp(int objIdx);
void CK4_Act_KeenLookDown(int objIdx);
void CK4_Act_KeenDead(int objIdx);
void CK4_Act_KeenGodMode(int objIdx);

void CK4_Act_Yorp(int objIdx);
void CK4_Act_Garg(int objIdx);
void CK4_Act_Vorticon(int objIdx);
void CK4_Act_VorticonElite(int objIdx);
void CK4_Act_Butler(int objIdx);
void CK4_Act_Scrub(int objIdx);
void CK4_Act_Mimrock(int objIdx);
void CK4_Act_Shockshund(int objIdx);
void CK4_Act_Council(int objIdx);
void CK4_Act_Berkeloid(int objIdx);
void CK4_Act_Inchworm(int objIdx);
void CK4_Act_Skate(int objIdx);
void CK4_Act_Wormmouth(int objIdx);
void CK4_Act_Arachnut(int objIdx);
void CK4_Act_MadMushroom(int objIdx);
void CK4_Act_Spite(int objIdx);
void CK4_Act_Boundary(int objIdx);
void CK4_Act_Centilife(int objIdx);
void CK4_Act_1UP(int objIdx);
void CK4_Act_Item(int objIdx);
void CK4_Act_PointItem(int objIdx);
void CK4_Act_Ankh(int objIdx);
void CK4_Act_Raygun(int objIdx);
void CK4_Act_Wetsuit(int objIdx);
void CK4_Act_Sandwich(int objIdx);
void CK4_Act_Rope(int objIdx);
void CK4_Act_Passcard(int objIdx);
void CK4_Act_KeenExit(int objIdx);
void CK4_Act_KeenEnter(int objIdx);
void CK4_Act_Platform(int objIdx);
void CK4_Act_PlatformVert(int objIdx);
void CK4_Act_PlatformHoriz(int objIdx);
void CK4_Act_Bridge(int objIdx);
void CK4_Act_BridgeSwitch(int objIdx);
void CK4_Act_Door(int objIdx);
void CK4_Act_GemDoor(int objIdx);
void CK4_Act_Switch(int objIdx);
void CK4_Act_LightSwitch(int objIdx);
void CK4_Act_Stalactite(int objIdx);
void CK4_Act_Thinking(int objIdx);
void CK4_Act_Falling(int objIdx);
void CK4_Act_Sliding(int objIdx);
void CK4_Act_Pushable(int objIdx);
void CK4_Act_CentilifeNotify(int objIdx);
void CK4_Act_ScoreBox(int objIdx);
void CK4_Act_Flag(int objIdx);
void CK4_Act_MapFlag(int objIdx);
void CK4_Act_FlippingFlag(int objIdx);

// Keen 5 actions
void CK5_Act_Keen(int objIdx);
void CK5_Act_KeenWalk(int objIdx);
void CK5_Act_KeenJump(int objIdx);
void CK5_Act_KeenPogo(int objIdx);
void CK5_Act_KeenShoot(int objIdx);
void CK5_Act_KeenClimb(int objIdx);
void CK5_Act_KeenLookUp(int objIdx);
void CK5_Act_KeenLookDown(int objIdx);
void CK5_Act_KeenDead(int objIdx);
void CK5_Act_KeenGodMode(int objIdx);

void CK5_Act_Shikadi(int objIdx);
void CK5_Act_ShikadiMaster(int objIdx);
void CK5_Act_Shelley(int objIdx);
void CK5_Act_Sphereful(int objIdx);
void CK5_Act_Mine(int objIdx);
void CK5_Act_Spindred(int objIdx);
void CK5_Act_Spirogem(int objIdx);
void CK5_Act_Amoeba(int objIdx);
void CK5_Act_QED(int objIdx);
void CK5_Act_VolteFace(int objIdx);
void CK5_Act_Bip(int objIdx);
void CK5_Act_Bipship(int objIdx);
void CK5_Act_Robored(int objIdx);
void CK5_Act_Shockshund(int objIdx);
void CK5_Act_Council(int objIdx);
void CK5_Act_Platform(int objIdx);
void CK5_Act_PlatformVert(int objIdx);
void CK5_Act_PlatformHoriz(int objIdx);
void CK5_Act_Bridge(int objIdx);
void CK5_Act_BridgeSwitch(int objIdx);
void CK5_Act_Door(int objIdx);
void CK5_Act_SecurityDoor(int objIdx);
void CK5_Act_Switch(int objIdx);
void CK5_Act_LightSwitch(int objIdx);
void CK5_Act_Stalactite(int objIdx);
void CK5_Act_Thinking(int objIdx);
void CK5_Act_Falling(int objIdx);
void CK5_Act_Sliding(int objIdx);
void CK5_Act_Pushable(int objIdx);
void CK5_Act_CentilifeNotify(int objIdx);
void CK5_Act_ScoreBox(int objIdx);
void CK5_Act_Flag(int objIdx);
void CK5_Act_MapFlag(int objIdx);
void CK5_Act_FlippingFlag(int objIdx);
void CK5_Act_Centilife(int objIdx);
void CK5_Act_1UP(int objIdx);
void CK5_Act_Item(int objIdx);
void CK5_Act_PointItem(int objIdx);
void CK5_Act_Ankh(int objIdx);
void CK5_Act_Raygun(int objIdx);
void CK5_Act_VCard(int objIdx);

// Keen 6 actions
void CK6_Act_Keen(int objIdx);
void CK6_Act_KeenWalk(int objIdx);
void CK6_Act_KeenJump(int objIdx);
void CK6_Act_KeenPogo(int objIdx);
void CK6_Act_KeenShoot(int objIdx);
void CK6_Act_KeenClimb(int objIdx);
void CK6_Act_KeenLookUp(int objIdx);
void CK6_Act_KeenLookDown(int objIdx);
void CK6_Act_KeenDead(int objIdx);
void CK6_Act_KeenGodMode(int objIdx);

void CK6_Act_Bloog(int objIdx);
void CK6_Act_Blooglet(int objIdx);
void CK6_Act_Flea(int objIdx);
void CK6_Act_Babobba(int objIdx);
void CK6_Act_Bip(int objIdx);
void CK6_Act_Bipship(int objIdx);
void CK6_Act_Gik(int objIdx);
void CK6_Act_Molly(int objIdx);
void CK6_Act_Nospike(int objIdx);
void CK6_Act_Orbatrix(int objIdx);
void CK6_Act_Satellite(int objIdx);
void CK6_Act_Shockshund(int objIdx);
void CK6_Act_Platform(int objIdx);
void CK6_Act_PlatformVert(int objIdx);
void CK6_Act_PlatformHoriz(int objIdx);
void CK6_Act_Bridge(int objIdx);
void CK6_Act_BridgeSwitch(int objIdx);
void CK6_Act_Door(int objIdx);
void CK6_Act_GemDoor(int objIdx);
void CK6_Act_Switch(int objIdx);
void CK6_Act_LightSwitch(int objIdx);
void CK6_Act_Stalactite(int objIdx);
void CK6_Act_Thinking(int objIdx);
void CK6_Act_Falling(int objIdx);
void CK6_Act_Sliding(int objIdx);
void CK6_Act_Pushable(int objIdx);
void CK6_Act_CentilifeNotify(int objIdx);
void CK6_Act_ScoreBox(int objIdx);
void CK6_Act_Flag(int objIdx);
void CK6_Act_MapFlag(int objIdx);
void CK6_Act_FlippingFlag(int objIdx);
void CK6_Act_Centilife(int objIdx);
void CK6_Act_1UP(int objIdx);
void CK6_Act_Item(int objIdx);
void CK6_Act_PointItem(int objIdx);
void CK6_Act_Ankh(int objIdx);
void CK6_Act_Raygun(int objIdx);
void CK6_Act_Sandwich(int objIdx);
void CK6_Act_Rope(int objIdx);
void CK6_Act_Passcard(int objIdx);
void CK6_Act_GravDroid(int objIdx);
void CK6_Act_SmashScreen(int objIdx);

// Episode setup functions
void CK4_SetupFunctions();
void CK5_SetupFunctions();
void CK6_SetupFunctions();

END_UPP_NAMESPACE

#endif // _Platformer2D_Core_EpisodeActions_h_