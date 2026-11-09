#ifndef _SweetSandy_CityGraph_h_
#define _SweetSandy_CityGraph_h_

// City Layout phase, Task 3: turns Task 2's streamline polylines into a
// planar graph - nodes at every streamline vertex and every genuine
// segment-segment crossing, edges between consecutive nodes along each
// original segment. Loosely ports MapGenerator's Graph
// (src/ts/impl/graph.ts), but restructured to avoid its two external-
// library dependencies (`isect`'s Bentley-Ottmann sweep, `d3-quadtree`)
// - see Manager/2-plan/.../003-city-layout/003-task3-graph.md and
// 001-planning.md's "What gets simplified, and why".
//
// Also sidesteps the source's `getNodesAlongSegment` machinery (walking
// dstep-sized steps along a segment to relocate nodes that drifted off
// it during polyline simplification): that machinery exists to cope with
// `simplify-js` reducing vertex density, and Task 2 deliberately doesn't
// simplify streamlines (see that task's own scope note) - every segment
// here is still an exact, undistorted piece of a real streamline, so
// intersection points can be placed and ordered on it directly via their
// own parametric position, no re-walking needed.
struct CityGraphNode : Moveable<CityGraphNode> {
	Vec2 pos;
	Vector<int> neighbors; // indices into CityGraph's own node list
};

// Shared fuzzy-merge radius for rebuilding a CityGraph from
// road_centerlines outside the original City Layout generation pass
// (RoadCreatorMeshGenerator, TrafficManager) - fuzzy merging is
// forgiving of small variations, so this doesn't need to bit-match
// whatever radius the original (separate, not exposed) CityGraph used
// internally, just be "reasonable" (see Road Network's own
// 004-task4-worldbuilder-integration.md); both consumers independently
// settled on the same 2.0 value regardless. Was two separate
// anonymous-namespace `const double kFuzzyRadius = 2.0` definitions
// (one in each of those two .cpp files) until BLITZ happened to merge
// them into the same translation unit and the redefinition became a
// real compile error - promoted to one shared constant here instead of
// just renaming one copy, so the two can't independently drift apart
// now that they're forced to live in the same file anyway.
const double kCityGraphRebuildFuzzyRadius = 2.0;

// Moveable: added when AI traffic (Task 2, 006-ai-traffic-and-
// pedestrians) became the first caller to store a CityGraph *inside a
// container* (TrafficManager's own VectorMap<WorldChunkCoord,
// CityGraph>, one graph kept alive per chunk with live cars) rather
// than just building one on the stack and reading it - every earlier
// consumer (MapGeneratorCityLayoutGenerator, RoadCreatorMeshGenerator)
// only ever used a single local instance. Safe: CityGraph's only data
// is one Vector<CityGraphNode>, already relocatable itself.
class CityGraph : public Moveable<CityGraph> {
	Vector<CityGraphNode> nodes;

	int FindOrAddNode(Vec2 pos, double fuzzy_radius);
	void AddEdge(int a, int b);

public:
	// Finds every segment-segment intersection across all streamlines
	// (all-pairs check - O(segment_count^2), fine at one chunk's scale;
	// see the .cpp for why this isn't grid-bucketed despite the task
	// doc's original proposal), subdivides each segment at its own
	// interior intersection points, and fuzzy-merges nodes within
	// fuzzy_radius of each other so nearby streamline endpoints/
	// intersections collapse into one junction instead of staying as
	// separate, disconnected points.
	void Build(const Vector<Vector<Vec2>>& streamlines, double fuzzy_radius);

	int GetNodeCount() const                { return nodes.GetCount(); }
	const CityGraphNode& GetNode(int i) const { return nodes[i]; }
};

#endif
