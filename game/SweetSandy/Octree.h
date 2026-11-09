#ifndef _SweetSandy_Octree_h_
#define _SweetSandy_Octree_h_

// Chunk spatial index + distance-based LOD streaming, over the heightmap
// x/z plane (see WorldChunkCoord). Each LOD level has its own cell size and
// load distance; UpdateStreaming assigns every point in range to exactly
// one LOD level (the finest whose load distance covers it) so chunks at
// different levels never overlap the same area. Loosely mirrors ez-tree's
// generateLODs() distance/hysteresis levels (see the plan's research notes)
// applied to whole world chunks instead of one tree's mesh detail.
//
// Simplification, deliberate for this task (see the plan / AGENTS.md): to
// find which chunks should be loaded, UpdateStreaming iterates the *finest*
// LOD level's grid out to the *coarsest* level's load distance - O((range/
// finest_cell)^2) per call. Fine at this task's scale (a few hundred
// chunks); a real top-down octree traversal (recurse coarse-to-fine, only
// descending where needed) is the natural optimization if/when a much
// larger load distance makes this too slow - not needed yet.
//
// Named WorldOctree, not Octree: uppsrc/Geometry/Octree.h already defines
// an unrelated Upp::Octree class, and this package links against Geometry
// transitively. Two same-named same-namespace classes with different
// layouts is an ODR violation - it compiled and linked cleanly but produced
// garbage `this` pointers (a SIGSEGV reading a double as a pointer) at
// runtime, since calls could bind to the wrong class's method depending on
// which translation unit's definition won. See AGENTS.md for the story.
class WorldOctree {
public:
	struct LodLevel : Moveable<LodLevel> {
		double cell_size = 1;
		double load_distance = 1; // chunks farther than this (at this level) are not loaded
	};

private:
	Vector<LodLevel> levels; // index = lod (0 = finest); ascending cell_size and load_distance
	VectorMap<WorldChunkCoord, WorldChunk> loaded;
	double unload_hysteresis = 1.15; // unload only past load_distance * this, avoids boundary thrashing

public:
	// levels[i].load_distance must be ascending (levels[0] smallest). Chunks
	// beyond levels.Top().load_distance are never streamed.
	void SetLodLevels(const Vector<LodLevel>& levels);
	void SetUnloadHysteresis(double factor) { unload_hysteresis = factor; }

	void UpdateStreaming(Vec3 viewer_pos);

	// Drops every currently-loaded chunk (does not touch levels/
	// hysteresis) - for (re)starting a fresh streaming session, e.g.
	// PlayScreen::StartRun picking a different track/seed after an
	// earlier run already streamed chunks in.
	void Clear() { loaded.Clear(); }

	int  GetLoadedChunkCount() const { return loaded.GetCount(); }
	const WorldChunk* FindChunk(const WorldChunkCoord& c) const;
	const WorldChunk& GetLoadedChunk(int i) const { return loaded[i]; }

	// Counters for tests (--selftest checks these instead of needing a
	// screenshot, per the project's established headless-first approach).
	int load_events = 0, unload_events = 0;
};

#endif
