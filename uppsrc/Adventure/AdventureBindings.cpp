#include "Adventure.h"
#include "AdventureBindings.h"

// Don't use NAMESPACE_UPP here because Adventure is a separate namespace
// using namespace Upp;

namespace Adventure {

// ============================================================================
// Helper Functions
// ============================================================================

Program* GetProgram(void* user_data)
{
	return (Program*)user_data;
}

// ============================================================================
// PyFunctionWrapper Implementation
// ============================================================================

EscValue PyFunctionWrapper::Call(const Vector<EscValue>& args)
{
	if(!prog || !func.IsFunction())
		return EscValue();

	// Convert EscValue args to PyValue
	Vector<PyValue> py_args;
	for(int i = 0; i < args.GetCount(); i++) {
		py_args.Add(EscToPyValue(args[i]));
	}

	// Call Python function using public accessor
	PyValue result = prog->GetPyVM().Call(func, py_args);

	// Convert result back to EscValue
	return PyToEscValue(result, prog);
}

// ============================================================================
// Helper Functions
// ============================================================================

// Convert EscValue to PyValue
PyValue EscToPyValue(const EscValue& ev)
{
	if(ev.IsArray()) {
		const Vector<EscValue>& esc_arr = ev.GetArray();
		PyValue py_list = PyValue::List();
		Vector<PyValue>& l = const_cast<Vector<PyValue>&>(py_list.GetArray());
		for(int i = 0; i < esc_arr.GetCount(); i++) {
			l.Add(EscToPyValue(esc_arr[i]));
		}
		return py_list;
	}
	if(ev.IsMap()) {
		const VectorMap<EscValue, EscValue>& esc_map = ev.GetMap();
		PyValue py_dict = PyValue::Dict();
		VectorMap<PyValue, PyValue>& d = py_dict.GetDictRW();
		for(int i = 0; i < esc_map.GetCount(); i++) {
			d.Add(EscToPyValue(esc_map.GetKey(i)), EscToPyValue(esc_map[i]));
		}
		return py_dict;
	}
	if(ev.IsStringLike()) {
		return PyValue(ev.ToString());
	}
	if(ev.IsInt()) {
		return PyValue((int64)ev.GetInt());
	}
	if(ev.IsNumber()) {
		return PyValue(ev.GetNumber());
	}
	return PyValue();
}

// Convert PyValue to EscValue (wraps Python functions)
EscValue PyToEscValue(const PyValue& pv, Program* prog)
{
	if(pv.GetType() == PY_STR) {
		return EscValue(pv.GetStr());
	}
	if(pv.IsInt()) {
		return EscValue(pv.AsInt());
	}
	if(pv.IsFloat()) {
		return EscValue(pv.AsDouble());
	}
	if(pv.GetType() == PY_LIST) {
		// Convert Python list to EscValue array
		const Vector<PyValue>& py_list = pv.GetArray();
		EscValue result;
		result.SetEmptyArray();
		for(int i = 0; i < py_list.GetCount(); i++) {
			result.ArrayAdd(PyToEscValue(py_list[i], prog));
		}
		return result;
	}
	if(pv.GetType() == PY_DICT) {
		// Convert Python dict to EscValue map
		const VectorMap<PyValue, PyValue>& py_dict = pv.GetDict();
		EscValue result;
		result.SetEmptyMap();
		VectorMap<EscValue, EscValue>& esc_map = result.AsMap();
		for(int i = 0; i < py_dict.GetCount(); i++) {
			EscValue key = PyToEscValue(py_dict.GetKey(i), prog);
			EscValue val = PyToEscValue(py_dict[i], prog);
			esc_map.Add(key, val);
		}
		return result;
	}
	if(pv.IsFunction()) {
		// Wrap Python function in PyFunctionWrapper
		// Store as userdata in EscValue
		PyFunctionWrapper* wrapper = new PyFunctionWrapper(pv, pv, "py_func", prog);
		EscValue result;
		// We need to store the wrapper pointer somehow
		// For now, use a map with special marker
		result.SetEmptyMap();
		result.AsMap().Add(EscValue("__pyfunc__"), EscValue((int64)(uintptr_t)wrapper));
		return result;
	}
	return EscValue();
}

// ============================================================================
// Core Functions - Room & Camera (12 total)
// ============================================================================

PyValue AdventureBindings::change_room(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 1) {
		return PyValue::None();
	}

	// Call Python-native ChangeRoomPy directly
	PyValue room_arg = args[0];
	PyValue fade_arg = args.GetCount() > 1 ? args[1] : PyValue();
	
	prog->ChangeRoomPy(room_arg, fade_arg);

	return PyValue::None();
}

PyValue AdventureBindings::camera_follow(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 1) {
		return PyValue::None();
	}

	EscValue actor = PyToEscValue(args[0]);
	prog->CameraFollow(actor);

	return PyValue::None();
}

PyValue AdventureBindings::camera_at(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 1) {
		return PyValue::None();
	}

	// Can be a position (int) or an actor (EscValue)
	if(args[0].IsInt()) {
		Point pos;
		pos.x = args[0].AsInt();
		pos.y = 0;
		prog->CameraAt(pos);
	} else {
		EscValue target = PyToEscValue(args[0]);
		prog->CameraAt(Point(0, 0)); // Would need proper implementation
	}

	return PyValue::None();
}

PyValue AdventureBindings::camera_pan_to(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 1) {
		return PyValue::None();
	}

	EscValue target = PyToEscValue(args[0]);
	prog->CameraPanTo(target);

	return PyValue::None();
}

PyValue AdventureBindings::put_at(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 3) {
		return PyValue::None();
	}

	EscValue obj = PyToEscValue(args[0]);
	int x = args[1].AsInt();
	int y = args[2].AsInt();
	EscValue room = args.GetCount() > 3 ? PyToEscValue(args[3]) : EscValue();

	// Call the C++ implementation
	prog->PutAt(obj, x, y, room);

	return PyValue::None();
}

PyValue AdventureBindings::walk_to(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 3) {
		LOG("walk_to requires 3 arguments: obj, x, y"); return PyValue();
		return PyValue();  // Return None
	}

	// Extract arguments
	EscValue obj = PyToEscValue(args[0]);
	int x = args[1].AsInt();
	int y = args[2].AsInt();

	// Call engine function
	prog->WalkTo(obj, x, y);

	return PyValue();  // Return None
}

PyValue AdventureBindings::do_anim(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 2) {
		LOG("Error: do_anim requires 2 arguments: obj, anim_name"); return PyValue();
	}

	EscValue obj = PyToEscValue(args[0]);
	String anim_name = args[1].GetStr().ToString();
	int param2 = 0;  // Optional parameter for some animations

	// Call the C++ implementation
	prog->DoAnim(obj, anim_name, param2);

	return PyValue::None();
}

PyValue AdventureBindings::get_room(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	// Return current room - this would need a getter in Program
	// For now, return None
	return PyValue::None();
}

PyValue AdventureBindings::get_actor(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	// Return current actor - this would need a getter in Program
	// For now, return None
	return PyValue::None();
}

PyValue AdventureBindings::is_in_room(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 2) {
		LOG("Error: is_in_room requires 2 arguments: obj, room"); return PyValue();
	}

	EscValue obj = PyToEscValue(args[0]);
	EscValue room = PyToEscValue(args[1]);

	// Check if object is in room by comparing in_room property
	// This would need proper implementation in Program
	bool in_room = false;  // Placeholder

	return PyValue(in_room);
}

PyValue AdventureBindings::get_distance(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 2) {
		LOG("Error: get_distance requires 2 arguments: obj1, obj2"); return PyValue();
	}

	EscValue obj1 = PyToEscValue(args[0]);
	EscValue obj2 = PyToEscValue(args[1]);

	// Call the C++ implementation
	double distance = prog->Proximity(obj1, obj2);

	return PyValue(distance);
}

PyValue AdventureBindings::face_direction(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 2) {
		LOG("Error: face_direction requires 2 arguments: obj, dir"); return PyValue();
	}

	EscValue obj = PyToEscValue(args[0]);
	String dir = args[1].GetStr().ToString();

	// Set the face_dir property
	// This would need proper implementation in Program
	// Valid values: "face_front", "face_left", "face_back", "face_right"

	return PyValue::None();
}

// ============================================================================
// UI Functions - Dialog & Text (10 total)
// ============================================================================

PyValue AdventureBindings::say_line(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 1) {
		return PyValue::None();
	}

	// Check if first arg is an actor (multi-arg form) or text (single-arg form)
	WString text;
	EscValue actor;

	if(args[0].GetType() == Upp::PY_STR) {
		// say_line(text)
		text = args[0].GetStr();
	} else if(args.GetCount() >= 2 && args[1].GetType() == Upp::PY_STR) {
		// say_line(actor, text, wait, duration)
		actor = PyToEscValue(args[0]);
		text = args[1].GetStr();
	}

	// Convert :newlines to actual newlines if present
	Upp::String str_text = text.ToString();
	str_text.Replace(":\\n", "\n"); return PyValue();

	// Call the C++ implementation
	prog->SayLine(str_text);

	return PyValue::None();
}

PyValue AdventureBindings::print_line(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 1) {
		return PyValue::None();
	}

	String text = args[0].GetStr().ToString();
	int x = args.GetCount() > 1 ? args[1].AsInt() : 0;
	int y = args.GetCount() > 2 ? args[2].AsInt() : 0;
	int color = args.GetCount() > 3 ? args[3].AsInt() : 1;
	bool shadow = args.GetCount() > 4 ? args[4].IsTrue() : false;
	bool centered = args.GetCount() > 5 ? args[5].IsTrue() : false;
	int width = args.GetCount() > 6 ? args[6].AsInt() : 0;
	bool outline = args.GetCount() > 7 ? args[7].IsTrue() : false;

	// Call the C++ implementation
	prog->PrintLine(text, x, y, color, shadow, centered, width, outline);

	return PyValue::None();
}

PyValue AdventureBindings::dialog_set(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 1) {
		LOG("Error: dialog_set requires 1 argument: options_array"); return PyValue();
	}

	// Extract array of dialog options
	Vector<String> options;
	if(args[0].GetType() == Upp::PY_LIST) {
		PyValue list = args[0];
		for(int i = 0; i < list.GetCount(); i++) {
			PyValue item = list.GetItem(i);
			if(item.GetType() == Upp::PY_STR) {
				options.Add(item.GetStr().ToString());
			}
		}
	}

	// Call the C++ implementation
	prog->DialogSet(options);

	return PyValue::None();
}

PyValue AdventureBindings::dialog_start(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 2) {
		LOG("Error: dialog_start requires 2 arguments: color, highlight_color"); return PyValue();
	}

	int color = args[0].AsInt();
	int highlight_color = args[1].AsInt();

	// Call the C++ implementation
	prog->DialogStart(color, highlight_color);

	return PyValue::None();
}

PyValue AdventureBindings::dialog_clear(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	// Clear dialog options
	prog->DialogClear();

	return PyValue::None();
}

PyValue AdventureBindings::wait_for_camera(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	// Wait for camera movement to complete
	// This would need integration with the camera system
	// For now, just return

	return PyValue::None();
}

PyValue AdventureBindings::fades(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 2) {
		LOG("Error: fades requires 2 arguments: fade_type, duration"); return PyValue();
	}

	int fade_type = args[0].AsInt();
	int duration = args[1].AsInt();

	// Call the C++ implementation
	// prog->Fades(fade_type, duration);

	return PyValue::None();
}

PyValue AdventureBindings::clear_dialog(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	// Clear active dialog - same as dialog_clear
	prog->DialogClear();

	return PyValue::None();
}

PyValue AdventureBindings::say_get(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	// Get last said line - would need a getter in Program
	// For now, return empty string
	return PyValue(""); return PyValue();
}

PyValue AdventureBindings::stop_talking(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	// Stop current dialog
	prog->StopTalking();

	return PyValue::None();
}

// ============================================================================
// Game Logic Functions - Scripts, Inventory, Objects (15 total)
// ============================================================================

PyValue AdventureBindings::break_time(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	int frames = 1; // Default to 1 frame if no argument
	if(args.GetCount() > 0 && !args[0].IsNone()) {
		frames = args[0].AsInt();
	}

	// In ESC, break_time yields control for N frames
	// This would need integration with the script scheduler
	prog->BreakTime(frames);

	return PyValue::None();
}

PyValue AdventureBindings::cutscene(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 2) {
		LOG("Error: cutscene requires 2 arguments: type, setup_fn"); return PyValue();
	}

	int type = args[0].AsInt();
	PyValue setup_fn = args[1];
	PyValue cleanup_fn = args.GetCount() > 2 ? args[2] : PyValue::None();

	// Call Python cutscene function directly
	if(setup_fn.IsFunction()) {
		Vector<PyValue> fn_args;
		fn_args.Add(PyValue());  // self/context placeholder
		prog->GetPyVM().Call(setup_fn, fn_args);
	}

	return PyValue::None();
}

PyValue AdventureBindings::pickup_obj(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 1) {
		LOG("Error: pickup_obj requires 1 argument: obj"); return PyValue();
	}

	EscValue obj = PyToEscValue(args[0]);
	EscValue actor = args.GetCount() > 1 ? PyToEscValue(args[1]) : EscValue();

	// Call the C++ implementation
	prog->PickupObj(obj, actor);

	return PyValue::None();
}

PyValue AdventureBindings::start_script(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 1) {
		LOG("Error: start_script requires 1 argument: script_fn"); return PyValue();
	}

	PyValue script_fn = args[0];
	bool background = args.GetCount() > 1 ? args[1].IsTrue() : false;
	
	// Extract additional arguments for the script function
	Vector<PyValue> script_args;
	for(int i = 2; i < args.GetCount(); i++) {
		script_args.Add(args[i]);
	}

	// Start a Python script using PyVM
	prog->StartScriptPyVM(script_fn, script_args, background);

	return PyValue::None();
}

PyValue AdventureBindings::stop_script(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 1) {
		LOG("Error: stop_script requires 1 argument: script_fn"); return PyValue();
	}

	// Stop a running script
	// This would need proper implementation

	return PyValue::None();
}

PyValue AdventureBindings::is_script_running(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 1) {
		LOG("Error: is_script_running requires 1 argument: script_fn"); return PyValue();
	}

	// Check if script is running - would need proper implementation
	// For now, return false
	return PyValue(false);
}

PyValue AdventureBindings::verb_set(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 1) {
		LOG("Error: verb_set requires 1 argument: verb_index"); return PyValue();
	}

	int verb_index = args[0].AsInt();

	// Set active verb - would need proper implementation
	// prog->verb_curr = verb_index;

	return PyValue::None();
}

PyValue AdventureBindings::verb_get(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	// Get active verb - would need proper implementation
	// For now, return default verb index (4 = "look at")
	return PyValue(4);
}

PyValue AdventureBindings::inventory_get(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	// Get inventory list - would need to iterate over actor's inventory
	// For now, return empty list
	return PyValue::List();
}

PyValue AdventureBindings::has_obj(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 1) {
		LOG("Error: has_obj requires 1 argument: obj"); return PyValue();
	}

	EscValue obj = PyToEscValue(args[0]);

	// Check if player has object in inventory
	// This would need proper implementation
	bool has = false;  // Placeholder

	return PyValue(has);
}

PyValue AdventureBindings::use_obj(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 2) {
		LOG("Error: use_obj requires 2 arguments: obj, target"); return PyValue();
	}

	EscValue obj = PyToEscValue(args[0]);
	EscValue target = PyToEscValue(args[1]);

	// Use object on target - would need proper implementation
	// This would trigger the object's "use" verb handler

	return PyValue::None();
}

PyValue AdventureBindings::set_selected_actor(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 1) {
		LOG("Error: set_selected_actor requires 1 argument: actor"); return PyValue();
	}

	EscValue actor = PyToEscValue(args[0]);

	// Call the C++ implementation
	prog->SetSelectedActor(actor);

	return PyValue::None();
}

PyValue AdventureBindings::get_selected_actor(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	// Call the C++ implementation
	PyValue actor = prog->GetSelectedActor();

	// Return as PyValue
	return actor;
}

PyValue AdventureBindings::open_door(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 1) {
		LOG("Error: open_door requires 1 argument: door"); return PyValue();
	}

	EscValue door = PyToEscValue(args[0]);
	EscValue paired_door = args.GetCount() > 1 ? PyToEscValue(args[1]) : EscValue();

	// Call the C++ implementation
	prog->OpenDoor(door, paired_door);

	return PyValue::None();
}

PyValue AdventureBindings::close_door(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 1) {
		LOG("Error: close_door requires 1 argument: door"); return PyValue();
	}

	EscValue door = PyToEscValue(args[0]);
	EscValue paired_door = args.GetCount() > 1 ? PyToEscValue(args[1]) : EscValue();

	// Call the C++ implementation
	prog->CloseDoor(door, paired_door);

	return PyValue::None();
}

// ============================================================================
// Drawing Functions (9 total)
// ============================================================================

PyValue AdventureBindings::set_trans_col(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 2) {
		return PyValue::None();
	}

	int color = args[0].AsInt();
	bool enable = args[1].IsTrue();

	// Call the C++ implementation
	// prog->SetTransparencyColor(color, enable);

	return PyValue::None();
}

PyValue AdventureBindings::map_draw(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 6) {
		return PyValue::None();
	}

	int src_x = args[0].AsInt();
	int src_y = args[1].AsInt();
	int dest_x = args[2].AsInt();
	int dest_y = args[3].AsInt();
	int width = args[4].AsInt();
	int height = args[5].AsInt();
	int flags = args.GetCount() > 6 ? args[6].AsInt() : 0;

	// Call the C++ implementation
	// prog->MapDraw(src_x, src_y, dest_x, dest_y, width, height, flags);

	return PyValue::None();
}

PyValue AdventureBindings::spr(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 3) {
		LOG("Error: spr requires 3 arguments: sprite, x, y"); return PyValue();
	}

	int sprite_num = args[0].AsInt();
	int x = args[1].AsInt();
	int y = args[2].AsInt();
	int w = args.GetCount() > 3 ? args[3].AsInt() : 1;
	int h = args.GetCount() > 4 ? args[4].AsInt() : 1;
	bool flip_x = args.GetCount() > 5 ? args[5].IsTrue() : false;
	bool flip_y = args.GetCount() > 6 ? args[6].IsTrue() : false;

	// Draw sprite - would need integration with graphics system
	// This is a PICO-8 legacy function

	return PyValue::None();
}

PyValue AdventureBindings::sspr(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 6) {
		LOG("Error: sspr requires 6 arguments: sx, sy, sw, sh, x, y"); return PyValue();
	}

	int sx = args[0].AsInt();
	int sy = args[1].AsInt();
	int sw = args[2].AsInt();
	int sh = args[3].AsInt();
	int x = args[4].AsInt();
	int y = args[5].AsInt();
	// Additional optional parameters
	int dw = args.GetCount() > 6 ? args[6].AsInt() : sw;
	int dh = args.GetCount() > 7 ? args[7].AsInt() : sh;
	bool flip_x = args.GetCount() > 8 ? args[8].IsTrue() : false;
	bool flip_y = args.GetCount() > 9 ? args[9].IsTrue() : false;

	// Draw sprite subregion - PICO-8 legacy function

	return PyValue::None();
}

PyValue AdventureBindings::rect(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 5) {
		LOG("Error: rect requires 5 arguments: x, y, w, h, color"); return PyValue();
	}

	int x = args[0].AsInt();
	int y = args[1].AsInt();
	int w = args[2].AsInt();
	int h = args[3].AsInt();
	int color = args[4].AsInt();

	// Draw rectangle - would need integration with graphics system

	return PyValue::None();
}

PyValue AdventureBindings::circle(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 4) {
		LOG("Error: circle requires 4 arguments: x, y, r, color"); return PyValue();
	}

	int x = args[0].AsInt();
	int y = args[1].AsInt();
	int r = args[2].AsInt();
	int color = args[3].AsInt();

	// Draw circle - would need integration with graphics system

	return PyValue::None();
}

PyValue AdventureBindings::line(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 5) {
		LOG("Error: line requires 5 arguments: x1, y1, x2, y2, color"); return PyValue();
	}

	int x1 = args[0].AsInt();
	int y1 = args[1].AsInt();
	int x2 = args[2].AsInt();
	int y2 = args[3].AsInt();
	int color = args[4].AsInt();

	// Draw line - would need integration with graphics system

	return PyValue::None();
}

PyValue AdventureBindings::pal(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 2) {
		LOG("Error: pal requires 2 arguments: from, to"); return PyValue();
	}

	int from = args[0].AsInt();
	int to = args[1].AsInt();

	// Set palette mapping - PICO-8 legacy function
	// This would map one color index to another

	return PyValue::None();
}

PyValue AdventureBindings::rectfill(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 5) {
		LOG("Error: rectfill requires 5 arguments: x1, y1, x2, y2, color"); return PyValue();
	}

	int x1 = args[0].AsInt();
	int y1 = args[1].AsInt();
	int x2 = args[2].AsInt();
	int y2 = args[3].AsInt();
	int color = args[4].AsInt();

	// Draw filled rectangle - would need integration with graphics system

	return PyValue::None();
}

// ============================================================================
// Audio Functions (2 total)
// ============================================================================

PyValue AdventureBindings::sfx(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 1) {
		LOG("Error: sfx requires 1 argument: sound_id"); return PyValue();
	}

	int sound_id = args[0].AsInt();

	// Play sound effect - would need integration with audio system
	// This is a PICO-8 legacy function (sfx0/sfx1 in ESC)

	return PyValue::None();
}

PyValue AdventureBindings::music(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 1) {
		LOG("Error: music requires 1 argument: track_id"); return PyValue();
	}

	int track_id = args[0].AsInt();

	// Play music track - would need integration with audio system

	return PyValue::None();
}

// ============================================================================
// Object Property Access (4 total)
// ============================================================================

PyValue AdventureBindings::obj_get_prop(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 2) {
		LOG("Error: obj_get_prop requires 2 arguments: obj, property"); return PyValue();
	}

	EscValue obj = PyToEscValue(args[0]);
	String prop = args[1].GetStr().ToString();

	// Get object property - would need proper EscValue property access
	// For now, return None
	return PyValue::None();
}

PyValue AdventureBindings::obj_set_prop(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 3) {
		LOG("Error: obj_set_prop requires 3 arguments: obj, property, value"); return PyValue();
	}

	EscValue obj = PyToEscValue(args[0]);
	String prop = args[1].GetStr().ToString();
	EscValue value = PyToEscValue(args[2]);

	// Set object property - would need proper EscValue property access
	// For now, just acknowledge

	return PyValue::None();
}

PyValue AdventureBindings::obj_get_x(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 1) {
		LOG("Error: obj_get_x requires 1 argument: obj"); return PyValue();
	}

	EscValue obj = PyToEscValue(args[0]);

	// Get object X position - would need proper property access
	// For now, return 0
	return PyValue(0);
}

PyValue AdventureBindings::obj_set_x(const Vector<PyValue>& args, void* user_data)
{
	Program* prog = GetProgram(user_data);

	if(args.GetCount() < 2) {
		LOG("Error: obj_set_x requires 2 arguments: obj, x"); return PyValue();
	}

	EscValue obj = PyToEscValue(args[0]);
	int x = args[1].AsInt();

	// Set object X position - would need proper property access
	// For now, just acknowledge

	return PyValue::None();
}

// ============================================================================
// Registration
// ============================================================================

void AdventureBindings::RegisterAll(PyVM& vm, Program& prog)
{
	LOG("RegisterAll: Starting to register Python bindings");
	PyValue globals = vm.GetGlobals();

	// ========================================================================
	// Core Functions - Room & Camera (12 total)
	// ========================================================================
	globals.SetItem(PyValue("change_room"), PyValue::Function("change_room", change_room, &prog));
	globals.SetItem(PyValue("camera_follow"), PyValue::Function("camera_follow", camera_follow, &prog));
	globals.SetItem(PyValue("camera_at"), PyValue::Function("camera_at", camera_at, &prog));
	globals.SetItem(PyValue("camera_pan_to"), PyValue::Function("camera_pan_to", camera_pan_to, &prog));
	globals.SetItem(PyValue("put_at"), PyValue::Function("put_at", put_at, &prog));
	globals.SetItem(PyValue("walk_to"), PyValue::Function("walk_to", walk_to, &prog));
	globals.SetItem(PyValue("do_anim"), PyValue::Function("do_anim", do_anim, &prog));
	globals.SetItem(PyValue("get_room"), PyValue::Function("get_room", get_room, &prog));
	globals.SetItem(PyValue("get_actor"), PyValue::Function("get_actor", get_actor, &prog));
	globals.SetItem(PyValue("is_in_room"), PyValue::Function("is_in_room", is_in_room, &prog));
	globals.SetItem(PyValue("get_distance"), PyValue::Function("get_distance", get_distance, &prog));
	globals.SetItem(PyValue("face_direction"), PyValue::Function("face_direction", face_direction, &prog));
	LOG("RegisterAll: Registered 12 core functions");

	// ========================================================================
	// UI Functions - Dialog & Text (10 total)
	// ========================================================================
	globals.SetItem(PyValue("say_line"), PyValue::Function("say_line", say_line, &prog));
	globals.SetItem(PyValue("print_line"), PyValue::Function("print_line", print_line, &prog));
	globals.SetItem(PyValue("dialog_set"), PyValue::Function("dialog_set", dialog_set, &prog));
	globals.SetItem(PyValue("dialog_start"), PyValue::Function("dialog_start", dialog_start, &prog));
	globals.SetItem(PyValue("dialog_clear"), PyValue::Function("dialog_clear", dialog_clear, &prog));
	globals.SetItem(PyValue("wait_for_camera"), PyValue::Function("wait_for_camera", wait_for_camera, &prog));
	globals.SetItem(PyValue("fades"), PyValue::Function("fades", fades, &prog));
	globals.SetItem(PyValue("clear_dialog"), PyValue::Function("clear_dialog", clear_dialog, &prog));
	globals.SetItem(PyValue("say_get"), PyValue::Function("say_get", say_get, &prog));
	globals.SetItem(PyValue("stop_talking"), PyValue::Function("stop_talking", stop_talking, &prog));
	LOG("RegisterAll: Registered 10 UI functions");

	// ========================================================================
	// Game Logic Functions - Scripts, Inventory, Objects (15 total)
	// ========================================================================
	globals.SetItem(PyValue("break_time"), PyValue::Function("break_time", break_time, &prog));
	globals.SetItem(PyValue("cutscene"), PyValue::Function("cutscene", cutscene, &prog));
	globals.SetItem(PyValue("pickup_obj"), PyValue::Function("pickup_obj", pickup_obj, &prog));
	globals.SetItem(PyValue("start_script"), PyValue::Function("start_script", start_script, &prog));
	globals.SetItem(PyValue("stop_script"), PyValue::Function("stop_script", stop_script, &prog));
	globals.SetItem(PyValue("is_script_running"), PyValue::Function("is_script_running", is_script_running, &prog));
	globals.SetItem(PyValue("verb_set"), PyValue::Function("verb_set", verb_set, &prog));
	globals.SetItem(PyValue("verb_get"), PyValue::Function("verb_get", verb_get, &prog));
	globals.SetItem(PyValue("inventory_get"), PyValue::Function("inventory_get", inventory_get, &prog));
	globals.SetItem(PyValue("has_obj"), PyValue::Function("has_obj", has_obj, &prog));
	globals.SetItem(PyValue("use_obj"), PyValue::Function("use_obj", use_obj, &prog));
	globals.SetItem(PyValue("set_selected_actor"), PyValue::Function("set_selected_actor", set_selected_actor, &prog));
	globals.SetItem(PyValue("get_selected_actor"), PyValue::Function("get_selected_actor", get_selected_actor, &prog));
	globals.SetItem(PyValue("open_door"), PyValue::Function("open_door", open_door, &prog));
	globals.SetItem(PyValue("close_door"), PyValue::Function("close_door", close_door, &prog));
	LOG("RegisterAll: Registered 15 game logic functions");

	// ========================================================================
	// Drawing Functions (9 total)
	// ========================================================================
	globals.SetItem(PyValue("set_trans_col"), PyValue::Function("set_trans_col", set_trans_col, &prog));
	globals.SetItem(PyValue("map_draw"), PyValue::Function("map_draw", map_draw, &prog));
	globals.SetItem(PyValue("spr"), PyValue::Function("spr", spr, &prog));
	globals.SetItem(PyValue("sspr"), PyValue::Function("sspr", sspr, &prog));
	globals.SetItem(PyValue("rect"), PyValue::Function("rect", rect, &prog));
	globals.SetItem(PyValue("circle"), PyValue::Function("circle", circle, &prog));
	globals.SetItem(PyValue("line"), PyValue::Function("line", line, &prog));
	globals.SetItem(PyValue("pal"), PyValue::Function("pal", pal, &prog));
	globals.SetItem(PyValue("rectfill"), PyValue::Function("rectfill", rectfill, &prog));
	LOG("RegisterAll: Registered 9 drawing functions");

	// ========================================================================
	// Audio Functions (2 total)
	// ========================================================================
	globals.SetItem(PyValue("sfx"), PyValue::Function("sfx", sfx, &prog));
	globals.SetItem(PyValue("music"), PyValue::Function("music", music, &prog));
	LOG("RegisterAll: Registered 2 audio functions");

	// ========================================================================
	// Object Property Access (4 total)
	// ========================================================================
	globals.SetItem(PyValue("obj_get_prop"), PyValue::Function("obj_get_prop", obj_get_prop, &prog));
	globals.SetItem(PyValue("obj_set_prop"), PyValue::Function("obj_set_prop", obj_set_prop, &prog));
	globals.SetItem(PyValue("obj_get_x"), PyValue::Function("obj_get_x", obj_get_x, &prog));
	globals.SetItem(PyValue("obj_set_x"), PyValue::Function("obj_set_x", obj_set_x, &prog));
}

}  // namespace Adventure
