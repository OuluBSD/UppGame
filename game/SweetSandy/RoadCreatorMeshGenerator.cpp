#include "SweetSandy.h"

NAMESPACE_UPP

namespace {

void AppendMesh(RoadMesh& dst, const RoadMesh& src)
{
	int offset = dst.vertices.GetCount();
	for(const RoadMeshVertex& v : src.vertices)
		dst.vertices.Add(v);
	for(int idx : src.triangles)
		dst.triangles.Add(idx + offset);
}

// Junction approach data is synthesized from the graph's own node
// positions/directions, not looked up from a specific ribbon's actual
// edge vertices - deliberate simplification (see 004-task4-worldbuilder-
// integration.md and AGENTS.md): precisely tracing "which ribbon
// contributes which edge at this specific graph edge" would need
// provenance bookkeeping CityGraph doesn't retain (it subdivides and
// fuzzy-merges away which original streamline any given node/edge came
// from). "Good enough to drive over without a gap" - the same standard
// Task 3's own junction fan was already built to, not a claim of pixel-
// perfect alignment with the real ribbon edges.
RoadJunctionApproach MakeApproach(Vec2 node_pos, Vec2 neighbor_pos)
{
	Vec2 dir = (neighbor_pos - node_pos).Normalized();
	Vec2 point = node_pos + dir * kRoadWidth;
	Vec2 perp(-dir.y, dir.x);
	RoadJunctionApproach ap;
	ap.left = point + perp * (kRoadWidth * 0.5);
	ap.right = point - perp * (kRoadWidth * 0.5);
	return ap;
}

} // anonymous namespace

void RoadCreatorMeshGenerator::Generate(const WorldChunk& chunk, WorldChunkContent& content)
{
	if(content.road_centerlines.IsEmpty())
		return;

	RoadMesh combined;

	// One ribbon per original streamline (not per graph edge - graph
	// edges are typically only ~1m long, one per original sample point;
	// building a ribbon per *streamline* instead avoids needing hundreds
	// of tiny ribbons plus a junction fan at every one of their ordinary
	// interior points). Each ribbon also flattens Terrain's own grid
	// under its own footprint (Task 2) as it's built.
	for(const Vector<Vec2>& centerline : content.road_centerlines) {
		if(centerline.GetCount() < 2)
			continue;
		RoadMesh ribbon = RoadMeshBuilder::BuildRibbon(centerline, kRoadWidth);
		terrain.FlattenUnderRibbon(centerline, ribbon);
		AppendMesh(combined, ribbon);
	}

	// Rebuild the graph from road_centerlines (City Layout Task 3's own
	// algorithm) - cheap and deterministic at this chunk's scale, avoids
	// coupling this generator to MapGeneratorCityLayoutGenerator's
	// internal (unexposed) graph instance.
	CityGraph graph;
	graph.Build(content.road_centerlines, kCityGraphRebuildFuzzyRadius);

	for(int i = 0; i < graph.GetNodeCount(); i++) {
		const CityGraphNode& node = graph.GetNode(i);
		int degree = node.neighbors.GetCount();
		// Degree 0: shouldn't happen for a well-formed graph (every node
		// has at least one edge by construction) - skip defensively.
		// Degree 2: an ordinary point along one continuous streamline,
		// not a real junction - the ribbon already covers it seamlessly,
		// nothing to fill (see RoadJunctionMesh.h's own degree-2 note).
		if(degree == 0 || degree == 2)
			continue;

		Vector<RoadJunctionApproach> approaches;
		for(int n : node.neighbors)
			approaches.Add(MakeApproach(node.pos, graph.GetNode(n).pos));

		RoadMesh fan = RoadJunctionMesh::Build(node.pos, approaches);
		AppendMesh(combined, fan);
	}

	content.road_mesh = pick(combined);
}

END_UPP_NAMESPACE
