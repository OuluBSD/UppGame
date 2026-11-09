#include "SweetSandy.h"

NAMESPACE_UPP

double CityBasisField::GetTensorWeight(Vec2 point, bool smooth) const
{
	double norm_dist = (point - centre).Length() / size;
	if(smooth)
		return pow(norm_dist, -decay);
	if(decay == 0 && norm_dist >= 1)
		return 0;
	return pow(max(0.0, 1 - norm_dist), decay);
}

CityTensor CityBasisField::GetWeightedTensor(Vec2 point, bool smooth) const
{
	return GetTensor(point).Scale(GetTensorWeight(point, smooth));
}

CityTensor CityGridField::GetTensor(Vec2 point) const
{
	return CityTensor(1, cos(2 * theta), sin(2 * theta));
}

CityTensor CityRadialField::GetTensor(Vec2 point) const
{
	Vec2 t = point - centre;
	double t1 = t.y * t.y - t.x * t.x;
	double t2 = -2 * t.x * t.y;
	return CityTensor(1, t1, t2);
}

END_UPP_NAMESPACE
