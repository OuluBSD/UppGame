#ifndef _SweetSandy_RoadClass_h_
#define _SweetSandy_RoadClass_h_

// Map Editor phase, Task 19: "different types of road" - the user's
// own speed-class breakdown (verbatim, km/h): 30 (city centre), 40
// (auto-centric, bike lanes not attached, near buildings), 50 (auto-
// centric, not close to homes), 60 (not close to buildings/people, or
// fenced), 80 (outside-city generic limit), 100 (freeway), 120
// (freeway between cities, well maintained). Each carries the actual
// parameters that drive RoadPathfinder.h (grade/curve limits) and
// future fence placement, not just a label. Grade/curve-radius figures
// loosely follow real-world highway design practice (lower design
// speed tolerates a steeper grade and a tighter curve - AASHTO-style),
// scaled down for this project's own smaller world - not copied from a
// real standard verbatim, a reasonable game-scale approximation.
struct RoadClassParams {
	double speed_kmh = 30;
	double max_grade_pct = 10;     // steepest |rise/run|*100 the pathfinder will route through (bridge spans are exempt - RoadPathfinder.h)
	double min_curve_radius = 15;  // meters - the path smoother works toward this; not a hard guarantee, see RoadPathfinder.h
	double width = 6;              // meters, the built ribbon's width
	bool   needs_fence = false;    // true: should be separated from nearby buildings/pedestrians by a fence (fence placement is future work - see 018/019 reports)
};

// name: "30"/"40"/"50"/"60"/"80"/"100"/"120" (km/h, the user's own
// shorthand). Returns false (out left unchanged) for anything else.
bool ParseRoadClass(const String& name, RoadClassParams& out);

#endif
