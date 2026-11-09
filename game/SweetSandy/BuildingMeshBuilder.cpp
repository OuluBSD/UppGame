#include "SweetSandy.h"

NAMESPACE_UPP

namespace {

// p0..p3 in boundary order (matches VegMeshBuilder's own MeshLeafQuad
// convention) - flat, single explicit normal for all 4 vertices (a
// building should read as flat-faced boxes, not smoothed blobs, unlike
// VegMesh's own rounded-leaf-normal trick).
void AddQuad(VegMesh& out, Vec3 p0, Vec3 p1, Vec3 p2, Vec3 p3, Vec3 normal)
{
	int base = out.vertices.GetCount();
	Vec3 pos[4] = { p0, p1, p2, p3 };
	Vec2 uv[4] = { Vec2(0, 0), Vec2(1, 0), Vec2(1, 1), Vec2(0, 1) };
	for(int i = 0; i < 4; i++) {
		VegMeshVertex v;
		v.pos = pos[i];
		v.normal = normal;
		v.uv = uv[i];
		out.vertices.Add(v);
	}
	out.triangles.Add(base); out.triangles.Add(base + 1); out.triangles.Add(base + 2);
	out.triangles.Add(base); out.triangles.Add(base + 2); out.triangles.Add(base + 3);
}

void AddTriangle(VegMesh& out, Vec3 p0, Vec3 p1, Vec3 p2, Vec3 normal)
{
	int base = out.vertices.GetCount();
	Vec3 pos[3] = { p0, p1, p2 };
	Vec2 uv[3] = { Vec2(0, 0), Vec2(1, 0), Vec2(0.5, 1) };
	for(int i = 0; i < 3; i++) {
		VegMeshVertex v;
		v.pos = pos[i];
		v.normal = normal;
		v.uv = uv[i];
		out.vertices.Add(v);
	}
	out.triangles.Add(base); out.triangles.Add(base + 1); out.triangles.Add(base + 2);
}

} // anonymous namespace

void BuildingMeshBuilder::Build(const PlannedBuilding& b, double ground_y, VegMesh& walls, VegMesh& roof)
{
	walls.vertices.Clear(); walls.triangles.Clear();
	roof.vertices.Clear(); roof.triangles.Clear();

	double hw = b.width * 0.5, hd = b.depth * 0.5;
	double y0 = ground_y, y1 = ground_y + b.height;

	// Footprint corners at wall-top height, boundary order (matches
	// PlanBuildingsForBlock's own local_corners convention: -x-z, +x-z,
	// +x+z, -x+z).
	Vec3 c0_bot(-hw, y0, -hd), c1_bot(hw, y0, -hd), c2_bot(hw, y0, hd), c3_bot(-hw, y0, hd);
	Vec3 c0(-hw, y1, -hd), c1(hw, y1, -hd), c2(hw, y1, hd), c3(-hw, y1, hd);

	// Walls: 4 vertical quads, one flat outward normal each (axis-
	// aligned in local space - matches DrawWedgeShell's own per-face
	// glNormal3d convention rather than smoothed per-vertex normals).
	AddQuad(walls, c0_bot, c1_bot, c1, c0, Vec3(0, 0, -1));  // south (-z)
	AddQuad(walls, c1_bot, c2_bot, c2, c1, Vec3(1, 0, 0));   // east (+x)
	AddQuad(walls, c2_bot, c3_bot, c3, c2, Vec3(0, 0, 1));   // north (+z)
	AddQuad(walls, c3_bot, c0_bot, c0, c3, Vec3(-1, 0, 0));  // west (-x)

	if(!b.gabled_roof) {
		AddQuad(roof, c0, c1, c2, c3, Vec3(0, 1, 0));
		return;
	}

	// Gabled: ridge along the LONGER footprint axis (so the roof
	// doesn't look absurdly tall/narrow on an elongated footprint) -
	// see 002-task2-building-mesh.md's own design note. Pitch is a
	// fixed, modest fraction of the SHORTER axis (the one the slopes
	// actually span), not configurable.
	double ridge_y = y1 + min(b.width, b.depth) * 0.35;

	// Slope normals below: for a slope spanning "run" horizontally (the
	// half-footprint distance from eave to ridge) and rising "rise"
	// vertically (ridge_y - y1), the outward normal is the 2D vector
	// (run, rise) rotated 90 degrees away from the ridge, i.e.
	// proportional to (run-axis-component=rise, vertical-component=run)
	// with sign chosen so it points away from the ridge and upward -
	// hand-derived via (p1-p0)x(p2-p0) on each quad's own actual
	// vertices and cross-checked against physical intuition (a nearly-
	// flat roof, rise << run, should have an almost-vertical normal;
	// a steep roof should tilt toward horizontal) before use, not
	// guessed - see 002-task2-building-mesh.md's own note that an
	// earlier draft of this function got these wrong.
	if(b.width >= b.depth) {
		// Ridge runs along local X; slopes span Z; gable ends at x=±hw.
		Vec3 r0(-hw, ridge_y, 0), r1(hw, ridge_y, 0);
		Vec3 south_normal = Vec3(0, hd, y1 - ridge_y).Normalized();
		Vec3 north_normal = Vec3(0, hd, ridge_y - y1).Normalized();
		AddQuad(roof, c0, c1, r1, r0, south_normal);
		AddQuad(roof, c3, r0, r1, c2, north_normal);
		AddTriangle(roof, c0, c3, r0, Vec3(-1, 0, 0));
		AddTriangle(roof, c1, r1, c2, Vec3(1, 0, 0));
	}
	else {
		// Ridge runs along local Z; slopes span X; gable ends at z=±hd.
		Vec3 r0(0, ridge_y, -hd), r1(0, ridge_y, hd);
		Vec3 west_normal = Vec3(y1 - ridge_y, hw, 0).Normalized();
		Vec3 east_normal = Vec3(ridge_y - y1, hw, 0).Normalized();
		AddQuad(roof, c0, r0, r1, c3, west_normal);
		AddQuad(roof, c1, c2, r1, r0, east_normal);
		AddTriangle(roof, c0, r0, c1, Vec3(0, 0, -1));
		AddTriangle(roof, c3, c2, r1, Vec3(0, 0, 1));
	}
}

END_UPP_NAMESPACE
