#ifndef _SweetSandy_CityTensorField_h_
#define _SweetSandy_CityTensorField_h_

// City Layout phase, Task 1: sums CityBasisFields into one tensor field.
// Port of tensor_field.ts's field-summation logic only - onLand/inParks/
// rotational noise deliberately not ported this task (this world has no
// water/parks yet; see 001-planning.md's "What gets simplified, and why").
class CityTensorField {
	Array<CityBasisField> fields; // owned, polymorphic (Grid/Radial) - see Core/Vcont.h Array<T>

public:
	void AddGrid(Vec2 centre, double size, double decay, double theta);
	void AddRadial(Vec2 centre, double size, double decay);

	int GetFieldCount() const { return fields.GetCount(); }

	// Sums every field's weighted tensor at `point`. Matches the
	// source's "no basis fields -> default grid-like (1,0,0) tensor"
	// fallback rather than a fully degenerate zero tensor.
	CityTensor SamplePoint(Vec2 point, bool smooth = false) const;
};

#endif
