#include "SweetSandy.h"

NAMESPACE_UPP

namespace {

// Local forward direction at centerline point i - forward-difference at
// the endpoints, centered difference at interior points (smoother than
// a forward-only difference everywhere, a deliberate small improvement
// over the source - see 001-task1-ribbon-mesh.md).
Vec2 ForwardAt(const Vector<Vec2>& pts, int i)
{
	int n = pts.GetCount();
	if(i == 0)
		return pts[1] - pts[0];
	if(i == n - 1)
		return pts[i] - pts[i - 1];
	return pts[i + 1] - pts[i - 1];
}

} // anonymous namespace

RoadMesh RoadMeshBuilder::BuildRibbon(const Vector<Vec2>& centerline, double width)
{
	int n = centerline.GetCount();
	ASSERT_(n >= 2, "RoadMeshBuilder::BuildRibbon: centerline needs at least 2 points");

	RoadMesh mesh;
	double half_width = width * 0.5;
	double left_len = 0, right_len = 0;

	for(int i = 0; i < n; i++) {
		Vec2 forward = ForwardAt(centerline, i);
		Vec2 left = Vec2(-forward.y, forward.x).Normalized();

		Vec2 left_pos = centerline[i] + left * half_width;
		Vec2 right_pos = centerline[i] - left * half_width;

		if(i > 0) {
			left_len += (left_pos - mesh.vertices[(i - 1) * 2].pos).Length();
			right_len += (right_pos - mesh.vertices[(i - 1) * 2 + 1].pos).Length();
		}

		RoadMeshVertex& lv = mesh.vertices.Add();
		lv.pos = left_pos;
		lv.uv = Vec2(0, left_len);

		RoadMeshVertex& rv = mesh.vertices.Add();
		rv.pos = right_pos;
		rv.uv = Vec2(1, right_len);

		if(i > 0) {
			int a = (i - 1) * 2;     // previous left
			int b = a + 1;           // previous right
			int c = i * 2;           // this left
			int d = c + 1;           // this right

			mesh.triangles.Add(a);
			mesh.triangles.Add(c);
			mesh.triangles.Add(b);

			mesh.triangles.Add(b);
			mesh.triangles.Add(c);
			mesh.triangles.Add(d);
		}
	}

	return mesh;
}

END_UPP_NAMESPACE
