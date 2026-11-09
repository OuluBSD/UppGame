#ifndef _SweetSandy_RoadLinkStore_h_
#define _SweetSandy_RoadLinkStore_h_

// Map Editor phase, Task 18: "join roads from city to rural places and
// to other cities" - the user's own next pipeline step after Task 17's
// terrain/POI work. Same plain-text, one-line-per-entry persistence as
// LandmarkStore.h/TerrainPaintStore.h (never touches Edit3D's own
// broken save/load path either) - a hand-authored road link is a
// handful of scalars (two endpoints + a width), same shape as every
// other hand-authored thing in this phase.
//
// Map Editor phase, Task 19: replaced the original straight-line-only
// design with RoadPathfinder.h's real grade-limited, bridge-capable
// pathfinder - stores just the two endpoints and a road class (never
// the computed path itself, same "deterministic, replayed not baked"
// reasoning as every other store in this phase - MaterializeRoadLinks
// recomputes it fresh each load, so a terrain edit upstream of a link
// automatically re-routes it next time, rather than serving a stale
// path).
const char* const kRoadLinkStorePath = "share/sweetsandy/levels/road_links.txt";

struct RoadLinkEntry : Moveable<RoadLinkEntry> {
	String name;
	double x1 = 0, z1 = 0;
	double x2 = 0, z2 = 0;
	String road_class = "50"; // "30".."120" - see RoadClass.h
};

// Returns an empty Vector (not an error) if the file doesn't exist yet
// - same convention as LoadLandmarks/LoadPaintStrokes.
Vector<RoadLinkEntry> LoadRoadLinks(const String& path);
void SaveRoadLinks(const String& path, const Vector<RoadLinkEntry>& entries);

// Runs RoadPathfinder::FindRoadPath for each entry (grade-limited,
// bridge-capable - see that file) and adds the result to `scene` via
// WorldGeomBridge's own AddRoadLinkObject, bright-cyan-tinted so it
// reads as hand-authored against the generated (gray asphalt) road
// network - same "materialize the persisted list into the scene" role
// as LandmarkStore's own MaterializeLandmarks. An entry whose class
// name doesn't parse, or whose path can't be found at all (see
// RoadPathResult::error), is logged and skipped - never taken down
// the whole materialize call over one bad entry, same "one bad line
// doesn't take down the rest" convention every other store in this
// phase already follows.
void MaterializeRoadLinks(GeomScene& scene, const Terrain& terrain, const Vector<RoadLinkEntry>& entries);

#endif
