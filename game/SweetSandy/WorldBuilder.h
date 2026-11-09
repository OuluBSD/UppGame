#ifndef _SweetSandy_WorldBuilder_h_
#define _SweetSandy_WorldBuilder_h_

// World Builder phase, Task 3: wires Task 1's WorldOctree and Task 2's
// WorldChunkCache to the pluggable generator stage(s) in WorldGenStages.h,
// proving the end-to-end streaming pipeline shape with stub content
// before any real generator algorithm (city layout, road mesh,
// vegetation) is ported - see 001-planning.md for why those are their
// own later phases.
class WorldBuilder {
	WorldOctree octree;
	WorldChunkCache cache;
	One<ITerrainErosionGenerator> erosion_gen; // Terrain Erosion phase, Task 3 - runs *first*, before any stage below that reads Terrain::HeightAt
	One<ICityLayoutGenerator> city_gen;
	One<IRoadMeshGenerator> road_gen; // Road Network phase, Task 4 - runs after city_gen for the same chunk
	One<IVegetationGenerator> veg_gen; // Vegetation phase, Task 4 - runs after city_gen for the same chunk (needs road_centerlines to avoid roads)
	One<IBuildingGenerator> building_gen; // Buildings phase, Task 3 - runs after city_gen for the same chunk (needs blocks/road_centerlines)
	One<IGroundGenerator> ground_gen; // Unified Ground phase, Task 4 - runs after city_gen/building_gen for the same chunk (needs blocks)
	VectorMap<WorldChunkCoord, WorldChunkContent> content; // one entry per currently-loaded chunk

	// Map Editor phase, Task 9: every per-chunk generation (streaming
	// *and* paint-triggered regeneration) now runs on this one
	// background worker thread instead of inline on Update()'s own
	// caller - see 012-map-editor/009-task9-async-chunk-regeneration.md.
	// `worker` only ever holds *borrowed* pointers into cache/erosion_gen/
	// .../ground_gen above (see Update()'s own Start() call) - declared
	// below all of them so C++'s member destruction order (reverse of
	// declaration) joins the worker thread (~WorldChunkGenWorker calls
	// Stop()) before any of those borrowed-from members are destroyed.
	// Do not reorder these declarations.
	WorldChunkGenWorker worker;
	bool worker_started = false; // Start() needs the Set*Generator calls to have already run - see Update()'s own lazy-start comment
	// Coords currently submitted to `worker` with no result installed
	// yet (Update()'s own new-chunk loop must never submit the same
	// coord twice while it's in here).
	Vector<WorldChunkCoord> pending;
	// Subset of `pending` whose in-flight result, once it arrives, must
	// be discarded and resubmitted rather than installed - set by
	// InvalidateContentNear when it touches a chunk whose *previous*
	// request is still in flight (a real possibility now that
	// generation takes real wall-clock time); avoids double-submitting
	// the same coord while also never installing outdated content.
	Vector<WorldChunkCoord> needs_resubmit;

	// Pops every result currently sitting in `worker`'s own result
	// queue, installing it into `content` (or discarding+resubmitting
	// it, per `needs_resubmit` - see that member's own comment; or just
	// discarding it, if the chunk unloaded while it was generating).
	// Appends every coord actually installed this call to
	// `newly_installed_out` (may be NULL - Update()'s own test-callers
	// that ignore the return value don't need it).
	void DrainCompleted(Vector<WorldChunkCoord>* newly_installed_out);

public:
	// (Re)initializes this WorldBuilder for a fresh streaming session -
	// opens the cache and resets any previously-loaded chunks/content
	// (safe to call again on an already-used instance, e.g.
	// PlayScreen::StartRun picking a different track/seed).
	void Create(const String& seed_version);

	// Map Editor phase, Task 9: stops `worker` (blocking until its
	// thread has actually joined) without touching octree/content/cache
	// - unlike Create(), which also resets those, for a caller that
	// isn't necessarily about to start a fresh session right away.
	// PlayScreen::StopRun MUST call this (it does) before its own caller
	// (StartRun) goes on to call Terrain::Load() for a new track: Load()
	// reassigns several of Terrain's own fields with NO lock at all
	// (island/island_seed/erosion_cache/grid_w/grid_d/height_scale/
	// world_size/cell_size - only `height`/`loaded_erosion_tiles` are
	// covered by Terrain's own height_lock, see Terrain.h) - a still-
	// running worker thread from the *previous* run concurrently calling
	// HeightAt/EnsureErosionTile against the same Terrain instance while
	// Load() reassigns those fields is a real data race, not a
	// theoretical one (KillTimeCallback alone only stops the *main*
	// thread from calling Update() again; it does nothing to a worker
	// thread already running independently). Create() also stops the
	// worker, but too late for this purpose - by the time StartRun calls
	// it, Terrain::Load() has already run.
	void Stop();
	void SetErosionGenerator(One<ITerrainErosionGenerator>&& gen) { erosion_gen = pick(gen); }
	void SetCityLayoutGenerator(One<ICityLayoutGenerator>&& gen) { city_gen = pick(gen); }
	void SetRoadMeshGenerator(One<IRoadMeshGenerator>&& gen)     { road_gen = pick(gen); }
	void SetVegetationGenerator(One<IVegetationGenerator>&& gen) { veg_gen = pick(gen); }
	void SetBuildingGenerator(One<IBuildingGenerator>&& gen)     { building_gen = pick(gen); }
	void SetGroundGenerator(One<IGroundGenerator>&& gen)         { ground_gen = pick(gen); }

	// Overrides the default LOD levels Create() sets up - callers with a
	// known world scale (e.g. PlayScreen, matching Terrain::world_size)
	// can pick tighter distances than the generic defaults tuned only
	// for this phase's own self-tests. Call after Create().
	void SetLodLevels(const Vector<WorldOctree::LodLevel>& levels) { octree.SetLodLevels(levels); }

	// WorldOctree::UpdateStreaming, then drops content/pending-tracking
	// for chunks that just unloaded, submits a generation request (to
	// `worker`) for chunks that are loaded but don't have content yet
	// and aren't already pending, then drains+installs whatever results
	// are ready (see DrainCompleted). Map Editor phase, Task 9: no
	// longer generates inline - a newly-loaded chunk's content is not
	// necessarily ready the instant this call returns; see this task's
	// own planning doc. Returns the coords actually installed (given
	// real content, not just newly pending) this call - PlayScreen uses
	// this to know which chunks need their ground collider *refined*
	// (rebuilt from now-final, erosion-complete height data) in addition
	// to whatever rough/immediate collider it already built on first
	// load. Existing callers that ignore the return value are
	// unaffected.
	Vector<WorldChunkCoord> Update(Vec3 viewer_pos);

	const WorldOctree& GetOctree() const { return octree; }
	const WorldChunkContent* FindContent(const WorldChunkCoord& c) const;

	// Map Editor phase, Task 5 (now asynchronous - Task 9): for every
	// currently-loaded chunk (any LOD) whose bounds overlap
	// [center-half_extent, center+half_extent] (world x/z, y ignored) -
	// invalidates its disk cache entry synchronously (cheap - a file
	// delete) and submits a fresh generation request, so already-cached
	// ground/road/building/vegetation content reflects a Terrain change
	// (e.g. a compiled terrain-paint edit) once that request completes -
	// no longer immediately, now that generation runs on `worker` (see
	// this task's own planning doc; WaitIdle() below is what a test
	// needing the old immediate-completion guarantee should call).
	// Deliberately does not re-run the erosion generator stage - that's
	// about first-loading an erosion tile, already loaded by the time
	// anything would call this. A chunk that isn't currently loaded is
	// left alone - nothing cached to go stale; it generates fresh from
	// the already-updated Terrain data whenever it does load.
	void InvalidateContentNear(Vec2 center, Vec2 half_extent);

	// Map Editor phase, Task 9: test-only - blocks until every request
	// submitted so far (by Update() or InvalidateContentNear) has been
	// installed into `content` (following any needs_resubmit chain to
	// its real end). Never called from the real gameplay path - the
	// entire point of this task is that gameplay does *not* wait for
	// this.
	void WaitIdle();
};

#endif
