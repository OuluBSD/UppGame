#include "Umbrella.h"
#include "CommandLineArguments.h"
#include "LevelManifest.h"

using namespace Upp;

UmbrellaArgs::UmbrellaArgs() {
	editorMode = false;
	testMode = false;
	newGameMode = false;
	worldIndex = -1;
	levelIndex = -1;
	levelPath = "";
	testScript = "";
}

void UmbrellaArgs::Parse(const Vector<String>& args) {
	for(int i = 0; i < args.GetCount(); i++) {
		const String& arg = args[i];

		if(arg == "--editor" || arg == "--editor-parastar") {
			editorMode = true;
		}
		else if(arg == "--test") {
			testMode = true;
			if(i + 1 < args.GetCount() && !args[i + 1].StartsWith("--")) {
				testScript = args[i + 1];
				i++;  // Skip next arg
			}
		}
		else if(arg == "--newgame" || arg == "-n") {
			newGameMode = true;
		}
		else if(arg == "--world" && i + 1 < args.GetCount()) {
			worldIndex = StrInt(args[i + 1]);
			i++;  // Skip next arg
		}
		else if(arg == "--level" && i + 1 < args.GetCount()) {
			levelIndex = StrInt(args[i + 1]);
			i++;  // Skip next arg
		}
		// If argument doesn't start with --, treat it as a file path
		else if(!arg.StartsWith("--") && !arg.StartsWith("-")) {
			levelPath = arg;
		}
	}
}

String UmbrellaArgs::GetLevelPath() const {
	// If explicit path provided, use it
	if(!levelPath.IsEmpty()) {
		return levelPath;
	}

	// If world and level indices provided, construct path
	if(worldIndex >= 0 && levelIndex >= 0) {
		return Format("share/mods/umbrella/levels/world%d-stage%d.json", worldIndex, levelIndex);
	}

	// Default to first level from manifest
	LevelManifest& m = GetLevelManifest();
	if(m.IsLoaded())
		return m.GetFirstLevel();
	return "share/mods/umbrella/levels/world1-stage1.json";
}
