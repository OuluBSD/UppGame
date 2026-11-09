#include "SweetSandy.h"

NAMESPACE_UPP

void CityTensorField::AddGrid(Vec2 centre, double size, double decay, double theta)
{
	fields.Add(new CityGridField(centre, size, decay, theta));
}

void CityTensorField::AddRadial(Vec2 centre, double size, double decay)
{
	fields.Add(new CityRadialField(centre, size, decay));
}

CityTensor CityTensorField::SamplePoint(Vec2 point, bool smooth) const
{
	if(fields.IsEmpty())
		return CityTensor(1, 0, 0); // default: matches the source's "acts like a flat grid" fallback

	CityTensor acc = CityTensor::Zero();
	for(int i = 0; i < fields.GetCount(); i++)
		acc.Add(fields[i].GetWeightedTensor(point, smooth), smooth);
	return acc;
}

END_UPP_NAMESPACE
