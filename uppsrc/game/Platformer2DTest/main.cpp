#include <Core/Core.h>
#include <CtrlLib/CtrlLib.h>

// Forward declaration
namespace Upp {
	int RunPlatformer2DTests();
}

GUI_APP_MAIN
{
	using namespace Upp;
	StdLogSetup(LOG_COUT|LOG_FILE);

	LOG("=== Platformer2D Test Suite ===");

	int result = RunPlatformer2DTests();

	LOG("=== Tests " << (result == 0 ? "PASSED" : "FAILED") << " ===");
	SetExitCode(result == 0 ? 0 : 1);
}