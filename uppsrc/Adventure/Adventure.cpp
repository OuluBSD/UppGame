#include "Adventure.h"

GUI_APP_MAIN {
	//MemoryBreakpoint(37975);

	using namespace Adventure;

	// Check for command-line argument (Python script path)
	String script_path;
	LOG("Adventure: CommandLine().GetCount()=" << CommandLine().GetCount());
	for(int i = 0; i < CommandLine().GetCount(); i++) {
		LOG("Adventure: CommandLine()[" << i << "]=" << CommandLine()[i]);
	}
	// If only one argument and it ends with .py, use it as script path
	if (CommandLine().GetCount() == 1 && CommandLine()[0].Find(".py") >= 0) {
		script_path = CommandLine()[0];
		LOG("Adventure: Using script from command line: " << script_path);
	} else if (CommandLine().GetCount() > 1) {
		script_path = CommandLine()[1];  // First argument after executable
		LOG("Adventure: Using script from command line: " << script_path);
	}

	ProgramApp a;
	if (a.Init(script_path)) {
		a.MarkInitialized();  // Mark initialization complete
		a.Show();  // Show window after successful initialization
		a.Run();
	}

}
