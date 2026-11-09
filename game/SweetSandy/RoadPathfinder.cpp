#include "SweetSandy.h"

NAMESPACE_UPP

// Map Editor phase, Task 19: two real sub-problems, kept as two
// separate, individually-testable pieces:
//
// 1. AStarLand - grade-limited grid search across dry land only.
//    Water nodes are simply impassable in this phase - a water
//    crossing is never routed "the long way around through shallow
//    bits", it always goes through the bridge logic below instead.
//    A plain linear-scan open list, not a binary heap: this project's
//    own CityGraph.h precedent ("all-pairs O(segment_count^2), fine
//    at one chunk's scale") for the same tradeoff - the grid is
//    deliberately capped small (see BuildGrid) so O(V^2) costs a few
//    milliseconds, and a real heap is more code for no measurable
//    benefit at this project's actual scale.
//
// 2. FindRoadPath - tries a land-only route first; if start and end
//    are separated by water, finds ONE crossing (the user's own
//    "bridges should be quite orthogonal to water crossings" - approximated
//    by comparing the direct start->end line's own crossing against a
//    shore-normal-aligned crossing from the same entry point, and
//    taking whichever is shorter), then land-routes each shore to it.
//    Only one bridge per route - a route needing to hop two separate
//    water bodies is a real, stated limitation (see RoadPathfinder.h).
//
// Ends with a Chaikin corner-cutting smoothing pass so the grid's own
// 8-direction zig-zag reads as a road, not a staircase - an
// approximation toward cls.min_curve_radius, not a hard guarantee;
// RoadPathResult::min_curve_radius_achieved reports what was actually
// reached so a caller can tell rather than assume.

namespace {

const int kNbrDx[8] = { 1, -1, 0, 0, 1, 1, -1, -1 };
const int kNbrDz[8] = { 0, 0, 1, -1, 1, -1, 1, -1 };

bool IsWaterAt(const Terrain& terrain, Vec2 p)
{
	return terrain.HeightAt(p.x, p.y) <= terrain.GetWaterLevel();
}

struct Grid {
	Vec2 origin;
	double cell = 10;
	int nx = 2, nz = 2;
	Vec2 WorldOf(int gx, int gz) const { return Vec2(origin.x + gx * cell, origin.y + gz * cell); }
	int Index(int gx, int gz) const { return gz * nx + gx; }
};

// Bounded to [start,end] plus a margin, cell size adaptive so the grid
// never exceeds ~70 cells per axis (~5000 nodes) regardless of route
// length - see this file's own top comment on why a linear-scan open
// list is fine at that size.
Grid BuildGrid(Vec2 start, Vec2 end)
{
	Vec2 lo(min(start.x, end.x), min(start.y, end.y));
	Vec2 hi(max(start.x, end.x), max(start.y, end.y));
	double dist = (end - start).Length();
	double pad = max(50.0, dist * 0.25);
	lo = Vec2(lo.x - pad, lo.y - pad);
	hi = Vec2(hi.x + pad, hi.y + pad);
	double diag = (hi - lo).Length();
	double cell = max(10.0, diag / 70.0);

	Grid g;
	g.origin = lo;
	g.cell = cell;
	g.nx = max(2, (int)((hi.x - lo.x) / cell) + 2);
	g.nz = max(2, (int)((hi.y - lo.y) / cell) + 2);
	return g;
}

// Empty on failure (no water-free, grade-legal route exists inside
// this grid's own bound) - not an exception/assert, since "no route"
// is an expected, real outcome FindRoadPath must handle (try a bridge,
// or fail with a clear message), not a bug.
Vector<Vec2> AStarLand(const Terrain& terrain, Vec2 start, Vec2 end, double max_grade_pct, double& max_grade_out)
{
	Grid g = BuildGrid(start, end);
	int n = g.nx * g.nz;
	const double kSqrt2 = sqrt(2.0);

	auto ClosestNode = [&](Vec2 p) {
		int gx = Clampi((int)round((p.x - g.origin.x) / g.cell), 0, g.nx - 1);
		int gz = Clampi((int)round((p.y - g.origin.y) / g.cell), 0, g.nz - 1);
		return g.Index(gx, gz);
	};
	int start_i = ClosestNode(start);
	int end_i = ClosestNode(end);

	Vector<double> gscore;      gscore.SetCount(n, 1e18);
	Vector<int>    came_from;   came_from.SetCount(n, -1);
	Vector<bool>   closed;      closed.SetCount(n, false);
	Vector<double> node_height; node_height.SetCount(n, 0.0);
	Vector<bool>   is_water;    is_water.SetCount(n, false);
	Vector<bool>   height_known;height_known.SetCount(n, false);

	auto NodeWorld = [&](int idx) { return g.WorldOf(idx % g.nx, idx / g.nx); };
	auto NodeHeight = [&](int idx) -> double {
		if(!height_known[idx]) {
			Vec2 w = NodeWorld(idx);
			node_height[idx] = terrain.HeightAt(w.x, w.y);
			is_water[idx] = node_height[idx] <= terrain.GetWaterLevel();
			height_known[idx] = true;
		}
		return node_height[idx];
	};
	NodeHeight(start_i);
	NodeHeight(end_i);
	if(is_water[start_i] || is_water[end_i])
		return Vector<Vec2>(); // caller's own bridge logic routes to/from a LAND point either side of the crossing, never straight into water

	gscore[start_i] = 0;
	Vector<int> open;
	open.Add(start_i);
	Vec2 end_w = NodeWorld(end_i);

	while(!open.IsEmpty()) {
		int best_k = 0;
		double best_f = 1e30;
		for(int k = 0; k < open.GetCount(); k++) {
			int idx = open[k];
			double f = gscore[idx] + (end_w - NodeWorld(idx)).Length();
			if(f < best_f) { best_f = f; best_k = k; }
		}
		int cur = open[best_k];
		open.Remove(best_k);
		if(closed[cur])
			continue;
		closed[cur] = true;
		if(cur == end_i)
			break;

		int cgx = cur % g.nx, cgz = cur / g.nx;
		for(int d = 0; d < 8; d++) {
			int ngx = cgx + kNbrDx[d], ngz = cgz + kNbrDz[d];
			if(ngx < 0 || ngx >= g.nx || ngz < 0 || ngz >= g.nz)
				continue;
			int nb = g.Index(ngx, ngz);
			if(closed[nb])
				continue;
			NodeHeight(nb);
			if(is_water[nb])
				continue; // land-only phase - see this file's own top comment
			double step_dist = (kNbrDx[d] != 0 && kNbrDz[d] != 0) ? g.cell * kSqrt2 : g.cell;
			double grade = fabs(NodeHeight(nb) - NodeHeight(cur)) / step_dist * 100.0;
			if(grade > max_grade_pct)
				continue; // too steep for this road class - a hard refusal, not a penalty, per "keep the gradient limited"
			double tentative = gscore[cur] + step_dist;
			if(tentative < gscore[nb]) {
				gscore[nb] = tentative;
				came_from[nb] = cur;
				open.Add(nb);
			}
		}
	}

	if(!closed[end_i])
		return Vector<Vec2>();

	Vector<Vec2> rev;
	int cur = end_i;
	while(true) {
		rev.Add(NodeWorld(cur));
		if(cur == start_i)
			break;
		cur = came_from[cur];
	}
	Vector<Vec2> path;
	for(int i = rev.GetCount() - 1; i >= 0; i--)
		path.Add(rev[i]);

	double achieved = 0;
	for(int i = 1; i < path.GetCount(); i++) {
		double d = (path[i] - path[i - 1]).Length();
		if(d < 1e-6)
			continue;
		double h0 = terrain.HeightAt(path[i - 1].x, path[i - 1].y);
		double h1 = terrain.HeightAt(path[i].x, path[i].y);
		achieved = max(achieved, fabs(h1 - h0) / d * 100.0);
	}
	max_grade_out = achieved;
	return path;
}

// Walks the direct line in ~5m steps to find the first/last water
// sample, then binary-searches each boundary for a tighter fix.
// Returns false if the line never touches water at all (no bridge
// needed - FindRoadPath's own first, land-only attempt already covers
// that case; this is only called after that attempt fails).
bool FindDirectCrossing(const Terrain& terrain, Vec2 start, Vec2 end, Vec2& entry_out, Vec2& exit_out)
{
	double dist = (end - start).Length();
	int steps = Clampi((int)(dist / 5.0), 2, 400);
	int first_water = -1, last_water = -1;
	for(int i = 0; i <= steps; i++) {
		double t = (double)i / steps;
		Vec2 p(start.x + (end.x - start.x) * t, start.y + (end.y - start.y) * t);
		if(IsWaterAt(terrain, p)) {
			if(first_water < 0)
				first_water = i;
			last_water = i;
		}
	}
	if(first_water < 0)
		return false;

	auto AtT = [&](double t) { return Vec2(start.x + (end.x - start.x) * t, start.y + (end.y - start.y) * t); };
	// Binary-refine entry between the last known-land sample and the first known-water sample.
	double lo_t = max(0.0, (double)(first_water - 1) / steps), hi_t = (double)first_water / steps;
	for(int it = 0; it < 15; it++) {
		double mid = (lo_t + hi_t) * 0.5;
		if(IsWaterAt(terrain, AtT(mid))) hi_t = mid; else lo_t = mid;
	}
	entry_out = AtT(lo_t);
	// Same for exit, between the last water sample and the next land sample.
	lo_t = (double)last_water / steps;
	hi_t = min(1.0, (double)(last_water + 1) / steps);
	for(int it = 0; it < 15; it++) {
		double mid = (lo_t + hi_t) * 0.5;
		if(IsWaterAt(terrain, AtT(mid))) lo_t = mid; else hi_t = mid;
	}
	exit_out = AtT(hi_t);
	return true;
}

// Steepest-descent direction of the height field at p (points from
// land toward open water near a shoreline) - a real civil-engineering
// convention: a bridge built perpendicular to the shore is the
// shortest possible span, so this is used as a candidate crossing
// heading, compared against the direct line's own crossing in
// FindRoadPath, shortest wins.
Vec2 ShoreNormal(const Terrain& terrain, Vec2 p)
{
	const double eps = 4.0;
	double hx1 = terrain.HeightAt(p.x + eps, p.y), hx0 = terrain.HeightAt(p.x - eps, p.y);
	double hz1 = terrain.HeightAt(p.x, p.y + eps), hz0 = terrain.HeightAt(p.x, p.y - eps);
	Vec2 downhill((hx0 - hx1) / (2 * eps), (hz0 - hz1) / (2 * eps));
	return downhill.Normalized();
}

// Walks from entry along dir in fixed steps looking for land again.
// Returns false (span too wide - not a viable single-span bridge) if
// max_span is exceeded before land is found.
bool WalkToLand(const Terrain& terrain, Vec2 entry, Vec2 dir, double max_span, Vec2& exit_out)
{
	const double step = 5.0;
	for(double d = step; d <= max_span; d += step) {
		Vec2 p(entry.x + dir.x * d, entry.y + dir.y * d);
		if(!IsWaterAt(terrain, p)) {
			exit_out = p;
			return true;
		}
	}
	return false;
}

// Chaikin corner-cutting, endpoints preserved exactly - softens the
// grid search's own 8-direction zig-zag into road-like curves.
// Applied only to a pure-land subsequence (a caller must not pass
// bridge points in - a real bridge deck is straight, not smoothed).
Vector<Vec2> ChaikinSmooth(const Vector<Vec2>& pts, int iterations)
{
	if(pts.GetCount() < 3)
		return clone(pts);
	Vector<Vec2> cur = clone(pts);
	for(int it = 0; it < iterations; it++) {
		Vector<Vec2> next;
		next.Add(cur[0]);
		for(int i = 0; i < cur.GetCount() - 1; i++) {
			Vec2 a = cur[i], b = cur[i + 1];
			next.Add(a + (b - a) * 0.25);
			next.Add(a + (b - a) * 0.75);
		}
		next.Add(cur[cur.GetCount() - 1]);
		cur = pick(next);
	}
	return cur;
}

// Circumradius of the circle through 3 points - measures how tight a
// turn at `b` actually is (a straight line has an infinite radius, a
// hairpin a tiny one). Returns a large sentinel for near-collinear
// points (no meaningful turn there).
double CircumRadius(Vec2 a, Vec2 b, Vec2 c)
{
	double ab = (b - a).Length(), bc = (c - b).Length(), ca = (a - c).Length();
	double area2 = fabs((b - a).Cross(c - a)); // 2x triangle area
	if(area2 < 1e-9)
		return 1e9;
	return (ab * bc * ca) / (2 * area2);
}

// Targets the actual curve-radius constraint directly, rather than
// just running more Chaikin passes and hoping - a corner whose own
// circumradius (CircumRadius) falls short of min_radius is pulled
// toward its two neighbors' midpoint, repeated until every corner
// clears the target or max_passes runs out (bounded - see this file's
// own top comment on "not a hard guarantee"; a route squeezed between
// two close obstacles may simply not have room for a wide enough
// curve, and this stops trying rather than spinning forever). Never
// moves the first/last point - those are the real, exact connection
// points to the rest of the route (a land approach's own endpoint, or
// this land segment's own start/end).
void RelaxCurvature(Vector<Vec2>& pts, double min_radius, int max_passes)
{
	for(int pass = 0; pass < max_passes; pass++) {
		bool any = false;
		for(int i = 1; i < pts.GetCount() - 1; i++) {
			double r = CircumRadius(pts[i - 1], pts[i], pts[i + 1]);
			if(r < min_radius) {
				Vec2 mid = (pts[i - 1] + pts[i + 1]) * 0.5;
				pts[i] = pts[i] + (mid - pts[i]) * 0.35;
				any = true;
			}
		}
		if(!any)
			break;
	}
}

} // anonymous namespace

RoadPathResult FindRoadPath(const Terrain& terrain, Vec2 start, Vec2 end, const RoadClassParams& cls)
{
	RoadPathResult result;

	double max_grade_land = 0;
	Vector<Vec2> land_only = AStarLand(terrain, start, end, cls.max_grade_pct, max_grade_land);
	Vector<Vec2> final_land_a, final_land_b;
	bool has_bridge = false;
	Vec2 bridge_entry, bridge_exit;

	if(!land_only.IsEmpty()) {
		final_land_a = pick(land_only);
	}
	else {
		Vec2 entry, exit_direct;
		if(!FindDirectCrossing(terrain, start, end, entry, exit_direct)) {
			// Not water-blocked (FindDirectCrossing found nothing) - the
			// land-only search simply failed on grade alone, e.g. cls is
			// too strict for the terrain between these two points.
			result.ok = false;
			result.error = "no route within max_grade_pct=" + AsString(cls.max_grade_pct)
			             + "% found between the two points (not a water crossing) - try a lower-speed road class or different endpoints";
			return result;
		}

		const double kMaxSpan = 500.0; // meters - beyond this, a single straight span isn't a realistic bridge for this project's scale
		Vec2 normal_dir = ShoreNormal(terrain, entry);
		Vec2 exit_normal;
		bool have_normal_exit = WalkToLand(terrain, entry, normal_dir, kMaxSpan, exit_normal);

		double span_direct = (exit_direct - entry).Length();
		double span_normal = have_normal_exit ? (exit_normal - entry).Length() : 1e18;
		// Shortest wins - a real bridge is built perpendicular to the
		// shore specifically because that's the shortest possible span;
		// the direct line is kept as a fallback for a shoreline shape
		// the gradient estimate doesn't suit well.
		if(span_normal < span_direct) {
			bridge_exit = exit_normal;
		}
		else {
			bridge_exit = exit_direct;
		}
		bridge_entry = entry;
		has_bridge = true;

		// Real bridges don't start exactly at the waterline - a short
		// engineered approach ramp/embankment (graded for the purpose,
		// not following the raw hillside) carries the road the last bit
		// onto the span. Without this, AStarLand's own search for
		// `start`/`end` has to climb whatever the natural shoreline
		// grade happens to be right at the water's edge - often the
		// steepest few meters of the whole crossing, and not something
		// re-routing around helps with (the shore is the shore).
		// Approximated as a fixed-length pull-back along the bridge's
		// own line before the land search starts, folded into the
		// bridge span itself (extended run, still bridge=true - see
		// WorldGeomBridge's own AddRoadLinkObject for how the deck
		// height ramps down to ground level at these extended ends).
		const double kApproachRamp = 25.0;
		Vec2 span_dir = (bridge_exit - bridge_entry).Normalized();
		Vec2 ramp_entry = Vec2(bridge_entry.x - span_dir.x * kApproachRamp, bridge_entry.y - span_dir.y * kApproachRamp);
		Vec2 ramp_exit = Vec2(bridge_exit.x + span_dir.x * kApproachRamp, bridge_exit.y + span_dir.y * kApproachRamp);
		if(IsWaterAt(terrain, ramp_entry))
			ramp_entry = bridge_entry; // rare - a very narrow spit; fall back to no ramp extension on this side
		if(IsWaterAt(terrain, ramp_exit))
			ramp_exit = bridge_exit;
		bridge_entry = ramp_entry;
		bridge_exit = ramp_exit;

		double grade_a = 0, grade_b = 0;
		final_land_a = AStarLand(terrain, start, bridge_entry, cls.max_grade_pct, grade_a);
		final_land_b = AStarLand(terrain, bridge_exit, end, cls.max_grade_pct, grade_b);
		max_grade_land = max(grade_a, grade_b);

		if(final_land_a.IsEmpty() || final_land_b.IsEmpty()) {
			result.ok = false;
			result.error = "found a water crossing but could not reach it (or the far shore) within max_grade_pct="
			             + AsString(cls.max_grade_pct) + "% - try a lower-speed road class";
			return result;
		}
	}

	// Smooth each land piece independently (never the bridge itself -
	// see ChaikinSmooth's own comment), then relax remaining tight
	// corners directly against cls.min_curve_radius.
	Vector<Vec2> smooth_a = ChaikinSmooth(final_land_a, 2);
	RelaxCurvature(smooth_a, cls.min_curve_radius, 25);

	result.ok = true;
	result.max_grade_pct_achieved = max_grade_land;
	result.bridge_point_count = 0;

	Vector<RoadPathPoint> final_points;
	for(Vec2 p : smooth_a) {
		RoadPathPoint rp; rp.pos = p; rp.bridge = false;
		final_points.Add(rp);
	}
	if(has_bridge) {
		// Bridge deck: straight, subdivided every ~10m for mesh detail,
		// never smoothed - a real bridge doesn't wiggle.
		double span = (bridge_exit - bridge_entry).Length();
		int nseg = max(1, (int)(span / 10.0));
		for(int i = 1; i <= nseg; i++) {
			double t = (double)i / nseg;
			RoadPathPoint rp;
			rp.pos = Vec2(bridge_entry.x + (bridge_exit.x - bridge_entry.x) * t,
			              bridge_entry.y + (bridge_exit.y - bridge_entry.y) * t);
			rp.bridge = true;
			final_points.Add(rp);
			result.bridge_point_count++;
		}
		Vector<Vec2> smooth_b = ChaikinSmooth(final_land_b, 2);
		RelaxCurvature(smooth_b, cls.min_curve_radius, 25);
		for(int i = 1; i < smooth_b.GetCount(); i++) { // skip [0] - it's ~bridge_exit already, avoid a near-duplicate point
			RoadPathPoint rp; rp.pos = smooth_b[i]; rp.bridge = false;
			final_points.Add(rp);
		}
	}

	// Achieved min curve radius, measured post-smoothing at every
	// interior LAND vertex (a bridge is straight by construction, no
	// meaningful radius to measure there).
	double min_radius = 1e9;
	for(int i = 1; i < final_points.GetCount() - 1; i++) {
		if(final_points[i - 1].bridge || final_points[i].bridge || final_points[i + 1].bridge)
			continue;
		double r = CircumRadius(final_points[i - 1].pos, final_points[i].pos, final_points[i + 1].pos);
		min_radius = min(min_radius, r);
	}
	result.min_curve_radius_achieved = min_radius;
	result.points = pick(final_points);
	return result;
}

END_UPP_NAMESPACE
