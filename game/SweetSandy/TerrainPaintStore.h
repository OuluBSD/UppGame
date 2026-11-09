#ifndef _SweetSandy_TerrainPaintStore_h_
#define _SweetSandy_TerrainPaintStore_h_

// Map Editor phase, Task 13: SweetSandy's own plain-text persistence
// for hand-authored terrain shaping - "do the terrain first, using
// that terrain generation algorithm with that erosion" (the user's
// own instruction: shape terrain, via this phase's own already-real
// procedural island+erosion pipeline plus this phase's own already-
// real brush system, Tasks 3-4, BEFORE city/road/building content is
// generated on top of it - not skip straight to placing discrete
// objects, which is what Task 12's own landmarks-only design did).
// Same "why not Edit3D's own save/load" reasoning as LandmarkStore.h -
// this never touches that broken path either. A stroke is a handful
// of scalars (TerrainPaintBrush's own fields), so the same plain-text,
// one-line-per-entry format applies directly.
//
// Replayed, not baked: every stored stroke is a real
// AppendTerrainPaint call (this phase's own existing action system,
// unmodified) against a *freshly loaded* Terrain each time
// (MapEditorWindow::EnsureTerrainLoaded), then one
// CompileTerrainPaintToTerrain - identical to how a live interactive
// PlayScreen painting session works, just driven by a stored stroke
// list instead of real-time mouse drags. Order matters (later strokes
// see earlier ones' own already-painted heights, exactly like
// overlapping real brush strokes would) - always replayed in the
// order stored.
const char* const kTerrainPaintStorePath = "share/sweetsandy/levels/terrain_paint.txt";

struct PaintStrokeEntry : Moveable<PaintStrokeEntry> {
	// Map Editor phase, Task 17: "raise" (TerrainPaintBrush additive
	// delta, the only kind that existed before this task), "flatten"
	// (TerrainPaintBrush::flatten - `delta` reinterpreted as an
	// absolute target world-Y) or "erode" (TerrainErosionBrush - a real
	// ErodeHeightmap touch-up; `delta`/`slope_limit` unused, `droplets`/
	// `seed` used instead). Defaults to "raise" so a file written before
	// this task (none exist yet, but the format is otherwise unchanged)
	// still parses as exactly what it always meant.
	String kind = "raise";
	double cx = 0, cz = 0;
	double radius = 8.0;
	double delta = 1.0;
	double slope_limit = 10.0;
	int    droplets = 150; // "erode" only
	dword  seed = 1;       // "erode" only
};

// Returns an empty Vector (not an error) if the file doesn't exist yet
// - matching "no terrain shaping done yet", same convention as
// LoadLandmarks.
Vector<PaintStrokeEntry> LoadPaintStrokes(const String& path);
void SavePaintStrokes(const String& path, const Vector<PaintStrokeEntry>& strokes);

// Map Editor phase, Task 15: loads the "island" track into `terrain`
// and replays every persisted stroke onto it - the exact sequence
// MapEditorWindow::EnsureTerrainLoaded already needed, factored out
// here so a caller that only needs a real, already-shaped Terrain
// (e.g. a heightmap export - no Edit3D/GeomScene/WorldBuilder
// involved at all) doesn't need a MapEditorWindow to get one. Owns its
// own transient ActionGraph/PaintBaseHeightCache (see
// TerrainPaintAction.h's own PaintBaseHeightCache comment on why a
// fresh one per call is fine here: this function's own replay is a
// one-shot "load and shape", not a live paint session that keeps
// adding strokes over time).
void LoadIslandTerrainWithPaint(Terrain& terrain);

#endif
