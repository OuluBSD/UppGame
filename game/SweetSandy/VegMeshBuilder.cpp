#include "SweetSandy.h"

NAMESPACE_UPP

namespace {

// Port of Tree#meshBranch - one branch's ring geometry (an open tube, no
// end caps - a branch's very tip is either covered by a child branch's
// own base ring, or for a true terminal tip just isn't capped in the
// source either).
void MeshBranch(VegMesh& out, const VegSkeletonBranch& branch, int section_stride, double segment_factor)
{
	int segments = max(3, (int)round(branch.segment_count * segment_factor));

	// Number of texture wraps around the branch's circumference -
	// ez-tree scales this by a bark.textureScale.x tunable this port has
	// no equivalent for (no texture/material system - see
	// VegTreeOptions.h's own "no textures" decision), so this always
	// uses ez-tree's own default multiplier (1.0). UVs are computed but
	// unconsumed regardless (same call already made for RoadMesh).
	int wraps_x = max(1, (int)round(branch.base_radius));

	// Sample every Nth ring, always keeping the first and last so branch
	// endpoints (and parent/child junctions) stay put across detail
	// levels.
	Vector<const VegBranchSection *> sampled;
	int n = branch.sections.GetCount();
	for(int i = 0; i < n; i += section_stride)
		sampled.Add(&branch.sections[i]);
	if((n - 1) % section_stride != 0)
		sampled.Add(&branch.sections.Top());

	int index_offset = out.vertices.GetCount();

	for(int k = 0; k < sampled.GetCount(); k++) {
		const VegBranchSection& section = *sampled[k];
		VegMeshVertex first;
		for(int j = 0; j < segments; j++) {
			double angle = 2.0 * M_PI * j / segments;
			Vec3 local(cos(angle), 0, sin(angle));
			VegMeshVertex v;
			v.pos = section.orientation.Apply(local * section.radius) + section.origin;
			v.normal = section.orientation.Apply(local).Normalized();
			// uv.y alternates by sampled ring position (k), not the
			// section's own original index, so section skipping keeps
			// the 0/1 tiling pattern - matches the source exactly.
			v.uv = Vec2((double)j / segments * wraps_x, (k % 2 == 0) ? 0.0 : 1.0);
			out.vertices.Add(v);
			if(j == 0)
				first = v;
		}
		// Duplicate the first vertex so there is continuity in the UV
		// mapping: u=wrapsX maps to the same texel as u=0 since wrapsX
		// is an integer.
		VegMeshVertex dup = first;
		dup.uv = Vec2(wraps_x, first.uv.y);
		out.vertices.Add(dup);
	}

	int stride = segments + 1; // +1 for the UV-seam duplicate vertex per ring
	for(int i = 0; i < sampled.GetCount() - 1; i++) {
		for(int j = 0; j < segments; j++) {
			int v1 = index_offset + i * stride + j;
			int v2 = index_offset + i * stride + (j + 1);
			int v3 = v1 + stride;
			int v4 = v2 + stride;
			out.triangles.Add(v1); out.triangles.Add(v3); out.triangles.Add(v2);
			out.triangles.Add(v2); out.triangles.Add(v3); out.triangles.Add(v4);
		}
	}
}

// Emits one leaf quad, pre-rotated by `rotation` (radians, around the
// leaf's own local Y axis, before `orientation` places it in world
// space) - called twice per leaf (0 and pi/2) for a double billboard.
// Port of the inner `createLeaf` closure in Tree#meshLeaf.
void MeshLeafQuad(VegMesh& out, Vec3 origin, VegQuat orientation, double size, bool rounded_normals, double rotation)
{
	int base = out.vertices.GetCount();
	double w = size, l = size;
	Vec3 local[4] = {
		Vec3(-w / 2, l, 0),
		Vec3(-w / 2, 0, 0),
		Vec3(w / 2, 0, 0),
		Vec3(w / 2, l, 0),
	};

	// Applying `rotation` around Y then `orientation` to a vector, in
	// sequence, equals applying the single composed quaternion
	// `orientation * y_rotation` once (our operator* matches THREE's
	// `a.multiply(b)` => `a = a*b`, i.e. "b applied first, in a's parent
	// frame, then a" - see VegQuat.h) - the direct quaternion equivalent
	// of the source's `v.applyEuler(Euler(0,rotation,0)).applyEuler(orientation)`.
	VegQuat combined = orientation * VegQuat::FromAxisAngle(Vec3(0, 1, 0), rotation);

	// The "flat" normal direction is always orientation.Apply((0,0,1)) -
	// NOT re-derived per quad from `rotation` - ported exactly as
	// written: both quads of a double billboard share the same base
	// outward direction, which combined with roundedNormals' per-vertex
	// blending below gives a consistent "puffy" look across both quads
	// instead of each quad's own flat, 90-degrees-apart normal (which
	// would look inconsistent under lighting for an X-shaped cross of
	// two quads) - a deliberate ez-tree choice, not a bug.
	Vec3 n = orientation.Apply(Vec3(0, 0, 1));

	Vec2 uvs[4] = { Vec2(0, 1), Vec2(0, 0), Vec2(1, 0), Vec2(1, 1) };

	for(int k = 0; k < 4; k++) {
		Vec3 world = combined.Apply(local[k]) + origin;
		VegMeshVertex v;
		v.pos = world;
		// The rounded-normals trick: blend the flat normal toward the
		// direction from the leaf's own origin to this corner, for a
		// fake rounded-canopy look without more geometry.
		v.normal = rounded_normals ? (n + world - origin).Normalized() : n;
		v.uv = uvs[k];
		out.vertices.Add(v);
	}
	out.triangles.Add(base); out.triangles.Add(base + 1); out.triangles.Add(base + 2);
	out.triangles.Add(base); out.triangles.Add(base + 2); out.triangles.Add(base + 3);
}

// Port of Tree#meshLeaf: one (single) or two perpendicular (double)
// billboard quads per skeleton leaf.
void MeshLeaf(VegMesh& out, const VegSkeletonLeaf& leaf, const VegTreeOptions& options, const VegMeshDetail& detail)
{
	double leaf_size = leaf.size * detail.leaf_scale;
	bool double_billboard = !detail.force_single_billboard && options.leaves.double_billboard;

	MeshLeafQuad(out, leaf.origin, leaf.orientation, leaf_size, options.leaves.rounded_normals, 0.0);
	if(double_billboard)
		MeshLeafQuad(out, leaf.origin, leaf.orientation, leaf_size, options.leaves.rounded_normals, M_PI / 2);
}

} // anonymous namespace

void VegMeshBuilder::Build(const VegSkeleton& skeleton, const VegTreeOptions& options,
                            const VegMeshDetail& detail, VegMesh& out_branches, VegMesh& out_leaves)
{
	out_branches.vertices.Clear();
	out_branches.triangles.Clear();
	out_leaves.vertices.Clear();
	out_leaves.triangles.Clear();

	int section_stride = max(1, detail.section_stride);
	int leaf_stride = max(1, detail.leaf_stride);

	for(const VegSkeletonBranch& b : skeleton.branches)
		MeshBranch(out_branches, b, section_stride, detail.segment_factor);

	for(int i = 0; i < skeleton.leaves.GetCount(); i += leaf_stride)
		MeshLeaf(out_leaves, skeleton.leaves[i], options, detail);
}

END_UPP_NAMESPACE
