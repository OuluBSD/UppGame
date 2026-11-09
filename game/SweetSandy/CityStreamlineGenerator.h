#ifndef _SweetSandy_CityStreamlineGenerator_h_
#define _SweetSandy_CityStreamlineGenerator_h_

// City Layout phase, Task 2: traces road-centerline polylines through a
// CityTensorField - port of MapGenerator's StreamlineGenerator
// (src/ts/impl/streamlines.ts), synchronous path only (no animate/
// update() step machinery for a live UI - see Manager/2-plan/.../
// 003-city-layout/002-task2-streamlines.md).
//
// Two source-level simplifications are carried over faithfully rather
// than re-added: `SEED_AT_ENDPOINTS` is `false` in the source itself
// (its candidate-seed-queue code path is dead there too - not ported),
// and `doesStreamlineCollideSelf` is unused in the source's own
// synchronous path (the source's own comment: "Didn't end up using" -
// also not ported). Polyline simplification (simplify-js in the source)
// is not ported this task either - see the plan doc's "What gets
// simplified, and why"; GetStreamlines() returns the raw, unsimplified
// polylines.
class CityStreamlineGenerator {
	const CityTensorField& field;
	CityStreamlineIntegrator integrator;
	Vec2 origin, world_dimensions;
	CityStreamlineParams params;

	// Only the squared thresholds the source's own code actually reads
	// (setParamsSq squares every numeric param indiscriminately, but
	// most of those squared values are never used - not ported).
	double dsep_sq, dtest_sq, dcirclejoin_sq, dstep_sq;

	CitySeparationGrid major_grid, minor_grid;
	Vector<Vector<Vec2>> streamlines_major, streamlines_minor;

	CitySeparationGrid& Grid(bool major)             { return major ? major_grid : minor_grid; }
	const CitySeparationGrid& Grid(bool major) const { return major ? major_grid : minor_grid; }
	Vector<Vector<Vec2>>& Streamlines(bool major)    { return major ? streamlines_major : streamlines_minor; }

	// One integration "front" (forward or backward) - port of the
	// source's StreamlineIntegration interface.
	struct IntegrationFront {
		Vec2 seed, original_dir, previous_direction, previous_point;
		Vector<Vec2> streamline;
		bool valid = true;
	};

	Vec2 SamplePoint() const;
	bool GetSeed(bool major, Vec2& seed_out) const; // false if no valid seed found within seed_tries attempts
	bool IsValidSample(bool major, Vec2 point, double d_sq, bool both_grids = false) const;
	bool PointInBounds(Vec2 v) const;
	static bool StreamlineTurned(Vec2 seed, Vec2 original_dir, Vec2 point, Vec2 direction);
	void StreamlineIntegrationStep(IntegrationFront& f, bool major, bool collide_both) const;
	Vector<Vec2> IntegrateStreamline(Vec2 seed, bool major) const;
	bool CreateStreamline(bool major);
	static bool ValidStreamline(const Vector<Vec2>& s) { return s.GetCount() > 5; }

	Vector<Vec2> PointsBetween(Vec2 v1, Vec2 v2, double dstep) const;
	bool GetBestNextPoint(Vec2 point, Vec2 previous_point, Vec2& best_out) const;
	void JoinDanglingStreamlines();

public:
	CityStreamlineGenerator(const CityTensorField& field, Vec2 origin, Vec2 world_dimensions,
	                         const CityStreamlineParams& params);

	// Alternates major/minor streamline attempts, stopping the whole
	// process the first time either direction fails to find a valid
	// seed within seed_tries tries (matches the source's loop exactly -
	// a streamline that integrates to fewer than 6 points is silently
	// dropped and does NOT stop the loop, only a seed-finding failure
	// does), then joins dangling ends. Populates GetStreamlines(major/minor).
	void CreateAllStreamlines();

	const Vector<Vector<Vec2>>& GetStreamlines(bool major) const { return major ? streamlines_major : streamlines_minor; }
};

#endif
