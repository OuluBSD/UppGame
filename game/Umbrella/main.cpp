#include "Umbrella.h"
#include "CommandLineArguments.h"
#include "MapEditor.h"
#include "MainMenuScreen.h"
#include "GameScreen.h"
#include "GameScriptBridge.h"
#include "LevelManifest.h"

#include <CtrlLib/CtrlLib.h>
#include <Draw/Draw.h>
#include <Core/Core.h>
#include <Sound/Sound.h>
#include <ByteVM/ByteVM.h>
#include <Ctrl/Automation/Automation.h>

using namespace Upp;

// Forward declaration
bool RunGameScreenTests();

// Automation test runner using GameScriptBridge
int RunAutomationTest(const String& scriptPath) {
	LOG("Running automation test: " << scriptPath);

	// Create game screen for headless testing
	// Note: GameScreen is a TopWindow, but we won't call Run()
	GameScreen* screen = new GameScreen("share/mods/umbrella/levels/world1-stage1.json");

	// Create script bridge
	GameScriptBridge bridge;
	bridge.SetGameScreen(screen);
	bridge.RegisterGameAPI();

	// Execute script
	bool success = bridge.ExecuteScript(scriptPath);

	delete screen;
	return success ? 0 : 1;
}

GUI_APP_MAIN
{
    StdLogSetup(LOG_COUT|LOG_FILE);

    // Load level manifest (singleton — available to all subsystems)
    GetLevelManifest().Load("share/mods/umbrella");

    // Parse command line arguments
    UmbrellaArgs cmdArgs;
    cmdArgs.Parse(CommandLine());

    if(cmdArgs.testMode) {
        if(cmdArgs.testScript.IsEmpty()) {
            // Fall back to C++ tests if no script provided
            bool testsPassed = RunGameScreenTests();
            SetExitCode(testsPassed ? 0 : 1);
        } else {
            // Run Python automation test
            int exitCode = RunAutomationTest(cmdArgs.testScript);
            SetExitCode(exitCode);
        }
        return;
    }

    if(cmdArgs.editorMode) {
        if(!cmdArgs.levelPath.IsEmpty()) {
            MapEditorApp(cmdArgs.levelPath).Run();
        } else {
            MapEditorApp().Run();
        }
    } else if(cmdArgs.newGameMode) {
        // Start specified or first level directly
        String levelToLoad = cmdArgs.GetLevelPath();
        if(FileExists(levelToLoad)) {
            LOG("Starting level: " << levelToLoad);
            GameScreen(levelToLoad).Run();
        } else {
            Exclamation("Level not found: " + levelToLoad);
        }
    } else {
        MainMenuScreen().Run();
    }
}