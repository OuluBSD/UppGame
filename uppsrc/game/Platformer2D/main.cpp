#include <Core/Core.h>
#include <CtrlLib/CtrlLib.h>

// Forward declaration
namespace Upp {
	bool Platformer2D_PlatformStartup(int argc, char* argv[]);
}

GUI_APP_MAIN
{
	using namespace Upp;
	StdLogSetup(LOG_COUT|LOG_FILE);
	
	LOG("=== Platformer2D Starting ===");
	
	const Vector<String>& args = CommandLine();
	int argc = args.GetCount();
	Vector<char*> argv;
	argv.Reserve(argc + 1);
	for (int i = 0; i < argc; i++)
		argv.Add((char*)args[i].Begin());
	argv.Add(nullptr);
	
	if (!Platformer2D_PlatformStartup(argc, argv.Begin()))
	{
		LOG("Platformer2D_PlatformStartup failed");
		return;
	}
	
	LOG("=== Platformer2D Exiting ===");
}