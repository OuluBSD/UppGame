#include "Adventure.h"
#include "AdventureBindings.h"

namespace Adventure {

// ============================================================================
// Script class implementation - PyVM support
// ============================================================================

Program::Script::Script() {
	is_python = false;
	calls_remaining = 1;
	py_func = PyValue();
	py_args.Clear();
	py_vm = nullptr;
}

void Program::Script::Clear() {
	// Call base class clear
	EscAnimProgram::Clear();
	
	// Clear PyVM members
	py_func = PyValue();
	py_args.Clear();
	calls_remaining = 1;
	is_python = false;
	py_vm = nullptr;
}

Program::Script& Program::Script::SetPyVM(PyVM& vm, const PyValue& func, const Vector<PyValue>& args, int calls) {
	Clear();
	
	// Store the Python function
	py_func = func;
	// Copy args manually since Vector doesn't have copy assignment
	py_args.Clear();
	for(int i = 0; i < args.GetCount(); i++)
		py_args.Add(args[i]);
	calls_remaining = calls;
	is_python = true;
	py_vm = &vm;
	
	// Mark as running (use native flag since we're managing execution ourselves)
	is_native = true;
	is_native_running = true;
	
	return *this;
}

void Program::Script::Iterate() {
	if (is_python) {
		// Handle PyVM execution
		ProcessPyVM();
	}
	else {
		// Fall back to base class behavior (ESC or native)
		EscAnimProgram::Iterate();
	}
}

bool Program::Script::ProcessPyVM() {
	LOG("Script::ProcessPyVM: is_python=" << is_python << ", calls_remaining=" << calls_remaining);
	
	if (!is_python) {
		return false;  // Not a Python script
	}
	
	if (!py_vm) {
		LOG("Script::ProcessPyVM: py_vm is null");
		is_native_running = false;
		return false;
	}
	
	if (!py_func.IsFunction()) {
		LOG("Script::ProcessPyVM: py_func is not a function");
		is_native_running = false;
		return false;
	}
	
	if (calls_remaining == 0) {
		LOG("Script::ProcessPyVM: no more calls remaining");
		is_native_running = false;
		return false;
	}
	
	// Call the Python function
	try {
		py_vm->Call(py_func, py_args);
		
		// Decrement calls remaining (unless infinite)
		if (calls_remaining > 0) {
			calls_remaining--;
		}
		
		// Check if we should stop
		if (calls_remaining == 0) {
			is_native_running = false;
			LOG("Script::ProcessPyVM: script completed");
			if (WhenStop) {
				WhenStop(*this);
			}
			return false;
		}
		
		return true;  // Still running
	}
	catch (...) {
		LOG("Script::ProcessPyVM: exception during Python function call");
		is_native_running = false;
		if (WhenStop) {
			WhenStop(*this);
		}
		return false;
	}
}

/*

void Script::Clear() {
	Stop();
	fn.Clear();
	a0 = EscValue();
	a1 = EscValue();
	flags = 0;
	paused_cam_following = 0;
	type = SCENE_NULL;
	is_esc = false;
	
}

Script& Script::Set(Gate0 cb, EscValue a0, EscValue a1) {
	Clear();
	this->a0 = a0;
	this->a1 = a1;
	is_esc = false;
	fn = cb;
	global = 0;
	running = true;
	return *this;
}

Script& Script::Set(EscGlobal& g, EscValue *self, EscValue fn, EscValue a0, EscValue a1) {
	Clear();
	is_esc = false;

	// Find & check lambda before setting fields
	EscValue lambda;
	String fn_name;
	if (fn.IsLambda()) {
		lambda = fn;
		fn_name = "<lambda>";
	}
	else {
		fn_name = fn.ToString();
		fn_name.Replace("\"", "");
		if (self && self->IsMap())
			lambda = self->MapGet(fn_name);
		if (!lambda.IsLambda()) {
			lambda = g.Get(fn_name, EscValue());
			if (!lambda.IsLambda()) {
				LOG("Key '" << fn_name << "' is not lambda");
				return *this;
			}
		}
	}

	Vector<EscValue> arg;
	if (!a0.IsVoid()) arg << a0;
	if (!a0.IsVoid() && !a1.IsVoid()) arg << a1;

	const HiLambda& l = lambda.GetLambda();
	if (arg.GetCount() != l.arg.GetCount()) {
		String argnames;
		for(int i = 0; i < l.arg.GetCount(); i++)
			argnames << (i ? ", " : "") << l.arg[i];
		LOG(Format("invalid number of arguments (%d passed, expected: %s)", arg.GetCount(), argnames));
		return *this;
	}

	// Set fields
	this->a0 = a0;
	this->a1 = a1;
	is_esc = true;
	global = &g;
	this->fn_name = fn_name;

	// Initialize esc runner
	op_limit = 1000000;
	esc = new Esc(g, l.code, op_limit, l.filename, l.line);
	auto& e = *esc;
	if (self)
		e.Self() = *self;
	for(int i = 0; i < l.arg.GetCount(); i++)
		e.Var().GetPut(l.arg[i]) = arg[i];

	//e.no_return = e.no_break = e.no_continue = true;
	//e.loop = 0;
	//e.skipexp = 0;

	running = true;
	LOG("Script::Set: started Esc lambda " << fn_name);

	return *this;
}

Script& Script::Start() {
	tc.KillSet(10, THISBACK(Execute));
	return *this;
}

Script& Script::Stop() {
	tc.Kill();
	running = false;
	return *this;
}

void Script::Execute() {
	if (!is_esc) {
		if (!fn || !fn()) {
			tc.Kill();
			running = false;
			LOG("Script::Set: stopped " << fn_name);
			WhenStop(this);
		}
	}
	else {
		tc.Kill();
		ASSERT_(0, "Do not execute esc outside main thread");
	}
}

bool Script::ProcessEsc() {
	LOG("Script::ProcessEsc");
	if (!esc || !RunEscSteps()) {
		tc.Kill();
		running = false;
		LOG("Script::ProcessEsc: stopped " << fn_name);
		WhenStop(this);
	}
	return running;
}

bool Script::RunEscSteps() {
	LOG("Script::RunEscSteps");
	auto& e = *esc;
	int op = 0;
//	try {
//		while(!e.IsEof() && e.no_return && e.no_break && e.no_continue && op < op_limit_at_once) {
//			e.DoStatement();
//			op++;
//		}
//	}
//	catch (CParser::Error e) {
//		LOG("Script::RunEscSteps: error: " << e);
//		return false;
//	}
//	
//	return !e.IsEof();

	
	e.Run();
	
	return false;
}
*/



void Program::Cutscene(SceneType type, EscValue* self, EscValue func_cutscene, EscValue func_override) {

	/*cut = {
		flags = type,
		thrd = cocreate(func_cutscene),
		override_ = func_override,
		paused_cam_following = EscToPyValue(cam_following_actor)
	};*/

	if (cutscene_override.IsNone()) {
		cutscene_override = EscToPyValue(func_override);

		Script& cut = AddCutscene("cutscene0");
		cut.user_type = type;
		cut.WhenStop = THISBACK(ClearCutsceneOverride);
		cut.Set(0, func_cutscene, PyToEscValue(room_curr));

		// set as active cutscene
		cutscene_curr = &cut;
	}
	else {
		StartScriptEsc(self, PyToEscValue(cutscene_override), 0);
	}
}

// PyVM version: calls Python functions for cutscene
void Program::CutscenePy(SceneType type, const PyValue& func_cutscene, const PyValue& func_override) {
	if(!func_cutscene.IsFunction()) {
		LOG("CutscenePy: func_cutscene is not a function");
		return;
	}

	// Call Python cutscene function with current room
	Vector<PyValue> args;
	args.Add(room_curr);
	py_vm.Call(func_cutscene, args);

	// Clear cutscene after completion
	cutscene_curr = nullptr;
}

void Program::ClearCutsceneOverride(EscAnimProgram& s) {
	cutscene_override = PyValue();

	// Cast to Script to compare
	Script* script_ptr = dynamic_cast<Script*>(&s);
	if (script_ptr && script_ptr == cutscene_curr)
		cutscene_curr = 0;
}


Program::Script& Program::AddScript(String name, int group) {
	return ctx.CreateProgramT<Script>(name, group);
}

Program::Script& Program::AddLocal(String name) {
	return AddScript(name, SCRIPT_LOCAL);
}

Program::Script& Program::AddGlobal(String name) {
	return AddScript(name, SCRIPT_GLOBAL);
}

Program::Script& Program::AddCutscene(String name) {
	return AddScript(name, SCRIPT_CUTSCENE);
}

EscAnimProgram& Program::StartScript(Gate0 func, bool bg, EscValue noun1, EscValue noun2) {
	RemoveStoppedScripts();
	
	// background || local?
	if (bg)
		return AddGlobal("script").Set(func, noun1, noun2);
	
	else
		return AddLocal("script").Set(func, noun1, noun2);
}

Program::Script& Program::StartScriptEsc(EscValue* self, EscValue script_name, bool bg, EscValue noun1, EscValue noun2) {

	//LOG("Program::StartScriptEsc: " << script_name);
	RemoveStoppedScripts();

	// Check if it's a Python function (stored as map with __pyfunc__ key)
	if (script_name.IsMap() && script_name.MapGet("__pyfunc__").GetInt() != 0) {
		// It's a Python function - extract wrapper and use PyVM execution
		int64 wrapper_ptr = script_name.MapGet("__pyfunc__").GetInt();
		PyFunctionWrapper* wrapper = (PyFunctionWrapper*)(uintptr_t)wrapper_ptr;
		
		Vector<PyValue> args;
		if (noun1.GetType() != PY_NONE)
			args.Add(EscToPyValue(noun1));
		if (noun2.GetType() != PY_NONE)
			args.Add(EscToPyValue(noun2));
		return StartScriptPyVM(wrapper->func, args, bg, 1);
	}

	// background || local?
	if (bg) {
		Script& s = AddGlobal("hi-script");
		s.Set(self, script_name, noun1, noun2);
		return s;
	}
	else {
		Script& s = AddLocal("hi-script");
		s.Set(self, script_name, noun1, noun2);
		return s;
	}
}

Program::Script& Program::StartScriptPyVM(const PyValue& func, const Vector<PyValue>& args, bool bg, int calls) {
	RemoveStoppedScripts();
	
	// background || local?
	if (bg) {
		Script& s = AddGlobal("py-script");
		s.SetPyVM(vm, func, args, calls);
		return s;
	}
	else {
		Script& s = AddLocal("py-script");
		s.SetPyVM(vm, func, args, calls);
		return s;
	}
}

bool Program::ScriptRunning(Script& func)  {
	// loop through both sets of scripts...
	ASSERT(ctx.HasProgram(func));
	bool b = func.IsRunning();
	return b;
}

void Program::StopScript(Script& func) {
	ctx.StopProgram(func);
}

void Program::RemoveStoppedScripts() {
	ctx.RemoveStopped();
}

// Esc script functions removed - all game logic now uses Python
// Original AddEscFunctions() and Esc* handlers deleted (lines 395-670)

}
