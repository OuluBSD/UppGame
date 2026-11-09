#ifndef _Umbrella_CommandLineArguments_h_
#define _Umbrella_CommandLineArguments_h_

#include <Core/Core.h>

using namespace Upp;

class UmbrellaArgs {
public:
	bool editorMode;
	bool testMode;
	bool newGameMode;
	int worldIndex;
	int levelIndex;
	String levelPath;
	String testScript;  // Path to Python test script for --test mode

	UmbrellaArgs();
	void Parse(const Vector<String>& args);
	String GetLevelPath() const;  // Compute path from world/level or use explicit path
};

#endif
