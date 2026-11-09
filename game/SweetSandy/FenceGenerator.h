#ifndef _SweetSandy_FenceGenerator_h_
#define _SweetSandy_FenceGenerator_h_

// Map Editor phase, Task 20: "fencing requires zoning first, and it's
// the collision line between road and zone, and it can have varying
// width" - the user's own words, verbatim, correcting Task 19's
// "fencing not attempted" gap. Computed, not hand-authored (same
// "deterministic, replayed not baked" reasoning as every generated
// thing in this phase) - a fence is a pure function of the current
// road links + zones + terrain, so a terrain edit or a moved zone
// automatically re-fences next time, rather than serving a stale
// layout.
//
// "The collision line between road and zone": for each road whose own
// RoadClassParams::needs_fence is true, and each zone that
// ZoneNeedsProtection (ZoneStore.h), the fence is built along the
// SUB-ARC of the zone's own boundary circle that falls within the
// road's buffered corridor (half its width, plus a fixed setback) -
// not the whole zone boundary, and not the whole road; only the part
// where the two actually collide. Bridge points are excluded (no
// fence floats out over open water).
struct FenceSegment : Moveable<FenceSegment> {
	Vector<Vec2> points;
	String kind = "wall";  // "wall" (thin, tall - slower fenced classes) | "dirtpile" (wide, low berm - faster classes)
	double width = 0.4;    // meters - "can have varying width": scales with the road class's own speed, see FenceGenerator.cpp
	double height = 1.8;   // meters
};

Vector<FenceSegment> ComputeFences(const Vector<RoadLinkEntry>& roads, const Vector<ZoneEntry>& zones, const Terrain& terrain);

// Adds each segment to `scene` via WorldGeomBridge's own AddFenceObject,
// tinted by kind (dark stone-gray "wall", earthy brown "dirtpile") so
// the two are visually distinct in a render/export, same
// "materialize the computed list into the scene" role as
// LandmarkStore/RoadLinkStore's own Materialize* functions.
void MaterializeFences(GeomScene& scene, const Terrain& terrain, const Vector<FenceSegment>& fences);

#endif
