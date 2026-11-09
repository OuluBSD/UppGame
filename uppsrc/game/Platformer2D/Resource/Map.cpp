#include "Map.h"
#include "Resource.h"
#include "Decompress.h"
#include "Cache.h"

NAMESPACE_UPP

GameMap g_currentMap;

GameMap::GameMap()
{
	for (int i = 0; i < 4; ++i) planes[i] = nullptr;
}

GameMap::~GameMap()
{
	Unload();
}

bool GameMap::Load(int mapNum)
{
	Unload();
	
	LOG("GameMap::Load: map " << mapNum << " - Not fully implemented");
	
	// TODO: Load from GAMEMAPS using ResourceManager
	// 1. Read MAPHEAD entry for mapNum
	// 2. Load each plane (Carmack + RLEW decompression)
	// 3. Load TILEINFO
	
	loaded = false;
	return false;
}

void GameMap::Unload()
{
	for (int i = 0; i < 4; ++i)
	{
		if (planes[i])
		{
			MemoryFree(planes[i]);
			planes[i] = nullptr;
		}
	}
	tileInfos.Clear();
	loaded = false;
}

uint16_t GameMap::GetTile(MapPlane plane, int x, int y) const
{
	if (!loaded) return 0;
	int p = (int)plane;
	if (p < 0 || p >= 4 || !planes[p]) return 0;
	if (x < 0 || x >= header.width || y < 0 || y >= header.height) return 0;
	return planes[p][y * header.width + x];
}

void GameMap::SetTile(MapPlane plane, int x, int y, uint16_t tile)
{
	if (!loaded) return;
	int p = (int)plane;
	if (p < 0 || p >= 4 || !planes[p]) return;
	if (x < 0 || x >= header.width || y < 0 || y >= header.height) return;
	planes[p][y * header.width + x] = tile;
}

bool GameMap::IsSolidPixel(int px, int py) const
{
	// Convert pixel to tile coordinates
	int tx = px / TILE_SIZE;
	int ty = py / TILE_SIZE;
	
	// Check collision plane
	uint16_t tile = GetTile(MAP_PLANE_COLLISION, tx, ty);
	if (tile == 0) return false;
	
	// Check tile info for solidity
	const TileInfo* info = GetTileInfo(tile);
	if (!info) return true; // Default to solid
	
	// Check if pixel is within tile bounds
	int tilePx = px % TILE_SIZE;
	int tilePy = py % TILE_SIZE;
	
	// Simplified: check tile flags
	return (info->flags & 0x1) != 0; // Assuming bit 0 = solid
}

bool GameMap::IsDeadlyPixel(int px, int py) const
{
	int tx = px / TILE_SIZE;
	int ty = py / TILE_SIZE;
	
	uint16_t tile = GetTile(MAP_PLANE_INFO, tx, ty);
	const TileInfo* info = GetTileInfo(tile);
	if (!info) return false;
	
	return (info->flags & 0x2) != 0; // Assuming bit 1 = deadly
}

const TileInfo* GameMap::GetTileInfo(uint16_t tileNum) const
{
	if (tileNum < tileInfos.GetCount())
		return &tileInfos[tileNum];
	return nullptr;
}

bool GameMap::LoadPlane(MapPlane plane, const uint8_t* compressed, size_t compressedSize)
{
	int p = (int)plane;
	if (p < 0 || p >= 4) return false;
	
	// Allocate plane buffer
	size_t planeWords = header.width * header.height;
	planes[p] = (uint16_t*)MemoryAlloc(planeWords * sizeof(uint16_t));
	if (!planes[p]) return false;
	
	// Decompress: Carmack then RLEW
	uint8_t* carmackOut = (uint8_t*)MemoryAlloc(planeWords * 2);
	if (!carmackOut)
	{
		MemoryFree(planes[p]);
		planes[p] = nullptr;
		return false;
	}
	
	DecompressInfo info = CarmackDecoder::Decode(compressed, compressedSize, (uint16_t*)carmackOut, planeWords);
	if (info.result != DecompressResult::OK)
	{
		MemoryFree(carmackOut);
		MemoryFree(planes[p]);
		planes[p] = nullptr;
		return false;
	}
	
	// Then RLEW decode
	info = RLEWDecoder::DecodeBytes(carmackOut, info.bytesWritten, planes[p], planeWords);
	MemoryFree(carmackOut);
	
	return info.result == DecompressResult::OK;
}

bool GameMap::LoadTileInfo()
{
	// TODO: Load from TILEINFO.CKx chunk
	tileInfos.SetCount(MAX_TILE_INFOS);
	return true;
}

bool MAP_LoadMap(int mapNum)
{
	return g_currentMap.Load(mapNum);
}

void MAP_UnloadMap()
{
	g_currentMap.Unload();
}

const GameMap* MAP_GetCurrentMap()
{
	return g_currentMap.IsLoaded() ? &g_currentMap : nullptr;
}

END_UPP_NAMESPACE