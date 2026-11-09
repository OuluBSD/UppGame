#ifndef _SweetSandy_ErosionTileCache_h_
#define _SweetSandy_ErosionTileCache_h_

// Bump whenever ErosionTile's own on-disk shape or the baking/erosion
// algorithm/parameters change in a way that makes previously-cached
// tiles wrong to keep serving - same "invalidate everything, no
// per-tile migration" convention CITY_GEN_VERSION already established
// for WorldChunkCache. Terrain::Load combines this into the cache's
// own seed_version (Terrain.cpp).
// Bumped to egv2 for Terrain Erosion phase, Task 4: added edge-
// blending toward the raw (pre-erosion) height near each tile's own
// border - a tile cached before this fix has a real physical cliff at
// its own edge where it meets any neighboring tile (each tile erodes
// independently, with zero knowledge of what the neighbor's own
// droplets did at the very same shared boundary line) - found via a
// real vehicle rolling over in 0.14m instead of clearing 500m right
// next to the world origin, which sits on a tile-grid corner.
#define EROSION_GEN_VERSION "egv2"

// Terrain Erosion phase, Task 1: disk cache for baked (and, from Task 2
// on, eroded) ErosionTiles - same DiskBlobCache-backed, seed-keyed,
// generate-once-reuse-forever shape WorldChunkCache already established
// for city content.
class ErosionTileCache {
	DiskBlobCache cache;

public:
	// Opens <AppRegistry cache dir>/erosion/<seed_version>/. A different
	// seed_version is a different cache root, same convention
	// WorldChunkCache::Open already uses.
	void Open(const String& seed_version);

	bool Load(const ErosionTileCoord& c, ErosionTile& out) const;
	void Save(const ErosionTileCoord& c, const ErosionTile& tile);

private:
	static String KeyFor(const ErosionTileCoord& c);
};

#endif
