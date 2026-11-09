#ifndef _SweetSandy_CityBasisField_h_
#define _SweetSandy_CityBasisField_h_

// City Layout phase, Task 1: basis fields, ported from MapGenerator's
// basis_field.ts (minus the dat.GUI folder/drag-listener methods - dead
// weight for a headless generator, see 001-planning.md). A basis field
// is one named "shape" of road direction (a rectilinear grid, a radial
// pattern around a park) with a falloff so it only matters near its own
// centre; CityTensorField sums several of these into the final field.
class CityBasisField {
protected:
	Vec2   centre;
	double size;
	double decay;

public:
	CityBasisField(Vec2 centre, double size, double decay) : centre(centre), size(size), decay(decay) {}
	virtual ~CityBasisField() {}

	virtual CityTensor GetTensor(Vec2 point) const = 0;
	CityTensor GetWeightedTensor(Vec2 point, bool smooth) const;

protected:
	// Interpolates between (0 and 1)^decay, based on distance from centre
	// relative to size - getTensorWeight in the source.
	double GetTensorWeight(Vec2 point, bool smooth) const;
};

// A rectilinear road pattern at a fixed angle - grid.ts's Grid.
class CityGridField : public CityBasisField {
	double theta;

public:
	CityGridField(Vec2 centre, double size, double decay, double theta)
		: CityBasisField(centre, size, decay), theta(theta) {}

	CityTensor GetTensor(Vec2 point) const override;
};

// A road pattern radiating out from (or circling around) a point - e.g.
// a roundabout or a park boundary - basis_field.ts's Radial.
class CityRadialField : public CityBasisField {
public:
	CityRadialField(Vec2 centre, double size, double decay) : CityBasisField(centre, size, decay) {}

	CityTensor GetTensor(Vec2 point) const override;
};

#endif
