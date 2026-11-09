#include "Adventure.h"
#include "AdventureBindings.h"

namespace Adventure {
	

PyValue Program::GetSelectedActor() {
	auto& global = ctx.global;
	//DUMPC(global.GetKeys());
	//ASSERT(global.Find("selected_actor") >= 0);
	return EscToPyValue(global.Get("selected_actor", EscValue()));
}


// actions to perform when object doesn't have an entry for (verb
void Program::UnsupportedAction(PyValue verb, PyValue obj1, PyValue obj2) {
	PyValue classes1 = Program::GetProp(obj1, "classes");
	bool is_actor = HasFlag(classes1, "class_actor");

	if (verb.GetPtr() == V_WALKTO.GetPtr())
		return;

	else if (verb.GetPtr() == V_PICKUP.GetPtr())
		SayLine(is_actor ? "i don't need them" : "i don't need that");

	else if (verb.GetPtr() == V_USE.GetPtr()) {
		PyValue classes2 = Program::GetProp(obj2, "classes");
		SayLine(is_actor ? "i can't just *use* someone" :
			(HasFlag(classes2, "class_actor") ?
				"i can't use that on someone!" : "that doesn't work"));
	}

	else if (verb.GetPtr() == V_GIVE.GetPtr())
		SayLine(is_actor ? "i don't think i should be giving this away" : "i can't do that");

	else if (verb.GetPtr() == V_LOOKAT.GetPtr())
		SayLine(is_actor ? "i think it's alive" : "looks pretty ordinary");

	else if (verb.GetPtr() == V_OPEN.GetPtr() || verb.GetPtr() == V_CLOSE.GetPtr())
		SayLine(String(is_actor ? "they don't" : "it doesn't")  + " seem to " + GetVerbString(Adventure::PyToEscValue(verb)));

	else if (verb.GetPtr() == V_PUSH.GetPtr() || verb.GetPtr() == V_PULL.GetPtr())
		SayLine(is_actor ? "moving them would accomplish nothing" : "it won't budge!");

	else if (verb.GetPtr() == V_TALKTO.GetPtr())
		SayLine(is_actor ? "erm ...  i don't think they want to talk" : "i am not talking to that!");

	else
		SayLine("hmm. no.");

}

void Program::PickupObj(SObj& obj, SObj& actor) {
	// use actor spectified, || default to selected
	if (actor.IsVoid()) {
		PyValue actor_py = GetSelectedActor();
		actor = Adventure::PyToEscValue(actor_py);
	}

	TODO
	/*add(actor.inventory, obj);
	obj.owner = actor;
	// remove it from room

	del(obj.in_room.objects, obj);*/
}

// uses actor's position and color
void Program::SayLineActor(SObj actor, String msg, bool use_caps, float duration) {
	if (duration == 0)
		duration = 1.0f;

	// trigger actor's talk anim
	talking_actor = EscToPyValue(actor);

	// offset to display speech above actors (dist in px from their feet)
	// call the base PrintLine to show actor line
	int x = actor("x",0);
	int y = actor("y",0);
	int w = actor("w",1);
	int h = actor("h",1);
	int col = actor("col",0);
	PrintLine(msg, x, y - h * 8 + 4, col, 1, use_caps, duration, false);
}

void Program::SayLine(String msg) {
	PyValue selected_actor = GetSelectedActor();
	if (selected_actor.GetType() != PY_NONE)
		SayLineActor(Adventure::PyToEscValue(selected_actor), msg, false, 0);
}

// stop everyone talking & remove displayed text
void Program::StopTalking() {
	talking_curr.Clear();
	talking_actor = PyValue();
}

void Program::StopActor(SObj& actor) {
	// 0=stopped, 1=walking, 2=arrived
	actor.MapSet("moving", 0);
	actor.MapSet("curr_anim", 0);
	
	// no need to) {DoAnim(idle) here, as actor_draw code handles this
	ClearCurrCmd();
}

// walk actor to position
void Program::WalkTo(SObj a, int x, int y) {
	Point actor_cell_pos = GetCellPos(a);
	PyValue map_py = Program::GetProp(room_curr, "map");
	const Vector<PyValue>& map_arr = map_py.GetArray();
	int map_x = Program::PyInt(map_arr[0]);
	int map_y = Program::PyInt(map_arr[1]);
	int celx = x / 8 + map_x;
	int cely = y / 8 + map_y;
	Point target_cell_pos(celx, cely);

	// use pathfinding!
	FindPath(actor_cell_pos, target_cell_pos, path);
	
	a.MapSet("moving", 1);
	
	for (int c = 0; c < path.GetCount(); c++) {
		Point p = path[c];
		
		// auto-adjust walk-speed for (depth
		double walk_speed = a("walk_speed");
		SObj scale = a("scale");
		SObj auto_scale = a("auto_scale", 1.0);
		double s = scale ? scale : auto_scale;
		double scaled_speed = walk_speed * s;

		//local y_speed = actor.walk_speed/2 // removed, messes up the a* pathfinding
		PyValue rc_map_py = Program::GetProp(room_curr, "map");
		const Vector<PyValue>& rc_map_arr = rc_map_py.GetArray();
		int rc_map_x = Program::PyInt(rc_map_arr[0]);
		int rc_map_y = Program::PyInt(rc_map_arr[1]);
		int px = (p.x - rc_map_x) * 8 + 4;
		int py = (p.y - rc_map_y) * 8 + 4;
		
		// last cell (walk to precise location, if (clicked in it)
		if (c == path.GetCount() && x >= px-4 && x <= px+4 && y >= py-4 && y <= py+4) {
			px = x;
			py = y;
		}
		
		int actor_x = a("x");
		int actor_y = a("y");
		double distance = sqrt(pow(px - actor_x, 2) + pow(py - actor_y, 2));
		double step_x = scaled_speed * (px - actor_x) / distance;
		double step_y = scaled_speed * (py - actor_y) / distance;
		
		// only walk if (we're not already there!
		if (distance > 0) {
		
			//walking
			
			int lim = distance / scaled_speed - 1;
			for (int i = 0; i < lim; i++) {
			
				// todo: need to somehow recalc here, else walk too fast/slow in depth planes
				
				// abort if (actor stopped
				int m = a("moving", 0);
				if (m == 0) {
					return;
				}
			    
				a.MapSet("flip", step_x < 0);
				    
				// choose walk anim based on dir
				//if (abs(step_x) < abs(step_y) {
				if (abs(step_x) < scaled_speed / 2) {
					// vertical walk
					a.MapSet("curr_anim", step_y > 0 ? a("walk_anim_front") : a("walk_anim_back"));
					a.MapSet("face_dir", step_y > 0 ? "face_front" : "face_back");
				}
				else {
					// horizontal walk
					a.MapSet("curr_anim", a("walk_anim_side"));
					
					// face dir (at end of walk)
					a.MapSet("face_dir", a("flip") ? "face_left" : "face_right");
				}
				
				// actually move actor
				actor_x = a("x");
				actor_y = a("y");
				a.MapSet("x", actor_x + step_x);
				a.MapSet("y", actor_y + step_y);
				
				// yield();
			}
			
		}
	}
	
	a.MapSet("moving", 2);
}

void Program::WaitForActor(SObj& actor) {
	if (actor.IsVoid()) {
		PyValue actor_py = GetSelectedActor();
		actor = Adventure::PyToEscValue(actor_py);
	}

	ASSERT(actor.IsMap());

	// wait for (actor to stop moving/turning
	while (actor.MapGet("moving").GetInt() != 2) {
		TODO // yield();
	}
}

bool Program::WalkScript() {
	auto& global = ctx.global;

	PyValue selected_actor = GetSelectedActor();
	WalkTo(Adventure::PyToEscValue(selected_actor), cursor_x + cam.x, cursor_y - stage_top);

	// clear current command
	ClearCurrCmd();
	return false;
}

bool Program::VerbScript(EscValue vc2) {
	auto& global = ctx.global;

	// if (obj not in inventory (or about to give/use it)...
	PyValue owner = Program::GetProp(noun1_curr, "owner");
	bool has_owner = owner.GetType() != PY_NONE;
	PyValue classes1 = Program::GetProp(noun1_curr, "classes");
	bool is_actor = HasFlag(classes1, "class_actor");

	if ((!has_owner ? !is_actor : EscToPyValue(vc2) != V_USE)
		|| noun2_curr.GetType() != PY_NONE) {
		PyValue selected_actor_py = GetSelectedActor();

		// walk to use pos and face dir
		// determine which item we're walking to
		PyValue walk_obj = noun2_curr.GetType() != PY_NONE ? noun2_curr : noun1_curr;

		//todo: find nearest usepos if (none set?
		Point dest_pos = GetUsePointPy(walk_obj);
		WalkToPy(selected_actor_py, dest_pos.x, dest_pos.y);

		// abort if (walk was interrupted
		int m = Program::PyInt(Program::GetProp(selected_actor_py, "moving"));
		if (m != 2) {
			return (false);
		}

		// face object/actor by default
		int use_dir = Program::PyInt(Program::GetProp(walk_obj, "use_dir"));

		// turn to use dir
		DoAnimPy(selected_actor_py, "face_towards", use_dir);
	}
	// does current object support active verb?
	if (IsValidVerb(verb_curr, noun1_curr)) {
		// finally, execute verb script
		PyValue nc_verbs = Program::GetProp(noun1_curr, "verbs");
		String verb_name = Program::PyStr(Program::GetProp(verb_curr, "name")).ToString();
		PyValue verb_lambda = Program::GetProp(nc_verbs, verb_name);
		ASSERT(verb_lambda.GetType() != PY_NONE);
		StartScriptEsc(0, PyToEscValue(verb_lambda, nullptr), false, PyToEscValue(noun1_curr, nullptr), PyToEscValue(noun2_curr, nullptr));
	}
	else {
		// check for door
		if (HasFlag(classes1, "class_door")) {
			// perform default door action
			String s = vc2;
			ASSERT(s.GetCount());
			Gate0 func;
			PyValue target_door = Program::GetProp(noun1_curr, "target_door");
			if      (s == "open")	OpenDoorPy(noun1_curr, target_door);
			else if (s == "close")	CloseDoorPy(noun1_curr, target_door);
			else if (s == "walkto")	ComeOutDoorPy(noun1_curr, target_door, 0);
		}
		else {
			// e.g. "i don't think that will work"
			UnsupportedAction(EscToPyValue(vc2), noun1_curr, noun2_curr);
		}
	}

	// clear current command
	ClearCurrCmd();
	return false;
}

// PyValue wrapper functions
void Program::WalkToPy(PyValue a, int x, int y) {
	SObj a_esc = PyToEscValue(a, nullptr);
	WalkTo(a_esc, x, y);
}

void Program::DoAnimPy(PyValue thing, const String& param1, int param2) {
	SObj thing_esc = PyToEscValue(thing, nullptr);
	DoAnim(thing_esc, param1, param2);
}

Point Program::GetUsePointPy(PyValue obj) {
	SObj obj_esc = PyToEscValue(obj, nullptr);
	return GetUsePoint(obj_esc);
}

void Program::OpenDoorPy(PyValue door_obj1, PyValue door_obj2) {
	SObj door1_esc = PyToEscValue(door_obj1, nullptr);
	SObj door2_esc = PyToEscValue(door_obj2, nullptr);
	OpenDoor(door1_esc, door2_esc);
}

void Program::CloseDoorPy(PyValue door_obj1, PyValue door_obj2) {
	SObj door1_esc = PyToEscValue(door_obj1, nullptr);
	SObj door2_esc = PyToEscValue(door_obj2, nullptr);
	CloseDoor(door1_esc, door2_esc);
}

void Program::ComeOutDoorPy(PyValue from_door, PyValue to_door, bool fade_effect) {
	SObj from_esc = PyToEscValue(from_door, nullptr);
	SObj to_esc = PyToEscValue(to_door, nullptr);
	ComeOutDoor(from_esc, to_esc, fade_effect);
}


}
