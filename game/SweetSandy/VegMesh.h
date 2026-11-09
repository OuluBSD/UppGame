#ifndef _SweetSandy_VegMesh_h_
#define _SweetSandy_VegMesh_h_

// Vegetation phase, Task 1: a generic 3D triangle mesh buffer (position +
// normal + uv per vertex, flat index list) - see Manager/2-plan/.../
// 005-vegetation/001-task1-math-mesh-foundation.md.
//
// RoadMesh (Road Network phase) doesn't fit here: it's deliberately 2D
// with no normals (a flat ribbon glued to Terrain's own height at use
// time - see RoadMesh.h). A branch's cylinder-ring geometry and a leaf's
// billboard quad are inherently 3D with real per-vertex normals (basic
// flat/lit shading for bark rings, plus the "rounded normals" trick for
// leaves - see Task 3's own doc) - hence a separate type rather than
// stretching RoadMesh's shape to cover both.
//
// Kept SweetSandy-local rather than moved into uppsrc/, matching
// RoadMesh's own precedent: a genuinely generic "3D mesh with normals"
// container is a plausible uppsrc/ candidate in the abstract, but with
// only this phase's own branch/leaf meshing (Task 3) as a consumer so
// far, the extra indirection of a shared package isn't earning its keep
// yet - revisit if a second, unrelated consumer shows up.
struct VegMeshVertex : Moveable<VegMeshVertex> {
	Vec3 pos;
	Vec3 normal;
	Vec2 uv; // computed for future-proofing, same call already made for
	         // RoadMesh - no texture/material system exists yet to
	         // consume this.

	void Serialize(Stream& s) { s % pos % normal % uv; }
};

struct VegMesh : Moveable<VegMesh> {
	Vector<VegMeshVertex> vertices;
	Vector<int> triangles; // indices into vertices, 3 per triangle

	void Serialize(Stream& s) { s % vertices % triangles; }
};

#endif
