#include "SweetSandy.h"

NAMESPACE_UPP

// -pi to pi, agnostic of winding direction - port of vector.ts's
// Vector.angleBetween (file-local, only GetBestNextPoint needs it).
static double CityVectorAngleBetween(Vec2 a, Vec2 b)
{
	double diff = atan2(a.y, a.x) - atan2(b.y, b.x);
	if(diff > M_PI)
		diff -= 2 * M_PI;
	else if(diff <= -M_PI)
		diff += 2 * M_PI;
	return diff;
}

CityStreamlineGenerator::CityStreamlineGenerator(const CityTensorField& field_, Vec2 origin_, Vec2 world_dimensions_,
                                                   const CityStreamlineParams& params_)
	: field(field_), integrator(field_, params_), origin(origin_), world_dimensions(world_dimensions_), params(params_),
	  major_grid(world_dimensions_, origin_, params_.dsep), minor_grid(world_dimensions_, origin_, params_.dsep)
{
	if(params.dtest > params.dsep)
		params.dtest = params.dsep; // enforce test < sep, matches the source

	dsep_sq = params.dsep * params.dsep;
	dtest_sq = params.dtest * params.dtest;
	dcirclejoin_sq = params.dcirclejoin * params.dcirclejoin;
	dstep_sq = params.dstep * params.dstep;
}

Vec2 CityStreamlineGenerator::SamplePoint() const
{
	return Vec2(Randomf() * world_dimensions.x, Randomf() * world_dimensions.y) + origin;
}

bool CityStreamlineGenerator::GetSeed(bool major, Vec2& seed_out) const
{
	Vec2 seed = SamplePoint();
	int i = 0;
	while(!IsValidSample(major, seed, dsep_sq)) {
		if(i >= params.seed_tries)
			return false;
		seed = SamplePoint();
		i++;
	}
	seed_out = seed;
	return true;
}

bool CityStreamlineGenerator::IsValidSample(bool major, Vec2 point, double d_sq, bool both_grids) const
{
	bool grid_valid = Grid(major).IsValidSample(point, d_sq);
	if(both_grids)
		grid_valid = grid_valid && Grid(!major).IsValidSample(point, d_sq);
	return integrator.OnLand(point) && grid_valid;
}

bool CityStreamlineGenerator::PointInBounds(Vec2 v) const
{
	return v.x >= origin.x && v.y >= origin.y
	    && v.x < world_dimensions.x + origin.x && v.y < world_dimensions.y + origin.y;
}

bool CityStreamlineGenerator::StreamlineTurned(Vec2 seed, Vec2 original_dir, Vec2 point, Vec2 direction)
{
	if(original_dir.Dot(direction) < 0) {
		Vec2 perpendicular(original_dir.y, -original_dir.x);
		bool is_left = (point - seed).Dot(perpendicular) < 0;
		bool direction_up = direction.Dot(perpendicular) > 0;
		return is_left == direction_up;
	}
	return false;
}

void CityStreamlineGenerator::StreamlineIntegrationStep(IntegrationFront& f, bool major, bool collide_both) const
{
	if(!f.valid)
		return;

	f.streamline.Add(f.previous_point);
	Vec2 next_direction = integrator.Integrate(f.previous_point, major);

	// Stop at a degenerate (near-zero) field point.
	if(next_direction.LengthSq() < 0.01) {
		f.valid = false;
		return;
	}

	// Keep travelling in the same direction (a line field has no
	// preferred sign - see CityTensor - so the integrator can flip
	// between consecutive samples).
	if(next_direction.Dot(f.previous_direction) < 0)
		next_direction = -next_direction;

	Vec2 next_point = f.previous_point + next_direction;

	if(PointInBounds(next_point)
	   && IsValidSample(major, next_point, dtest_sq, collide_both)
	   && !StreamlineTurned(f.seed, f.original_dir, next_point, next_direction)) {
		f.previous_point = next_point;
		f.previous_direction = next_direction;
	}
	else {
		// One more (terminal) step, then stop.
		f.streamline.Add(next_point);
		f.valid = false;
	}
}

Vector<Vec2> CityStreamlineGenerator::IntegrateStreamline(Vec2 seed, bool major) const
{
	int count = 0;
	bool points_escaped = false; // true once the two integration fronts have moved dcirclejoin apart
	bool collide_both = Randomf() < params.collide_early;

	Vec2 d = integrator.Integrate(seed, major);

	IntegrationFront forward;
	forward.seed = seed;
	forward.original_dir = d;
	forward.streamline.Add(seed);
	forward.previous_direction = d;
	forward.previous_point = seed + d;
	forward.valid = PointInBounds(forward.previous_point);

	Vec2 neg_d = -d;
	IntegrationFront backward;
	backward.seed = seed;
	backward.original_dir = neg_d;
	backward.previous_direction = neg_d;
	backward.previous_point = seed + neg_d;
	backward.valid = PointInBounds(backward.previous_point);

	// Integrating both directions at once (rather than one then the
	// other) means a closed loop's two ends meet with matching
	// accumulated error, so circles join up cleanly.
	while(count < params.path_iterations && (forward.valid || backward.valid)) {
		StreamlineIntegrationStep(forward, major, collide_both);
		StreamlineIntegrationStep(backward, major, collide_both);

		double sq_dist = (forward.previous_point - backward.previous_point).LengthSq();

		if(!points_escaped && sq_dist > dcirclejoin_sq)
			points_escaped = true;

		if(points_escaped && sq_dist <= dcirclejoin_sq) {
			forward.streamline.Add(forward.previous_point);
			forward.streamline.Add(backward.previous_point);
			backward.streamline.Add(backward.previous_point);
			break;
		}
		count++;
	}

	Vector<Vec2> result;
	for(int i = backward.streamline.GetCount() - 1; i >= 0; i--)
		result.Add(backward.streamline[i]);
	result.Append(forward.streamline);
	return result;
}

bool CityStreamlineGenerator::CreateStreamline(bool major)
{
	Vec2 seed;
	if(!GetSeed(major, seed))
		return false;

	Vector<Vec2> streamline = IntegrateStreamline(seed, major);
	if(ValidStreamline(streamline)) {
		Grid(major).AddPolyline(streamline);
		Streamlines(major).Add(pick(streamline));
	}
	return true;
}

Vector<Vec2> CityStreamlineGenerator::PointsBetween(Vec2 v1, Vec2 v2, double dstep) const
{
	double d = (v1 - v2).Length();
	int n_points = (int)floor(d / dstep);
	Vector<Vec2> out;
	if(n_points == 0)
		return out;

	Vec2 step_vector = v2 - v1;

	// Faithful port of an apparent off-by-one in the source
	// (streamlines.ts's pointsBetween): `next` is tested for the
	// CURRENT loop index but only recomputed using that SAME index at
	// the end of the same iteration (the increment hasn't run yet), so
	// the recompute is a same-value no-op and only takes effect one
	// iteration late. Net effect: the point nearest v1 is pushed twice
	// and the point nearest v2 is never reached (falls one step short).
	// This looks like an unintentional bug upstream, not a deliberate
	// design choice (unlike CityTensor::Add's r=2), but it's kept
	// exactly as observed rather than silently corrected, since this
	// phase's whole point is porting the actual algorithm - it only
	// affects the cosmetic "extra points inserted while joining a
	// dangling streamline end" path, never core streamline tracing.
	int i = 1;
	Vec2 next = v1 + step_vector * ((double)i / n_points);
	for(i = 1; i <= n_points; i++) {
		if(integrator.Integrate(next, true).LengthSq() > 0.001)
			out.Add(next);
		else
			return out;
		next = v1 + step_vector * ((double)i / n_points);
	}
	return out;
}

bool CityStreamlineGenerator::GetBestNextPoint(Vec2 point, Vec2 previous_point, Vec2& best_out) const
{
	Vector<Vec2> nearby = major_grid.GetNearbyPoints(point, params.dlookahead);
	nearby.Append(minor_grid.GetNearbyPoints(point, params.dlookahead));

	Vec2 direction = point - previous_point;

	bool found = false;
	Vec2 closest_sample;
	double closest_distance = 1e300;

	for(Vec2 sample : nearby) {
		if(sample == point || sample == previous_point)
			continue;

		Vec2 difference = sample - point;
		if(difference.Dot(direction) < 0)
			continue; // backwards

		double distance_to_sample = (point - sample).LengthSq();
		if(distance_to_sample < 2 * dstep_sq) {
			closest_sample = sample;
			found = true;
			break;
		}

		double angle_between = fabs(CityVectorAngleBetween(direction, difference));
		if(angle_between < params.joinangle && distance_to_sample < closest_distance) {
			closest_distance = distance_to_sample;
			closest_sample = sample;
			found = true;
		}
	}

	if(found)
		best_out = closest_sample + direction.Normalized() * (params.simplify_tolerance * 4);
	return found;
}

void CityStreamlineGenerator::JoinDanglingStreamlines()
{
	for(bool major : { true, false }) {
		Vector<Vector<Vec2>>& lines = Streamlines(major);
		for(int si = 0; si < lines.GetCount(); si++) {
			Vector<Vec2>& streamline = lines[si];
			if(streamline[0] == streamline.Top())
				continue; // circle, already closed

			Vec2 new_start;
			if(GetBestNextPoint(streamline[0], streamline[4], new_start)) {
				for(Vec2 p : PointsBetween(streamline[0], new_start, params.dstep)) {
					streamline.Insert(0, p);
					Grid(major).AddSample(p);
				}
			}

			Vec2 new_end;
			if(GetBestNextPoint(streamline.Top(), streamline[streamline.GetCount() - 4], new_end)) {
				for(Vec2 p : PointsBetween(streamline.Top(), new_end, params.dstep)) {
					streamline.Add(p);
					Grid(major).AddSample(p);
				}
			}
		}
	}
}

void CityStreamlineGenerator::CreateAllStreamlines()
{
	// Alternates major/minor; stops the moment either direction fails
	// to find a valid seed within seed_tries tries - matches the
	// source's loop exactly, including that a too-short (discarded)
	// streamline does NOT stop the loop, only a seed-finding failure does.
	bool major = true;
	while(CreateStreamline(major))
		major = !major;

	JoinDanglingStreamlines();
}

END_UPP_NAMESPACE
