#ifndef _SweetSandy_VegTreeOptions_h_
#define _SweetSandy_VegTreeOptions_h_

// Vegetation phase, Task 2: port of ez-tree's TreeOptions (options.js) -
// see Manager/2-plan/.../005-vegetation/002-task2-skeleton-generation.md.
//
// Per-branch-level values are Vector<T> indexed by level directly
// (angle[level], not a JS object keyed by "1"/"2"/...), sized to
// branch.levels + 1 by whoever builds a VegTreeOptions (a preset loader,
// Task 4 - not enforced here, same "caller's responsibility" convention
// MapGeneratorCityLayoutGenerator's own CityStreamlineParams already
// follows for its own tunables).
//
// No bark.maps/leaves.map texture slots (no texture/material system in
// this game - see 001-planning.md's "no textures" simplification) -
// bark/leaf *tint* colors stay, they're just flat fill colors for
// Task 3's meshing/PlayScreen's drawing, not material inputs.

enum class VegTreeType { Deciduous, Evergreen };

struct VegBranchOptions {
	int levels = 3; // 0 = trunk only

	Vector<double> angle;      // index 1..levels (level 0 has no parent to angle from)
	Vector<int> children;      // index 0..levels-1 (level `levels` is terminal - spawns leaves, not children)
	Vec3 force_direction = Vec3(0, 1, 0);
	double force_strength = 0.01;
	Vector<double> gnarliness; // index 0..levels
	Vector<double> length;     // index 0..levels, meters (real-world-scaled presets are Task 4's concern)
	Vector<double> radius;     // index 0..levels, meters
	Vector<int> sections;      // index 0..levels
	Vector<int> segments;      // index 0..levels
	Vector<double> start;      // index 1..levels, 0..1 along the parent's own length
	Vector<double> taper;      // index 0..levels, deciduous only (see VegTreeSkeletonGenerator.cpp)
	Vector<double> twist;      // index 0..levels, radians
};

struct VegLeafOptions {
	double angle_deg = 10;
	int count = 1;
	double start = 0; // 0..1 along the terminal branch's own length
	double size = 2.5;
	double size_variance = 0.7;
	bool double_billboard = true;
	bool rounded_normals = true;
};

struct VegTreeOptions {
	dword seed = 0; // fed directly into SeedRandom - see VegTreeSkeletonGenerator.h
	VegTreeType type = VegTreeType::Deciduous;
	VegBranchOptions branch;
	VegLeafOptions leaves;
	Color bark_tint = Color(255, 255, 255);
	Color leaf_tint = Color(255, 255, 255);
};

#endif
