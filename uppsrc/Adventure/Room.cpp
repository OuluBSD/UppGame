#include "Adventure.h"
#include "AdventureBindings.h"

namespace Adventure {



const SObj* Program::FindRoom(const String& name) const {
	const auto& arr = rooms.GetArray();
	for (const auto& val : arr) {
		if(val.GetType() == PY_STR) {
			WString room_str = val.GetStr();
			//LOG(room_str);
			if (room_str == WString(name))
				return FindDeep(name);
		}
	}
	return 0;
}

SObj Program::GetInRoom(SObj o) {
	//LOG(o.ToString());
	if (o.IsMap()) {
		const auto& map = o.GetMap();
		int i = map.Find("in_room");
		if (i >= 0) {
			SObj in_room = o.MapGet("in_room");
			if (in_room.IsMap())
				return in_room;

			DUMPM(o.GetMap());
			TODO
			/*LOG(in_room.ToString());
			String name = in_room;
			if (name.GetCount()) {
				const SObj* ptr = FindDeep(name);
				if (ptr) {
					ASSERT(ptr->IsMap());
					return *ptr;
				}
			}*/
		}
	}
	return SObj();
}

PyValue Program::GetInRoomPy(PyValue o) {
	if (o.GetType() == PY_DICT) {
		PyValue in_room = GetProp(o, "in_room");
		if (in_room.GetType() == PY_DICT)
			return in_room;
	}
	return PyValue();
}

// open one (or more) doors
void Program::OpenDoor(SObj door_obj1, SObj door_obj2) {
	StateType state1 = GetState(door_obj1);
	
	if (state1 == STATE_OPEN) {
		SayLine("it's already open");
	}
	else {
		SetState(door_obj1, STATE_OPEN);
		if (door_obj2)
			SetState(door_obj2, STATE_OPEN);
	}
}

// close one (or more) doors
void Program::CloseDoor(SObj door_obj1, SObj door_obj2) {
	StateType state1 = GetState(door_obj1);
	if (state1 == STATE_CLOSED) {
		SayLine("it's already closed");
	}
	else {
		SetState(door_obj1, STATE_CLOSED);
		if (door_obj2)
			SetState(door_obj2, STATE_CLOSED);
	}
}

void Program::ComeOutDoor(SObj from_door, SObj to_door, bool fade_effect) {
	// check param
	/*if (to_door == NULL) {
		ShowError("target door does not exist");
		return;
	}*/
	PyValue selected_actor = GetSelectedActor();

	StateType from_state = GetState(from_door);
	if (from_state != STATE_OPEN) {
		SayLine("the door is closed");
		return;
	}

	// go to new room!
	SObj new_room = GetInRoom(PyToEscValue(selected_actor));

	if (new_room.GetType() != room_curr.GetType() || !new_room.IsMap()) {
		ChangeRoom(new_room, fade_effect); // switch to new room and ...

		// ...auto-position actor at to_door in new room...
		Point pos = GetUsePoint(to_door);
		PutAt(PyToEscValue(selected_actor), pos.x, pos.y, new_room);
	}
	
	FaceDir to_dir = GetFaceDir(to_door);
	FaceDir opp_dir;
	
	if (to_dir) {
		//  ... in opposite use direction!
		opp_dir = (FaceDir)(((int)to_dir + 1) % 4 + 1);
		
		// opp_dir = to_dir + 2
		// if (opp_dir > 4 {
		// opp_dir -= 4
		// }
	}
	else {
		opp_dir = FACE_FRONT;
	}

	Program::SetProp(selected_actor, "face_dir", PyValue(GetFaceString(opp_dir)));

	// is target dir left? flip?
	Program::SetProp(selected_actor, "flip", PyValue(GetFaceDirPy(selected_actor) == FACE_LEFT));

}

void Program::ChangeRoom(SObj new_room, SObj fade_) {
	// Legacy Esc room conversion - convert to PyValue and call ChangeRoomPy
	// This is for backward compatibility with Esc code
	PyValue py_room;
	if(new_room.IsMap()) {
		const VectorMap<EscValue, EscValue>& esc_map = new_room.GetMap();
		py_room = PyValue::Dict();
		VectorMap<PyValue, PyValue>& d = py_room.GetDictRW();
		for(int i = 0; i < esc_map.GetCount(); i++) {
			if(esc_map.GetKey(i).IsStringLike()) {
				d.Add(PyValue(esc_map.GetKey(i).ToString()), EscToPyValue(esc_map[i]));
			}
		}
	}
	ChangeRoomPy(py_room, EscToPyValue(fade_));
}

void Program::PutAt(SObj obj, int x, int y, SObj room) {
	//GetReference(obj, true);
	//GetReference(room, true);
	//LOG(obj.ToString());
	//LOG(room.ToString());
	ASSERT(obj.IsMap());
	ASSERT(room.IsMap());

	if (room.IsMap()) {
		if (!HasFlag(Classes(obj), "class_actor")) {
			SObj in_room = obj.MapGet("in_room");
			//if (in_room.IsArray())
			//	GetReference(in_room, true);
			ASSERT(in_room.IsMap() || in_room.IsVoid());
			if (in_room.IsMap()) {
				SObj objects = in_room.MapGet("objects");
				Vector<EscValue>& arr = (Vector<EscValue>&)objects.GetArray();
				VectorRemoveKey(arr, obj);
			}
			obj.MapSet("owner", EscValue());
			SObj objects = room.MapGet("objects");
			ASSERT(objects.IsArray());
			objects.ArrayAdd(obj);
		}
	}
	obj.MapSet("in_room", room);

	//LOG(obj.ToString());
	ASSERT(obj.IsMap());
	obj.MapSet("x", x);
	obj.MapSet("y", y);
}

void Program::ChangeRoomPy(PyValue new_room, PyValue fade) {
	// Main implementation - Python native
	int fade_val = Program::PyInt(fade);

	// stop any existing fade
	if (fade_script) {
		StopScript(*fade_script);
		fade_script = 0;
	}

	// fade out existing room (if any)
	if (fade_val && room_curr.GetType() == PY_DICT) {
		Fades(fade_val, 1);
	}

	// execute the exit() callback of old room (Python)
	if (room_curr.GetType() == PY_DICT) {
		PyValue exit_fn = GetDictItem(room_curr, "exit");
		if(exit_fn.IsFunction()) {
			Vector<PyValue> args;
			args.Add(room_curr);
			py_vm.Call(exit_fn, args);
		}
	}

	// stop all active (local) scripts
	ctx.RemoveProgramGroup(SCRIPT_LOCAL);

	// clear current command
	ClearCurrCmd();

	// change to new room
	room_curr = new_room;

	// reset camera pos in new room
	if (cam_following_actor.IsNone() || GetInRoomPy(cam_following_actor).IsNone())
		cam = Point(0,0);

	// stop everyone talking & remove displayed text
	StopTalking();

	// fade up again?
	if (fade_val) {
		StartScript(THISBACK2(Fades, fade_val, -1), true);
	}
	else {
		fade_iris = 0;
	}

	// execute the enter() callback of new room (Python)
	if (room_curr.GetType() == PY_DICT) {
		PyValue enter_fn = GetDictItem(room_curr, "enter");
		if(enter_fn.IsFunction()) {
			Vector<PyValue> args;
			args.Add(room_curr);
			py_vm.Call(enter_fn, args);
		}
	}
}

}
