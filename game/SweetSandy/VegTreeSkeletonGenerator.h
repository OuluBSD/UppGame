#ifndef _SweetSandy_VegTreeSkeletonGenerator_h_
#define _SweetSandy_VegTreeSkeletonGenerator_h_

// Vegetation phase, Task 2: port of ez-tree's Tree#generateSkeleton/
// #growBranch/generateChildBranches/generateLeaves/#recordLeaf - see
// Manager/2-plan/.../005-vegetation/002-task2-skeleton-generation.md
// for the full design (deciduous vs. evergreen rules, two suspected
// upstream quirks found and kept faithfully, and the deliberate
// quaternion-only deviation from the source's per-section Euler
// round-trip) and VegTreeSkeletonGenerator.cpp for the line-by-line
// reasoning.
class VegTreeSkeletonGenerator {
public:
	// Seeds this project's own SeedRandom() from options.seed at the very
	// start (not a ported RNG class - see 001-planning.md), so this call
	// is self-contained and order-independent regardless of what else
	// has touched the shared global RNG stream before it - the same
	// convention MapGeneratorCityLayoutGenerator::Generate already
	// follows for its own world_seed/chunk determinism.
	static VegSkeleton Generate(const VegTreeOptions& options);
};

#endif
