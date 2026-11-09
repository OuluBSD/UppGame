#ifndef _SweetSandy_MapGeneratorCityLayoutGenerator_h_
#define _SweetSandy_MapGeneratorCityLayoutGenerator_h_

// City Layout phase, Task 5: the real ICityLayoutGenerator, replacing
// phase 2's stub. Wires Tasks 1-4 together per chunk: builds a
// CityTensorField covering the chunk (Task 1), traces road-centerline
// streamlines over the chunk's local bounds (Task 2), builds a planar
// graph from them (Task 3), and extracts city-block polygons (Task 4) -
// see Manager/2-plan/.../003-city-layout/005-task5-worldbuilder-
// integration.md.
//
// Each *finest-level* (lod 0) chunk gets its own independent tensor
// field/streamline run (the task doc's "lean toward this for an initial
// working version" option) - visible seams where streamlines don't
// connect across a lod-0 chunk boundary are an accepted, fixable-later
// look for proving the port works end-to-end, not a blocker.
//
// A *coarser* LOD chunk does NOT get its own independent run - it is
// assembled from the same underlying lod-0 cells WorldOctree would use
// for that area if it were streaming at full detail there, merged into
// one WorldChunkContent (see Generate()'s own comment). Found via a
// real user report after Vegetation Task 4 shipped: retuning the octree
// LOD distances to actually be observable while driving (see AGENTS.md's
// "Post-Vegetation follow-up") made lod>=1 content load close enough to
// see clearly, which exposed that two LOD levels covering the same
// physical area previously showed two *unrelated* street networks
// (each level re-ran its own independent, differently-seeded tensor
// field over its own, differently-sized bounds) - visibly overlapping,
// crossing lines at LOD boundaries, not just a seam. "Same layout at
// every LOD" fixes that by construction: a coarse chunk's content is
// literally built from the identical per-lod0-cell generation a real
// lod0 chunk there would produce, not a coarser approximation of it.
//
// Bump this whenever a change anywhere in the per-chunk generation
// pipeline would produce *different* WorldChunkContent for the same
// (world_seed, chunk coord) than before - not just this file's own
// CityTensorField/streamline/graph/block algorithms or
// CityStreamlineParams tuning, but also whatever IRoadMeshGenerator
// WorldBuilder runs afterward for the same chunk (Road Network phase),
// since both generators' output lands in the same cached
// WorldChunkContent blob under this one key. Fold it into whatever
// seed_version string gets passed to WorldChunkCache (see
// PlayScreen::StartRun) - otherwise the cache keeps serving pre-change
// content forever, since WorldChunkCache has no way to know the
// generation pipeline changed if the caller doesn't tell it to via the
// key. Found the hard way three times now: first changing
// CityStreamlineParams::dsep 45 -> 15 and still seeing the old 45m
// layout; then adding Road Network Task 4's road_mesh field and
// RoadCreatorMeshGenerator without bumping this - cached chunks from
// before that change kept coming back with an empty road_mesh
// (city_gen/road_gen never even ran again); now bumped preemptively
// for Vegetation Task 4's own vegetation field and VegChunkGenerator,
// before ever capturing a first --citypic of it, specifically to skip
// a fourth repeat of the same debugging session. Bumped a fifth time
// (cgv4 -> cgv5) for a new *kind* of trigger, not a generator-algorithm
// change: PlayScreen::StartRun's own WorldOctree LOD cell sizes changed
// (retuned per the "octree LOD doesn't seem to stream with player
// position" investigation - see that commit). A chunk coordinate's
// (lod,x,y,z) tuple is only meaningful relative to the cell_size that
// produced it (WorldOctree::UpdateStreaming derives world-space center/
// half_extent from it at runtime, never stores them in the cache key) -
// changing cell_size means the *same* coordinate tuple now refers to a
// completely different physical area, so old cached content under that
// coordinate would be silently wrong for the new chunk layout if served
// again. Not a generator algorithm change, but squarely inside what
// this comment already says to bump for either way. Bumped a sixth time
// (cgv5 -> cgv6) for a real generator-algorithm change this time: coarser
// LOD chunks are no longer their own independent tensor-field run (see
// this file's own top-of-file comment) - a cached coarse chunk from
// before this change would keep showing the old, LOD-boundary-mismatched
// layout forever otherwise. Bumped a seventh time (cgv6 -> cgv7) for
// Buildings phase Task 3: WorldChunkContent's own serialized shape
// changed (a new `buildings` field) - a chunk cached before this change
// has no buildings data at all, not just stale content, so this needs
// the same "invalidate everything, no per-chunk migration" treatment
// this constant's own name already implies, not a special case.
// Bumped an eighth time (cgv7 -> cgv8) for Unified Ground phase
// Task 1: VegTreeInstance gained a `preset` field (which leaf texture
// to bind) - a chunk cached before this change deserializes into the
// enum's own zero value (Oak) regardless of what it actually was,
// silently mislabeling any cached Pine tree.
// Bumped a ninth time (cgv8 -> cgv9) for Unified Ground phase Task 2:
// WorldBuilder's own generator call order changed (building_gen now
// runs before veg_gen) and VegChunkGenerator gained a building-
// avoidance rejection rule - a chunk cached before this change may
// have trees placed inside buildings, the exact bug being fixed, so
// old cached content must not be served again.
// Bumped a tenth time (cgv9 -> cgv10) for Unified Ground phase Task 4:
// WorldChunkContent gained a `ground` field (the real per-block/
// fallback grass meshes) - a chunk cached before this change has no
// ground data at all, not just stale content.
#define CITY_GEN_VERSION "cgv10"

class MapGeneratorCityLayoutGenerator : public ICityLayoutGenerator {
	String world_seed;
	double finest_cell_size; // only read for coord.lod > 0 requests - see Generate()

	// The actual tensor-field/streamline algorithm for one lod-0-sized
	// cell's own bounds, seeded by that cell's own coordinate - the
	// whole of what Generate() used to do directly, unchanged in
	// substance, just factored out so it can run once (a real lod-0
	// chunk) or several times and get merged (a coarser chunk covering
	// several such cells).
	void GenerateCell(const WorldChunkCoord& cell_coord, Vec2 cell_centre, Vec2 cell_world_dims,
	                   Vector<Vector<Vec2>>& out_road_centerlines) const;

public:
	// world_seed drives which layout a given chunk coordinate generates
	// (a different seed is a different city, same chunk coordinate
	// notwithstanding) - independent of WorldChunkCache's own
	// seed_version, which only controls *cache* invalidation. Pass the
	// same string to both if "changing the seed regenerates everything"
	// is the desired behavior (the normal case).
	//
	// finest_cell_size must match whatever cell_size the caller's own
	// WorldOctree uses for lod 0 (PlayScreen::StartRun's own
	// WorldOctree::LodLevel - pass levels[0].cell_size there, not a
	// separately-picked number) - it is how a coarser chunk figures out
	// which lod-0 cells to assemble itself from. Defaults to this
	// project's own current lod-0 value for callers (mostly tests) that
	// only ever request coord.lod == 0, where it has no effect at all.
	explicit MapGeneratorCityLayoutGenerator(const String& world_seed, double finest_cell_size = 30.0)
		: world_seed(world_seed), finest_cell_size(finest_cell_size) {}

	void Generate(const WorldChunk& chunk, WorldChunkContent& content) override;
};

#endif
