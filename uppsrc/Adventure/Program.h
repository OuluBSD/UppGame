#ifndef _Adventure_Program_h_
#define _Adventure_Program_h_


namespace Adventure {

#define PALETTE_SIZE 16

/*struct ScriptObject {
	
};

using SObj = ScriptObject;
*/

typedef int PaletteColor;
typedef int PaletteImage;


Color GetPicoPalette(PaletteColor idx);

struct Room {
	
};

using SObj = EscValue;
using StrMap = VectorMap<String,String>;
using StrVec = Vector<String>;

Color ReadColor(const SObj& o, EscValue key, Color def);
bool TryReadColor(const SObj& o, EscValue key, Color& c);
Color ReadColor(const PyValue& o, const char* key, Color def);
bool TryReadColor(const PyValue& o, const char* key, Color& c);
bool ReadFlag(const SObj& o, String key);
bool HasArrayValue(SObj arr, SObj value);

typedef enum {
	SCENE_NULL,
	SCENE_NO_VERBS,
	SCENE_QUICK_CUT,
	SCENE_CUT_NO_FOLLOW // no verbs & no follow,
} SceneType;

typedef enum {
	STATE_OPEN,
	STATE_CLOSED,
} StateType;

typedef enum {
	FACE_NULL,
	FACE_FRONT,
	FACE_LEFT,
	FACE_BACK,
	FACE_RIGHT,
} FaceDir;

typedef enum {
	BTN_LEFT,
	BTN_RIGHT,
	BTN_UP,
	BTN_DOWN,
	BTN_O,
	BTN_X,
	
	BTN_COUNT,
} GamepadButton;

typedef enum {
	MBMASK_LEFT = 0x1,
	MBMASK_RIGHT = 0x2,
	MBMASK_MIDDLE = 0x4,
} MouseButtonMask;

struct Bounds {
	int cam_off_x, x1, y1, x, y;
};

struct Sentence : Moveable<Sentence> {
	int num;
	String msg;
	Vector<String> lines;
	int char_width;
	Bounds bounds;
};

struct Dialog {
	Array<Sentence> sentences;
	int col = 0;
	int hlcol = 0;
	bool visible = 0;
	bool enabled = false;
	
	
	Sentence& Add();
	void Clear();
	
	operator bool() const {return enabled;}
	
};
/*
struct Script : public EscAnimProgram {
	int op_limit = 1000000;
	static const int op_limit_at_once = 100;
	
	SceneType type = SCENE_NULL;
	bool running = false;
	Gate0 fn;
	TimeCallback tc;
	SObj* paused_cam_following = 0;
	dword flags = 0;
	EscGlobal* global = 0;
	String fn_name;
	EscValue a0, a1;
	One<Esc> esc;
	bool is_esc = false;
	Callback1<Script*> WhenStop;
	
	typedef Script CLASSNAME;
	void Clear();
	Script& Set(Gate0 cb, EscValue a0=EscValue(), EscValue a1=EscValue());
	Script& Set(EscGlobal& g, EscValue *self, EscValue fn, EscValue a0=EscValue(), EscValue a1=EscValue());
	Script& Start();
	Script& Stop();
	void Execute();
	bool ProcessEsc();
	bool RunEscSteps();
	
};*/

struct TalkingState {
	Vector<String> msg_lines;
	int x = 0;
	int y = 0;
	int col = 0;
	int align = 0;
	double time_left = 0;
	int char_width = 0;
	bool use_caps = 0;
	bool big_font = 0;
	bool enabled = false;
	
	operator bool() const {return enabled;}
	
};

struct ZPlane : Moveable<ZPlane> {
	Vector<EscValue> objs;
	Vector<PyValue> objs_py;


};

class ProgramDraw;

class Program {
	
protected:
	friend class ProgramDraw;
	
	typedef Program CLASSNAME;
	
	
	ProgramDraw* draw = 0;
	
	bool show_collision = true;
	bool show_pathfinding = true;
	bool show_depth = true;
	bool enable_diag_squeeze = false;
	
	/*typedef enum {
		V_DEFAULT,
		
		V_OPEN,
		V_CLOSE,
		V_GIVE,
		V_PICKUP,
		V_LOOKAT,
		V_TALKTO,
		V_PUSH,
		V_PULL,
		V_USE,
		V_WALKTO,
		
		V_COUNT,
		
		V_BEGIN = V_OPEN,
		V_END = V_COUNT,
	} Verb;*/
	
	/*const char* verbs[(int)V_COUNT] {
		"<null>",
		
		"open",
		"close",
		"give",
		"pickup",
		"look at",
		"talk to",
		"push",
		"pull",
		"use",
		"walk to",
	};*/
	
	typedef enum {
		POS_NULL,
		POS_LEFT,
		POS_RIGHT,
		POS_ABOVE,
		POS_CENTER,
		POS_INFRONT
	} UsePos;
	
	/*struct Thing {
		int x = 0;
		int y = 0;
		int face_dir = 0;
		bool flip = 0;
		String curr_anim;
		int anim_pos = 0;
		int tmr = 0;
		
	};*/
	
	enum {
		SCRIPT_GLOBAL,
		SCRIPT_LOCAL,
		SCRIPT_CUTSCENE,
	};

	/*
	Array<Script> global_scripts;	// table of scripts that are at game-level (background)
	Array<Script> local_scripts;	// table of scripts that are actively running
	Array<Script> cutscenes;		// table of scripts for (the active Cutscene(s)
	*/

	// Script class supporting both PyVM (Python) and ESC (legacy) execution
	struct Script : public EscAnimProgram {
		// PyVM members for Python script execution
		PyValue py_func;              // Python function to call
		Vector<PyValue> py_args;      // Arguments for Python function
		int calls_remaining;          // For multi-call scripts (-1 = infinite)
		bool is_python;               // true if this is a Python script
		PyVM* py_vm;                  // Pointer to PyVM instance

		typedef Script CLASSNAME;
		Script();
		void Clear();
		
		// Python script setup
		Script& SetPyVM(PyVM& vm, const PyValue& func, const Vector<PyValue>& args = Vector<PyValue>(), int calls = 1);
		
		// Override Iterate to handle PyVM execution (hide override since base is not virtual)
		void Iterate();
		
		// Execute Python function
		bool ProcessPyVM();
	};

	using ScriptBase = EscAnimProgram;  // Keep reference to base for backward compat
	
	
	Script& AddScript(String name, int group);
	Script& AddLocal(String name);
	Script& AddGlobal(String name);
	Script& AddCutscene(String name);

	Script* cutscene_curr = 0;
	
	
	double dbg_sleep_multiplier = 0.4;
	
	int verb_default = 4;
	
	PaletteColor verb_maincol;
	PaletteColor verb_hovcol;
	PaletteColor verb_shadcol;
	PaletteColor verb_defcol;
	PaletteColor ui_cursor_cols[6];
	static const int ui_cursor_cols_len = 6;
	
	PaletteImage ui_cursorspr;
	PaletteImage ui_uparrowspr;
	PaletteImage ui_dnarrowspr;
	
	bool cam_shake = false;
	Point cam_pan_to;
	PyValue cam_following_actor;  // PyValue - actor being followed by camera
	Thread thrd;

	int stage_top = 16;
	Point cam;
	float cam_shake_amount = 0;
	float cursor_x = 63.5;
	float cursor_y = 63.5;
	float last_cursor_x = 0;
	float last_cursor_y = 0;
	int cursor_tmr = 0;
	int cursor_colpos = 1;

	Script* cam_script = 0;

	String cmd_curr;
	Sentence* selected_sentence = 0;
	Dialog dialog_curr;

	PyValue hover_curr_arrow;

	int fade_iris = 0;
	int cutscene_cooloff = 0;
	Script* fade_script = 0;

	Array<TalkingState> talking_curr;
	PyValue talking_actor;

	PyValue ui_arrows;
	PyValue arrow[2];

	PyValue hover_curr_default_verb;
	PyValue hover_curr_verb;
	Sentence* hover_curr_sentence = 0;

	PyValue verb_curr;
	PyValue noun1_curr;
	PyValue noun2_curr;
	PyValue hover_curr_object;
	bool executing_cmd = false;
	bool is_mouse_clicked = false;

	float cam_shake_x = 0;
	float cam_shake_y = 0;

	bool pressed[BTN_COUNT];
	dword mouse_pressed = 0;

	// Script
	PyVM vm;  // PyVM for Python script execution
	PyVM py_vm;  // Alias for vm (for consistency)
	EscAnimContext ctx;  // EscAnimContext for animation system (keep for now)
	
	// Python game state (converted from EscValue)
	PyValue rooms;
	PyValue cutscene_override;
	PyValue verbs;
	PyValue V_DEFAULT, V_USE, V_GIVE, V_PUSH, V_PULL, V_WALKTO, V_PICKUP, V_LOOKAT, V_OPEN, V_CLOSE, V_TALKTO;
	PyValue room_curr;
	
	// Note: SObj (EscValue) still used for game objects/actors - to be converted later
	Script* scr_obj = 0;
	int V_COUNT = 0;
	Index<String> verb_idx;
	
	int fade_iter = 0;
	
	Vector<ZPlane> draw_zplanes; // table of tables for (each of the (8) zplanes for (drawing depth
	Vector<Point> path;
	
	
	Vector<uint16> map, gff;
	Size map_sz, gff_sz;
	
	
	
	
public:

	Program();

	bool InitPyVM(const String& script_path = String());  // Initialize PyVM and register bindings
	// AddEscFunctions() removed - Esc scripts no longer supported
	bool ReadGame();
	EscValue RunLambda1(EscValue* self, const EscValue& l, const EscValue& arg0);
	void ProcessEsc();
	void ResetPalette();
	void ResetUI();
	void Shake(bool enabled);
	PyValue FindDefaultVerb(SObj obj);
	void UnsupportedAction(PyValue verb, PyValue obj1, PyValue obj2);
	void CameraAt(const Point& val);

	// PyVM helper functions
	static PyValue GetDictItem(const PyValue& dict, const char* key);
	PyVM& GetPyVM() { return py_vm; }  // Access to PyVM for bindings
	
	// PyValue accessors
	static PyValue GetProp(const PyValue& obj, const char* key);
	static void SetProp(PyValue& obj, const char* key, const PyValue& val);
	static int PyInt(const PyValue& v, int def = 0);
	static WString PyStr(const PyValue& v);
	static PyValue ClassesPy(const PyValue& s);
	static bool HasFlag(const PyValue& obj, String key);
	PyValue GetSelectedActorPy();

	// Esc* handlers removed - use Python instead

	void ClearCutsceneOverride(EscAnimProgram& s);
	void CameraFollow(SObj actor);
	void CameraFollowPy(PyValue actor);
	void ChangeRoom(SObj new_room, SObj fade);  // Now uses PyValue internally for room_curr
	void ChangeRoomPy(PyValue new_room, PyValue fade);  // PyVM version
	bool CamScript0();
	bool CamScript1();
	bool VerbScript(EscValue vc2);
	bool WalkScript();
	void CameraPanTo(SObj& val);
	bool ScriptRunning(Script& script);
	void Cutscene(SceneType type, EscValue* self, EscValue func_cutscene, EscValue func_override);
	void CutscenePy(SceneType type, const PyValue& func_cutscene, const PyValue& func_override);
	void DialogAdd(const String& msg);
	void DialogStart(int col, int hlcol);
	void DialogHide();
	void DialogClear();
	void DialogEnd();
	void DialogSet(StrVec& msg_table);
	Point GetUsePoint(SObj& obj);
	Point GetUsePointPy(PyValue obj);
	void DoAnim(SObj thing, const String& param1, int& param2);
	void DoAnimPy(PyValue thing, const String& param1, int param2);
	void OpenDoor(SObj door_obj1, SObj door_obj2);
	void OpenDoorPy(PyValue door_obj1, PyValue door_obj2);
	void CloseDoor(SObj door_obj1, SObj door_obj2);
	void CloseDoorPy(PyValue door_obj1, PyValue door_obj2);
	void ComeOutDoor(SObj from_door, SObj to_door, bool fade_effect);
	void ComeOutDoorPy(PyValue from_door, PyValue to_door, bool fade_effect);
	bool Fades(int fade, int dir);
	bool IsValidVerb(PyValue verb, PyValue object);
	void PickupObj(SObj& obj, SObj& actor);
	EscAnimProgram& StartScript(Gate0 func, bool bg, EscValue noun1=EscValue(), EscValue noun2=EscValue());
	Script& StartScriptEsc(EscValue* self, EscValue func, bool bg, EscValue noun1=EscValue(), EscValue noun2=EscValue());
	Script& StartScriptPyVM(const PyValue& func, const Vector<PyValue>& args, bool bg, int calls = 1);
	void StopScript(Script& func);
	void RemoveStoppedScripts();
	void BreakTime(int jiffies=0);
	//void WaitForMessage();
	void SayLineActor(SObj actor, String msg, bool use_caps=false, float duration=1.0f);
	void SayLine(String msg);
	void StopTalking();
	void PrintLine(String msg, int x, int y, int col, int align, bool use_caps, float duration, bool big_font);
	void PutAt(SObj obj, int x, int y, SObj room);
	void StopActor(SObj& actor);
	void WalkTo(SObj a, int x, int y);
	void WalkToPy(PyValue a, int x, int y);
	void WaitForActor(SObj& actor);
	double Proximity(SObj& obj1, SObj& obj2);
	PyValue GetVerb(int idx);
	String GetVerbString(int idx);
	String GetVerbString(SObj v);
	void ClearCurrCmd();
	void Update();
	void UpdateMouseClickState();
	void PlayerControl();
	void InputButtonPressed(dword button_index);
	void CheckCollisions();
	void ResetZPlanes();
	void RecalcZPlane(SObj& obj);
	void RecalcZPlanePy(const PyValue& obj);
	bool InitGame();
	bool IsTable(SObj& t);
	Point CenterCamera(Point val);
	Point CenterCamera(SObj& val);
	Point CenterCameraPy(PyValue val);
	Point GetCellPos(SObj& obj);
	bool IsCellWalkable(int celx, int cely);
	void CreateTextLines(String msg, int max_line_length, Vector<String>& lines);
	int GetLongestLineSize(const Vector<String>& lines);
	bool HasFlag(const SObj& obj, String key);
	void RecalculateBounds(SObj obj, int w, int h, int cam_off_x, int cam_off_y);
	void RecalculateBoundsPy(PyValue obj, int w, int h, int cam_off_x, int cam_off_y);
	void ShowError(String msg);
	void ExplodeData(SObj& obj);
	bool IsCursorColliding(const SObj& obj);
	bool IsCursorColliding(const Sentence& obj);
	bool IsCursorCollidingPy(const PyValue& obj);
	String SmallCaps(const String& s);
	String Autotype(const String& str_value);
	void FindPath(Point start, Point goal, Vector<Point>& pt);
	double GetHeuristic(Point chk, Point goal);
	void AddTextObject(EscEscape& e, String txt, int x, int y, int col, int align, bool use_caps, float duration, bool big_font);
	
	bool Init(const String& script_path = String());
	
	const SObj* FindRoom(const String& name) const;
	const SObj* FindDeep(const String& name) const;
	const SObj* FindDeep(const String& name, const SObj* o) const;
	EscValue Classes(SObj s);
	String State(SObj& s);
	//String GetInRoomString(SObj& o);
	SObj GetInRoom(SObj o);
	PyValue GetInRoomPy(PyValue o);
	Point GetXY(SObj o);
	Point GetXYPy(PyValue o);
	Point GetOffset(SObj o);
	Size GetSize(SObj o);
	UsePos GetUsePos(SObj o);
	static FaceDir GetFaceDir(SObj o);
	static FaceDir GetFaceDirPy(PyValue o);
	StateType GetState(SObj o);
	String GetFaceString(FaceDir d);
	PyValue GetSelectedActor();
	dword GetMouseButtonMask() const {return mouse_pressed;}
	bool IsPressed(GamepadButton b) const;
	bool IsMousePressed(MouseButtonMask m) const;
	bool IsAnyMousePressed() const {return mouse_pressed;}
	bool IsMouseLeftPressed() const {return IsMousePressed(MBMASK_LEFT);}
	bool IsMouseRightPressed() const {return IsMousePressed(MBMASK_RIGHT);}
	
	void SetState(SObj o, StateType s);
	void SetSelectedActor(SObj o);
	
	//static void LoadBuiltinGfxStr(const char* s, Image& out);
	static void LoadBuiltinGfxStr(const char* s, Vector<byte>& out, Size& sz);
	static void LoadBuiltinGfxStr(const char* s, Vector<uint16>& out, Size& sz);
	
};


class ProgramDraw : public Ctrl {
	
	Image gfx, lbl;
	Vector<byte> gfx_data, lbl_data;
	Size gfx_sz, lbl_sz;
	Size canvas_sz;
	double size_mul = 1.0;
	
	Font fnt;
	PaletteColor dyn_palette[PALETTE_SIZE];
	Color palette[PALETTE_SIZE];
	Program* p = 0;
	bool show_debuginfo = true;
	
	TimeStop frame_ts;
	
	
	Color GetPaletteColor(int i) const {
		ASSERT(i >= 0 && i < PALETTE_SIZE);
		i = dyn_palette[i];
		ASSERT(i >= 0 && i < PALETTE_SIZE);
		return palette[i];
	}
	
	void LoadBuiltinGfx();
	void Animate(SObj obj);
	void AnimatePy(PyValue obj);
	void GetPaletteImage(const byte* src, Size src_sz, Image& out);
	

protected:
	friend class Program;
	ImageDraw* img_draw = 0;


public:
	
	ProgramDraw();

	void SetProgram(Program& p) {this->p = &p; p.draw = this;}
	
	void Paint(Draw& d) override;
	
	void PaintLog(Draw& d, String s, int x, int y, PaletteColor clr);
	
	
	void PaintRoom(Draw& d);
	void PaintTalking(Draw& d, double dt);
	void OutlineText(Draw& d, String str, int x, int y, int c0, int c1, bool use_caps, bool big_font);
	void PaintObject(Draw& d, SObj obj);
	void PaintObjectPy(Draw& d, PyValue obj);
	void PaintActor(Draw& d, SObj actor);
	void PaintCommand(Draw& d);
	void PaintUI(Draw& d);
	void PaintDialog(Draw& d);
	void PaintCursor(Draw& d);
	void PaintSprite(Draw& d, const Image& src, PaletteImage n, int x, int y, bool flip_x, bool flip_y, int scale);
	void PaintSprite(Draw& d, const Image& src, PaletteImage n, int x, int y, int w, int h, bool transcol, bool flip_x, bool flip_y, int scale);
	void PaintMap(Draw& d, int x, int y, int dst_x, int dst_y, int w, int h);
	
	void SetPalette(int idx, PaletteColor clr);
	void ResetPalette();
	void ReplaceColors(SObj o);
	void ReplaceColorsPy(PyValue o);
	void FadePalette(float perc);
	
	
	void MouseMove(Point p, dword keyflags) override;
	void LeftDown(Point p, dword keyflags) override;
	void LeftUp(Point p, dword keyflags) override;
	void RightDouble(Point p, dword keyflags) override;
	void RightUp(Point p, dword keyflags) override;
	bool Key(dword key, int count) override;
	
	// TODO remove
	void SetTransCol(int transcol, bool enabled);
	
};


class ProgramApp : public TopWindow {
	ProgramDraw draw;
	Program prog;
	TimeCallback tc;

public:
	typedef ProgramApp CLASSNAME;

	ProgramApp();

	void ProcessScript();
	void MarkInitialized();  // Call after Init() to mark initialization complete

	bool Init(const String& script_path = String()) {return prog.Init(script_path);}

};

}

#endif
