#ifndef _SweetSandy_VegSkeleton_h_
#define _SweetSandy_VegSkeleton_h_

// Vegetation phase, Task 2: the grown tree's shape - every branch's
// section frames, every leaf's placement - with no geometry yet
// (Task 3 meshes this). See Manager/2-plan/.../005-vegetation/
// 002-task2-skeleton-generation.md.
struct VegBranchSection : Moveable<VegBranchSection> {
	Vec3 origin;
	VegQuat orientation;
	double radius = 0;
};

struct VegSkeletonBranch : Moveable<VegSkeletonBranch> {
	Vector<VegBranchSection> sections;
	int segment_count = 0;  // carried through for Task 3's ring meshing
	double base_radius = 0; // the branch's own starting radius (pre-taper) - carried through for Task 3's bark-texture wrap calc
};

struct VegSkeletonLeaf : Moveable<VegSkeletonLeaf> {
	Vec3 origin;
	VegQuat orientation;
	double size = 0;
};

struct VegSkeleton : Moveable<VegSkeleton> {
	Vector<VegSkeletonBranch> branches;
	Vector<VegSkeletonLeaf> leaves;
};

#endif
