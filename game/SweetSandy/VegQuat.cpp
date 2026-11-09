#include "SweetSandy.h"

NAMESPACE_UPP

VegQuat VegQuat::FromAxisAngle(Vec3 axis, double angle_rad)
{
	Vec3 a = axis.Normalized();
	double half = angle_rad * 0.5;
	double s = sin(half);
	VegQuat q;
	q.x = a.x * s;
	q.y = a.y * s;
	q.z = a.z * s;
	q.w = cos(half);
	return q;
}

// Standard Hamilton product, same term-for-term layout as THREE.js's
// Quaternion::multiplyQuaternions(a, b) so this port matches its
// semantics exactly rather than just "some" valid quaternion multiply
// convention (of which there are two, differing in composition order).
VegQuat VegQuat::operator*(const VegQuat& b) const
{
	const VegQuat& a = *this;
	VegQuat r;
	r.x = a.x * b.w + a.w * b.x + a.y * b.z - a.z * b.y;
	r.y = a.y * b.w + a.w * b.y + a.z * b.x - a.x * b.z;
	r.z = a.z * b.w + a.w * b.z + a.x * b.y - a.y * b.x;
	r.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
	return r;
}

// Port of THREE.js's Vector3::applyQuaternion - the "t = 2*cross(q.xyz,
// v); v + q.w*t + cross(q.xyz, t)" formula, algebraically equivalent to
// q * (v,0) * conjugate(q) but without materializing the intermediate
// quaternion product.
Vec3 VegQuat::Apply(Vec3 v) const
{
	double tx = 2 * (y * v.z - z * v.y);
	double ty = 2 * (z * v.x - x * v.z);
	double tz = 2 * (x * v.y - y * v.x);
	return Vec3(
		v.x + w * tx + y * tz - z * ty,
		v.y + w * ty + z * tx - x * tz,
		v.z + w * tz + x * ty - y * tx);
}

VegQuat VegQuat::Normalized() const
{
	double len = sqrt(x * x + y * y + z * z + w * w);
	if(len < 1e-12)
		return Identity();
	VegQuat q;
	q.x = x / len;
	q.y = y / len;
	q.z = z / len;
	q.w = w / len;
	return q;
}

// Port of THREE.js's Quaternion::slerp, kept as a pure function (a, b, t)
// -> result instead of the source's in-place "this.slerp(qb, t)" to
// match this codebase's own value-type convention (Vec2/Vec3 return new
// values rather than mutate).
VegQuat VegQuat::Slerp(const VegQuat& a, const VegQuat& b, double t)
{
	if(t == 0)
		return a;
	if(t == 1)
		return b;

	double bx = b.x, by = b.y, bz = b.z, bw = b.w;
	double cos_half_theta = a.w * bw + a.x * bx + a.y * by + a.z * bz;

	// Take the shorter path around the 4D hypersphere: negating b (same
	// rotation, opposite sign) when the dot product is negative.
	if(cos_half_theta < 0) {
		bw = -bw; bx = -bx; by = -by; bz = -bz;
		cos_half_theta = -cos_half_theta;
	}

	if(cos_half_theta >= 1.0)
		return a;

	double sqr_sin_half_theta = 1.0 - cos_half_theta * cos_half_theta;

	// a and b are (nearly) antipodal-adjusted-identical - linear blend,
	// renormalized, avoids a division by ~0 in the sin(halfTheta) path
	// below. DBL_EPSILON-scale threshold, same as the source's own
	// Number.EPSILON check.
	if(sqr_sin_half_theta <= 2.220446049250313e-16) {
		double s = 1 - t;
		VegQuat r;
		r.w = s * a.w + t * bw;
		r.x = s * a.x + t * bx;
		r.y = s * a.y + t * by;
		r.z = s * a.z + t * bz;
		return r.Normalized();
	}

	double sin_half_theta = sqrt(sqr_sin_half_theta);
	double half_theta = atan2(sin_half_theta, cos_half_theta);
	double ratio_a = sin((1 - t) * half_theta) / sin_half_theta;
	double ratio_b = sin(t * half_theta) / sin_half_theta;

	VegQuat r;
	r.w = a.w * ratio_a + bw * ratio_b;
	r.x = a.x * ratio_a + bx * ratio_b;
	r.y = a.y * ratio_a + by * ratio_b;
	r.z = a.z * ratio_a + bz * ratio_b;
	return r;
}

// Port of THREE.js's Quaternion::rotateTowards (== angleTo + a clamped-
// fraction slerp): angle = 2*acos(|dot(from,to)|) is the true angular
// distance between the two orientations (as unit quaternions double-
// cover SO(3), the abs() picks the shorter of the two representations);
// t = min(1, max_angle_rad / angle) turns "rotate by at most this many
// radians" into the fraction Slerp needs.
VegQuat VegQuat::RotateTowards(const VegQuat& from, const VegQuat& to, double max_angle_rad)
{
	double dot = from.x * to.x + from.y * to.y + from.z * to.z + from.w * to.w;
	dot = Clampd(dot, -1.0, 1.0);
	double angle = 2 * acos(fabs(dot));
	if(angle < 1e-12)
		return from;
	double t = Clampd(max_angle_rad / angle, 0.0, 1.0);
	return Slerp(from, to, t);
}

END_UPP_NAMESPACE
