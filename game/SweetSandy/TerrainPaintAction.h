#ifndef _SweetSandy_TerrainPaintAction_h_
#define _SweetSandy_TerrainPaintAction_h_

// Map Editor phase, Task 3: the first real ActionGraph action type -
// a terrain brush stroke. See Manager repo
// 012-map-editor/003-task3-terrain-paint-action.md for the full
// design. Deliberately does NOT touch Terrain.h/.cpp - the paint
// action's own execute() calls the real, existing Terrain::HeightAt
// as its fallback baseline, but Terrain's own runtime behavior is
// unchanged by this task (wiring the overlay into live HeightAt calls
// so the game/editor view actually reflects a stroke is Task 4's job).

// Query points are quantized to this fixed grid - deliberately the
// same spacing ErosionTile itself bakes at, so painted points line up
// with the real underlying data instead of an arbitrary offset, and
// (just as importantly) so two strokes whose circles overlap actually
// land on a *shared* ActionGraph key and get a real captured
// dependency, rather than two near-miss keys that look independent.
extern const double kPaintGridStep;

int    PaintGridIndex(double world_coord);
String PaintHeightKey(int ix, int iz);

// Map Editor phase, Task 4's own fix: the *pristine*, pre-paint height
// at each grid node, cached once per node the first time ANY action
// touches it - session-scoped (one instance shared across every
// AppendTerrainPaint call in an editing session, not per-action).
// Necessary because CompileTerrainPaintToTerrain mutates the real
// Terrain in place - without this cache, a stroke's own execute()
// would fall back to Terrain::HeightAt on replay (its own ActionGraph
// store entry gets cleared first, see ActionGraph::ExecuteOne), and
// once compiled at least once, that call would read back the stroke's
// *own* previously-compiled output instead of the true original
// terrain, compounding on every edit+recompile cycle instead of
// recomputing fresh. See 004-task4-integration-verification.md's own
// report for how this was actually found.
typedef VectorMap<String, double> PaintBaseHeightCache;

struct TerrainPaintBrush {
	double cx = 0, cz = 0;      // world-space brush center
	double radius = 8.0;        // world meters
	double delta = 1.0;         // RAISE mode (flatten=false): height offset added at the center, linear falloff to 0 at radius
	double slope_limit = 10.0;  // max allowed |center-neighbor| / kPaintGridStep (see fit_check)

	// Map Editor phase, Task 17: "manual flat city area with brush to
	// terrain" - the user's own next pipeline stage, after terrain
	// generation (Task 13/15). A flatten brush blends the surface toward
	// one absolute world-Y (`delta`, reinterpreted - see below) instead
	// of adding an offset to whatever's already there, exactly what
	// leveling a building/city site needs. Kept inside the same
	// TerrainPaintBrush/AppendTerrainPaint machinery (not a new action
	// type) since the ONLY thing that changes is what execute() writes
	// per node - center/radius/falloff/fit_check are all identical.
	// `delta` doubles as the absolute target world-Y when flatten=true
	// (not a second field) - deliberately reusing it rather than adding
	// `target_height` alongside: every existing caller (self-tests,
	// PlayScreen's live brush, TerrainPaintStore) already sets `delta`
	// and leaves `flatten` at its default false, so this is a strictly
	// additive, non-breaking extension.
	bool flatten = false;

	Value ToValue() const;
	static TerrainPaintBrush FromValue(const Value& v);
};

// Map Editor phase, Task 17: "erosion again (slight), to make flat city
// area natural" - the user's own next pipeline stage after flattening.
// Reuses the SAME real hydraulic-erosion algorithm Terrain Erosion
// phase Task 2 already ported (HydraulicErosion.h's own
// ErodeHeightmap) - not a fake/approximated "roughen this up" noise
// pass - against a small local window of the paint grid, so a flatten
// stroke's own unnaturally perfect plane gets real erosion texture
// instead of staying a mathematically flat plate.
struct TerrainErosionBrush {
	double cx = 0, cz = 0;   // world-space center
	double radius = 20.0;    // world meters
	int    droplets = 150;   // ErodeHeightmap's own num_droplets - "slight" is a low droplet count, not a parameter change
	dword  seed = 1;         // deterministic - same seed replays identically (LoadIslandTerrainWithPaint's own "restart every time" contract)

	Value ToValue() const;
	static TerrainErosionBrush FromValue(const Value& v);
};

// Appends and immediately executes a new terrain-paint action against
// `graph`. `terrain` and `base_cache` are captured by reference into
// the stored execute/fit_check closures - both must outlive graph's
// use of this action (same lifetime contract WorldBuilder's own
// generators already place on their Terrain&); `base_cache` should be
// the *same* instance across every AppendTerrainPaint call in one
// editing session, so overlapping strokes agree on the same pristine
// baseline.
ActionId AppendTerrainPaint(ActionGraph& graph, Terrain& terrain, PaintBaseHeightCache& base_cache,
                             const TerrainPaintBrush& brush);

// Edits an existing terrain-paint action's brush and replays it plus
// its transitive dependents (see ActionGraph::SetParam). Returns ids
// that failed their fit_check this pass.
Vector<ActionId> SetTerrainPaintBrush(ActionGraph& graph, ActionId id, const TerrainPaintBrush& brush);

// Map Editor phase, Task 17: same shape/contract as AppendTerrainPaint
// (captured dependencies via ReadGridHeight, same fit_check slope
// guard - see TerrainPaintAction.cpp's own shared CheckPaintSlope) but
// execute() runs a real ErodeHeightmap pass over the brush's own local
// window instead of the additive-circle formula.
ActionId AppendTerrainErosion(ActionGraph& graph, Terrain& terrain, PaintBaseHeightCache& base_cache,
                               const TerrainErosionBrush& brush);

// Map Editor phase, Task 4: "compiles" every terrain-paint key
// currently in graph's own store into terrain's real, already-loaded
// erosion-tile data (Terrain::ApplyPaintedHeight) - after this call,
// any ordinary Terrain::HeightAt caller (rendering, physics, other
// generators) sees the edits, with zero changes to HeightAt itself.
// Applies the *current* value for every painted key (whatever the
// graph's own cascade already resolved it to, including a still-
// flagged-ERROR action's own write - compiling doesn't add a new
// rollback-on-failure behavior nothing designed) - safe/cheap to call
// again after every paint or edit.
void CompileTerrainPaintToTerrain(const ActionGraph& graph, Terrain& terrain);

// Map Editor phase, Task 5: invalidates + immediately regenerates
// every currently-loaded WorldChunk (WorldBuilder::InvalidateContentNear)
// whose bounds overlap any painted grid node currently in graph's own
// store, so already-cached ground/road/building/vegetation content
// (not just Terrain::HeightAt itself - CompileTerrainPaintToTerrain's
// own scope) reflects the edit. Call after CompileTerrainPaintToTerrain.
// A no-op if the graph has no painted keys at all. Kept separate from
// CompileTerrainPaintToTerrain (not merged into it) because not every
// caller of that function has a WorldBuilder at hand - this phase's
// own Terrain-only self-tests construct no city/world streaming at all.
void InvalidateWorldChunksForPaint(const ActionGraph& graph, WorldBuilder& city);

#endif
