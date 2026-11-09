#ifndef _SweetSandy_CityStreamlineIntegrator_h_
#define _SweetSandy_CityStreamlineIntegrator_h_

// City Layout phase, Task 2: parameters shared by the integrator and the
// streamline generator - port of streamlines.ts's StreamlineParams
// interface (see Manager/2-plan/.../003-city-layout/
// 002-task2-streamlines.md). Angles in radians, not degrees.
struct CityStreamlineParams : Moveable<CityStreamlineParams> {
	double dsep = 20;              // streamline seed separating distance
	double dtest = 15;             // streamline integration separating distance (clamped <= dsep in CityStreamlineGenerator)
	double dstep = 1;              // integration step size
	double dcirclejoin = 5;        // how far to look to join circles (e.g. 2x dstep)
	double dlookahead = 40;        // how far to look ahead to join up dangling ends
	double joinangle = 0.1;        // max angle (radians) to join roads
	int    path_iterations = 1000; // path integration iteration limit
	int    seed_tries = 300;       // max failed seed attempts before giving up
	double simplify_tolerance = 0.5;
	double collide_early = 0;      // chance [0,1] of early (both-grid) collision testing
};

// RK4 line integrator through a CityTensorField's major/minor
// eigenvector direction - port of integrator.ts's RK4Integrator. Kept
// faithful to the source's *simplified* RK4 (samples the midpoint/
// endpoint offset uniformly by dstep/2 in both axes, not along the
// running direction - not textbook RK4) rather than "fixed", since
// fixing it would change the generated road shapes from what the ported
// algorithm actually produces.
class CityStreamlineIntegrator {
	const CityTensorField& field;
	CityStreamlineParams params;

	Vec2 SampleFieldVector(Vec2 point, bool major) const;

public:
	CityStreamlineIntegrator(const CityTensorField& field, const CityStreamlineParams& params)
		: field(field), params(params) {}

	Vec2 Integrate(Vec2 point, bool major) const;

	// This world has no water/parks yet (Task 1's own scope decision) -
	// always on land. Extend when/if the terrain gains water to exclude.
	bool OnLand(Vec2 point) const { return true; }
};

#endif
