#ifndef _Platformer2D_Resource_Map_h_
#define _Platformer2D_Resource_Map_h_

#include <game/Platformer2D/Platformer2D.h>
#include <game/Platformer2D/Resource/Resource.h>

NAMESPACE_UPP

inline constexpr int MAP_WIDTH = 64;  // Tiles (original Keen maps are 64x64 tiles)
inline constexpr int MAP_HEIGHT = 64;
inline constexpr int TILE_SIZE = 16;  // 16x16 pixels per tile

// ============================================================================
// Tile Info (from TILEINFO.CKx)
// ============================================================================

struct TileInfo
{
	uint16_t left;    // Left wall
	uint16_t right;   // Right wall
	uint16_t top;     // Top wall
	uint16_t bottom;  // Bottom wall
	uint16_t floor;   // Floor type
	uint16_t ceil;    // Ceiling type
	uint16_t infoplane; // Info plane value
	uint16_t flags;   // Tile flags (anim, deadly, etc.)
	
	TileInfo() { Zero(*this); }
};

inline constexpr int MAX_TILE_INFOS = 400; // Typical max

// ============================================================================
// Map Header (from MAPHEAD.CKx)
// ============================================================================

struct MapHeader
{
	uint32_t planeOffsets[4]; // Offsets in GAMEMAPS for each plane
	uint16_t planeLengths[4]; // Compressed lengths
	uint16_t width;           // Map width in tiles (usually 64)
	uint16_t height;          // Map height in tiles (usually 64)
	char name[16];            // Map name (null-terminated)
	
	MapHeader() { Zero(*this); }
};

// ============================================================================
// Map Data Container
// ============================================================================

class GameMap
{
public:
	GameMap();
	~GameMap();
	
	bool Load(int mapNum);
	void Unload();
	
	// Tile access (in tile coordinates)
	uint16_t GetTile(MapPlane plane, int x, int y) const;
	void SetTile(MapPlane plane, int x, int y, uint16_t tile);
	
	// Pixel access (for collision)
	bool IsSolidPixel(int px, int py) const;
	bool IsDeadlyPixel(int px, int py) const;
	
	// Map info
	const MapHeader& GetHeader() const { return header; }
	const char* GetName() const { return header.name; }
	int GetWidth() const { return header.width; }
	int GetHeight() const { return header.height; }
	bool IsLoaded() const { return loaded; }
	
	// Tile info lookup
	const TileInfo* GetTileInfo(uint16_t tileNum) const;
	
private:
	MapHeader header;
	uint16_t* planes[4] = { nullptr, nullptr, nullptr, nullptr };
	Vector<TileInfo> tileInfos;
	bool loaded = false;
	
	bool LoadPlane(MapPlane plane, const uint8_t* compressed, size_t compressedSize);
	bool LoadTileInfo();
};

// ============================================================================
// Global Map Access
// ============================================================================

extern GameMap g_currentMap;

bool MAP_LoadMap(int mapNum);
void MAP_UnloadMap();
const GameMap* MAP_GetCurrentMap();

END_UPP_NAMESPACE

#endif // _Platformer2D_Resource_Map_h_