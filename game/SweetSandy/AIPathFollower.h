#ifndef _SweetSandy_AIPathFollower_h_
#define _SweetSandy_AIPathFollower_h_

// AI traffic and pedestrians phase, Task 1: the kinematic stepper both
// TrafficManager (Task 2) and PedestrianManager (Task 3) use to turn "a
// polyline + a speed" into "this tick's position and heading". Pure
// movement math only - no spawning, no rendering, no ODE collision, no
// routing/graph logic (that's Task 2's own concern; this struct's input
// path is just handed to it as a plain Vector<Vec2>). See
// Manager/2-plan/.../006-ai-traffic-and-pedestrians/
// 001-task1-path-follower.md.
struct AIPathFollower {
	Vector<Vec2> path;
	bool closed_loop = false; // true: wraps forever (a pedestrian's own
	                          // block-perimeter ring); false: Advance()
	                          // returns false at the end (a traffic car's
	                          // own open route, Task 2 re-routes)
	int segment_index = 0;
	double segment_t = 0; // 0..1 progress along [segment_index, segment_index+1]

	Vec2 position;
	double heading = 0; // radians; direction of travel along the current segment

	// Resets to the very start of a new path. `new_path` must have at
	// least 2 points - Advance() on a shorter path just returns false
	// immediately without moving (see AIPathFollower.cpp's own note; not
	// expected from any real path this project generates, so not
	// specially validated beyond that).
	void SetPath(const Vector<Vec2>& new_path, bool closed);

	// Advances by `distance` meters (may cross several segment
	// boundaries in one call). Returns false the moment an *open* path's
	// own end is reached (position/heading are left at the very last
	// point, not beyond it) - the caller decides what happens next. A
	// closed path never returns false - wrapping past the last point
	// continues from the first.
	//
	// A closed path's own implicit closing segment (the last point back
	// to the first) is NOT repeated in `path` itself - `path` holds each
	// distinct vertex once, and the loop's own segment count is
	// `path.GetCount()` (not `GetCount()-1`, which is what an *open*
	// path uses); PointAt()/SegmentCount() below are what makes both
	// cases share one Advance()/UpdatePose() implementation instead of
	// needing separate code paths.
	bool Advance(double distance);

private:
	int SegmentCount() const { return closed_loop ? path.GetCount() : path.GetCount() - 1; }
	Vec2 PointAt(int i) const { return path[i % path.GetCount()]; }
	void UpdatePose();
};

#endif
