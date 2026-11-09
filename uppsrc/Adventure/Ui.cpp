#include "Adventure.h"
#include "AdventureBindings.h"

namespace Adventure {



void Program::ResetUI() {
	verb_maincol = 12;   // main color (lt blue)
	verb_hovcol = 7;     // hover color (white)
	verb_shadcol = 1;    // shadow (dk blue)
	verb_defcol = 10;    // default action (yellow)
	ui_cursorspr = 224;  // default cursor sprite
	ui_uparrowspr = 208; // default up arrow sprite
	ui_dnarrowspr = 240; // default up arrow sprite
	
	// default cols to use when animating cursor
	ui_cursor_cols[0] = 7;
	ui_cursor_cols[1] = 12;
	ui_cursor_cols[2] = 13;
	ui_cursor_cols[3] = 13;
	ui_cursor_cols[4] = 12;
	ui_cursor_cols[5] = 7;
	
	for(int i = 0; i < BTN_COUNT; i++)
		pressed[i] = false;
	mouse_pressed = 0;
	
	
	ui_arrows = PyValue::List();
	Vector<PyValue>& arr = const_cast<Vector<PyValue>&>(ui_arrows.GetArray());

	PyValue up = PyValue::Dict();
	VectorMap<PyValue, PyValue>& up_dict = up.GetDictRW();
	up_dict.Add(PyValue(WString("spr")), PyValue((int64)ui_uparrowspr));
	up_dict.Add(PyValue(WString("x")), PyValue(75));
	up_dict.Add(PyValue(WString("y")), PyValue(stage_top + 60));
	arr.Add(up);

	PyValue down = PyValue::Dict();
	VectorMap<PyValue, PyValue>& down_dict = down.GetDictRW();
	down_dict.Add(PyValue(WString("spr")), PyValue((int64)ui_dnarrowspr));
	down_dict.Add(PyValue(WString("x")), PyValue(75));
	down_dict.Add(PyValue(WString("y")), PyValue(stage_top + 72));
	arr.Add(down);

}

void Program::UpdateMouseClickState() {
	is_mouse_clicked = mouse_pressed != 0;
}

// handle button inputs
void Program::PlayerControl() {
	
	// check for (skip/override's
	if (talking_curr.GetCount() && !is_mouse_clicked && (IsPressed(BTN_O) || IsMouseLeftPressed())) {
		// skip current talking message
		talking_curr[0].time_left = 0;
		is_mouse_clicked = true;
		return;
	}
	
	// cutscene? (or skip?)
	if (cutscene_curr) {
		if ((IsPressed(BTN_X) || IsMouseRightPressed()) && cutscene_curr->IsRunning()) {
			cutscene_curr->Stop();
			return;
		}
		
		// either way - don't allow other user actions!
		UpdateMouseClickState();
		return;
	}
	
	
	if (IsPressed(BTN_LEFT)) {
		cursor_x -= 1;
	}
	if (IsPressed(BTN_RIGHT)) {
		cursor_x += 1;
	}
	if (IsPressed(BTN_UP)) {
		cursor_y -= 1;
	}
	if (IsPressed(BTN_DOWN)) {
		cursor_y += 1;
	}
	if (IsPressed(BTN_O)) {
		InputButtonPressed(MBMASK_LEFT);
	}
	if (IsPressed(BTN_X)) {
		InputButtonPressed(MBMASK_RIGHT);
	}
	
	// don't repeat action if (same press/click
	if (IsAnyMousePressed() && !is_mouse_clicked) {
		InputButtonPressed(GetMouseButtonMask());
	}
	// store for (comparison next cycle
	last_cursor_x = cursor_x;
	last_cursor_y = cursor_y;
	
	UpdateMouseClickState();
}

// 1 = z/lmb, 2 = x/rmb, (4=middle)
void Program::InputButtonPressed(dword button_index) {
	auto& global = ctx.global;

	PyValue selected_actor = GetSelectedActor();

	// abort if (no actor selected at this point
	if (selected_actor.IsNone())
		return;
		
	// check for (sentence selection
	if (dialog_curr && dialog_curr.visible) {
		if (hover_curr_sentence)
			selected_sentence = hover_curr_sentence;
		// skip remaining
		return;
	}
	
	// if (already executing, clear current command
	// (allow abort of commands by) {ing other actions, like walking)
	if (executing_cmd)
		ClearCurrCmd();

	PyValue noun1_curr, noun2_curr;
	if (hover_curr_verb.GetType() != PY_NONE) {
		// change verb and now reset any active objects
		verb_curr = GetVerb(Program::PyInt(hover_curr_verb));
	}
	else if (hover_curr_object.GetType() != PY_NONE) {
		// if (valid obj, complete command
		// else, abort command (clear verb, etc.)
		if (button_index == 1) {
		}
		// if (already have obj #1
		if (noun1_curr.GetType() != PY_NONE && !executing_cmd) {
			// complete with obj #2
			noun2_curr = hover_curr_object;
		}
		else {
			noun1_curr = hover_curr_object;
		}

		PyValue default_verb = GetVerb(Program::PyInt(verb_default));
		if (verb_curr.GetType() == PY_LIST && Program::PyInt(Program::GetProp(verb_curr, "2")) == Program::PyInt(Program::GetProp(default_verb, "2")) && Program::GetProp(hover_curr_object, "owner").GetType() != PY_NONE) {
			// inventory item, perform look-at
			verb_curr = GetVerb(Program::PyInt(Program::GetProp(verbs, "verb_default")));
		}

		else if (hover_curr_default_verb.GetType() != PY_NONE) {
			// perform default verb action (if (present)
			verb_curr = GetVerb(Program::PyInt(hover_curr_default_verb));

			// force repaint of command (to reflect default verb)
			//PaintCommand();
		}

		// ui arrow clicked
		else if (hover_curr_arrow.GetType() != PY_NONE) {
			int inv_pos = Program::PyInt(Program::GetProp(selected_actor, "inv_pos"));

			// up arrow
			const Vector<PyValue>& arrows_arr = ui_arrows.GetArray();
			if (hover_curr_arrow.GetPtr() == arrows_arr[1].GetPtr()) {
				if (inv_pos > 0)
					inv_pos -= 1;
			}
			else { // down arrow
				PyValue inv = Program::GetProp(selected_actor, "inventory");
				int inv_len = inv.GetArray().GetCount();
				if (inv_pos + 2 < inv_len/4) {
					inv_pos += 1;
				}
			}
			return;
		}
		//else
		// what else could there be? actors!?
	}

	PyValue vc2 = verb_curr;

	// attempt to use verb on object (if is not already executing verb)
	if (noun1_curr.GetType() != PY_NONE) {
		// are we starting a 'use' command?
		if (vc2.GetPtr() == V_USE.GetPtr() || vc2.GetPtr() == V_GIVE.GetPtr()) {
			if (noun2_curr.GetType() != PY_NONE) {
				// 'use' part 2
			}
			else if (Program::GetProp(noun1_curr, "use_with").GetType() != PY_NONE && Program::GetProp(noun1_curr, "owner").GetPtr() == selected_actor.GetPtr()) {
				// 'use' part 1 (e.g. "use hammer")
				// wait for (noun2 to be set
				return;
			}
		}

		// execute verb script
		executing_cmd = true;
		StartScript(THISBACK1(VerbScript, PyToEscValue(vc2)), 0);
	}
	else if (cursor_y > stage_top && cursor_y < stage_top + 64) {
		// in map area
		executing_cmd = true;
		
		// attempt to walk to target
		StartScript(THISBACK(WalkScript), 0);
	}
	
}

EscValue Program::RunLambda1(EscValue* self, const EscValue& l, const EscValue& arg0) {
	auto& global = ctx.global;
	
	ASSERT(l.IsLambda());
	
	try {
		Vector<EscValue> args;
		args.Add(arg0);
		return Execute(global, self, l, args, 10000);
	}
	catch(CParser::Error e) {
		LOG("Program::RunLambda1: error: " << e << "\n");
		ASSERT(0);
	}
	
	return EscValue();
}

void Upt(int max_length, WString& curword, WString& currline, Vector<String>& lines) {
	if (curword.GetCount() + currline.GetCount() > max_length) {
		lines.Add(currline.ToString());
		currline.Clear();
	}
	currline += curword;
	curword.Clear();
}

// auto-break message into lines
void Program::CreateTextLines(String msg_, int max_line_length, Vector<String>& lines) { //, comma_is_newline)
	WString msg = msg_.ToWString();
	
	//  > ";" new line, shown immediately
	lines.SetCount(0);
	WString currline, curword, curchar;
	
	for(int i = 0; i < msg.GetCount(); i++) {
		int curchar = msg[i];
		curword.Cat(curchar);

		if (curchar == ' ' || curword.GetCount() > max_line_length-1) {
			Upt(max_line_length, curword, currline, lines);
		}
		else if (curword.GetCount() > max_line_length-1) {
			curword.Cat('-');
			Upt(max_line_length, curword, currline, lines);
		}
		else if (curchar == ';') {
			// line break
			currline += curword.Mid(1, curword.GetCount()-1);
			curword.Clear();
			Upt(0, curword, currline, lines);
		}
	}
	
	Upt(max_line_length, curword, currline, lines);
	if (currline.GetCount())
		lines.Add(currline.ToString());
	
}

// find longest line
int Program::GetLongestLineSize(const Vector<String>& lines) {
	int longest_line = 0;
	for (const String& l : lines)
		longest_line = max(longest_line, l.GetCount());
	return longest_line;
}


bool Program::IsPressed(GamepadButton b) const {
	ASSERT(b >= (GamepadButton)0 && b < BTN_COUNT);
	
	if (b >= (GamepadButton)0 && b < BTN_COUNT)
		return pressed[b];
	
	return false;
}

bool Program::IsMousePressed(MouseButtonMask m) const {
	return mouse_pressed & m;
}


}
