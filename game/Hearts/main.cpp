#include "Hearts.h"

#ifdef flagWIN32
#define CY win32_CY_
#define FAR win32_FAR_
#include <windows.h>
#undef CY
#undef FAR
#endif

using namespace Upp;

#ifdef PLATFORM_WIN32
static void AttachParentConsole()
{
	if (!AttachConsole(ATTACH_PARENT_PROCESS))
		return;

	FILE* out = nullptr;
	FILE* err = nullptr;
	IGNORE_RESULT(freopen_s(&out, "CONOUT$", "w", stdout));
	IGNORE_RESULT(freopen_s(&err, "CONOUT$", "w", stderr));
}
#endif

static bool SaveHeartsSnapshot(const String& path, HeartsCtrl& ctrl)
{
	Image img = ctrl.RenderSnapshot();
	RealizeDirectory(GetFileFolder(path));
	return PNGEncoder().SaveFile(path, img);
}

class HeartsWindow : public TopWindow {
public:
	HeartsCtrl ctrl;
	MenuBar menu;
	bool dump_layout = false;
	bool dump_render = false;
	int dump_timeout_ms = 2000;

	typedef HeartsWindow CLASSNAME;

	HeartsWindow(bool dump_layout = false, bool dump_render = false, int dump_timeout_ms = 2000)
	: dump_layout(dump_layout)
	, dump_render(dump_render)
	, dump_timeout_ms(dump_timeout_ms) {
		Title("Hearts Royal 💎");
		SetRect(0, 0, 800, 600);
		SetMinSize(Size(600, 450));

		// Enable standard window controls
		MinimizeBox();
		MaximizeBox();
		Sizeable();

		// Add components
		Add(ctrl.SizePos());
		AddFrame(menu);

		// Initialize menu bar
		menu.Set(THISBACK(MainMenu));

		if (dump_layout) {
			Maximize();
			SetTimeCallback(dump_timeout_ms, [this] {
				ctrl.DumpLayout();
				Close();
			}, 1);
		}
		if (dump_render) {
			Maximize();
			ctrl.SetDebugRenderTrace();
			SetTimeCallback(0, [this] {
				ctrl.StartGame();
			}, 2);
			SetTimeCallback(dump_timeout_ms, [this] {
				Close();
			}, 3);
		}
	}

	void MainMenu(Bar& bar) {
		bar.Add("File", THISBACK(FileMenu));
		bar.Add("Game", THISBACK(GameMenu));
		bar.Add("Help", THISBACK(HelpMenu));
	}

	void FileMenu(Bar& bar) {
		bar.Add("New Game", THISBACK(NewGame));
		bar.Add("Exit", THISBACK(ExitGame));
	}

	void GameMenu(Bar& bar) {
		bar.Add("Difficulty", THISBACK(DifficultyMenu));
		bar.Add("High Scores", THISBACK(HighScores));
	}

	void DifficultyMenu(Bar& bar) {
		bar.Add("Easy", THISBACK(SetDifficultyEasy))
		   .Radio(ctrl.difficulty == HeartsCtrl::DIFFICULTY_EASY);
		bar.Add("Medium", THISBACK(SetDifficultyMedium))
		   .Radio(ctrl.difficulty == HeartsCtrl::DIFFICULTY_MEDIUM);
		bar.Add("Hard", THISBACK(SetDifficultyHard))
		   .Radio(ctrl.difficulty == HeartsCtrl::DIFFICULTY_HARD);
	}

	void HelpMenu(Bar& bar) {
		bar.Add("About", THISBACK(About));
	}

	void NewGame() {
		ctrl.StartGame();
	}

	void ExitGame() {
		Close();
	}

	void HighScores() {
		ctrl.ShowHighScores();
	}

	void SetDifficultyEasy() {
		ctrl.difficulty = HeartsCtrl::DIFFICULTY_EASY;
	}

	void SetDifficultyMedium() {
		ctrl.difficulty = HeartsCtrl::DIFFICULTY_MEDIUM;
	}

	void SetDifficultyHard() {
		ctrl.difficulty = HeartsCtrl::DIFFICULTY_HARD;
	}

	void About() {
		ctrl.ShowAbout();
	}
};

GUI_APP_MAIN {
#ifdef PLATFORM_WIN32
	AttachParentConsole();
#endif
	StdLogSetup(LOG_COUT | LOG_FILE);

	const Vector<String>& args = CommandLine();
	bool dump_layout = false;
	bool dump_render = false;
	bool dump_snapshot = false;
	Vector<int> click_human_indices;
	String dump_snapshot_path = AppendFileName(GetCurrentDirectory(), "tmp/HeartsSnapshot.png");
	int dump_timeout_ms = 2000;
	for (int i = 0; i < args.GetCount(); i++) {
		if (args[i] == "--dump-layout")
			dump_layout = true;
		else if (args[i] == "--dump-render")
			dump_render = true;
		else if (args[i] == "--dump-snapshot")
			dump_snapshot = true;
		else if (args[i] == "--dump-snapshot-path" && i + 1 < args.GetCount())
			dump_snapshot_path = args[++i];
		else if (args[i] == "--click-human-index" && i + 1 < args.GetCount())
			click_human_indices.Add(ScanInt(args[++i]));
		else if (args[i] == "--dump-layout-timeout-ms" && i + 1 < args.GetCount())
			dump_timeout_ms = max(0, ScanInt(args[++i]));
	}

	HeartsWindow win(dump_layout, dump_render, dump_timeout_ms);
	if (dump_snapshot) {
		Size sz = Ctrl::GetPrimaryScreenArea().GetSize();
		win.ctrl.SetRect(0, 0, max(1, sz.cx), max(1, sz.cy));
		win.ctrl.SetDebugRenderTrace();
		win.ctrl.StartGame();
		if (win.ctrl.state.phase == "PASSING" && win.ctrl.state.players[0].GetCount() > 0)
			win.ctrl.OnCardClick(win.ctrl.state.players[0][0]);
		if (!SaveHeartsSnapshot(dump_snapshot_path, win.ctrl))
			Exclamation("Could not save snapshot: " + DeQtf(dump_snapshot_path));
		return;
	}
	if (dump_layout) {
		win.ctrl.StartGame();
		win.ctrl.DumpLayout();
		return;
	}
	if (dump_render) {
		win.ctrl.SetDebugRenderTrace();
		win.ctrl.StartGame();
		return;
	}
	if (!click_human_indices.IsEmpty()) {
		Size sz = Ctrl::GetPrimaryScreenArea().GetSize();
		win.ctrl.SetRect(0, 0, max(1, sz.cx), max(1, sz.cy));
		win.ctrl.Layout();
		win.ctrl.StartGame();
		win.ctrl.Layout();

		auto DumpSelection = [&] (const String& label) {
			String line = "HEARTS_CLICK_TRACE " + label + " selected=" + AsString(win.ctrl.selected_cards.GetCount());
			for (const Card& card : win.ctrl.selected_cards)
				line << " " << card.ToString();
			Cout() << line << "\n";
			LOG(line);
		};

		ArrayMap<String, Ctrl>& ctrls = win.ctrl.ui.GetCtrls();
		DumpSelection("initial");
		for (int click_index : click_human_indices) {
			String ctrl_name = Format("HumanCard%d", click_index);
			int ctrl_pos = ctrls.Find(ctrl_name);
			if (ctrl_pos < 0) {
				String line = Format("HEARTS_CLICK_TRACE missing ctrl=%s", ctrl_name);
				Cout() << line << "\n";
				LOG(line);
				continue;
			}
			Rect rect = ctrls[ctrl_pos].GetRect();
			Point click_point = rect.CenterPoint();
			String mouse_line = Format("HEARTS_MOUSE sim event=LEFTDOWN child=ui target=%s point=(%d,%d) rect=%s",
				ctrl_name, click_point.x, click_point.y, AsString(rect));
			Cout() << mouse_line << "\n";
			LOG(mouse_line);
			String line = Format("HEARTS_CLICK_TRACE click index=%d ctrl=%s point=(%d,%d) rect=%s",
				click_index, ctrl_name, click_point.x, click_point.y, AsString(rect));
			Cout() << line << "\n";
			LOG(line);
			win.ctrl.MouseEvent(Ctrl::LEFTDOWN, click_point, 0, 0);
			Ctrl& clicked = ctrls[ctrl_pos];
			String rect_line = Format("HEARTS_RECT after index=%d ctrl=%s rect=%s",
				click_index, ctrl_name, AsString(clicked.GetRect()));
			Cout() << rect_line << "\n";
			LOG(rect_line);
			DumpSelection(Format("after index=%d", click_index));
		}
		return;
	}
	win.ctrl.StartGame();
	win.Run();
}
