#include "Physics.h"
#include <game/Platformer2D/Resource/Resource.h>
#include <algorithm>

NAMESPACE_UPP

// ============================================================================
// Global instances
// ============================================================================

ObjPhysData g_oldRects;
ObjPhysDataDelta g_deltaRects;
int16_t g_nextX = 0, g_nextY = 0;
bool g_keenIgnoreVertClip = false;

// Forward declarations for tile queries (defined later)
static int16_t GetTileTop(int tileX, int tileY, int layer);
static int16_t GetTileBottom(int tileX, int tileY, int layer);
static int16_t GetTileLeft(int tileX, int tileY, int layer);
static int16_t GetTileRight(int tileX, int tileY, int layer);

// Unit/Tile conversion helpers (inline from header)
inline int UnitToTile(int unit) { return unit >> 8; }
inline int TileToUnit(int tile) { return tile << 8; }
inline int UnitToPixel(int unit) { return unit >> 4; }
inline int PixelToUnit(int pixel) { return pixel << 4; }
inline int PixelToTile(int pixel) { return (pixel + 255) >> 8; }

// Slope height table (from OmniSpeak ck_phys.c)
const int16_t g_physSlopeHeight[8][16] = {
	{0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100},
	{0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
	{0x000, 0x08, 0x010, 0x018, 0x020, 0x028, 0x030, 0x038, 0x040, 0x048, 0x050, 0x058, 0x060, 0x068, 0x070, 0x078},
	{0x080, 0x088, 0x090, 0x098, 0x0a0, 0x0a8, 0x0b0, 0x0b8, 0x0c0, 0x0c8, 0x0d0, 0x0d8, 0x0e0, 0x0e8, 0x0f0, 0x0f8},
	{0x000, 0x010, 0x020, 0x030, 0x040, 0x050, 0x060, 0x070, 0x080, 0x090, 0x0a0, 0x0b0, 0x0c0, 0x0d0, 0x0e0, 0x0f0},
	{0x078, 0x070, 0x068, 0x060, 0x058, 0x050, 0x048, 0x040, 0x038, 0x030, 0x028, 0x020, 0x018, 0x010, 0x008, 0x000},
	{0x0F8, 0x0F0, 0x0E8, 0x0E0, 0x0D8, 0x0D0, 0x0C8, 0x0C0, 0x0B8, 0x0B0, 0x0A8, 0x0A0, 0x098, 0x090, 0x088, 0x080},
	{0x0F0, 0x0E0, 0x0D0, 0x0C0, 0x0B0, 0x0A0, 0x090, 0x080, 0x070, 0x060, 0x050, 0x040, 0x030, 0x020, 0x010, 0x000}
};

// ============================================================================
// Physics Functions
// ============================================================================

void ResetClipRects(Object* obj)
{
	// NOTE: As tile rects are rarely used, keen does not calculate them here.
	const CA_GfxInfo* gfxInfo = ResourceManager::GetGfxInfo();
	if (!gfxInfo) return;
	
	int spriteNumber = obj->GetGfxChunk() - gfxInfo->offSprites;
	
	// TODO: Implement GetSpriteTableEntry in ResourceManager
	VH_SpriteTableEntry ste = {0, 0, 0, 0}; // Placeholder
	
	obj->SetField(Object::FLD_CLIP_UNIT_X1, obj->GetPosX() + ste.xl);
	obj->SetField(Object::FLD_CLIP_UNIT_X2, obj->GetPosX() + ste.xh);
	obj->SetField(Object::FLD_CLIP_UNIT_Y1, obj->GetPosY() + ste.yl);
	obj->SetField(Object::FLD_CLIP_UNIT_Y2, obj->GetPosY() + ste.yh);
	
	obj->SetField(Object::FLD_CLIP_UNIT_XMID, (obj->GetField(Object::FLD_CLIP_UNIT_X2) - obj->GetField(Object::FLD_CLIP_UNIT_X1)) / 2 + obj->GetField(Object::FLD_CLIP_UNIT_X1));
	
	obj->SetField(Object::FLD_CLIP_TILE_X1, UnitToTile(obj->GetField(Object::FLD_CLIP_UNIT_X1)));
	obj->SetField(Object::FLD_CLIP_TILE_X2, UnitToTile(obj->GetField(Object::FLD_CLIP_UNIT_X2)));
	obj->SetField(Object::FLD_CLIP_TILE_Y1, UnitToTile(obj->GetField(Object::FLD_CLIP_UNIT_Y1)));
	obj->SetField(Object::FLD_CLIP_TILE_Y2, UnitToTile(obj->GetField(Object::FLD_CLIP_UNIT_Y2)));
	
	obj->SetField(Object::FLD_CLIP_TILE_XMID, UnitToTile(obj->GetField(Object::FLD_CLIP_UNIT_XMID)));
}

void SetOldClipRects(Object* obj)
{
	g_oldRects.unitX1 = obj->GetField(Object::FLD_CLIP_UNIT_X1);
	g_oldRects.unitX2 = obj->GetField(Object::FLD_CLIP_UNIT_X2);
	g_oldRects.unitY1 = obj->GetField(Object::FLD_CLIP_UNIT_Y1);
	g_oldRects.unitY2 = obj->GetField(Object::FLD_CLIP_UNIT_Y2);
	
	g_oldRects.unitXmid = obj->GetField(Object::FLD_CLIP_UNIT_XMID);
	
	g_oldRects.tileX1 = obj->GetField(Object::FLD_CLIP_TILE_X1);
	g_oldRects.tileX2 = obj->GetField(Object::FLD_CLIP_TILE_X2);
	g_oldRects.tileY1 = obj->GetField(Object::FLD_CLIP_TILE_Y1);
	g_oldRects.tileY2 = obj->GetField(Object::FLD_CLIP_TILE_Y2);
	
	g_oldRects.tileXmid = obj->GetField(Object::FLD_CLIP_TILE_XMID);
}

void SetDeltaClipRects(Object* obj)
{
	g_deltaRects.unitX1 = obj->GetField(Object::FLD_CLIP_UNIT_X1) - g_oldRects.unitX1;
	g_deltaRects.unitY1 = obj->GetField(Object::FLD_CLIP_UNIT_Y1) - g_oldRects.unitY1;
	g_deltaRects.unitX2 = obj->GetField(Object::FLD_CLIP_UNIT_X2) - g_oldRects.unitX2;
	g_deltaRects.unitY2 = obj->GetField(Object::FLD_CLIP_UNIT_Y2) - g_oldRects.unitY2;
	
	g_deltaRects.unitXmid = obj->GetField(Object::FLD_CLIP_UNIT_XMID) - g_oldRects.unitXmid;
	
	// We don't calculate tile deltas.
}

void PhysUpdateX(Object* obj, int16_t deltaUnitX)
{
	obj->SetPosX(obj->GetPosX() + deltaUnitX);
	obj->SetField(Object::FLD_CLIP_UNIT_X1, obj->GetField(Object::FLD_CLIP_UNIT_X1) + deltaUnitX);
	obj->SetField(Object::FLD_CLIP_UNIT_X2, obj->GetField(Object::FLD_CLIP_UNIT_X2) + deltaUnitX);
	
	// It looks like only Keen 6 updates unitXmid here.
	if (g_currentEpisodeEnum == Episode::KEEN6)
		obj->SetField(Object::FLD_CLIP_UNIT_XMID, obj->GetField(Object::FLD_CLIP_UNIT_XMID) + deltaUnitX);
	
	//Now update the tile rect
	// WARNING: Keen _doesn't_ update tileXmid!
	
	obj->SetField(Object::FLD_CLIP_TILE_X1, UnitToTile(obj->GetField(Object::FLD_CLIP_UNIT_X1)));
	obj->SetField(Object::FLD_CLIP_TILE_X2, UnitToTile(obj->GetField(Object::FLD_CLIP_UNIT_X2)));
}

void PhysUpdateY(Object* obj, int16_t deltaUnitY)
{
	obj->SetPosY(obj->GetPosY() + deltaUnitY);
	obj->SetField(Object::FLD_CLIP_UNIT_Y1, obj->GetField(Object::FLD_CLIP_UNIT_Y1) + deltaUnitY);
	obj->SetField(Object::FLD_CLIP_UNIT_Y2, obj->GetField(Object::FLD_CLIP_UNIT_Y2) + deltaUnitY);
	
	obj->SetField(Object::FLD_CLIP_TILE_Y1, UnitToTile(obj->GetField(Object::FLD_CLIP_UNIT_Y1)));
	obj->SetField(Object::FLD_CLIP_TILE_Y2, UnitToTile(obj->GetField(Object::FLD_CLIP_UNIT_Y2)));
}

bool NotStuckInWall(Object* obj)
{
	const int16_t tileX1 = obj->GetField(Object::FLD_CLIP_TILE_X1);
	const int16_t tileX2 = obj->GetField(Object::FLD_CLIP_TILE_X2);
	const int16_t tileY1 = obj->GetField(Object::FLD_CLIP_TILE_Y1);
	const int16_t tileY2 = obj->GetField(Object::FLD_CLIP_TILE_Y2);

	for (int16_t y = tileY1; y <= tileY2; ++y)
	{
		for (int16_t x = tileX1; x <= tileX2; ++x)
		{
			const uint16_t tile = ResourceManager::GetTileAt(x, y, 1);
			if (ResourceManager::TileForeTop(tile) || ResourceManager::TileForeRight(tile)
				|| ResourceManager::TileForeBottom(tile) || ResourceManager::TileForeLeft(tile))
				return false;
		}
	}
	return true;
}

void PhysKeenClipDown(Object* obj)
{
	// Amount we're moving in each direction.
	int16_t deltaX;
	// The part of the slope we care about (in px)
	int16_t midTileXOffset;
	// The top of the tile at Keen's feet.
	int16_t topTI;

	const int16_t uX1 = obj->GetField(Object::FLD_CLIP_UNIT_X1);
	const int16_t uX2 = obj->GetField(Object::FLD_CLIP_UNIT_X2);
	const int16_t uXmid = obj->GetField(Object::FLD_CLIP_UNIT_XMID);
	const int16_t uY2 = obj->GetField(Object::FLD_CLIP_UNIT_Y2);
	const int16_t tX1 = obj->GetField(Object::FLD_CLIP_TILE_X1);
	const int16_t tX2 = obj->GetField(Object::FLD_CLIP_TILE_X2);
	const int16_t tY2 = obj->GetField(Object::FLD_CLIP_TILE_Y2);

	// Performing some special clipping for Keen.
	bool spaceAbove;
	if (obj->GetXDirection() == 1)
	{
		// We care about the lefthand-most side of any slope we're about to touch.
		midTileXOffset = 0;
		deltaX = uX2 - uXmid;
		spaceAbove = (GetTileTop(tX2, tY2 - 1, 1) == 0);
		topTI = GetTileTop(tX2, tY2, 1);

		// If we're being blocked by something, return.
		if (GetTileLeft(tX2, tY2 - 2, 1) || GetTileLeft(tX2, tY2 - 1, 1) || GetTileTop(tX2, tY2 - 1, 1))
			return;
	}
	else
	{
		// Moving left: we care about the righthand-most side of the slope.
		midTileXOffset = 15;
		deltaX = uX1 - uXmid;
		spaceAbove = (GetTileTop(tX1, tY2 - 1, 1) == 0);
		topTI = GetTileTop(tX1, tY2, 1);

		if (GetTileRight(tX1, tY2 - 2, 1) || GetTileRight(tX1, tY2 - 1, 1) || GetTileTop(tX1, tY2 - 1, 1))
			return;
	}

	// If we're about to land on something flat.
	if (spaceAbove && (topTI == 1))
	{
		int16_t slope = g_physSlopeHeight[topTI & 7][midTileXOffset];
		int16_t deltaY = TileToUnit(tY2) + slope - 1 - uY2;
		if (deltaY <= 0 && -g_deltaRects.unitY2 <= deltaY)
		{
			obj->SetField(Object::FLD_TOP_TI, (uint16_t)topTI);
			PhysUpdateY(obj, deltaY);
			PhysUpdateX(obj, deltaX);
		}
	}
}

void PhysKeenClipUp(Object* obj)
{
	int16_t deltaY;
	int16_t midTileXOffset;
	int16_t bottomTI;
	bool spaceBelow;

	const int16_t uY1 = obj->GetField(Object::FLD_CLIP_UNIT_Y1);
	const int16_t tX1 = obj->GetField(Object::FLD_CLIP_TILE_X1);
	const int16_t tX2 = obj->GetField(Object::FLD_CLIP_TILE_X2);
	const int16_t tY1 = obj->GetField(Object::FLD_CLIP_TILE_Y1);

	if (obj->GetXDirection() == 1)
	{
		midTileXOffset = 0;
		bottomTI = GetTileBottom(tX2, tY1, 1);
		spaceBelow = (GetTileBottom(tX2, tY1 + 1, 1) == 0);
		if (GetTileLeft(tX2, tY1 + 2, 1) || GetTileLeft(tX2, tY1 + 3, 1) || GetTileBottom(tX2, tY1 + 1, 1))
			return;
	}
	else
	{
		midTileXOffset = 15;
		bottomTI = GetTileBottom(tX1, tY1, 1);
		spaceBelow = (GetTileBottom(tX1, tY1 + 1, 1) == 0);
		if (GetTileRight(tX1, tY1 + 2, 1) || GetTileRight(tX1, tY1 + 3, 1) || GetTileBottom(tX1, tY1 + 1, 1))
			return;
	}

	if (spaceBelow && bottomTI)
	{
		int16_t slopeAmt = g_physSlopeHeight[bottomTI & 0x07][midTileXOffset];
		deltaY = TileToUnit(tY1 + 1) - slopeAmt - uY1;
		if (deltaY >= 0 && -g_deltaRects.unitY1 >= deltaY)
		{
			obj->SetField(Object::FLD_BOTTOM_TI, (uint16_t)bottomTI);
			PhysUpdateY(obj, deltaY);
		}
	}
}

void PhysClipVert(Object* obj)
{
	const int16_t midTileXOffset = UnitToPixel(obj->GetField(Object::FLD_CLIP_UNIT_XMID)) & 0x0F;

	int16_t vertDisplace = -abs(g_deltaRects.unitXmid) - g_deltaRects.unitY2 - 16; // Move above slope first.

	const int16_t tXmid = obj->GetField(Object::FLD_CLIP_TILE_XMID);
	const int16_t tY1 = obj->GetField(Object::FLD_CLIP_TILE_Y1);
	const int16_t tY2 = obj->GetField(Object::FLD_CLIP_TILE_Y2);
	const int16_t uY1 = obj->GetField(Object::FLD_CLIP_UNIT_Y1);
	const int16_t uY2 = obj->GetField(Object::FLD_CLIP_UNIT_Y2);

	for (int16_t y = g_oldRects.tileY2 - 1; tY2 >= y; ++y)
	{
		const uint16_t tile = ResourceManager::GetTileAt(tXmid, y, 1);
		const uint8_t topTI = ResourceManager::TileForeTop(tile);
		if (topTI)
		{
			int16_t slopeAmt = g_physSlopeHeight[topTI & 0x07][midTileXOffset];
			int16_t objYOffset = uY2 - (y * 256);
			int16_t dy = (slopeAmt - objYOffset) - 1;
			if (dy < 0 && dy >= vertDisplace)
			{
				obj->SetField(Object::FLD_TOP_TI, topTI);
				PhysUpdateY(obj, dy);
				return;
			}
		}
	}

	vertDisplace = abs(g_deltaRects.unitXmid) - g_deltaRects.unitY1 + 16;

	for (int16_t y = g_oldRects.tileY1 + 1; y >= tY1; --y)
	{
		const uint16_t tile = ResourceManager::GetTileAt(tXmid, y, 1);
		const uint8_t bottomTI = ResourceManager::TileForeBottom(tile);
		if (bottomTI)
		{
			int16_t objYOffset = uY1 - TileToUnit(y + 1);
			int16_t slopeAmt = -g_physSlopeHeight[bottomTI & 0x07][midTileXOffset];
			int16_t moved = g_nextY + slopeAmt - objYOffset;
			if ((slopeAmt - objYOffset > 0) && ((slopeAmt - objYOffset) <= vertDisplace)
				&& moved < 256 && moved > -256)
			{
				obj->SetField(Object::FLD_BOTTOM_TI, bottomTI);
				PhysUpdateY(obj, -objYOffset + slopeAmt);
			}
		}
	}
}

void PhysClipHorz(Object* obj)
{
	int16_t tileY1 = obj->GetField(Object::FLD_CLIP_TILE_Y1);
	int16_t tileY2 = obj->GetField(Object::FLD_CLIP_TILE_Y2);

	// If we're on a slope, don't collide vertically with the slope tiles.
	if (obj->GetField(Object::FLD_TOP_TI) > 1)
		tileY2--;
	if (obj->GetField(Object::FLD_BOTTOM_TI) > 1)
		tileY1++;

	// Check if our right side is intersecting with a wall.
	const int16_t tileX1 = obj->GetField(Object::FLD_CLIP_TILE_X1);
	for (int16_t y = tileY1; y <= tileY2; ++y)
	{
		const uint16_t tile = ResourceManager::GetTileAt(tileX1, y, 1);
		const uint8_t rightTI = ResourceManager::TileForeRight(tile);
		obj->SetField(Object::FLD_RIGHT_TI, rightTI);
		if (rightTI)
		{
			// Push us right until we're no-longer intersecting.
			PhysUpdateX(obj, TileToUnit(tileX1 + 1) - obj->GetField(Object::FLD_CLIP_UNIT_X1));
			return;
		}
	}

	// Similarly for the right side (left side of intersecting tile).
	const int16_t tileX2 = obj->GetField(Object::FLD_CLIP_TILE_X2);
	for (int16_t y = tileY1; y <= tileY2; ++y)
	{
		const uint16_t tile = ResourceManager::GetTileAt(tileX2, y, 1);
		const uint8_t leftTI = ResourceManager::TileForeLeft(tile);
		obj->SetField(Object::FLD_LEFT_TI, leftTI);
		if (leftTI)
		{
			// Push us left until we're no-longer intersecting.
			PhysUpdateX(obj, TileToUnit(tileX2) - 1 - obj->GetField(Object::FLD_CLIP_UNIT_X2));
			return;
		}
	}
}

void PhysGravity(Object* obj)
{
	// TODO: Full implementation
}

void PhysGravityHigh(Object* obj)
{
	// OmniSpeak: applies high gravity every other tick
	// Since we don't have SD_GetLastTimeCount/SD_GetSpriteSync in headless mode,
	// this runs once per frame with a simplified model.
	if (obj->GetVelY() < 0 && obj->GetVelY() >= -4)
	{
		g_nextY += obj->GetVelY();
		obj->SetVelY(0);
		return;
	}
	obj->SetVelY(obj->GetVelY() + 4);
	if (obj->GetVelY() > 70)
		obj->SetVelY(70);
	g_nextY += obj->GetVelY();
}

void PhysGravityMid(Object* obj)
{
	if (obj->GetVelY() < 0 && obj->GetVelY() >= -3)
	{
		g_nextY += obj->GetVelY();
		obj->SetVelY(0);
		return;
	}
	obj->SetVelY(obj->GetVelY() + 3);
	if (obj->GetVelY() > 70)
		obj->SetVelY(70);
	g_nextY += obj->GetVelY();
}

void PhysGravityLow(Object* obj)
{
	// Original has a weird condition: (tickCount ? 0 : 1) & 3
	// For headless, apply gravity every frame with a reduced step
	obj->SetVelY(obj->GetVelY() + 1);
	if (obj->GetVelY() > 70)
		obj->SetVelY(70);
	g_nextY += obj->GetVelY();
}

void PhysDampHorz(Object* obj)
{
	bool movingLeft = (obj->GetVelX() < 0);
	int16_t xAdj = 0;
	if (obj->GetVelX() > 0)
		xAdj = -1;
	else if (obj->GetVelX() < 0)
		xAdj = 1;

	// Apply every other tick
	obj->SetVelX(obj->GetVelX() + xAdj);
	if ((obj->GetVelX() < 0) != movingLeft)
		obj->SetVelX(0);
	g_nextX += obj->GetVelX();
}

void PhysAccelHorz(Object* obj, int16_t accX, int16_t velLimit)
{
	bool isNegative = (obj->GetVelX() < 0);
	// Apply every other tick
	obj->SetVelX(obj->GetVelX() + accX);
	if ((obj->GetVelX() < 0) != isNegative)
	{
		isNegative = (obj->GetVelX() < 0);
		obj->SetXDirection(isNegative ? -1 : 1);
	}
	if (obj->GetVelX() > velLimit)
		obj->SetVelX(velLimit);
	else if (obj->GetVelX() < -velLimit)
		obj->SetVelX(-velLimit);
	g_nextX += obj->GetVelX();
}

void PhysAccelHorz2(Object* obj, int16_t accX, int16_t velLimit)
{
	obj->SetVelX(obj->GetVelX() + accX);
	if (obj->GetVelX() > velLimit)
		obj->SetVelX(velLimit);
	else if (obj->GetVelX() < -velLimit)
		obj->SetVelX(-velLimit);
	g_nextX += obj->GetVelX();
}

void PhysAccelVert1(Object* obj, int16_t accY, int16_t velLimit)
{
	obj->SetVelY(obj->GetVelY() + accY);
	if (obj->GetVelY() > velLimit)
		obj->SetVelY(velLimit);
	else if (obj->GetVelY() < -velLimit)
		obj->SetVelY(-velLimit);
	g_nextY += obj->GetVelY();
}

int16_t GetTileTop(int tileX, int tileY, int layer)
{
	return ResourceManager::TileForeTop(ResourceManager::GetTileAt(tileX, tileY, layer));
}

int16_t GetTileBottom(int tileX, int tileY, int layer)
{
	return ResourceManager::TileForeBottom(ResourceManager::GetTileAt(tileX, tileY, layer));
}

int16_t GetTileLeft(int tileX, int tileY, int layer)
{
	return ResourceManager::TileForeLeft(ResourceManager::GetTileAt(tileX, tileY, layer));
}

int16_t GetTileRight(int tileX, int tileY, int layer)
{
	return ResourceManager::TileForeRight(ResourceManager::GetTileAt(tileX, tileY, layer));
}

bool TileIsSolid(int tileX, int tileY, int layer)
{
	const uint16_t tile = ResourceManager::GetTileAt(tileX, tileY, layer);
	return ResourceManager::TileForeTop(tile) != 0
		|| ResourceManager::TileForeBottom(tile) != 0
		|| ResourceManager::TileForeLeft(tile) != 0
		|| ResourceManager::TileForeRight(tile) != 0;
}

bool TileIsPlatform(int tileX, int tileY, int layer)
{
	// 0x19 is the tile-info "top" marker for a platform tile (see OmniSpeak
	// CK_PhysUpdateNormalObj, which tests topTI == 0x19).
	const uint16_t tile = ResourceManager::GetTileAt(tileX, tileY, layer);
	return ResourceManager::TileForeTop(tile) == 0x19;
}

bool TileIsSlope(int tileX, int tileY, int layer)
{
	// The low three bits of the top/bottom value hold the slope type; values
	// 0 and 1 are flat, values > 1 are slopes (see ck_physSlopeHeight).
	const uint16_t tile = ResourceManager::GetTileAt(tileX, tileY, layer);
	return (ResourceManager::TileForeTop(tile) & 0x07) > 1
		|| (ResourceManager::TileForeBottom(tile) & 0x07) > 1;
}

bool CheckObjectCollision(Object* obj1, Object* obj2)
{
	if (!obj1 || !obj2) return false;
	if (obj1->GetActive() == OBJ_INACTIVE || obj2->GetActive() == OBJ_INACTIVE) return false;
	
	// AABB collision check in unit coordinates
	return (obj1->GetField(Object::FLD_CLIP_UNIT_X2) > obj2->GetField(Object::FLD_CLIP_UNIT_X1) &&
			obj1->GetField(Object::FLD_CLIP_UNIT_X1) < obj2->GetField(Object::FLD_CLIP_UNIT_X2) &&
			obj1->GetField(Object::FLD_CLIP_UNIT_Y1) < obj2->GetField(Object::FLD_CLIP_UNIT_Y2) &&
			obj1->GetField(Object::FLD_CLIP_UNIT_Y2) > obj2->GetField(Object::FLD_CLIP_UNIT_Y1));
}

void RunCollisions()
{
	for (int i = 0; i < 100; ++i)
	{
		Object* obj1 = &g_objects[i];
		if (obj1->GetActive() == OBJ_INACTIVE) continue;
		
		for (int j = i + 1; j < 100; ++j)
		{
			Object* obj2 = &g_objects[j];
			if (obj2->GetActive() == OBJ_INACTIVE) continue;
			
			if (CheckObjectCollision(obj1, obj2))
			{
				// Run collide functions
				Action* action1 = LookupActionFromDosPointer(obj1->GetField(Object::FLD_ACTION_PTR));
				Action* action2 = LookupActionFromDosPointer(obj2->GetField(Object::FLD_ACTION_PTR));
				
				if (action1 && action1->collide)
					action1->collide(i, j);
				
				if (action2 && action2->collide)
					action2->collide(j, i);
			}
		}
	}
}

void PhysPlatform(Object* obj)
{
	// TODO: Full implementation
}

void KeenRidePlatform(Object* keen)
{
	// TODO: Full implementation
}

void PhysUpdateNormalObj(Object* obj)
{
	uint16_t oldUnitX = obj->GetPosX();
	uint16_t oldUnitY = obj->GetPosY();
	bool wasNotOnPlatform = false;

	if (obj->GetField(Object::FLD_ACTION_TIMER) > 0) {
		// stickToGround is stored in action->stickToGround
		// We'll need to look it up from the action pointer
		Action* act = LookupActionFromDosPointer(obj->GetField(Object::FLD_ACTION_PTR));
		if (act && act->stickToGround)
		{
			if (obj->GetField(Object::FLD_TOP_TI) == 0x19)
			{
				g_nextY = 145;
			}
			else
			{
				if (g_nextX > 0)
				{
					g_nextY = g_nextX + 16;
				}
				else
				{
					g_nextY = -g_nextX + 16;
				}
				wasNotOnPlatform = true;
			}
		}
	}

	if (g_nextX > 239)
		g_nextX = 239;
	else if (g_nextX < -239)
		g_nextX = -239;

	if (g_nextY > 255)
		g_nextY = 255;
	else if (g_nextY < -239)
		g_nextY = -239;

	obj->SetPosX(obj->GetPosX() + g_nextX);
	obj->SetPosY(obj->GetPosY() + g_nextY);

	obj->SetVisible(1);

	if (obj->GetGfxChunk() != 0)
	{
		SetOldClipRects(obj);
		ResetClipRects(obj);

		// Reset tile clipping vars
		obj->SetField(Object::FLD_TOP_TI, 0);
		obj->SetField(Object::FLD_BOTTOM_TI, 0);
		obj->SetField(Object::FLD_LEFT_TI, 0);
		obj->SetField(Object::FLD_RIGHT_TI, 0);

		if (obj->GetField(Object::FLD_CLIPPED) != 0)
		{
			SetDeltaClipRects(obj);
			PhysClipVert(obj);

			// Keen-specific slope following (not for Keen 6 with ck_keenIgnoreVertClip)
			if (obj == g_keenObj && (g_currentEpisodeEnum != Episode::KEEN6 || !g_keenIgnoreVertClip))
			{
				if (obj->GetField(Object::FLD_TOP_TI) == 0 && g_deltaRects.unitY2 > 0)
				{
					PhysKeenClipDown(obj);
				}
				if (obj->GetField(Object::FLD_BOTTOM_TI) == 0 && g_deltaRects.unitY1 < 0)
				{
					PhysKeenClipUp(obj);
				}
			}

			PhysClipHorz(obj);

			// Keen 6: pushed towards wall while on sloped floor
			if (g_currentEpisodeEnum == Episode::KEEN6 && obj == g_keenObj &&
				(obj->GetField(Object::FLD_TOP_TI) & 7) > 1 &&
				(obj->GetField(Object::FLD_RIGHT_TI) || obj->GetField(Object::FLD_LEFT_TI)))
			{
				int16_t midTileXOffset = UnitToPixel(obj->GetField(Object::FLD_CLIP_UNIT_XMID)) & 0x0F;
				int16_t tXmid = obj->GetField(Object::FLD_CLIP_TILE_XMID);
				int16_t tY2 = obj->GetField(Object::FLD_CLIP_TILE_Y2);

				for (int16_t y = g_oldRects.tileY2; tY2 + 1 >= y; ++y)
				{
					uint16_t tile = ResourceManager::GetTileAt(tXmid, y, 1);
					uint8_t topTI = ResourceManager::TileForeTop(tile);
					if (topTI)
					{
						int16_t slopeAmt = g_physSlopeHeight[topTI & 0x07][midTileXOffset];
						int16_t objYOffset = obj->GetField(Object::FLD_CLIP_UNIT_Y2) - (y * 256);
						int16_t dy = (slopeAmt - objYOffset) - 1;

						obj->SetField(Object::FLD_TOP_TI, topTI);
						PhysUpdateY(obj, dy);
						return;
					}
				}
			}
		}

		// Falling off platform reset
		if (obj->GetField(Object::FLD_TOP_TI) == 0 && wasNotOnPlatform)
		{
			obj->SetPosX(oldUnitX + g_nextX);
			obj->SetPosY(oldUnitY);
			ResetClipRects(obj);
		}

		obj->SetField(Object::FLD_DELTA_POS_X, obj->GetField(Object::FLD_DELTA_POS_X) + (int16_t)(obj->GetPosX() - oldUnitX));
		obj->SetField(Object::FLD_DELTA_POS_Y, obj->GetField(Object::FLD_DELTA_POS_Y) + (int16_t)(obj->GetPosY() - oldUnitY));
	}
}

void PhysUpdateSimpleObj(Object* obj)
{
	uint16_t oldUnitX = obj->GetPosX();
	uint16_t oldUnitY = obj->GetPosY();

	obj->SetPosX(obj->GetPosX() + g_nextX);
	obj->SetPosY(obj->GetPosY() + g_nextY);

	obj->SetVisible(1);

	if (obj->GetGfxChunk() != 0)
	{
		SetOldClipRects(obj);
		ResetClipRects(obj);

		if (obj->GetField(Object::FLD_CLIPPED) != 0)
		{
			SetDeltaClipRects(obj);
			PhysClipVert(obj);
			PhysClipHorz(obj);
		}

		obj->SetField(Object::FLD_DELTA_POS_X, obj->GetField(Object::FLD_DELTA_POS_X) + (int16_t)(obj->GetPosX() - oldUnitX));
		obj->SetField(Object::FLD_DELTA_POS_Y, obj->GetField(Object::FLD_DELTA_POS_Y) + (int16_t)(obj->GetPosY() - oldUnitY));
	}
}

void PhysPushX(Object* pushee, Object* pusher)
{
	int16_t deltaPosX = pusher->GetField(Object::FLD_DELTA_POS_X) - pushee->GetField(Object::FLD_DELTA_POS_X);
	int16_t deltaX_1 = pusher->GetField(Object::FLD_CLIP_UNIT_X2) - pushee->GetField(Object::FLD_CLIP_UNIT_X1);
	int16_t deltaX_2 = pushee->GetField(Object::FLD_CLIP_UNIT_X2) - pusher->GetField(Object::FLD_CLIP_UNIT_X1);

	Action* act = LookupActionFromDosPointer(pushee->GetField(Object::FLD_ACTION_PTR));
	bool stickToGround = act && act->stickToGround;

	// Push object to the right
	if ((deltaX_1 > 0) && (deltaX_1 <= deltaPosX))
	{
		g_nextX = deltaX_1;
		if (stickToGround)
			g_nextY = -deltaX_1 + 16;
		PhysUpdateNormalObj(pushee);
		pushee->SetField(Object::FLD_RIGHT_TI, 1);
		return;
	}

	// Push object to the left
	if ((deltaX_2 > 0) && (-deltaPosX >= deltaX_2))
	{
		g_nextX = -deltaX_2;
		if (stickToGround)
			g_nextY = deltaX_2 + 16;
		PhysUpdateNormalObj(pushee);
		pushee->SetField(Object::FLD_LEFT_TI, 1);
		return;
	}
}

void PhysPushY(Object* pushee, Object* pusher)
{
	int16_t deltaDeltaY = pushee->GetField(Object::FLD_DELTA_POS_Y) - pusher->GetField(Object::FLD_DELTA_POS_Y);
	int16_t deltaClipY = pushee->GetField(Object::FLD_CLIP_UNIT_Y2) - pusher->GetField(Object::FLD_CLIP_UNIT_Y1);

	if ((deltaClipY >= 0) && (deltaClipY <= deltaDeltaY))
	{
		if (pushee == g_keenObj)
			g_keenState.platform = pusher;

		g_nextY = -deltaClipY;
		Action* act = LookupActionFromDosPointer(pushee->GetField(Object::FLD_ACTION_PTR));
		bool stickToGround = act && act->stickToGround;
		PhysUpdateNormalObj(pushee);
		if (pushee->GetField(Object::FLD_BOTTOM_TI) == 0)
			pushee->SetField(Object::FLD_TOP_TI, 0x19);
	}
}

void PhysPushXY(Object* passenger, Object* platform, bool squish)
{
	int16_t dx = platform->GetField(Object::FLD_DELTA_POS_X) - passenger->GetField(Object::FLD_DELTA_POS_X);
	g_nextX = g_nextY = 0;
	int16_t dLeft = platform->GetField(Object::FLD_CLIP_UNIT_X2) - passenger->GetField(Object::FLD_CLIP_UNIT_X1);
	int16_t dRight = passenger->GetField(Object::FLD_CLIP_UNIT_X2) - platform->GetField(Object::FLD_CLIP_UNIT_X1);

	// Push in all four directions
	if (dLeft > 0 && dx + 1 >= dLeft)
	{
		g_nextX = dLeft;
		passenger->SetVelX(0);
		PhysUpdateSimpleObj(passenger);

		if (squish && passenger->GetField(Object::FLD_LEFT_TI))
		{
			// CK_KillKeen();
		}

		passenger->SetField(Object::FLD_RIGHT_TI, 1);
		return;
	}

	if (dRight > 0 && -dx + 1 >= dRight)
	{
		g_nextX = -dRight;
		passenger->SetVelX(0);
		PhysUpdateSimpleObj(passenger);

		if (squish && passenger->GetField(Object::FLD_RIGHT_TI))
		{
			// CK_KillKeen();
		}

		passenger->SetField(Object::FLD_LEFT_TI, 1);
		return;
	}

	int16_t dy = passenger->GetField(Object::FLD_DELTA_POS_Y) - platform->GetField(Object::FLD_DELTA_POS_Y);
	int16_t dTop = platform->GetField(Object::FLD_CLIP_UNIT_Y2) - passenger->GetField(Object::FLD_CLIP_UNIT_Y1);
	int16_t dBottom = passenger->GetField(Object::FLD_CLIP_UNIT_Y2) - platform->GetField(Object::FLD_CLIP_UNIT_Y1);

	if (dBottom >= 0 && dBottom <= dy)
	{
		if (passenger == g_keenObj)
			g_keenState.platform = platform;

		g_nextY = -dBottom;
		PhysUpdateSimpleObj(passenger);

		if (squish && passenger->GetField(Object::FLD_BOTTOM_TI))
		{
			// CK_KillKeen();
		}

		if (passenger->GetField(Object::FLD_BOTTOM_TI) == 0)
			passenger->SetField(Object::FLD_TOP_TI, 0x19);

		return;
	}

	if (dTop >= 0 && dTop <= dy)
	{
		g_nextY = dTop;
		PhysUpdateNormalObj(passenger);
		if (squish && passenger->GetField(Object::FLD_TOP_TI))
		{
			// CK_KillKeen();
		}

		passenger->SetField(Object::FLD_BOTTOM_TI, 0x19);
		return;
	}
}

void SetAction(Object* obj, Action* act)
{
	obj->SetField(Object::FLD_ACTION_PTR, act ? act->compatDosPointer : 0);

	if (act)
	{
		if (act->chunkRight && obj->GetXDirection() > 0)
			obj->SetGfxChunk(act->chunkRight);
		else if (act->chunkLeft)
			obj->SetGfxChunk(act->chunkLeft);

		if (obj->GetGfxChunk() == (uint16_t)-1)
			obj->SetGfxChunk(0);
	}

	g_nextX = 0;
	g_nextY = 0;

	// Save old clipping type
	uint16_t oldClipping = obj->GetField(Object::FLD_CLIPPED);
	obj->SetField(Object::FLD_CLIPPED, 0); // CLIP_not

	if (obj->GetField(Object::FLD_CLIPPED) == 1) // CLIP_simple
		PhysFullClipToWalls(obj);
	else
		PhysUpdateNormalObj(obj);

	obj->SetField(Object::FLD_CLIPPED, oldClipping);
}

void SetAction2(Object* obj, Action* act)
{
	obj->SetField(Object::FLD_ACTION_PTR, act ? act->compatDosPointer : 0);
	obj->SetField(Object::FLD_ACTION_TIMER, 0);

	if (act)
	{
		if (act->chunkRight)
		{
			obj->SetGfxChunk((obj->GetXDirection() > 0) ? act->chunkRight : act->chunkLeft);
		}

		if (obj->GetGfxChunk() == (uint16_t)-1)
			obj->SetGfxChunk(0);
	}

	obj->SetVisible(1);
	g_nextX = 0;
	g_nextY = 0;

	if (obj->GetField(Object::FLD_TOP_TI) != 0x19)
		PhysUpdateNormalObj(obj);
}

void PhysFullClipToWalls(Object* obj)
{
	uint16_t oldUnitX = obj->GetPosX();
	uint16_t oldUnitY = obj->GetPosY();

	if (g_nextX > 239)
		g_nextX = 239;
	else if (g_nextX < -239)
		g_nextX = -239;
	if (g_nextY > 239)
		g_nextY = 239;
	else if (g_nextY < -239)
		g_nextY = -239;

	obj->SetPosX(obj->GetPosX() + g_nextX);
	obj->SetPosY(obj->GetPosY() + g_nextY);
	obj->SetVisible(1);

	// Object class specific deltas (for Keen 4)
	int16_t delX = 0x280; // Default: Scubakeen
	int16_t delY = 0x180;

	obj->SetField(Object::FLD_CLIP_UNIT_X2, obj->GetPosX() + delX);
	obj->SetField(Object::FLD_CLIP_UNIT_X1, obj->GetPosX());
	obj->SetField(Object::FLD_CLIP_UNIT_Y1, obj->GetPosY());
	obj->SetField(Object::FLD_CLIP_UNIT_Y2, obj->GetPosY() + delY);
	obj->SetField(Object::FLD_CLIP_TILE_X1, UnitToTile(obj->GetField(Object::FLD_CLIP_UNIT_X1)));
	obj->SetField(Object::FLD_CLIP_TILE_X2, UnitToTile(obj->GetField(Object::FLD_CLIP_UNIT_X2)));
	obj->SetField(Object::FLD_CLIP_TILE_Y1, UnitToTile(obj->GetField(Object::FLD_CLIP_UNIT_Y1)));
	obj->SetField(Object::FLD_CLIP_TILE_Y2, UnitToTile(obj->GetField(Object::FLD_CLIP_UNIT_Y2)));

	// Reset tile clipping vars
	obj->SetField(Object::FLD_TOP_TI, 0);
	obj->SetField(Object::FLD_BOTTOM_TI, 0);
	obj->SetField(Object::FLD_LEFT_TI, 0);
	obj->SetField(Object::FLD_RIGHT_TI, 0);

	if (!NotStuckInWall(obj))
	{
		PhysUpdateX(obj, -g_nextX);
		if (NotStuckInWall(obj))
		{
			if (g_nextX > 0)
				obj->SetField(Object::FLD_LEFT_TI, 1);
			else
				obj->SetField(Object::FLD_RIGHT_TI, 1);
		}
		else
		{
			if (g_nextY > 0)
				obj->SetField(Object::FLD_TOP_TI, 1);
			else
				obj->SetField(Object::FLD_BOTTOM_TI, 1);

			PhysUpdateX(obj, g_nextX);
			PhysUpdateY(obj, -g_nextY);
			if (!NotStuckInWall(obj))
			{
				PhysUpdateX(obj, -g_nextX);
				if (g_nextX > 0)
					obj->SetField(Object::FLD_LEFT_TI, 1);
				else
					obj->SetField(Object::FLD_RIGHT_TI, 1);
			}
		}
	}

	obj->SetField(Object::FLD_DELTA_POS_X, obj->GetField(Object::FLD_DELTA_POS_X) + (int16_t)(obj->GetPosX() - oldUnitX));
	obj->SetField(Object::FLD_DELTA_POS_Y, obj->GetField(Object::FLD_DELTA_POS_Y) + (int16_t)(obj->GetPosY() - oldUnitY));

	// Don't call ResetClipRects here - tile rects should not be updated
	// Need sprite table entry - for now use placeholder
}

void CentreCamera(Object* obj)
{
	// TODO: Full implementation
}

void MapCamera(Object* obj)
{
	// TODO: Full implementation
}

void NormalCamera(Object* obj)
{
	// TODO: Full implementation
}

void UpdateActiveRect()
{
	// Update the rectangle of active objects
	g_activeX0Tile = max(UnitToTile(g_scrollXUnit) - 10, 0);
	g_activeX1Tile = max(UnitToTile(g_scrollXUnit) + PixelToTile(320) + 10, 0);
	g_activeY0Tile = max(UnitToTile(g_scrollYUnit) - 10, 0);
	g_activeY1Tile = max(UnitToTile(g_scrollYUnit) + PixelToTile(208) + 10, 0);
}

END_UPP_NAMESPACE