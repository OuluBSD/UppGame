#include "SweetSandy.h"

NAMESPACE_UPP

CityTensor::CityTensor(double r_, double m0_, double m1_) : r(r_), m0(m0_), m1(m1_)
{
	RecalcTheta();
}

void CityTensor::RecalcTheta()
{
	theta = r == 0 ? 0 : atan2(m1 / r, m0 / r) / 2;
}

CityTensor CityTensor::FromAngle(double angle)
{
	return CityTensor(1, cos(angle * 4), sin(angle * 4));
}

CityTensor CityTensor::FromVector(const Vec2& v)
{
	double t1 = v.x * v.x - v.y * v.y;
	double t2 = 2 * v.x * v.y;
	double t3 = t1 * t1 - t2 * t2;
	double t4 = 2 * t1 * t2;
	return CityTensor(1, t3, t4);
}

CityTensor& CityTensor::Add(const CityTensor& t, bool smooth)
{
	double new_m0 = m0 * r + t.m0 * t.r;
	double new_m1 = m1 * r + t.m1 * t.r;
	m0 = new_m0;
	m1 = new_m1;

	if(smooth) {
		r = sqrt(m0 * m0 + m1 * m1);
		if(r > 1e-12) {
			m0 /= r;
			m1 /= r;
		}
	}
	else {
		r = 2;
	}

	RecalcTheta();
	return *this;
}

CityTensor& CityTensor::Scale(double s)
{
	r *= s;
	RecalcTheta();
	return *this;
}

CityTensor& CityTensor::Rotate(double delta_theta)
{
	if(delta_theta == 0)
		return *this;

	double new_theta = theta + delta_theta;
	if(new_theta < M_PI)
		new_theta += M_PI;
	if(new_theta >= M_PI)
		new_theta -= M_PI;

	m0 = cos(2 * new_theta) * r;
	m1 = sin(2 * new_theta) * r;
	theta = new_theta;
	return *this;
}

Vec2 CityTensor::GetMajor() const
{
	if(r == 0)
		return Vec2(0, 0);
	return Vec2(cos(theta), sin(theta));
}

Vec2 CityTensor::GetMinor() const
{
	if(r == 0)
		return Vec2(0, 0);
	double angle = theta + M_PI / 2;
	return Vec2(cos(angle), sin(angle));
}

END_UPP_NAMESPACE
