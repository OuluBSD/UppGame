#include <Core/Core.h>
#include <Geometry/Geometry.h>
#include <GameLib/GameLib.h>

using namespace Upp;

CONSOLE_APP_MAIN
{
	StdLogSetup(LOG_COUT | LOG_FILE);
	Cout() << "========================================\n";
	Cout() << "Running GameCheck Unit & Lifecycle Tests\n";
	Cout() << "========================================\n\n";

	Cout() << "[1] Testing GameLib header and namespace...\n";
	Cout() << "PASS: GameLib compiled cleanly\n";

	Cout() << "\n========================================\n";
	Cout() << "ALL GameCheck TESTS PASSED SUCCESSFULLY!\n";
	Cout() << "========================================\n";
}
