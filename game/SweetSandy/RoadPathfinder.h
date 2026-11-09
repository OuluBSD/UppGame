#ifndef _SweetSandy_RoadPathfinder_h_
#define _SweetSandy_RoadPathfinder_h_

// Map Editor phase, Task 19: "focus on road path finder next, which
// tries to keep the gradient limited. And bridges should be quite
// orthogonal to water crossings... curves would need to be mild
// enough" - the user's own request, replacing Task 18's plain straight
// line (RoadLinkStore.h) with a real grid-search path that refuses to
// climb steeper than the road class allows and inserts a real bridge
// (a short, shore-perpendicular span, not a claim about actual
// float/pier engineering) wherever a water crossing is unavoidable.
// See RoadPathfinder.cpp's own top comment for the algorithm.
struct RoadPathPoint : Moveable<RoadPathPoint> {
	Vec2 pos;
	bool bridge = false; // this point is part of a water crossing - RoadLinkStore renders bridge points differently (a level deck, not terrain-hugging)
};

struct RoadPathResult : Moveable<RoadPathResult> {
	Vector<RoadPathPoint> points; // >=2 on success, empty on failure
	bool ok = false;
	String error;                       // set when !ok
	double max_grade_pct_achieved = 0;  // steepest non-bridge grade actually used along the found path
	double min_curve_radius_achieved = 1e9; // smallest circumradius found at any interior vertex after smoothing - see RoadPathfinder.cpp's own "not a hard guarantee" note
	int bridge_point_count = 0;
};

RoadPathResult FindRoadPath(const Terrain& terrain, Vec2 start, Vec2 end, const RoadClassParams& cls);

#endif
