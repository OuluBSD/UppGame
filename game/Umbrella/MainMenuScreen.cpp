#include "Umbrella.h"
#include "MainMenuScreen.h"
#include "MapEditor.h"
#include "GameScreen.h"
#include "LevelManifest.h"

using namespace Upp;

MainMenuScreen::MainMenuScreen() {
	Title("Umbrella - Main Menu");
	Sizeable().Zoomable();
	SetRect(0, 0, 800, 600);
	inLevelSelect = false;

	// Title
	titleLabel.SetLabel("UMBRELLA GAME");
	titleLabel.SetFont(Arial(32).Bold());
	titleLabel.SetAlign(ALIGN_CENTER);
	Add(titleLabel.HSizePos(100, 100).TopPos(100, 50));

	// Play Button (launches first level)
	playButton.SetLabel("Play Game");
	playButton << [=] { ShowGameScreen(); };
	Add(playButton.HSizePos(250, 250).TopPos(220, 40));

	// Level Select Button
	levelSelectButton.SetLabel("Level Select");
	levelSelectButton << [=] { ShowLevelSelect(); };
	Add(levelSelectButton.HSizePos(250, 250).TopPos(270, 40));

	// Editor Button
	editorButton.SetLabel("Map Editor");
	editorButton << [=] { ShowEditorScreen(); };
	Add(editorButton.HSizePos(250, 250).TopPos(320, 40));

	// Quit Button
	quitButton.SetLabel("Quit");
	quitButton << [=] { Break(); };
	Add(quitButton.HSizePos(250, 250).TopPos(370, 40));

	// Level select title (hidden initially)
	levelSelectTitle.SetLabel("SELECT LEVEL");
	levelSelectTitle.SetFont(Arial(28).Bold());
	levelSelectTitle.SetAlign(ALIGN_CENTER);

	// Back button for level select
	backButton.SetLabel("< Back");
	backButton << [=] { ShowMainMenu(); };
}

void MainMenuScreen::ShowMainMenu() {
	inLevelSelect = false;

	// Remove level select controls
	RemoveChild(&levelSelectTitle);
	RemoveChild(&backButton);
	for(int i = 0; i < levelButtons.GetCount(); i++)
		RemoveChild(&levelButtons[i]);
	levelButtons.Clear();

	// Show main menu controls
	Add(titleLabel.HSizePos(100, 100).TopPos(100, 50));
	Add(playButton.HSizePos(250, 250).TopPos(220, 40));
	Add(levelSelectButton.HSizePos(250, 250).TopPos(270, 40));
	Add(editorButton.HSizePos(250, 250).TopPos(320, 40));
	Add(quitButton.HSizePos(250, 250).TopPos(370, 40));
}

void MainMenuScreen::ShowLevelSelect() {
	inLevelSelect = true;

	// Remove main menu controls
	RemoveChild(&titleLabel);
	RemoveChild(&playButton);
	RemoveChild(&levelSelectButton);
	RemoveChild(&editorButton);
	RemoveChild(&quitButton);

	// Load manifest
	LevelManifest& manifest = GetLevelManifest();
	if(!manifest.IsLoaded()) {
		manifest.Load("share/mods/umbrella");
	}

	// Show level select header
	Add(levelSelectTitle.HSizePos(100, 100).TopPos(20, 40));
	Add(backButton.LeftPos(20, 80).TopPos(20, 30));

	// Parse manifest JSON directly to get world structure
	String manifestText = LoadFile("share/mods/umbrella/levels/manifest.json");
	Value json = ParseJSON(manifestText);
	Value worlds = json["worlds"];

	levelButtons.Clear();

	int startY = 80;
	int btnW = 90, btnH = 32;
	int gapX = 8, gapY = 8;
	int labelX = 20;

	for(int w = 0; w < worlds.GetCount(); w++) {
		int worldId = worlds[w]["id"];
		Value lvls = worlds[w]["levels"];
		int rowY = startY + w * (btnH + gapY + 28);

		// World label
		Button& worldLabel = levelButtons.Add();
		worldLabel.SetLabel(Format("World %d", worldId));
		worldLabel.Disable();
		Add(worldLabel.LeftPos(labelX, 80).TopPos(rowY, btnH));

		// Level buttons
		for(int l = 0; l < lvls.GetCount(); l++) {
			String relPath = lvls[l];
			String fullPath = AppendFileName("share/mods/umbrella", relPath);
			String label = Format("%d-%d", worldId, l + 1);

			Button& btn = levelButtons.Add();
			btn.SetLabel(label);
			btn << [=] { LaunchLevel(fullPath); };
			int bx = labelX + 90 + l * (btnW + gapX);
			Add(btn.LeftPos(bx, btnW).TopPos(rowY, btnH));
		}
	}
}

void MainMenuScreen::LaunchLevel(const String& path) {
	if(FileExists(path)) {
		Close();
		GameScreen(path).Run();
	} else {
		Exclamation("Level not found: " + path);
	}
}

void MainMenuScreen::ShowGameScreen() {
	// Use manifest to get first level
	LevelManifest& manifest = GetLevelManifest();
	if(!manifest.IsLoaded()) {
		manifest.Load("share/mods/umbrella");
	}

	String levelPath = manifest.GetFirstLevel();
	if(levelPath.IsEmpty())
		levelPath = "share/mods/umbrella/levels/world1-stage1.json";

	LaunchLevel(levelPath);
}

void MainMenuScreen::ShowEditorScreen() {
	Close();
	MapEditorApp().Run();
}
