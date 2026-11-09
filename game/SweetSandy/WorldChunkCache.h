#ifndef _SweetSandy_WorldChunkCache_h_
#define _SweetSandy_WorldChunkCache_h_

// World Builder phase, Task 2: disk cache for generated WorldChunk content
// ("mielellään eri LOD blokit olisi cachettu kovalevylle"). This class only
// knows how to turn a WorldChunkCoord + seed_version into a cache key - the
// actual directory layout / LRU eviction mechanics live in
// uppsrc/Core/DiskBlobCache.h, since none of that is specific to this game.
//
// `data` is an opaque blob for this task - no real generator exists yet
// (that's Task 3+); this only proves read/write/evict works.
class WorldChunkCache {
	DiskBlobCache cache;

public:
	// Opens <AppRegistry cache dir>/world/<seed_version>/. A different
	// seed_version is a different cache root, so changing the world seed
	// or a generator's algorithm version automatically stops old cached
	// content from being read as if it were still valid.
	void Open(const String& seed_version);

	bool Load(const WorldChunkCoord& c, String& data_out) const;
	void Save(const WorldChunkCoord& c, const String& data);
	// Map Editor phase, Task 5: removes one chunk's cached entry - a
	// later Load() for the same coord misses, forcing a real
	// regeneration instead of silently serving stale content (e.g.
	// after a terrain paint edit changed the Terrain data this
	// chunk's own content was generated from).
	void Invalidate(const WorldChunkCoord& c);

	int64 EvictLru(int64 max_total_bytes) { return cache.EvictLru(max_total_bytes); }
	int64 GetTotalSize() const            { return cache.GetTotalSize(); }

private:
	static String KeyFor(const WorldChunkCoord& c);
};

#endif
