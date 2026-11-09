#include "Adventure.h"
#include "AdventureBindings.h"

namespace Adventure {


void Program::Shake(bool enabled) {
	cam_shake = enabled;
	cam_shake_amount = enabled ? 1 : cam_shake_amount;
}

void Program::CameraAt(const Point& val) {
	// point center of camera at val, clear other cam values
	Point c = CenterCamera(val);
	cam = c;
	cam_pan_to = c;
	cam_following_actor = 0;
}

void Program::CameraFollow(SObj actor) {
	//GetReference(actor, true);
	ASSERT(actor.IsMap());

	if (cam_script) {
		StopScript(*cam_script); // bg script
		cam_script = 0;
	}

	// set target, clear other cam values
	cam_following_actor = EscToPyValue(actor);
	cam_pan_to = GetXY(actor);

	cam_script = &dynamic_cast<Script&>(StartScript(THISBACK(CamScript0), true)); // bg script

	// auto-switch to room actor resides in
	PyValue r = GetInRoomPy(cam_following_actor);
	if (r.GetType() == PY_DICT && r != room_curr)
		ChangeRoomPy(r, 1);
}

bool Program::CamScript0() {
	// keep the camera following actor
	// (until further notice)
	if (cam_following_actor.GetType() == PY_DICT) {
		// keep camera within "room" bounds
		if (GetInRoomPy(cam_following_actor) == room_curr)
			cam = CenterCameraPy(cam_following_actor);
		return true;
	}
	else
		return false;
}

void Program::CameraFollowPy(PyValue actor) {
	if (actor.GetType() != PY_DICT)
		return;

	if (cam_script) {
		StopScript(*cam_script);
		cam_script = 0;
	}

	cam_following_actor = actor;
	cam_pan_to = GetXYPy(actor);

	cam_script = &dynamic_cast<Script&>(StartScript(THISBACK(CamScript0), true));

	// auto-switch to room actor resides in
	PyValue r = GetInRoomPy(cam_following_actor);
	if (r.GetType() == PY_DICT && r != room_curr)
		ChangeRoomPy(r, PyValue(1));
}

void Program::CameraPanTo(SObj& val) {
	// set target, but keep camera within "room" bounds, clear other cam values
	Point c = CenterCamera(val);
	cam_pan_to = c;
	cam_following_actor = 0;
	cam_script = 0;;

	cam_script = &dynamic_cast<Script&>(StartScript(THISBACK(CamScript1), true)); // bg script
}

bool Program::CamScript1() {
	
	// update the camera pan until reaches dest
	if (cam.x != cam_pan_to.x) {
		int diff = cam_pan_to.x - cam.x;
		if (abs(diff) >= 2)
			diff /= 2;
		cam.x += diff;
		return true;
	}
	
	// pan complete
	cam_pan_to = cam;
	return false;
}

bool Program::Fades(int fade, int dir) {
	// dir: 1=down, -1=up
	int fade_amount = 25 - 25 * dir;

	fade_amount += dir*2;

	if (fade_amount >= 0 && fade_amount <= 50) {
		// iris) {wn/up
		if (fade == 1)
			fade_iris = min(fade_amount, 32);
		
		fade_iter++;
		if (fade_iter < 1000)
			return true;
	}
	
	fade_iter = 0;
	return false;
}

Point Program::CenterCamera(Point val) {
	ASSERT(room_curr.GetType() == PY_DICT);
	PyValue map_prop = Program::GetProp(room_curr, "map");
	const Vector<PyValue>& map_arr = map_prop.GetArray();
	int map_w = Program::PyInt(map_arr[2]);

	Point pt(0,0);
	pt.x = clamp(val.x-64, 0, (map_w*8) -128 );
	return pt;
}

Point Program::CenterCamera(SObj& val) {
	//LOG(val.ToString());

	// check params for (obj/actor
	// keep camera within "room" bounds
	Point p(0,0);

	//DUMP(val);

	if (val.IsMap())
		p.x = val.MapGet("x").GetInt();

	else if (val.IsInt())
		p.x = val.GetInt();

	return CenterCamera(p);
	//int map_w = room_curr.MapGet("map_w").GetInt();
	//return mid(0, (istable(val) and val.x or val)-64, (room_curr.map_w*8) -128 )
}

Point Program::CenterCameraPy(PyValue val) {
	Point p(0,0);

	if (val.GetType() == PY_DICT)
		p.x = Program::PyInt(Program::GetProp(val, "x"));
	else if (val.IsInt())
		p.x = Program::PyInt(val);

	return CenterCamera(p);
}


}
