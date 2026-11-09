#include "SweetSandy.h"

NAMESPACE_UPP

void AppShell::AddScreen(const char *id, Ctrl& ctrl, const char *tab_text)
{
	// TabCtrl::Item::Slave() adds the child via plain Ctrl::Add(Ctrl&),
	// which does NOT size it - a freshly constructed Ctrl starts at
	// rect(0,0,0,0). Without this, every screen renders into a 0x0 area
	// (see AGENTS.md: this was the root cause of the near-blank window).
	ctrl.SizePos();
	shell.Add(ctrl, tab_text);
	screen.Add(id, &ctrl);
}

AppShell::AppShell()
{
	Title("SweetSandy");
	Sizeable().Zoomable();
	SetRect(0, 0, 1280, 800);

	Add(shell.SizePos());
	// TabCtrl::SetStyle stores a pointer to its argument, so the header-less
	// style needs static storage duration, not a constructor-local one.
	static TabCtrl::Style headerless_style = [] {
		TabCtrl::Style s = TabCtrl::StyleDefault();
		s.tabheight = 0;
		s.margin = 0;
		return s;
	}();
	shell.SetStyle(headerless_style);

	menu_screen.Create();
	newgame_screen.Create();
	settings_screen.Create();
	multiplayer_screen.Create("Moninpeli");
	credits_screen.Create("Credits");
	tutorial_screen.Create("Tutorial");
	play_screen.Create();

	AddScreen(SCREEN_MENU, *menu_screen, "Menu");
	AddScreen(SCREEN_NEWGAME, *newgame_screen, "New game");
	AddScreen(SCREEN_SETTINGS, *settings_screen, "Settings");
	AddScreen(SCREEN_MULTIPLAYER, *multiplayer_screen, "Multiplayer");
	AddScreen(SCREEN_CREDITS, *credits_screen, "Credits");
	AddScreen(SCREEN_TUTORIAL, *tutorial_screen, "Tutorial");
	AddScreen(SCREEN_PLAY, *play_screen, "Play");

	menu_screen->WhenSelect = [=](String id) {
		if(IsNull(id))
			Close();
		else
			ShowScreen(id);
	};
	newgame_screen->WhenBack    = [=] { ShowScreen(SCREEN_MENU); };
	newgame_screen->WhenStart   = [=](String track_id, String vehicle_id) {
		LOG("AppShell: starting run track=" << track_id << " vehicle=" << vehicle_id);
		StartRunById(track_id, vehicle_id);
	};
	settings_screen->WhenBack   = [=] { ShowScreen(SCREEN_MENU); };
	multiplayer_screen->WhenBack = [=] { ShowScreen(SCREEN_MENU); };
	credits_screen->WhenBack    = [=] { ShowScreen(SCREEN_MENU); };
	tutorial_screen->WhenBack   = [=] { ShowScreen(SCREEN_MENU); };
	play_screen->WhenBack       = [=] { play_screen->StopRun(); ShowScreen(SCREEN_MENU); };

	ShowScreen(SCREEN_MENU);
}

void AppShell::StartRunById(const String& track_id, const String& vehicle_id)
{
	const TrackDef *t = FindTrack(track_id);
	const VehicleDef *v = FindVehicle(vehicle_id);
	ASSERT_(t, "AppShell::StartRunById: unknown track id '" + track_id + "'");
	ASSERT_(v, "AppShell::StartRunById: unknown vehicle id '" + vehicle_id + "'");
	selected_track_id = t->id;
	selected_vehicle_id = v->id;
	play_screen->StartRun(*t, *v);
	ShowScreen(SCREEN_PLAY);
	play_screen->SetFocus();
}

void AppShell::StartDefaultRun()
{
	StartRunById(GetTrackCatalog()[0].id, GetVehicleCatalog()[0].id);
}

PlayScreen& AppShell::GetPlayScreen()
{
	return *play_screen;
}

MapEditorWindow& AppShell::GetMapEditor()
{
	ASSERT_(map_editor, "AppShell::GetMapEditor: ToggleMapEditor (open) must be called first");
	return *map_editor;
}

void AppShell::ShowScreen(const String& id)
{
	int i = screen.Find(id);
	ASSERT_(i >= 0, "AppShell::ShowScreen: unknown screen id");
	shell.Set(*screen[i]);
}

void AppShell::ToggleMapEditor()
{
	if(map_editor && map_editor->IsOpen()) {
		// DockWindow::Close(DockableCtrl&) hides TopWindow::Close() in
		// this scope (C++ name hiding) - qualify to get the "close the
		// whole window" overload, not "close one dock pane".
		map_editor->TopWindow::Close();
		return;
	}
	if(!map_editor)
		map_editor.Create();
	map_editor->Open();
}

bool AppShell::Key(dword key, int count)
{
	if(key == K_F1) {
		ToggleMapEditor();
		return true;
	}
	return TopWindow::Key(key, count);
}

END_UPP_NAMESPACE
