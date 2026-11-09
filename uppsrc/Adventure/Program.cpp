#include "Adventure.h"
#include "AdventureBindings.h"
#include "builtin.brc"

namespace Adventure {


bool ReadFlag(const SObj& o, String key) {
	TODO
}





void ProgramDraw::LoadBuiltinGfx() {
	//Program::LoadBuiltinGfxStr(builtin_gfx, gfx_data, gfx_sz);
	//Program::LoadBuiltinGfxStr(builtin_lbl, lbl_data, lbl_sz);
	gfx_sz = Size(128,128);
	lbl_sz = Size(128,128);
	int gfx_len = gfx_sz.cx * gfx_sz.cy;
	int lbl_len = lbl_sz.cx * lbl_sz.cy;
	ASSERT(gfx_len == builtin_gfx_length);
	ASSERT(lbl_len == builtin_lbl_length);
	
	GetPaletteImage(builtin_gfx, gfx_sz, gfx);
	GetPaletteImage(builtin_lbl, lbl_sz, lbl);
}



Program::Program() {
	draw_zplanes.SetCount(128+1);

	map_sz = Size(256,32);
	gff_sz = Size(256,2);

	map.SetCount(builtin_map_length);
	memcpy(map.Begin(), builtin_map, builtin_map_length);

	gff.SetCount(builtin_gff_length);
	memcpy(gff.Begin(), builtin_gff, builtin_gff_length);

	ResetUI();
}

bool Program::InitPyVM(const String& script_path) {
	LOG("InitPyVM: Starting initialization");

	// Initialize PyVM and register all bindings
	AdventureBindings::RegisterAll(vm, *this);
	LOG("InitPyVM: Registered bindings");

	// Load Python game script
	// Priority: 1) Command-line argument, 2) Demo.py, 3) Game.py
	String game_py_path;
	
	if (!script_path.IsEmpty()) {
		game_py_path = script_path;
		LOG("InitPyVM: Using script from command line: " << game_py_path);
	} else {
		game_py_path = GetDataFile("Demo.py");
		LOG("InitPyVM: Looking for Demo.py at: " << game_py_path);
	}

	bool found = FileExists(game_py_path);
	LOG("InitPyVM: Script exists=" << found);
	
	if (!found && script_path.IsEmpty()) {
		game_py_path = GetDataFile("Game.py");
		LOG("InitPyVM: Demo.py not found, trying Game.py at: " << game_py_path);
		found = FileExists(game_py_path);
		LOG("InitPyVM: Game.py exists=" << found);
	}

	if (found) {
		String src = LoadFile(game_py_path);
		LOG("InitPyVM: Loaded " << src.GetCount() << " bytes");

		if (!src.IsEmpty()) {
			String module_name = game_py_path.Find("Demo.py") >= 0 ? "demo" : "game";
			LOG("InitPyVM: Loading module '" << module_name << "'");
			
			if (vm.LoadModule(module_name, src, game_py_path)) {
				LOG("InitPyVM: SUCCESS - " << module_name << ".py loaded");

				// Inject bindings into module's globals so Python code can call them
				PyValue sys = vm.GetGlobals().GetItem(PyValue("sys"));
				if(sys.GetType() == PY_DICT) {
					PyValue modules = sys.GetItem(PyValue("modules"));
					if(modules.GetType() == PY_DICT) {
						PyValue mod = modules.GetItem(PyValue(module_name));
						if(mod.GetType() == PY_DICT) {
							LOG("InitPyVM: Injecting bindings into module globals");
							// Copy all bindings from vm.globals to module globals
							const VectorMap<PyValue, PyValue>& vm_globals = vm.GetGlobals().GetDict();
							VectorMap<PyValue, PyValue>& mod_dict = mod.GetDictRW();
							for(int i = 0; i < vm_globals.GetCount(); i++) {
								PyValue key = vm_globals.GetKey(i);
								PyValue val = vm_globals[i];
								if(val.IsFunction()) {
									mod_dict.Add(key, val);
								}
							}
							LOG("InitPyVM: Bindings injected");
						}
					}
				}
			} else {
				LOG("InitPyVM: FAILED - LoadModule returned error");
				return false;
			}
		} else {
			LOG("InitPyVM: FAILED - Python file is empty");
			return false;
		}
	} else {
		LOG("InitPyVM: FAILED - Python file not found at either location");
		return false;
	}

	return true;
}

// PyVM helper: Get value from dict by string key
PyValue Program::GetDictItem(const PyValue& dict, const char* key)
{
	if(dict.GetType() != PY_DICT)
		return PyValue();
	const VectorMap<PyValue, PyValue>& d = dict.GetDict();
	for(int i = 0; i < d.GetCount(); i++) {
		if(d.GetKey(i).GetType() == PY_STR && d.GetKey(i).GetStr() == WString(key))
			return d[i];
	}
	return PyValue();
}

bool Program::Init(const String& script_path) {

	ResetUI();

	ClearCurrCmd();

	ctx.Clear();

	fade_iris = 0;
	cutscene_cooloff = 0;

	ctx.InitializeEmptyScene();

	// Initialize PyVM and register bindings
	if (!InitPyVM(script_path))
		return false;

	// Initialize ESC VM for backward compatibility (optional for Python-only games)
	String game_esc_path = GetDataFile("Game.esc");
	if (FileExists(game_esc_path)) {
		if (!ctx.AddCodePath(game_esc_path))
			return false;
	} else {
		LOG("Init: Game.esc not found, using Python-only mode");
	}

	if (!ctx.Init(false))
		return false;

	// AddEscFunctions() removed - Esc scripts no longer supported

	// init all the rooms/objects/actors
	if (!InitGame())
		return false;

	if (!ReadGame())
		return false;

	ctx.KeepRunning();

	return true;
}

// PyValue accessors
PyValue Program::GetProp(const PyValue& obj, const char* key) {
	if(obj.GetType() != PY_DICT)
		return PyValue();
	const VectorMap<PyValue, PyValue>& d = obj.GetDict();
	for(int i = 0; i < d.GetCount(); i++) {
		if(d.GetKey(i).GetType() == PY_STR && d.GetKey(i).GetStr() == WString(key))
			return d[i];
	}
	return PyValue();
}

void Program::SetProp(PyValue& obj, const char* key, const PyValue& val) {
	if(obj.GetType() != PY_DICT)
		return;
	VectorMap<PyValue, PyValue>& d = obj.GetDictRW();
	PyValue key_val(WString(key));
	int idx = d.Find(key_val);
	if(idx >= 0)
		d[idx] = val;
	else
		d.Add(key_val, val);
}

int Program::PyInt(const PyValue& v, int def) {
	if(v.IsInt()) return (int)v.AsInt();
	if(v.IsFloat()) return (int)v.AsDouble();
	return def;
}

WString Program::PyStr(const PyValue& v) {
	return v.GetType() == PY_STR ? v.GetStr() : WString();
}

PyValue Program::ClassesPy(const PyValue& s) {
	return GetProp(s, "classes");
}

bool Program::HasFlag(const PyValue& obj, String key) {
	PyValue classes = ClassesPy(obj);
	if(classes.GetType() != PY_LIST)
		return false;
	const Vector<PyValue>& arr = classes.GetArray();
	for(int i = 0; i < arr.GetCount(); i++) {
		if(arr[i].GetType() == PY_STR && arr[i].GetStr() == WString(key))
			return true;
	}
	return false;
}

PyValue Program::GetSelectedActorPy() {
	return cam_following_actor;
}

/*void Program::ProcessEsc() {
	//LOG("Program::ProcessEsc: start");
	
	int i = 0;
	for (Script& s : global_scripts) {
		if (s.is_esc && s.IsRunning()) {
			s.ProcessEsc();
			i++;
		}
	}
	
	for (Script& s : local_scripts) {
		if (s.is_esc && s.IsRunning()) {
			s.ProcessEsc();
			i++;
		}
	}
	
	for (Script& s : cutscenes) {
		if (s.is_esc && s.IsRunning()) {
			s.ProcessEsc();
			i++;
		}
	}
	
	//LOG("Program::ProcessEsc: end " << i);
}*/

const SObj* Program::FindDeep(const String& name) const {
	DUMP(name);
	TODO
	//return FindDeep(name, &game);
}

const SObj* Program::FindDeep(const String& name, const SObj* o) const {
	if (!o || !o->IsMap())
		return 0;
	const auto& m = o->GetMap();
	int i = m.Find(name);
	if (i >= 0)
		return &m[i];
	const auto& v = m.GetValues();
	for (const auto& val : v) {
		const SObj* r = FindDeep(name, &val);
		if (r)
			return r;
	}
	return 0;
}

EscValue Program::Classes(SObj s) {
	if (!s.IsMap()) {DUMP(s);}
	ASSERT(s.IsMap());
	//DUMP(s);
	return s("classes");
}

String Program::State(SObj& s) {
	TODO
}

/*String Program::GetInRoomString(SObj& o) {
	if (o.IsMap()) {
		SObj in_room = o.MapGet("in_room");
		if (in_room.IsArray()) {
			String name = in_room.ToString();
			return name;
		}
	}
	DUMP(o);
	ASSERT(0);
	return String();
}*/







PyValue Program::FindDefaultVerb(SObj obj) {
	PyValue default_verb = V_LOOKAT;

	if (HasFlag(Classes(obj), "class_talkable"))
		default_verb = V_TALKTO;
	else if (HasFlag(Classes(obj), "class_openable")) {
		if (State(obj) == "state_closed")
			default_verb = V_OPEN;
		else
			default_verb = V_CLOSE;
	}

	return default_verb;
}





bool Program::IsValidVerb(PyValue verb, PyValue object) {
	// check if object has verbs property
	PyValue verbs = Program::GetProp(object, "verbs");
	if (verbs.GetType() != PY_DICT)
		return false;
	
	// look for verb in object's verbs
	PyValue verb_name = Program::GetProp(verb, "name");
	if (verb_name.GetType() != PY_STR)
		return false;
	
	PyValue found_verb = Program::GetProp(verbs, verb_name.GetStr().ToString());
	return found_verb.GetType() != PY_NONE;
}


void Program::BreakTime(int jiffies) {
	// wait for (cycles specified (min 1 cycle)
	jiffies = max(1, jiffies);
	for (int x = 0; x < jiffies; x++) {
		TODO // yield();
	}
}

/*void Program::WaitForMessage() {
	while (talking_curr) {
		yield();
	}
}*/


#if 0
void Program::GetReference(SObj& obj, bool everywhere) {
	if (!obj.IsMap()) {
		String name = obj;
		//LOG(name);
		int i = global.Find(name);
		if (i >= 0) {
			obj = global[i];
			return;
		}
		
		/*const SObj* f = FindDeep(name, &room_curr);
		if (f) {
			obj = *f;
			return;
		}
		if (everywhere) {
			f = FindDeep(name, &game);
			if (f) {
				obj = *f;
				return;
			}
		}*/
		ASSERT(0);
	}
}
#endif


double Program::Proximity(SObj& obj1, SObj& obj2) {
	// calc dist between objects, || big value for (different room
	Point pt1 = GetXY(obj1);
	Point pt2 = GetXY(obj2);
	return
		GetInRoom(obj1) == GetInRoom(obj2)
			? sqrt((pt1.x - pt2.x) ^ 2 + (pt1.y - pt2.y) ^ 2)
			: 1000;
}




/*const StrMap& Program::get_keys(SObj& obj) {
	local keys = {};
	for (k,v in pairs(obj)) {
		add(keys,k);
	}
	return keys;
}*/

PyValue Program::GetVerb(int idx) {
	if(idx < 0 || idx >= V_COUNT)
		return PyValue();
	const Vector<PyValue>& verbs_arr = verbs.GetArray();
	if(idx >= verbs_arr.GetCount())
		return PyValue();
	return verbs_arr[idx];
}

String Program::GetVerbString(int i) {
	if (i >= 0 && i < V_COUNT) {
		const Vector<PyValue>& verbs_arr = verbs.GetArray();
		if(i >= verbs_arr.GetCount())
			return "<error>";
		PyValue v = verbs_arr[i];
		if(v.GetType() == PY_DICT) {
			const VectorMap<PyValue, PyValue>& d = v.GetDict();
			for(int j = 0; j < d.GetCount(); j++) {
				if(d.GetKey(j).GetStr() == WString("text")) {
					return d[j].GetStr().ToString();
				}
			}
		}
	}
	return "<error>";
}

String Program::GetVerbString(SObj v) {
	if (v.IsVoid())
		return String();
	if (!v.IsMap()) {
		LOG(v.ToString());
	}
	ASSERT(v.IsMap());
	String name = v.MapGet("name");
	ASSERT(name.GetCount() > 0);
	return name;
}

void Program::ClearCurrCmd() {
	// reset all command values
	verb_curr = V_DEFAULT;
	executing_cmd = 0;
	cmd_curr.Clear();
	noun1_curr = PyValue();
	noun2_curr = PyValue();
	//me.Clear();
}



void Program::Update() {
	PyValue selected_actor = GetSelectedActor();

	// See EscAnimContext::Iterate
	// It is implemented here
	ASSERT(ctx.IsRunning());

	// global scripts (always updated - regardless of cutscene)
	ctx.ProcessAndRemoveGroup(SCRIPT_GLOBAL);

	// update active cutscene (if (any)
	if (cutscene_curr) {
		Script& cut = *cutscene_curr;
		
		if (!cut.IsRunning() || !cut.Process()) {
			// cutscene ended, restore prev state

			// restore follow-cam if (flag allows (and had a value!)
			dword& flags = cut.user_flags;
			EscValue& paused_cam_following = cut.user;
			if (flags != 3 && paused_cam_following) {
				CameraFollow(paused_cam_following);
				
				// assume to re-select prev actor
				SetSelectedActor(paused_cam_following);
			}
			
			// now delete cutscene
			ctx.RemoveStoppedGroup(SCRIPT_CUTSCENE);

			// any more cutscenes?
			Vector<EscAnimProgram*> cutscenes = ctx.FindGroupPrograms(SCRIPT_CUTSCENE);
			if (!cutscenes.IsEmpty()) {
				cutscene_curr = dynamic_cast<Script*>(cutscenes.Top());
				ASSERT(cutscene_curr);
			}
			else {
				// start countdown (delay to ensure cutscenes/dialogs all over!)
				if (flags != 2)
					cutscene_cooloff = 3;
				
				cutscene_curr = NULL;
			}
		}
	}
	else {
		// no cutscene...
		// update all the active scripts
		// (will auto-remove those that have ended)

		// local/room-level scripts
		ctx.ProcessAndRemoveGroup(SCRIPT_LOCAL);
	}
	
	ctx.p.Data();
	if (!ctx.IsRunning())
		ctx.WhenStop();
	
	// player/ui control
	PlayerControl();

	// check for (collisions
	CheckCollisions();

	// update camera shake (if (active)
	cam_shake_x = 1.5 - Randomf() * 3;
	cam_shake_y = 1.5 - Randomf() * 3;
	cam_shake_x = (int)(cam_shake_x * cam_shake_amount);
	cam_shake_y = (int)(cam_shake_y * cam_shake_amount);
	
	if (!cam_shake)
		cam_shake_amount = cam_shake_amount > 0.05 ? cam_shake_amount * 0.90 : 0;
	
}







// function show_collision_box(obj)
// 	local obj_bounds = obj.bounds
// 	if (show_collision
// 	 && obj_bounds
// 	then
// 		rect(obj_bounds.x, obj_bounds.y, obj_bounds.x1, obj_bounds.y1, 8)
// 	}
// }

bool Program::IsTable(SObj& t) {
	TODO
	///return type(t) == "table";
}



Point Program::GetCellPos(SObj& obj) {
	Point p;
	const Vector<PyValue>& map_arr = room_curr.GetArray();
	int map_x = map_arr.GetCount() > 0 ? PyInt(map_arr[0]) : 0;
	int map_y = map_arr.GetCount() > 1 ? PyInt(map_arr[1]) : 0;
	int obj_x = obj("x").GetInt();
	int obj_y = obj("y").GetInt();
	p.x = obj_x/8 + map_x;
	p.y = obj_y/8 + map_y;
	return p;
}


bool Program::HasFlag(const SObj& obj, String key) {
	ASSERT(obj.IsVoid() || obj.IsArray());
	/*if (obj.IsMap()) {
		const auto& map = obj.GetMap();
		return map.Find(key) >= 0;
	}
	else */
	//DUMP(obj);
	if (obj.IsArray()) {
		const auto& arr = obj.GetArray();
		for (const auto& v : arr)
			if ((String)v == key)
				return true;
	}
	return false;
}




//
// helper functions
//



void Program::ShowError(String msg) {
	TODO
	//PrintLine("-error-;"..msg,5+cam_x,5,8,0);
}

void Program::ExplodeData(SObj& obj) {
	TODO
	/*for (l in all(split(obj.data, "\n"))) {
		local pairs=split(l, "=");
		if (#pairs==2) {
			obj[pairs[1]] = Autotype(pairs[2]);
		}
		else {
			printh(" > invalid data: ["..pairs[1].."]");
		}
	}*/
}

// split s on delimiter, ignoring leading and trailing space and tab

/*function split(s, delimiter) {
	local retval, start_pos, last_char_pos = {}, 0, 0;

	for (i=1,#s) {
		local curr_letter = sub(s,i,i);
		if (curr_letter == delimiter) {
			add(retval, sub(s,start_pos,last_char_pos));
			start_pos, last_char_pos = 0, 0;
		}
		else if (curr_letter != " "
		 && curr_letter != "\t") {
			// curr letter is useful
			last_char_pos, start_pos = i, start_pos == 0 ? i : start_pos;
		}
	}
	// add remaining content?
	if (start_pos + last_char_pos > 0) {
		add(retval, sub(s,start_pos,last_char_pos));
	}
	return retval
}*/

String Program::Autotype(const String& str_value) {
	TODO
	/*local first_letter = sub(str_value, 1, 1);
	
	if (str_value == "true") {
		return true;
	}
	else if (str_value == "false") {
		return false;
	}
	else if (tonum(str_value)) {
		// must be number
		return tonum(str_value);
	}
	else if (first_letter == "{") {
		// array - so split it
		local temp = sub(str_value, 2, #str_value-1);
		retarray = {};
		for (val in all(split(temp, ","))) {
			add(retarray, Autotype(val))
			;
		}
		return retarray;
	}
	else {//if (first_letter == "\"" {
		// string - so) { nothing
		return str_value;
	}*/
}



String Program::SmallCaps(const String& s) {
	TODO
	/*
	local t = "";
	for (i = 1, #s) {
		local c = ord(s, i);
		t.. = chr(c > 96 && c < 123 ? c - 32 : c);
	}
	return t;
	*/
}


bool HasArrayValue(SObj arr, SObj value) {
	ASSERT(arr.IsArray());
	if (!arr.IsArray()) return false;
	const auto& a = arr.GetArray();
	for (const SObj& o : a)
		if (o == value)
			return true;
	return false;
}




}
