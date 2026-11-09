#include "SweetSandy.h"

NAMESPACE_UPP

namespace {

struct JunctionPoint : Moveable<JunctionPoint> {
	Vec2 pos;
	int approach_index;
	double angle;
};

} // anonymous namespace

RoadMesh RoadJunctionMesh::Build(Vec2 centre, const Vector<RoadJunctionApproach>& approaches)
{
	RoadMesh mesh;
	int n = approaches.GetCount();
	if(n == 0)
		return mesh;

	if(n == 1) {
		// Dead end: a simple closed cap, not a fan (nothing to fan
		// around with only one approach) - see RoadJunctionMesh.h.
		RoadMeshVertex& l = mesh.vertices.Add();
		l.pos = approaches[0].left;
		RoadMeshVertex& r = mesh.vertices.Add();
		r.pos = approaches[0].right;
		RoadMeshVertex& c = mesh.vertices.Add();
		c.pos = centre;
		mesh.triangles.Add(0);
		mesh.triangles.Add(1);
		mesh.triangles.Add(2);
		return mesh;
	}

	// Collect every approach's near-junction edge point, sort them by
	// angle around the centre - this turns "which roads are adjacent
	// around the junction" into a simple circular walk.
	Vector<JunctionPoint> pts;
	for(int i = 0; i < n; i++) {
		JunctionPoint& l = pts.Add();
		l.pos = approaches[i].left;
		l.approach_index = i;
		l.angle = atan2(l.pos.y - centre.y, l.pos.x - centre.x);

		JunctionPoint& r = pts.Add();
		r.pos = approaches[i].right;
		r.approach_index = i;
		r.angle = atan2(r.pos.y - centre.y, r.pos.x - centre.x);
	}
	Sort(pts, [](const JunctionPoint& a, const JunctionPoint& b) { return a.angle < b.angle; });

	int centre_index = mesh.vertices.GetCount();
	RoadMeshVertex& c = mesh.vertices.Add();
	c.pos = centre;

	// Walk the sorted circle: the gap between two CONSECUTIVE points
	// belonging to DIFFERENT approaches is empty ground the junction
	// needs to fill; the gap between the two points of the SAME
	// approach is that road's own width, already covered by its own
	// ribbon (Task 1) - skip it, filling it too would double up
	// geometry over the road itself.
	int m = pts.GetCount();
	for(int i = 0; i < m; i++) {
		const JunctionPoint& p0 = pts[i];
		const JunctionPoint& p1 = pts[(i + 1) % m];
		if(p0.approach_index == p1.approach_index)
			continue;

		int i0 = mesh.vertices.GetCount();
		RoadMeshVertex& v0 = mesh.vertices.Add();
		v0.pos = p0.pos;
		int i1 = mesh.vertices.GetCount();
		RoadMeshVertex& v1 = mesh.vertices.Add();
		v1.pos = p1.pos;

		mesh.triangles.Add(i0);
		mesh.triangles.Add(i1);
		mesh.triangles.Add(centre_index);
	}

	return mesh;
}

END_UPP_NAMESPACE
