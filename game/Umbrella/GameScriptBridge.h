#ifndef _Umbrella_GameScriptBridge_h_
#define _Umbrella_GameScriptBridge_h_

#include <ByteVM/ByteVM.h>
#include "GameScreen.h"

using namespace Upp;

class GameScriptBridge {
private:
	PyVM vm;
	GameScreen* gameScreen;  // Reference to active game

public:
	GameScriptBridge();

	void SetGameScreen(GameScreen* screen) { gameScreen = screen; }
	GameScreen* GetGameScreen() { return gameScreen; }

	PyVM& GetVM() { return vm; }

	// Register all game API functions
	void RegisterGameAPI();

	// Execute script file
	bool ExecuteScript(const String& scriptPath);
};

#endif
