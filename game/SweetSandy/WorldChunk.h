#ifndef _SweetSandy_WorldChunk_h_
#define _SweetSandy_WorldChunk_h_

// Identifies one chunk: its LOD level (0 = finest/nearest) and its grid
// coordinate *at that level's cell size* (see WorldOctree::LodLevel::cell_size).
// y is carried for a genuine 3D octree (see AGENTS.md/the plan for why this
// is a real octree, not a quadtree wearing its name) but Task 1's streaming
// is 2.5D (over the heightmap x/z plane) and always uses y=0 - a later
// phase with real vertical content (multi-level buildings, stacked
// structures) is what would start varying it.
struct WorldChunkCoord : Moveable<WorldChunkCoord> {
	int lod = 0;
	int x = 0, y = 0, z = 0;

	bool operator==(const WorldChunkCoord& o) const {
		return lod == o.lod && x == o.x && y == o.y && z == o.z;
	}
};

inline hash_t GetHashValue(const WorldChunkCoord& c) {
	return CombineHash(c.lod, c.x, c.y, c.z);
}

enum WorldChunkState { CHUNK_UNLOADED, CHUNK_LOADED };

// A streamed chunk. Content is deliberately empty in this task - Task 3
// adds a real (if still stub) payload once generator stages exist;
// WorldOctree itself only manages *which* chunks are loaded/unloaded and
// their bounds.
struct WorldChunk : Moveable<WorldChunk> {
	WorldChunkCoord coord;
	Vec3 center, half_extent; // world-space bounds
	WorldChunkState state = CHUNK_UNLOADED;
};

// Which chunk a world x/z position falls into at a given cell_size and
// lod - matches WorldOctree::UpdateStreaming's own grid math exactly
// (floor(world / cell_size)). Shared here (not duplicated) because a
// second real consumer now needs the identical computation and staying
// in sync is load-bearing: MapGeneratorCityLayoutGenerator derives a
// coarser LOD chunk's content from the *same* finest-level cells
// WorldOctree itself would use for a real lod0 chunk there (see
// 003-city-layout/AGENTS.md's "post-Vegetation follow-up" entry) - if
// this math ever drifted from WorldOctree's own, a coarse chunk's
// derived layout would silently stop matching what lod0 actually shows.
inline WorldChunkCoord WorldChunkCoordAt(double world_x, double world_z, double cell_size, int lod)
{
	WorldChunkCoord c;
	c.lod = lod;
	c.x = (int)floor(world_x / cell_size);
	c.y = 0;
	c.z = (int)floor(world_z / cell_size);
	return c;
}

inline Vec3 WorldChunkCenterAt(const WorldChunkCoord& c, double cell_size)
{
	return Vec3((c.x + 0.5) * cell_size, 0, (c.z + 0.5) * cell_size);
}

#endif
