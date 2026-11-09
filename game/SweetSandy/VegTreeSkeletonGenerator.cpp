#include "SweetSandy.h"

NAMESPACE_UPP

namespace {

// A branch not yet grown - the work-queue item. Deliberately not named
// `Branch` (or `VegBranch`) to keep it unambiguous next to
// `VegBranchOptions`/`VegBranchSection`/`VegSkeletonBranch`, all of
// which mean something else.
struct VegPendingBranch : Moveable<VegPendingBranch> {
	Vec3 origin;
	VegQuat orientation;
	double length = 0;
	double radius = 0;
	int level = 0;
	int section_count = 0;
	int segment_count = 0;
};

// Fisher-Yates shuffle of [0..count-1] using this project's own RNG, so
// results stay seed-reproducible - port of tree.js's shuffledIndices.
Vector<int> ShuffledIndices(int count)
{
	Vector<int> arr;
	for(int k = 0; k < count; k++)
		arr.Add(k);
	for(int k = count - 1; k > 0; k--) {
		int r = (int)(Randomf() * (k + 1));
		Swap(arr[k], arr[r]);
	}
	return arr;
}

void RecordLeaf(VegSkeleton& skeleton, const VegTreeOptions& options, Vec3 origin, VegQuat orientation)
{
	double size = options.leaves.size *
		(1 + RandomRange(-options.leaves.size_variance, options.leaves.size_variance));
	VegSkeletonLeaf leaf;
	leaf.origin = origin;
	leaf.orientation = orientation;
	leaf.size = size;
	skeleton.leaves.Add(leaf);
}

struct SampledPoint {
	Vec3 origin;
	VegQuat orientation;
	const VegBranchSection *section_a;
	const VegBranchSection *section_b;
	double alpha; // linear position within [section_a, section_b], 0..1
};

// Ported from the shared core of generateChildBranches/generateLeaves:
// given a 0..1 fraction along the parent branch's own length, find the
// straddling section pair and the interpolated origin/orientation
// there.
SampledPoint SampleAlongSections(const Vector<VegBranchSection>& sections, double start_fraction)
{
	int n = sections.GetCount();
	int section_index = (int)(start_fraction * (n - 1));
	const VegBranchSection& a = sections[section_index];
	const VegBranchSection& b = (section_index == n - 1) ? a : sections[section_index + 1];

	double alpha = (start_fraction - (double)section_index / (n - 1)) * (n - 1);

	SampledPoint p;
	p.origin = a.origin + (b.origin - a.origin) * alpha;
	// Orientation interpolates from B toward A as alpha goes 0->1 - the
	// REVERSE of how origin interpolates from A toward B just above.
	// Matches the source exactly (tree.js generateChildBranches/
	// generateLeaves both do `qB.slerp(qA, alpha)`) - almost certainly
	// an unintentional inconsistency upstream (nothing else in either
	// function treats "A" and "B" with reversed roles), but per this
	// project's standing policy on suspected-but-unconfirmed upstream
	// quirks (see AGENTS.md: CityTensor::Add's r=2 hardcode, the
	// pointsBetween off-by-one), ported faithfully rather than silently
	// "fixed" - our own presets (Task 4) get tuned against this actual
	// behavior, not a guessed "intended" one.
	p.orientation = VegQuat::Slerp(b.orientation, a.orientation, alpha);
	p.section_a = &a;
	p.section_b = &b;
	p.alpha = alpha;
	return p;
}

void GenerateChildBranches(Vector<VegPendingBranch>& queue, const VegTreeOptions& options,
                            int count, int level, const Vector<VegBranchSection>& sections)
{
	double radial_offset = Randomf();
	double start_min = options.branch.start[level];
	double height_step = (1.0 - start_min) / count;
	Vector<int> angle_slots = ShuffledIndices(count);

	for(int i = 0; i < count; i++) {
		// Stratified sampling along the parent's own length: jitter
		// within slot [i, i+1] so children spread evenly but not
		// perfectly periodically.
		double child_branch_start = start_min + (i + Randomf()) * height_step;
		SampledPoint p = SampleAlongSections(sections, child_branch_start);

		// radius[level] (the level's own configured radius) scaled by
		// how tapered the PARENT branch is at this point (the
		// interpolated fraction of the parent's own base radius) -
		// ported exactly as written, an unusual-looking but real part
		// of the source's own formula.
		double child_branch_radius = options.branch.radius[level] *
			((1 - p.alpha) * p.section_a->radius + p.alpha * p.section_b->radius);

		// Stratified radial angle: each child gets a 2*pi/count slot,
		// jittered +-half a slot; angle_slots independently permutes
		// slot assignment so the height slot and angle slot are
		// uncorrelated (otherwise evergreens, where length depends on
		// height, spiral their longest branches to one side).
		double radial_jitter = RandomRange(-0.5, 0.5);
		double radial_angle = 2.0 * M_PI * (radial_offset + (angle_slots[i] + radial_jitter) / count);
		VegQuat q1 = VegQuat::FromAxisAngle(Vec3(1, 0, 0), options.branch.angle[level] * M_PI / 180.0);
		VegQuat q2 = VegQuat::FromAxisAngle(Vec3(0, 1, 0), radial_angle);
		VegQuat q3 = p.orientation;
		VegQuat child_orientation = q3 * (q2 * q1);

		double child_branch_length = options.branch.length[level] *
			(options.type == VegTreeType::Evergreen ? 1.0 - child_branch_start : 1.0);

		VegPendingBranch child;
		child.origin = p.origin;
		child.orientation = child_orientation;
		child.length = child_branch_length;
		child.radius = child_branch_radius;
		child.level = level;
		child.section_count = options.branch.sections[level];
		child.segment_count = options.branch.segments[level];
		queue.Add(child);
	}
}

void GenerateLeaves(VegSkeleton& skeleton, const VegTreeOptions& options, const Vector<VegBranchSection>& sections)
{
	double radial_offset = Randomf();
	int count = options.leaves.count;
	double start_min = options.leaves.start;
	double height_step = (1.0 - start_min) / count;
	Vector<int> angle_slots = ShuffledIndices(count);

	for(int i = 0; i < count; i++) {
		double leaf_start = start_min + (i + Randomf()) * height_step;
		SampledPoint p = SampleAlongSections(sections, leaf_start);

		double radial_jitter = RandomRange(-0.5, 0.5);
		double radial_angle = 2.0 * M_PI * (radial_offset + (angle_slots[i] + radial_jitter) / count);
		VegQuat q1 = VegQuat::FromAxisAngle(Vec3(1, 0, 0), options.leaves.angle_deg * M_PI / 180.0);
		VegQuat q2 = VegQuat::FromAxisAngle(Vec3(0, 1, 0), radial_angle);
		VegQuat q3 = p.orientation;
		VegQuat leaf_orientation = q3 * (q2 * q1);

		RecordLeaf(skeleton, options, p.origin, leaf_orientation);
	}
}

// Grows one branch's own section frames, queues its children (if any),
// and records its leaves (if terminal) - port of tree.js's #growBranch.
void GrowBranch(VegSkeleton& skeleton, Vector<VegPendingBranch>& queue, const VegTreeOptions& options,
                 const VegPendingBranch& branch)
{
	Vec3 origin = branch.origin;
	VegQuat orientation = branch.orientation;

	// Ported as written: the source's own divisor here
	// (`branch.length / branch.sectionCount / (type === 'Deciduous' ?
	// levels-1 : 1)`) compares against the capitalized literal
	// 'Deciduous', but options.type only ever holds the lowercase
	// 'deciduous'/'evergreen' (enums.js) - the comparison can never be
	// true, so the `levels-1` divisor is unreachable dead code in
	// ez-tree itself (almost certainly a typo for TreeType.Deciduous,
	// used correctly a few lines below in the very same function for
	// the radius taper). Ported faithfully - i.e. this division never
	// happens - rather than silently "fixed", same policy as the
	// orientation-interpolation quirk in SampleAlongSections above.
	double section_length = branch.length / branch.section_count;

	Vector<VegBranchSection> sections;

	for(int i = 0; i <= branch.section_count; i++) {
		double section_radius = branch.radius;
		if(i == branch.section_count && branch.level == options.branch.levels) {
			// Final section of the final branch level: taper to
			// effectively zero regardless of tree type.
			section_radius = 0.001;
		}
		else if(options.type == VegTreeType::Deciduous) {
			section_radius *= 1 - options.branch.taper[branch.level] * ((double)i / branch.section_count);
		}
		else { // Evergreen: no terminal branch, so it always tapers fully to a point (taper[level] unused)
			section_radius *= 1 - (double)i / branch.section_count;
		}

		VegBranchSection section;
		section.origin = origin;
		section.orientation = orientation;
		section.radius = section_radius;
		sections.Add(section);

		origin = origin + orientation.Apply(Vec3(0, section_length, 0));

		// Gnarliness perturbation: deliberately quaternion-composed
		// here rather than replicating the source's per-section Euler
		// round-trip (sectionOrientation.x/z += ...; setFromEuler;
		// ...; setFromQuaternion) - see 001-task1-math-mesh-
		// foundation.md's own note that VegQuat exists specifically so
		// this task wouldn't need a full order-dependent Euler type
		// just for a randomized noise step. Composed as two independent
		// small local-frame tilts (around the section's own current X
		// then Z axis) rather than THREE.Euler's absolute-component
		// accumulation - same statistical character (a small, RNG-
		// scaled perturbation every section, larger for thinner
		// sections), not a bit-exact reproduction of the source's
		// specific Euler-order coupling.
		double gnarliness = max(1.0, 1.0 / sqrt(section_radius)) * options.branch.gnarliness[branch.level];
		double gx = RandomRange(-gnarliness, gnarliness);
		double gz = RandomRange(-gnarliness, gnarliness);
		orientation = orientation * VegQuat::FromAxisAngle(Vec3(1, 0, 0), gx) * VegQuat::FromAxisAngle(Vec3(0, 0, 1), gz);

		VegQuat q_section = orientation;
		VegQuat q_twist = VegQuat::FromAxisAngle(Vec3(0, 1, 0), options.branch.twist[branch.level]);
		q_section = q_section * q_twist;

		// Bend the section's own "up" toward force.direction (positive
		// strength) or away from it (negative), thin branches bending
		// more (step scales with 1/radius). The (sectionUp x target)
		// axis makes force.direction behave as a real world axis - see
		// tree.js's own comment on why this replaced an earlier,
		// degenerate slerp-toward-force form.
		Vec3 section_up = q_section.Apply(Vec3(0, 1, 0));
		Vec3 target = options.branch.force_direction.Normalized();
		Vec3 axis = section_up.Cross(target);
		double sin_full = axis.Length();
		if(sin_full > 1e-6) {
			axis = axis * (1.0 / sin_full);
			double full_angle = atan2(sin_full, section_up.Dot(target));
			double step = options.branch.force_strength / section_radius;
			double clamped = Clampd(step, -full_angle, full_angle);
			VegQuat q_force = VegQuat::FromAxisAngle(axis, clamped);
			q_section = q_force * q_section;
		}

		// No trellis force - excluded entirely, see 001-planning.md.

		orientation = q_section;
	}

	// Deciduous trees grow a terminal branch continuing straight out of
	// a non-terminal branch's own tip; evergreens never do (they taper
	// to a point via the radius formula above instead). Reads `sections`
	// (not yet moved into the skeleton below) and `branch`'s own
	// section/segment count - both still needed by generateLeaves/
	// generateChildBranches right after, so the branch only gets pushed
	// into `skeleton.branches` at the very end of this function.
	const VegBranchSection& last = sections.Top();
	if(options.type == VegTreeType::Deciduous) {
		if(branch.level < options.branch.levels) {
			VegPendingBranch terminal;
			terminal.origin = last.origin;
			terminal.orientation = last.orientation;
			terminal.length = options.branch.length[branch.level + 1];
			terminal.radius = last.radius;
			terminal.level = branch.level + 1;
			// Section/segment count come from the PARENT branch, not
			// options.branch.sections/segments[level+1] - the terminal
			// continuation must share its parent's own ring resolution
			// since it grows seamlessly out of the parent's own tip
			// (tree.js's own comment on this exact point).
			terminal.section_count = branch.section_count;
			terminal.segment_count = branch.segment_count;
			queue.Add(terminal);
		}
		else {
			RecordLeaf(skeleton, options, last.origin, last.orientation);
		}
	}

	if(branch.level == options.branch.levels)
		GenerateLeaves(skeleton, options, sections);
	else if(branch.level < options.branch.levels)
		GenerateChildBranches(queue, options, options.branch.children[branch.level], branch.level + 1, sections);

	VegSkeletonBranch skeleton_branch;
	skeleton_branch.sections = pick(sections);
	skeleton_branch.segment_count = branch.segment_count;
	skeleton_branch.base_radius = branch.radius;
	skeleton.branches.Add(pick(skeleton_branch));
}

} // anonymous namespace

VegSkeleton VegTreeSkeletonGenerator::Generate(const VegTreeOptions& options)
{
	SeedRandom(options.seed);

	VegSkeleton skeleton;
	Vector<VegPendingBranch> queue;

	VegPendingBranch trunk;
	trunk.origin = Vec3(0, 0, 0);
	trunk.orientation = VegQuat::Identity();
	trunk.length = options.branch.length[0];
	trunk.radius = options.branch.radius[0];
	trunk.level = 0;
	trunk.section_count = options.branch.sections[0];
	trunk.segment_count = options.branch.segments[0];
	queue.Add(trunk);

	while(!queue.IsEmpty()) {
		VegPendingBranch branch = pick(queue[0]);
		queue.Remove(0);
		GrowBranch(skeleton, queue, options, branch);
	}

	return skeleton;
}

END_UPP_NAMESPACE
