#ifndef _SweetSandy_VegMeshBuilder_h_
#define _SweetSandy_VegMeshBuilder_h_

// Vegetation phase, Task 3: port of ez-tree's Tree#meshSkeleton/
// #meshBranch/#meshLeaf - turns Task 2's skeleton into real triangulated
// geometry (VegMesh, Task 1). See Manager/2-plan/.../005-vegetation/
// 003-task3-meshing.md and VegMeshBuilder.cpp for the line-by-line
// reasoning.

// LOD detail knobs, ported from ez-tree's own LODDetail. Default
// (VegMeshDetail{}) reproduces the full-detail mesh - same "default
// level = no reduction" convention the source's own JSDoc documents.
// Actual per-level values (roughly matching ez-tree's own
// defaultLODLevels proportions) are chosen against this project's own
// octree LOD distances in Task 4, not fixed here.
struct VegMeshDetail : Moveable<VegMeshDetail> {
	int section_stride = 1;              // keep every Nth section ring
	double segment_factor = 1.0;         // radial segment multiplier
	int leaf_stride = 1;                 // keep every Nth leaf
	double leaf_scale = 1.0;             // size multiplier for kept leaves
	bool force_single_billboard = false; // override options.leaves.double_billboard for this level
};

class VegMeshBuilder {
public:
	// Branches and leaves stay two separate VegMesh outputs (not merged
	// into one) because they need two different flat fill colors (bark
	// tint vs. leaf tint) - matches ez-tree keeping branchesMesh/
	// leavesMesh as two separate THREE.Mesh objects for the same reason,
	// now that there's no material system to carry the distinction
	// instead. Consumes no RNG (the skeleton already fixed every
	// position/size/orientation), so this can run repeatedly against one
	// skeleton at different VegMeshDetail levels - Task 4's own LOD use.
	static void Build(const VegSkeleton& skeleton, const VegTreeOptions& options,
	                   const VegMeshDetail& detail, VegMesh& out_branches, VegMesh& out_leaves);
};

#endif
