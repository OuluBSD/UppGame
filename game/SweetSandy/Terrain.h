#ifndef _SweetSandy_Terrain_h_
#define _SweetSandy_Terrain_h_

// Heightmap-based terrain: one grid of world-space heights drives both the
// immediate-mode GL render (DrawGL) and the ODE collision geom
// (BuildOdeHeightfield) - a single source of truth, per the plan.
class Terrain {
	int           grid_w = 0, grid_d = 0; // sample counts along x and z
	Vector<float> height;                 // grid_w * grid_d, row-major, meters
	double        world_size = 200;       // meters, both x and z extent
	double        cell_size = 1;          // world_size / (grid_w - 1)
	double        height_scale = 12;      // meters, used to normalize colors

	// The currently-live heightfield geom, if any - Road Network phase,
	// Task 4: BuildOdeHeightfield is now safe to call repeatedly within
	// the same still-alive space (a chunk's roads flatten more of the
	// grid, so collision needs rebuilding to reflect it), destroying
	// the previous live geom first. Reset to 0 (without touching the
	// physics backend) by Load() - a fresh Load() means the space that
	// owned any earlier geom is presumed gone (PlayScreen::StopRun/
	// pw.Destroy already freed it - see PhysBackend's own
	// PhysDestroySpace, which frees a heightfield geom's own height
	// data as part of destroying its owning space), so that old ID must
	// never be destroyed here. Physics Abstraction Task 2: the paired
	// height-data lifetime (formerly this class's own hf_data member)
	// is now owned by PhysBackend itself - see PhysCreateHeightfield/
	// PhysDestroyGeom/PhysDestroySpace.
	PhysGeomId hf_geom = 0;

	// Island Terrain phase, Task 4: when set (TrackDef.kind ==
	// TERRAIN_ISLAND), HeightAt/NormalAt delegate to this instead of the
	// baked grid above - a stateless procedural height function has no
	// grid to bake, and no "outside" for it to flatten at (the whole
	// reason this phase exists - see Manager/2-plan/.../
	// 010-island-terrain/001-planning.md's "how this coexists with the
	// existing Terrain class"). The small hand-authored test tracks
	// (default/obstacle_course) never set this - zero behavior change
	// for them.
	One<IslandTerrain> island;
	dword island_seed = 0; // set alongside `island` in Load() - reused to seed each erosion tile deterministically (see EnsureErosionTile)

	// Terrain Erosion phase, Task 3: eroded tiles currently held in
	// memory (loaded from disk, or baked+eroded+saved if this is the
	// first time this tile has ever been visited for this seed) -
	// HeightAt prefers a loaded tile over `island`'s own raw function
	// wherever one covers the query point. Never evicted - at ~264KB
	// per tile, even a hundred loaded at once is a non-issue at this
	// project's current scale; revisit only if that ever changes (see
	// 011-terrain-erosion/003-task3-pipeline-wiring.md's own note).
	VectorMap<ErosionTileCoord, ErosionTile> loaded_erosion_tiles;
	ErosionTileCache erosion_cache; // disk-backed, opened alongside island mode in Load()

	// Map Editor phase, Task 9: protects `height` and
	// `loaded_erosion_tiles` - both now read (HeightAt, every frame,
	// from both the main thread and the background world-gen worker
	// thread) and written (EnsureErosionTile/ApplyPaintedHeight/
	// FlattenUnderRibbon, from either thread) concurrently, once chunk
	// generation moved off the main thread. Multiple simultaneous
	// readers never block each other - only the rare writes (a tile's
	// first bake, a paint edit, road flattening) take the exclusive
	// path. See each locked function's own comment for exactly what's
	// held and why (in particular: the expensive erosion bake itself
	// runs with NO lock held at all - only the final, fast install is
	// synchronized - see EnsureErosionTile).
	mutable RWMutex height_lock;

	double HeightAtCell(int gx, int gz) const;

public:
	Terrain() {}
	Terrain(const Terrain&) = delete;
	void operator=(const Terrain&) = delete;

	// Loads def.heightmap_path via StreamRaster (grayscale -> meters using
	// def.height_scale). Falls back to a deterministic procedural bumpy
	// surface when the file does not exist, so the demo never fails to
	// start for a missing asset.
	void Load(const TrackDef& def);

	double GetWorldSize() const { return world_size; }

	// Island Terrain phase, Task 4.
	bool   IsIsland() const { return island; }
	// Only meaningful when IsIsland() - the fixed sea level a
	// translucent water plane should be drawn at (PlayScreen::GLPaint's
	// own DrawWaterGL call).
	double GetWaterLevel() const;

	// Terrain Erosion phase, Task 3: loads (from disk) or bakes+erodes+
	// saves (if missing) the tile at coord, keeping it in memory for
	// fast repeated HeightAt sampling - only meaningful when IsIsland().
	// A no-op if this exact coord is already loaded.
	void EnsureErosionTile(ErosionTileCoord coord);
	// Ensures every erosion tile overlapping [center-half_extent,
	// center+half_extent] (world x/z, y ignored) is loaded - checks all
	// 4 corners of the AABB, not just its center, since a WorldChunk can
	// straddle a tile edge near the boundary. A no-op when !IsIsland().
	void EnsureErosionCoverage(Vec2 center, Vec2 half_extent);
	// Test-only accessor - confirms EnsureErosionCoverage's own corner
	// checking actually loads every tile a chunk's AABB overlaps, not
	// just the one under its center.
	int GetLoadedErosionTileCount() const { return loaded_erosion_tiles.GetCount(); }

	// world_x/world_z are in [-world_size/2, world_size/2] for the
	// small baked-grid tracks; safely evaluable *anywhere* when
	// IsIsland() (see `island`'s own comment above).
	double HeightAt(double world_x, double world_z) const;
	Vec3   NormalAt(double world_x, double world_z) const;

	void DrawGL() const;

	// Safe to call more than once within the same still-alive space (see
	// hf_geom's own comment) - destroys the previous live geom first, so
	// callers don't accumulate stale heightfield geoms in the space.
	PhysGeomId BuildOdeHeightfield(PhysSpaceId space);

	// Road Network phase, Task 2: flattens this Terrain's own heightmap
	// grid under one generated road's footprint, so the ground actually
	// follows the road instead of the road floating disconnected above/
	// through a bumpy heightmap - see Manager/2-plan/.../
	// 004-road-network/002-task2-terrain-conforming.md. Call once, at
	// generation time - not every frame - and before BuildOdeHeightfield
	// if collision should reflect it (BuildOdeHeightfield copies the grid
	// once into ODE's own storage; re-flattening after that call does not
	// retroactively update an already-built collision geom - a Task 4
	// concern, not solved here). Island Terrain phase, Task 4: a no-op
	// when IsIsland() - there is no grid to mutate, and roads there are
	// meant to simply follow the height function's own contour (see
	// 010-island-terrain/001-planning.md's own note on this).
	void FlattenUnderRibbon(const Vector<Vec2>& centerline, const RoadMesh& mesh);

	// Map Editor phase, Task 4: writes one painted height value into
	// the loaded erosion tile covering (world_x, world_z) - "compiling"
	// TerrainPaintAction's own ActionGraph store into the same in-
	// memory tile cache HeightAt already prefers, so any HeightAt
	// caller (rendering, physics, other generators) sees the edit with
	// no changes to HeightAt itself. Ensures a covering tile is loaded
	// first (baking+eroding it if this area has never been touched at
	// all). Only meaningful when IsIsland() - the reverse of
	// FlattenUnderRibbon's own condition, since only island mode has a
	// loaded ErosionTile to mutate - a no-op otherwise. Does not by
	// itself update any already-built live ODE heightfield collider -
	// PlayScreen::RebuildGroundCollidersNear (Map Editor phase, Task 7)
	// is what keeps those in sync after a call here.
	void ApplyPaintedHeight(double world_x, double world_z, double new_height);

	// Map Editor phase, Task 6: marches a ray (dir assumed normalized)
	// against this terrain's own HeightAt, stopping at the first sign
	// change in (point.y - HeightAt(point.x, point.z)) - correct for a
	// heightfield (no overhangs to miss), refined by a short bisection
	// for sub-step precision. Returns false if no crossing is found
	// within max_dist (e.g. the ray points away from the ground
	// entirely). The reusable, camera-agnostic core behind screen-space
	// mouse picking (see PlayScreen::RaycastFromScreen) - kept here,
	// not PlayScreen-private, so it's directly unit-testable with a
	// real Terrain and no window/camera machinery at all.
	bool RaycastHeightAt(Vec3 origin, Vec3 dir, double max_dist, Vec3& hit_out) const;
};

#endif
