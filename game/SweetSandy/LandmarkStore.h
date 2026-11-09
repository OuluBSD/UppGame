#ifndef _SweetSandy_LandmarkStore_h_
#define _SweetSandy_LandmarkStore_h_

// Map Editor phase, Task 12: SweetSandy's own plain-text persistence
// for hand-authored level content ("place things by hand" - the
// user's own explicit redirect toward a scriptable access point, not
// mouse-driven GUI editing). Deliberately NOT ModelerApp's own
// Edit3D::SaveScene3D/LoadScene3D: a real, reproducible bug was found
// there - GeomCamera/GeomTransform's own vec3/quat fields serialize as
// an empty {"data": ""} blob (confirmed by inspecting a saved
// .scene3d file directly), and loading that back panics
// ("PANIC: Type has not been set yet", inside uppsrc/Core/Util.h's
// own TypedStringHasher - a pre-existing issue in this large,
// unfamiliar framework, not something safe to patch blindly here).
// Sidestepped entirely rather than fixed: this store only ever
// persists a handful of scalar fields (name/kind/position/color) in a
// plain line-oriented text format SweetSandy fully controls, and the
// procedural content itself never needs saving at all - it's
// deterministic (same seed -> same bake, already proven bit-identical
// across repeated runs), so every `--mapedit` invocation just re-bakes
// it fresh (a couple of seconds) and re-materializes whatever
// hand-authored entries this store holds on top - see
// WorldGeomBridge.h's own AddMarkerObject/AddStartGateLandmark, which
// this store's own MaterializeLandmarks calls.
struct LandmarkEntry : Moveable<LandmarkEntry> {
	String kind; // "marker" or "gate"
	String name; // unique - used both as the GeomObject's own name and as this store's own lookup key
	double x = 0, z = 0;
	// Height *above ground* at (x,z), not an absolute world Y -
	// resolved via Terrain::HeightAt at materialize time (see
	// MaterializeLandmarks), same convention SpawnMarker's own
	// interactive placement already uses ("+1m so it doesn't clip").
	// Deliberately not an absolute Y: a caller (a person or this
	// project's own agent) placing a landmark from world x/z alone,
	// without first looking up the real terrain height there by hand,
	// is the whole point of this being an "easy" access point - see
	// this file's own header comment. A real bug this exact design
	// choice fixed: the first --mapedit add-marker call in this task's
	// own history passed y=0 meaning "world Y zero" under the old
	// design, landing the marker ~140m underground (this island's own
	// real terrain height there) - invisible in every render until
	// this field's meaning changed.
	double y = 1.0;
	int r = 255, g = 0, b = 220; // marker default: bright magenta - see AddMarkerObject's own comment on why
};

// Reads every entry from `path` - returns an empty Vector (not an
// error) if the file doesn't exist yet, matching "no landmarks placed
// yet" rather than forcing every caller to check existence first.
Vector<LandmarkEntry> LoadLandmarks(const String& path);
// Overwrites `path` with exactly `entries` - the whole file is always
// rewritten (not appended), since entries are typically loaded,
// modified in memory, then saved back as a unit.
void SaveLandmarks(const String& path, const Vector<LandmarkEntry>& entries);

// Adds one GeomObject per entry (AddMarkerObject for "marker",
// AddStartGateLandmark for "gate") into `scene` - called after a
// procedural BuildWorldGeomObjects bake, on the same scene, to layer
// hand-authored content on top. `terrain` resolves ground height for
// "gate" entries the same way AddStartGateLandmark's own caller
// already does.
void MaterializeLandmarks(GeomScene& scene, const Terrain& terrain, const Vector<LandmarkEntry>& entries);

#endif
