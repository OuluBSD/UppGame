#include "GameLauncher.h"

using namespace Upp;

GUI_APP_MAIN
{
	const Vector<String>& args = CommandLine();
	String path;
	RunMode mode = RunMode::Run;
	bool stdout_log = true;
	bool exit_on_assert = false;
	int force_close_after_ms = -1;

	for(int i = 0; i < args.GetCount(); i++) {
		const String& arg = args[i];
		if(arg == "--run") {
			mode = RunMode::Run;
			continue;
		}
		if(arg == "--debug") {
			mode = RunMode::Debug;
			continue;
		}
		if(arg == "--profile") {
			mode = RunMode::Profile;
			continue;
		}
		if(arg == "--stdout-log") {
			stdout_log = true;
			continue;
		}
		if(arg == "--no-stdout-log") {
			stdout_log = false;
			continue;
		}
		if(arg == "--exit-on-assert") {
			exit_on_assert = true;
			continue;
		}
		if(arg.StartsWith("--force-close-after-ms=")) {
			force_close_after_ms = max(0, ScanInt(arg.Mid(23)));
			continue;
		}
		if(arg == "--help") {
			PromptOK("Usage: GameLauncher <path/to/game.gamestate> [--debug|--profile]");
			return;
		}
		if(!arg.StartsWith("--") && path.IsEmpty())
			path = arg;
	}

	if(path.IsEmpty()) {
		Exclamation("Usage: GameLauncher <path/to/game.gamestate> [--debug|--profile]");
		return;
	}

	path = NormalizePath(GetFullPath(path));
	if(!FileExists(path)) {
		Exclamation("Game file not found:\n" + path);
		return;
	}

	SetCurrentDirectory(GetFileDirectory(path));
	CardGameDocumentHost::log_to_stdout = stdout_log;
	CardGameDocumentHost::exit_on_assert = exit_on_assert;

	if(force_close_after_ms >= 0) {
		SetTimeCallback(force_close_after_ms, [] {
			CloseAllStandaloneGameWindows();
		}, (void*)0x47A1);
	}

	RunStandaloneGameWindow(path, mode);
}
