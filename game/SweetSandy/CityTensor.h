#ifndef _SweetSandy_CityTensor_h_
#define _SweetSandy_CityTensor_h_

// City Layout phase, Task 1: 2D "line field" tensor - port of
// MapGenerator's Tensor class (src/ts/impl/tensor.ts), see
// Manager/2-plan/.../003-city-layout/001-task1-tensor-field.md.
//
// A road direction at a point has no preferred sign (a street pointing
// "east" is the same street as one pointing "west"), so it can't be
// represented as a plain vector - summing two vectors pointing in
// opposite-but-collinear directions would cancel out to zero instead of
// reinforcing the shared direction. The standard fix (Chen et al. 2008)
// is a symmetric, traceless 2x2 tensor, stored compactly here as
// magnitude `r` and the double-angle pair `(m0, m1) = (r*cos(2*theta),
// r*sin(2*theta))` - summing several of these component-wise and
// re-deriving theta from the sum is what lets several basis fields (a
// grid downtown, a radial field around a park, ...) combine into one
// field whose eigenvectors give the locally-preferred road direction.
//
// Simplified from the source: theta is recalculated eagerly on every
// mutation instead of lazily via an `oldTheta` dirty flag - same result,
// this isn't a hot enough path to need the laziness.
class CityTensor {
	double r = 0;
	double m0 = 0, m1 = 0; // [r*cos(2*theta), r*sin(2*theta)]
	double theta = 0;

	void RecalcTheta();

public:
	CityTensor() {}
	CityTensor(double r, double m0, double m1);

	static CityTensor FromAngle(double angle);   // Tensor.fromAngle
	static CityTensor FromVector(const Vec2& v); // Tensor.fromVector
	static CityTensor Zero()                     { return CityTensor(0, 0, 0); }

	CityTensor& Add(const CityTensor& t, bool smooth);
	CityTensor& Scale(double s);
	CityTensor& Rotate(double delta_theta); // radians

	Vec2   GetMajor() const;
	Vec2   GetMinor() const;
	double GetR() const     { return r; }
	double GetTheta() const { return theta; }
};

#endif
