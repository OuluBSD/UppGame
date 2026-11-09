#ifndef _SweetSandy_VegQuat_h_
#define _SweetSandy_VegQuat_h_

// Vegetation phase, Task 1: a rotation-composing type for branch
// orientation - see Manager/2-plan/.../005-vegetation/
// 001-task1-math-mesh-foundation.md.
//
// Named `VegQuat`, deliberately not `Quat`/`Quaternion`: uppsrc/Geometry/
// Matrix.h already declares `struct Quaternion` at the same BLITZ-merged
// Upp namespace scope this game's code lives in (Geometry links in
// transitively via this project's existing package graph) - the exact
// ODR-collision class of bug the WorldOctree incident taught this
// project to grep for before introducing a name (see AGENTS.md). Grepped
// clean at planning time; `VegQuat`/`VegMesh`/`VegMeshVertex` re-grepped
// clean again at this task's own implementation time.
//
// Operations are ported to match THREE.js's own Quaternion semantics
// exactly (ez-tree's skeleton-growth math - Task 2 - is written against
// those semantics and would silently misbehave under a differently-
// conventioned quaternion library): `operator*` composes like
// `THREE.Quaternion.multiply` (`a * b` applies b's rotation in the frame
// *before* a's), and `RotateTowards` is an angle-*limited* step (not a
// 0..1 lerp fraction), matching `THREE.Quaternion.rotateTowards` exactly
// - see that method's own note below.
struct VegQuat : Moveable<VegQuat> {
	double x = 0, y = 0, z = 0, w = 1; // identity by default

	static VegQuat Identity() { return VegQuat(); }
	static VegQuat FromAxisAngle(Vec3 axis, double angle_rad);

	// a * b: b's rotation is applied first (in a's parent frame), then
	// a's - matches THREE.Quaternion::multiply(q)'s own "this = this * q"
	// semantics exactly. Get this composition order backwards and small
	// perturbations (gnarliness, force-bending) still look plausible,
	// but angular placement (children/leaves spiraling around a branch)
	// comes out subtly wrong - verified directly, not just "looks like a
	// tree" (see the task doc's own note).
	VegQuat operator*(const VegQuat& b) const;

	Vec3 Apply(Vec3 v) const; // rotate v by this quaternion
	VegQuat Normalized() const;

	static VegQuat Slerp(const VegQuat& a, const VegQuat& b, double t);

	// Rotates `from` toward `to` by AT MOST max_angle_rad - an angle-
	// limited step, not a 0..1 lerp fraction. If the angle between `from`
	// and `to` is already <= max_angle_rad, returns `to` exactly.
	// max_angle_rad <= 0 returns `from` exactly. Matches
	// THREE.Quaternion::rotateTowards's own semantics (angleTo + a
	// clamped-fraction slerp) - ez-tree's trellis-force code (excluded
	// from this port, see 001-planning.md) relied on exactly this
	// "clamped step" behavior, not a fractional blend.
	static VegQuat RotateTowards(const VegQuat& from, const VegQuat& to, double max_angle_rad);

	void Serialize(Stream& s) { s % x % y % z % w; }
};

#endif
