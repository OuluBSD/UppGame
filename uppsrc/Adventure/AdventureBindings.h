#ifndef _Adventure_AdventureBindings_h_
#define _Adventure_AdventureBindings_h_

#include "Adventure.h"
#include <ByteVM/ByteVM.h>

namespace Adventure {

// Don't use NAMESPACE_UPP here - we're declaring classes in the Adventure namespace

// Python function wrapper - stores Python function reference for Esc calls
struct PyFunctionWrapper : Moveable<PyFunctionWrapper> {
	PyValue func;       // Python function object
	PyValue module;     // Python module where function lives (for globals)
	String name;        // Function name for debugging
	Program* prog;      // Program pointer for PyVM access
	
	PyFunctionWrapper() : prog(nullptr) {}
	PyFunctionWrapper(const PyValue& f, const PyValue& m, const String& n, Program* p) 
		: func(f), module(m), name(n), prog(p) {}
	
	// Call the Python function from Esc context
	EscValue Call(const Vector<EscValue>& args);
};

// Helper functions
Program* GetProgram(void* user_data);
PyValue GetDictItem(const PyValue& dict, const char* key);
String PyStr(const PyValue& v);
int PyInt(const PyValue& v, int def = 0);

// Convert EscValue to PyValue (for calling Python from Esc)
PyValue EscToPyValue(const EscValue& ev);

// Convert PyValue to EscValue (for calling Esc from Python)
// Wraps Python functions so they can be called from Esc scripts
EscValue PyToEscValue(const PyValue& pv, Program* prog = nullptr);

class AdventureBindings {
public:
	// Register all bindings with the PyVM
	static void RegisterAll(Upp::PyVM& vm, Program& prog);

	// ========================================================================
	// Core Functions - Room & Camera (12 total)
	// ========================================================================
	static Upp::PyValue change_room(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue camera_follow(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue camera_at(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue camera_pan_to(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue put_at(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue walk_to(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue do_anim(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue get_room(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue get_actor(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue is_in_room(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue get_distance(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue face_direction(const Upp::Vector<Upp::PyValue>& args, void* user_data);

	// ========================================================================
	// UI Functions - Dialog & Text (10 total)
	// ========================================================================
	static Upp::PyValue say_line(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue print_line(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue dialog_set(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue dialog_start(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue dialog_clear(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue wait_for_camera(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue fades(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue clear_dialog(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue say_get(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue stop_talking(const Upp::Vector<Upp::PyValue>& args, void* user_data);

	// ========================================================================
	// Game Logic Functions - Scripts, Inventory, Objects (15 total)
	// ========================================================================
	static Upp::PyValue break_time(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue cutscene(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue pickup_obj(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue start_script(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue stop_script(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue is_script_running(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue verb_set(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue verb_get(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue inventory_get(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue has_obj(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue use_obj(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue set_selected_actor(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue get_selected_actor(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue open_door(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue close_door(const Upp::Vector<Upp::PyValue>& args, void* user_data);

	// ========================================================================
	// Drawing Functions (9 total)
	// ========================================================================
	static Upp::PyValue set_trans_col(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue map_draw(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue spr(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue sspr(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue rect(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue circle(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue line(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue pal(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue rectfill(const Upp::Vector<Upp::PyValue>& args, void* user_data);

	// ========================================================================
	// Audio Functions (2 total)
	// ========================================================================
	static Upp::PyValue sfx(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue music(const Upp::Vector<Upp::PyValue>& args, void* user_data);

	// ========================================================================
	// Object Property Access (4 total)
	// ========================================================================
	static Upp::PyValue obj_get_prop(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue obj_set_prop(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue obj_get_x(const Upp::Vector<Upp::PyValue>& args, void* user_data);
	static Upp::PyValue obj_set_x(const Upp::Vector<Upp::PyValue>& args, void* user_data);
};

}  // namespace Adventure

#endif
