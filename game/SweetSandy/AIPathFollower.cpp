#include "SweetSandy.h"

NAMESPACE_UPP

void AIPathFollower::SetPath(const Vector<Vec2>& new_path, bool closed)
{
	path = clone(new_path);
	closed_loop = closed;
	segment_index = 0;
	segment_t = 0;
	if(path.GetCount() >= 2)
		UpdatePose();
}

bool AIPathFollower::Advance(double distance)
{
	// A path shorter than 2 points can't be followed at all - not
	// expected from any real path this project generates
	// (road_centerlines/block perimeters always have several points),
	// so this just reports "can't move" rather than crashing.
	if(path.GetCount() < 2)
		return false;

	int seg_count = SegmentCount();
	while(distance > 1e-9) {
		Vec2 a = PointAt(segment_index);
		Vec2 b = PointAt(segment_index + 1);
		double seg_len = (b - a).Length();
		double remaining = seg_len * (1 - segment_t);

		if(distance < remaining) {
			segment_t += distance / seg_len;
			distance = 0;
		}
		else {
			distance -= remaining;
			segment_index++;
			segment_t = 0;
			if(segment_index >= seg_count) {
				if(closed_loop) {
					segment_index = 0;
				}
				else {
					// Clamp at the last segment's own end - position/
					// heading stay at the true final point, not
					// advanced past it.
					segment_index = seg_count - 1;
					segment_t = 1;
					UpdatePose();
					return false;
				}
			}
		}
	}
	UpdatePose();
	return true;
}

void AIPathFollower::UpdatePose()
{
	Vec2 a = PointAt(segment_index);
	Vec2 b = PointAt(segment_index + 1);
	position = a + (b - a) * segment_t;
	Vec2 dir = (b - a).Normalized();
	heading = atan2(dir.y, dir.x);
}

END_UPP_NAMESPACE
