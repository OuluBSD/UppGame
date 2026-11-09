#include "SweetSandy.h"

NAMESPACE_UPP

Vec2 CityStreamlineIntegrator::SampleFieldVector(Vec2 point, bool major) const
{
	CityTensor t = field.SamplePoint(point);
	return major ? t.GetMajor() : t.GetMinor();
}

Vec2 CityStreamlineIntegrator::Integrate(Vec2 point, bool major) const
{
	Vec2 k1 = SampleFieldVector(point, major);
	Vec2 k23 = SampleFieldVector(point + Vec2(params.dstep / 2, params.dstep / 2), major);
	Vec2 k4 = SampleFieldVector(point + Vec2(params.dstep, params.dstep), major);
	return (k1 + k23 * 4 + k4) * (params.dstep / 6);
}

END_UPP_NAMESPACE
