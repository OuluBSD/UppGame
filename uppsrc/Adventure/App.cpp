#include "Adventure.h"

namespace Adventure {


// Flag to track if initialization is complete
static bool s_initialized = false;


ProgramApp::ProgramApp() {
	Title("Program App");
	Sizeable().MaximizeBox().MinimizeBox();

	// Set initial window size directly (GetRect() doesn't work before window is created)
	SetRect(0, 0, 512, 512);

	Add(draw.SizePos());

	tc.Set(-1, THISBACK(ProcessScript));

	draw.SetProgram(prog);
	
	// Don't show window until initialization completes
	Hide();
	s_initialized = false;
}

void ProgramApp::ProcessScript() {
	// Only start processing after initialization is complete
	if (!s_initialized)
		return;
		
	prog.Update();
	draw.Refresh();
}

// Called after Init() succeeds to mark initialization complete
void ProgramApp::MarkInitialized() {
	s_initialized = true;
}


}
