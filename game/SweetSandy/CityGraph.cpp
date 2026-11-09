#include "SweetSandy.h"

NAMESPACE_UPP

namespace {

struct CityGraphSeg : Moveable<CityGraphSeg> {
	Vec2 a, b;
};

struct CityGraphSubdivision : Moveable<CityGraphSubdivision> {
	double t;
	Vec2 pos;
};

// Standard parametric 2D segment intersection via cross products. Two
// exactly-parallel (or collinear) segments are reported as not
// intersecting - a reasonable simplification for road centerlines,
// which is what the source's isect library would treat as a degenerate
// case too (real streamlines essentially never end up exactly parallel
// and overlapping).
bool SegmentIntersect(Vec2 a, Vec2 b, Vec2 c, Vec2 d, Vec2& out_point, double& out_t, double& out_s)
{
	Vec2 r = b - a;
	Vec2 q = d - c;
	double denom = r.Cross(q);
	if(fabs(denom) < 1e-12)
		return false;

	Vec2 ac = c - a;
	double t = ac.Cross(q) / denom;
	double s = ac.Cross(r) / denom;
	if(t < 0 || t > 1 || s < 0 || s > 1)
		return false;

	out_point = a + r * t;
	out_t = t;
	out_s = s;
	return true;
}

bool ContainsInt(const Vector<int>& v, int x)
{
	for(int i : v)
		if(i == x)
			return true;
	return false;
}

} // anonymous namespace

int CityGraph::FindOrAddNode(Vec2 pos, double fuzzy_radius)
{
	double radius_sq = fuzzy_radius * fuzzy_radius;
	int best = -1;
	double best_dist_sq = 0;
	for(int i = 0; i < nodes.GetCount(); i++) {
		double d = (nodes[i].pos - pos).LengthSq();
		if(d < radius_sq && (best < 0 || d < best_dist_sq)) {
			best = i;
			best_dist_sq = d;
		}
	}
	if(best >= 0)
		return best;

	CityGraphNode& n = nodes.Add();
	n.pos = pos;
	return nodes.GetCount() - 1;
}

void CityGraph::AddEdge(int a, int b)
{
	if(a == b)
		return;
	if(!ContainsInt(nodes[a].neighbors, b))
		nodes[a].neighbors.Add(b);
	if(!ContainsInt(nodes[b].neighbors, a))
		nodes[b].neighbors.Add(a);
}

void CityGraph::Build(const Vector<Vector<Vec2>>& streamlines, double fuzzy_radius)
{
	nodes.Clear();

	Vector<CityGraphSeg> segs;
	for(const Vector<Vec2>& line : streamlines)
		for(int i = 0; i + 1 < line.GetCount(); i++) {
			CityGraphSeg& s = segs.Add();
			s.a = line[i];
			s.b = line[i + 1];
		}

	Vector<Vector<CityGraphSubdivision>> subdivisions;
	subdivisions.SetCount(segs.GetCount());
	for(int si = 0; si < segs.GetCount(); si++) {
		CityGraphSubdivision& s0 = subdivisions[si].Add();
		s0.t = 0;
		s0.pos = segs[si].a;
		CityGraphSubdivision& s1 = subdivisions[si].Add();
		s1.t = 1;
		s1.pos = segs[si].b;
	}

	// All-pairs segment intersection. O(segment_count^2), not grid-
	// bucketed despite 003-task3-graph.md's original proposal - at one
	// chunk's scale (a few hundred to low thousands of segments, not a
	// whole city) this is simple, dependency-free, and fast enough; the
	// same "revisit only if profiling says so" call WorldOctree's own
	// UpdateStreaming already made for its O((range/cell)^2) scan.
	const double EPS = 1e-9;
	for(int si = 0; si < segs.GetCount(); si++) {
		for(int sj = si + 1; sj < segs.GetCount(); sj++) {
			Vec2 ip;
			double t_si, t_sj;
			if(!SegmentIntersect(segs[si].a, segs[si].b, segs[sj].a, segs[sj].b, ip, t_si, t_sj))
				continue;

			// Only interior intersections become new subdivision points -
			// an intersection at t=0 or t=1 is just a shared endpoint,
			// already covered by that segment's own endpoint entry.
			if(t_si > EPS && t_si < 1 - EPS) {
				CityGraphSubdivision& s = subdivisions[si].Add();
				s.t = t_si;
				s.pos = ip;
			}
			if(t_sj > EPS && t_sj < 1 - EPS) {
				CityGraphSubdivision& s = subdivisions[sj].Add();
				s.t = t_sj;
				s.pos = ip;
			}
		}
	}

	for(int si = 0; si < segs.GetCount(); si++) {
		Sort(subdivisions[si], [](const CityGraphSubdivision& a, const CityGraphSubdivision& b) { return a.t < b.t; });

		int prev_node = -1;
		for(const CityGraphSubdivision& sub : subdivisions[si]) {
			int node = FindOrAddNode(sub.pos, fuzzy_radius);
			if(prev_node >= 0 && prev_node != node)
				AddEdge(prev_node, node);
			prev_node = node;
		}
	}
}

END_UPP_NAMESPACE
