#ifndef _SweetSandy_CityBlockFinder_h_
#define _SweetSandy_CityBlockFinder_h_

// City Layout phase, Task 4: extracts closed city-block polygons (the
// planar graph's faces) from Task 3's CityGraph. Port of
// polygon_finder.ts's recursiveWalk/getRightmostNode - pure graph/
// geometry, no external-library dependency in the source either, so
// this ports directly (see Manager/2-plan/.../003-city-layout/
// 004-task4-polygons.md). Water/park filtering (filterPolygonsByWater
// in the source) is not ported - this world has no water yet, matching
// every other phase-1 scope decision around that (Task 1's
// CityTensorField::SamplePoint, Task 2's CityStreamlineIntegrator::OnLand).
class CityBlockFinder {
public:
	// For every directed edge (node, neighbor) where node currently has
	// at least 2 neighbors, walks "always turn rightmost at the next
	// junction" until it returns to a node already visited in this
	// walk, tracing out the boundary of the face to the walk's right.
	// Each directed edge is consumed once its face is found (mirrors
	// the source's removePolygonAdjacencies), so the same face is never
	// returned twice from its two directions. A walk that hits a dead
	// end (a node whose only neighbor is the one just arrived from)
	// finds no face and is discarded - the source's own documented
	// limitation ("currently ignores polygons with dead end inside"),
	// kept as-is rather than fixed. max_walk_length bounds every walk
	// (matches the source's maxLength) so a pathological/unbounded
	// graph can't hang this.
	static Vector<Vector<Vec2>> FindBlocks(const CityGraph& graph, int max_walk_length);
};

#endif
