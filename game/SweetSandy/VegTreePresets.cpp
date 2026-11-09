#include "SweetSandy.h"

NAMESPACE_UPP

namespace {

// Oak: deciduous, ~9.5m tall (trunk 5m + two terminal continuations at
// 3m/1.5m, per VegTreeSkeletonGenerator's own deciduous terminal-branch
// chain - see 002-task2-skeleton-generation.md). Two branch levels kept
// deliberately low (not ez-tree's own levels=3) per 001-planning.md's
// draw-cost risk - "low-poly by design", matching the smallest end of
// ez-tree's own presets rather than its highest-detail demo trees.
VegTreeOptions MakeOakPreset(dword seed)
{
	VegTreeOptions opt;
	opt.seed = seed;
	opt.type = VegTreeType::Deciduous;

	opt.branch.levels = 2;
	for(double v : { 0.0, 55.0, 50.0 })    opt.branch.angle.Add(v);
	for(int v : { 4, 2, 0 })               opt.branch.children.Add(v);
	opt.branch.force_direction = Vec3(0, 1, 0);
	opt.branch.force_strength = 0.02;
	for(double v : { 0.08, 0.10, 0.15 })   opt.branch.gnarliness.Add(v);
	for(double v : { 5.0, 3.0, 1.5 })      opt.branch.length.Add(v);
	for(double v : { 0.35, 0.15, 0.05 })   opt.branch.radius.Add(v);
	for(int v : { 6, 4, 3 })               opt.branch.sections.Add(v);
	for(int v : { 7, 5, 4 })               opt.branch.segments.Add(v);
	for(double v : { 0.0, 0.4, 0.3 })      opt.branch.start.Add(v);
	for(double v : { 0.6, 0.6, 0.6 })      opt.branch.taper.Add(v);
	for(double v : { 0.0, 0.0, 0.0 })      opt.branch.twist.Add(v);

	opt.leaves.angle_deg = 40;
	opt.leaves.count = 5;
	opt.leaves.start = 0.2;
	opt.leaves.size = 1.0;
	opt.leaves.size_variance = 0.4;
	opt.leaves.double_billboard = true;
	opt.leaves.rounded_normals = true;

	opt.bark_tint = Color(90, 60, 40);
	opt.leaf_tint = Color(60, 140, 50);
	return opt;
}

// Pine: evergreen, one branch level with many children (needle-cluster
// branches) spanning the trunk's own length - the natural cone silhouette
// evergreen's own "child length scales with 1-start" formula produces
// (see VegTreeSkeletonGenerator.cpp's GenerateChildBranches). ~8m tall
// (trunk length[0], since evergreen tapers fully to a point at its own
// tip - no terminal continuation to add more height).
VegTreeOptions MakePinePreset(dword seed)
{
	VegTreeOptions opt;
	opt.seed = seed;
	opt.type = VegTreeType::Evergreen;

	opt.branch.levels = 1;
	for(double v : { 0.0, 100.0 })  opt.branch.angle.Add(v);
	for(int v : { 18, 0 })          opt.branch.children.Add(v);
	opt.branch.force_direction = Vec3(0, 1, 0);
	opt.branch.force_strength = -0.01; // slight droop, matching a real evergreen's branches
	for(double v : { 0.05, 0.05 })  opt.branch.gnarliness.Add(v);
	for(double v : { 8.0, 3.5 })    opt.branch.length.Add(v);
	for(double v : { 0.30, 0.05 })  opt.branch.radius.Add(v);
	for(int v : { 8, 3 })           opt.branch.sections.Add(v);
	for(int v : { 7, 4 })           opt.branch.segments.Add(v);
	for(double v : { 0.0, 0.05 })   opt.branch.start.Add(v);
	for(double v : { 1.0, 1.0 })    opt.branch.taper.Add(v); // unused by the evergreen radius formula, set anyway for completeness
	for(double v : { 0.0, 0.0 })    opt.branch.twist.Add(v);

	opt.leaves.angle_deg = 35;
	opt.leaves.count = 12;
	opt.leaves.start = 0.1;
	opt.leaves.size = 0.7;
	opt.leaves.size_variance = 0.2;
	opt.leaves.double_billboard = true;
	opt.leaves.rounded_normals = true;

	opt.bark_tint = Color(70, 50, 35);
	opt.leaf_tint = Color(40, 90, 45);
	return opt;
}

} // anonymous namespace

VegTreeOptions MakeVegTreePreset(VegTreePreset preset, dword seed)
{
	switch(preset) {
	case VegTreePreset::Pine: return MakePinePreset(seed);
	default:                  return MakeOakPreset(seed);
	}
}

END_UPP_NAMESPACE
