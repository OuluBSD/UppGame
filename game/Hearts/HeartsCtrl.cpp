#include "Hearts.h"

#include <cstdio>

NAMESPACE_UPP

static String ResolveHeartsFormPath()
{
	Vector<String> candidates;
	candidates.Add(AppendFileName(GetCurrentDirectory(), "game/Hearts/Hearts.form"));
	candidates.Add(AppendFileName(GetFileDirectory(GetExeFilePath()), "../game/Hearts/Hearts.form"));
	candidates.Add(ConfigFile("Hearts.form"));
	for (const String& candidate : candidates) {
		if (FileExists(candidate))
			return candidate;
	}
	return ConfigFile("Hearts.form");
}

static Ctrl* GetNamedCtrl(Form& form, const String& name)
{
	ArrayMap<String, Ctrl>& ctrls = form.GetCtrls();
	int index = ctrls.Find(name);
	return index >= 0 ? &ctrls[index] : nullptr;
}

static String ResolveHeartsCardArtPath(const String& theme, const String& file_name)
{
	auto MakeRelativePath = [](const String& base_theme, const String& name) {
		return AppendFileName(AppendFileName("imgs", "cards"), AppendFileName(base_theme, name));
	};

	Vector<String> candidates;
	String requested_theme = TrimBoth(theme);
	if (requested_theme.IsEmpty())
		requested_theme = "default";

	auto AddCandidates = [&](const String& use_theme) {
		String rel = MakeRelativePath(use_theme, file_name);
		candidates.Add(ShareDirFile(rel));
		candidates.Add(AppendFileName(GetFileDirectory(GetExeFilePath()), AppendFileName("..", AppendFileName("share", rel))));
		candidates.Add(AppendFileName(GetCurrentDirectory(), AppendFileName("share", rel)));
	};

	AddCandidates(requested_theme);
	if (requested_theme != "default")
		AddCandidates("default");

	for (const String& candidate : candidates) {
		if (FileExists(candidate))
			return candidate;
	}
	return String();
}

static String FormatScreenRect(const Rect& r)
{
	char buffer[64];
	snprintf(buffer, sizeof(buffer), "(%d,%d %dx%d)", r.left, r.top, r.Width(), r.Height());
	return buffer;
}

static String TraceMouseEventName(int event)
{
	switch(event) {
	case Ctrl::MOUSEENTER: return "MOUSEENTER";
	case Ctrl::MOUSEMOVE: return "MOUSEMOVE";
	case Ctrl::MOUSELEAVE: return "MOUSELEAVE";
	case Ctrl::LEFTDOWN: return "LEFTDOWN";
	case Ctrl::LEFTUP: return "LEFTUP";
	case Ctrl::LEFTDOUBLE: return "LEFTDOUBLE";
	case Ctrl::LEFTTRIPLE: return "LEFTTRIPLE";
	case Ctrl::LEFTDRAG: return "LEFTDRAG";
	case Ctrl::RIGHTDOWN: return "RIGHTDOWN";
	case Ctrl::RIGHTUP: return "RIGHTUP";
	case Ctrl::RIGHTDOUBLE: return "RIGHTDOUBLE";
	case Ctrl::MIDDLEDOWN: return "MIDDLEDOWN";
	case Ctrl::MIDDLEUP: return "MIDDLEUP";
	case Ctrl::MOUSEWHEEL: return "MOUSEWHEEL";
	case Ctrl::MOUSEHWHEEL: return "MOUSEHWHEEL";
	}
	return Format("EVENT_%d", event);
}

static String FindCtrlName(Form& form, Ctrl *ctrl)
{
	if (!ctrl)
		return "<null>";
	ArrayMap<String, Ctrl>& ctrls = form.GetCtrls();
	for (int i = 0; i < ctrls.GetCount(); i++)
		if (&ctrls[i] == ctrl)
			return ctrls.GetKey(i);
	return "<unknown>";
}

static Rect LerpRect(const Rect& from, const Rect& to, double t)
{
	auto Lerp = [t](int a, int b) {
		return int(a + (b - a) * t + (b >= a ? 0.5 : -0.5));
	};
	return Rect(Lerp(from.left, to.left), Lerp(from.top, to.top),
	            Lerp(from.right, to.right), Lerp(from.bottom, to.bottom));
}

bool HeartsCtrl::RectsEqual(const Rect& a, const Rect& b)
{
	return a.left == b.left && a.top == b.top && a.right == b.right && a.bottom == b.bottom;
}

static Vector<Rect> MakeRowLayout(const Size& window, int count, const Size& card_size, int margin, bool top_row)
{
	Vector<Rect> layout;
	layout.Reserve(max(0, count));
	if (count <= 0)
		return layout;

	int available = max(0, window.cx - 2 * margin - card_size.cx);
	int gap = count > 1 ? available / (count - 1) : 0;
	gap = clamp(gap, 24, 64);
	int row_width = card_size.cx + gap * (count - 1);
	int left = max(margin, (window.cx - row_width) / 2);
	int top = top_row ? margin : max(margin, window.cy - margin - card_size.cy);
	for (int i = 0; i < count; i++)
		layout.Add(RectC(left + i * gap, top, card_size.cx, card_size.cy));
	return layout;
}

void HeartsAnimationLayer::Paint(Draw& w)
{
	if (owner)
		owner->PaintAnimations(w);
}

static void TraceCardLoad(const String& card_id, const String& file_name, const String& resolved_path,
                          const String& load_outcome, const Size& image_size)
{
	String line = Format("HEARTS_LOAD card=%s file=%s resolved=%s load=%s image=%dx%d",
	                     card_id, file_name,
	                     resolved_path.IsEmpty() ? "<missing>" : resolved_path,
	                     load_outcome, image_size.cx, image_size.cy);
	FileAppend out(AppendFileName(GetCurrentDirectory(), "tmp/HeartsTrace.txt"));
	out.Put(line + "\n");
}

HeartsCtrl::HeartsCtrl() {
	String form_path = ResolveHeartsFormPath();
	if (!ui.Load(form_path))
		ASSERT_(false, "Could not load Hearts.form");
	ui.SetScaleMode(Form::SCALE_FIT);
	if (!ui.Layout("Default"))
		ASSERT_(false, "Hearts.form missing Default layout");
	Add(ui.SizePos());
	anim_layer.owner = this;
	anim_layer.Transparent();
	anim_layer.IgnoreMouse();
	Add(anim_layer.SizePos());
	ui.SignalHandler = callback(this, &HeartsCtrl::HandleUiSignal);
	ArrayMap<String, Ctrl>& ctrls = ui.GetCtrls();
	for (int i = 0; i < ctrls.GetCount(); i++) {
		if (ImageCtrl* img = dynamic_cast<ImageCtrl*>(&ctrls[i])) {
			String name = ctrls.GetKey(i);
			if (name.EndsWith("Card") || name.Find("Card") >= 0) {
				img->SetTransitionMode(Ctrl::TRANSITION_NONE);
				img->SetTransitionDuration(1);
			}
		}
	}

	autoplay_enabled = false;
	started = false;
	pending_pass_player = -1;
	pass_animating = false;
	collecting_trick = false;
	collecting_winner = -1;
	collecting_points = 0;
	debug_render_trace = false;
	animating = false;
	human_card_animating = false;
	human_card_anim_progress = 1.0;
	difficulty = DIFFICULTY_HARD;
	human_card_image_keys.SetCount(4 * 13);
	trick_card_image_keys.SetCount(4);

	BackPaint();
}

void HeartsCtrl::StartGame() {
	ResetGame();
	state.LogCallback = [=](const String& msg) {
		Cout() << msg << "\n";
		LOG(msg);
	};
	state.Deal();
	started = true;
	
	if (state.phase == "PASSING") {
		state.SelectPass(1, HeartsAI::ChoosePassCards(state.players[1]));
		state.SelectPass(2, HeartsAI::ChoosePassCards(state.players[2]));
		state.SelectPass(3, HeartsAI::ChoosePassCards(state.players[3]));
	} else if (state.phase == "PLAYING" && state.turn != 0) {
		ScheduleAiStep(500);
	}
	
	RefreshUI();
}

void HeartsCtrl::HandleUiSignal(const String& path, const String& op, const String& action) {
	if (op != "OnAction")
		return;
	if (action == "NewGame")
		StartGame();
	else if (action == "PassCards")
		OnPassClick();
	else if (action == "ClearSelection")
		OnClearClick();
	else if (action.StartsWith("HumanCard")) {
		int index = ScanInt(action.Mid(9));
		if (index >= 0 && index < state.players[0].GetCount())
			OnCardClick(state.players[0][index]);
	}
}

void HeartsCtrl::ResetGame() {
	anim_cards.Clear();
	animating = false;
	selected_cards.Clear();
	started = false;
	pending_pass_player = -1;
	pass_animating = false;
	collecting_trick = false;
	collecting_winner = -1;
	collecting_points = 0;
	human_card_animating = false;
	human_card_anim_progress = 1.0;
	KillTimeCallback(1);
	KillTimeCallback(2);
}

void HeartsCtrl::RefreshUI() {
	Layout();
	UpdateHUD();
	UpdateForm();
}

void HeartsCtrl::UpdateHUD() {
	bool show_pass = (state.phase == "PASSING" && pending_pass_player < 0);
	if (Ctrl* ctrl = GetNamedCtrl(ui, "PassButton")) {
		if (show_pass) {
			ctrl->Show();
			ctrl->Enable(selected_cards.GetCount() == 3);
		}
		else
			ctrl->Hide();
	}
	if (Ctrl* ctrl = GetNamedCtrl(ui, "ClearButton")) {
		if (show_pass) {
			ctrl->Show();
			ctrl->Enable(selected_cards.GetCount() > 0);
		}
		else
			ctrl->Hide();
	}
}

void HeartsCtrl::ScheduleAiStep(int delay_ms) {
	KillTimeCallback(1);
	SetTimeCallback(delay_ms, THISBACK(AiStep), 1);
}

void HeartsCtrl::AiStep() {
	if (state.game_over) {
		FinishAutoplayIfNeeded();
		return;
	}
	if (pass_animating) return;
	if (state.trick_pending) return;

	if (state.phase == "PLAYING" && state.turn != 0) {
		int turn = state.turn;
		const Vector<Card>& hand = state.players[turn];
		Card chosen;

		if (difficulty == DIFFICULTY_EASY) {
			Vector<Card> valid;
			for (const Card& c : hand) {
				if (state.ValidatePlay(turn, c).success) {
					valid.Add(c);
				}
			}
			if (!valid.IsEmpty()) {
				chosen = valid[Random(valid.GetCount())];
			}
		} else if (difficulty == DIFFICULTY_MEDIUM) {
			Vector<Card> valid;
			for (const Card& c : hand) {
				if (state.ValidatePlay(turn, c).success) {
					valid.Add(c);
				}
			}
			if (!valid.IsEmpty()) {
				chosen = valid[0];
				int r = RankIndex(chosen.rank);
				for (int i = 1; i < valid.GetCount(); i++) {
					int r2 = RankIndex(valid[i].rank);
					if (r2 < r) {
						r = r2;
						chosen = valid[i];
					}
				}
			}
		} else {
			chosen = HeartsAI::ChooseCard(turn, hand, state.trick, state.leading_suit, state.hearts_broken);
		}

		if (!chosen.id.IsEmpty()) {
			state.PlayCard(turn, chosen);
			RefreshUI();
			
			if (state.trick_pending) {
				SetTimeCallback(1000, THISBACK(FinishTrickCollect), 2);
			} else if (state.turn != 0) {
				ScheduleAiStep(500);
			}
		}
	}
}

void HeartsCtrl::FinishTrickCollect() {
	state.ResolveTrick();
	RefreshUI();
	
	if (state.phase == "PLAYING" && state.turn != 0) {
		ScheduleAiStep(500);
	} else if (state.phase == "ROUND_END") {
		state.BeginNextRound();
		if (state.phase == "PASSING") {
			state.SelectPass(1, HeartsAI::ChoosePassCards(state.players[1]));
			state.SelectPass(2, HeartsAI::ChoosePassCards(state.players[2]));
			state.SelectPass(3, HeartsAI::ChoosePassCards(state.players[3]));
		} else if (state.phase == "PLAYING" && state.turn != 0) {
			ScheduleAiStep(500);
		}
		RefreshUI();
	}
}

void HeartsCtrl::StartTrickCollect() {}
void HeartsCtrl::StartPassAnimation() {}
void HeartsCtrl::FinishPassAnimation() {}
void HeartsCtrl::NextRound() {}
void HeartsCtrl::FinishAutoplayIfNeeded() {}
void HeartsCtrl::AddCardAnimation(const Card& card, Point src, Point dst, int angle, bool back) {}

void HeartsCtrl::PaintAnimations(Draw&)
{
}

Point HeartsCtrl::GetHandCenter(int player_idx) const {
	int cx = GetSize().cx;
	int cy = GetSize().cy;
	int centerX = cx / 2;
	int centerY = cy / 2;
	
	switch (player_idx) {
		case 0: return Point(centerX, cy - 80); // South
		case 1: return Point(cx - 80, centerY); // East
		case 2: return Point(centerX, 80);      // North
		case 3: return Point(80, centerY);       // West
	}
	return Point(0, 0);
}

Point HeartsCtrl::GetTrickCenter(int player_idx) const {
	int cx = GetSize().cx;
	int cy = GetSize().cy;
	int centerX = cx / 2;
	int centerY = cy / 2;
	int offset = int(min(cx, cy) * 0.12);
	
	switch (player_idx) {
		case 0: return Point(centerX, centerY + offset);
		case 1: return Point(centerX + offset, centerY);
		case 2: return Point(centerX, centerY - offset);
		case 3: return Point(centerX - offset, centerY);
	}
	return Point(0, 0);
}

void HeartsCtrl::ShowHighScores() {
	TopWindow win;
	win.Title("High Scores 🏆");
	win.SetRect(0, 0, 300, 200);
	
	Label lbl;
	lbl.SetText("1. Spearhead - 0 pts\n2. Director - 15 pts\n3. Curator - 34 pts\n4. Human - 48 pts");
	lbl.SetAlign(ALIGN_CENTER);
	win.Add(lbl.SizePos());
	
	win.Run();
}

void HeartsCtrl::ShowAbout() {
	PromptOK("[A1 Hearts Royal 💎]\n\nC++ Hearts Game implementation using U++ framework.\n\nCreated by Spearhead, 2026.");
}

void HeartsCtrl::OnCardClick(const Card& card) {
	if (state.phase == "PASSING") {
		int idx = FindIndex(selected_cards, card);
		if (idx != -1) {
			selected_cards.Remove(idx);
		} else {
			if (selected_cards.GetCount() < 3) {
				selected_cards.Add(card);
			}
		}
		RefreshUI();
	} else if (state.phase == "PLAYING" && state.turn == 0) {
		PlayResult res = state.PlayCard(0, card);
		if (res.success) {
			RefreshUI();
			if (state.trick_pending) {
				SetTimeCallback(1000, THISBACK(FinishTrickCollect), 2);
			} else if (state.turn != 0) {
				ScheduleAiStep(500);
			}
		} else {
			Exclamation(res.message);
		}
	}
}

void HeartsCtrl::OnPassClick() {
	if (state.phase == "PASSING" && selected_cards.GetCount() == 3) {
		state.SelectPass(0, selected_cards);
		selected_cards.Clear();
		RefreshUI();
		
		if (state.phase == "PLAYING" && state.turn != 0) {
			ScheduleAiStep(500);
		}
	}
}

void HeartsCtrl::OnClearClick() {
	selected_cards.Clear();
	RefreshUI();
}

void HeartsCtrl::UpdateForm() {
	static const char* player_prefix[4] = { "South", "East", "North", "West" };
	static const int trick_slots = 4;

	if (Ctrl* ctrl = GetNamedCtrl(ui, "TitleLabel"))
		if (Label* label = dynamic_cast<Label*>(ctrl))
			label->SetLabel("HEARTS ROYAL 💎");

	if (Ctrl* ctrl = GetNamedCtrl(ui, "StartHintLabel"))
		if (Label* label = dynamic_cast<Label*>(ctrl))
			label->SetLabel(started ? "" : "Click New Game to start playing.");

	String phase_info = "Phase: " + state.phase;
	if (state.phase == "PASSING") {
		int pass_direction = state.round_number % 4;
		if (pass_direction == 1) phase_info += " (Pass Left)";
		else if (pass_direction == 2) phase_info += " (Pass Right)";
		else if (pass_direction == 3) phase_info += " (Pass Across)";
	}
	if (Ctrl* ctrl = GetNamedCtrl(ui, "PhaseLabel"))
		if (Label* label = dynamic_cast<Label*>(ctrl))
			label->SetLabel(phase_info);

	if (Ctrl* ctrl = GetNamedCtrl(ui, "GameOverLabel"))
		if (Label* label = dynamic_cast<Label*>(ctrl))
			label->SetLabel(state.game_over ? "GAME OVER!" : "");

	for (int i = 0; i < 4; i++) {
		String name = Format("ScoreLabel%d", i);
		if (Ctrl* ctrl = GetNamedCtrl(ui, name))
			if (Label* label = dynamic_cast<Label*>(ctrl)) {
				String text = Format("%s | %d pts", i == 0 ? "You (P0)" : Format("AI (P%d)", i), state.scores[i]);
				if (state.phase == "PLAYING" && state.turn == i)
					text += " ◄";
				label->SetLabel(text);
				label->SetInk(state.turn == i ? Color(255, 215, 0) : White());
			}
	}

	if (!started) {
		for (int i = 0; i < 13; i++) {
			if (Ctrl* ctrl = GetNamedCtrl(ui, Format("HumanCard%d", i)))
				ctrl->Hide();
			for (int p = 1; p < 4; p++)
				if (Ctrl* other = GetNamedCtrl(ui, String(player_prefix[p]) + "Card" + AsString(i)))
					other->Hide();
		}
		for (int i = 0; i < trick_slots; i++)
			if (Ctrl* trick = GetNamedCtrl(ui, Format("TrickCard%d", i)))
				trick->Hide();
		return;
	}

	for (int i = 0; i < 4; i++) {
		const Vector<Card>& hand = state.players[i];
		for (int j = 0; j < 13; j++) {
			String ctrl_name = i == 0 ? Format("HumanCard%d", j)
			                           : String(player_prefix[i]) + "Card" + AsString(j);
			Ctrl* ctrl = GetNamedCtrl(ui, ctrl_name);
			if (j >= hand.GetCount()) {
				if (ctrl)
					ctrl->Hide();
				continue;
			}

			Image art = LoadCardImage(hand[j], i != 0);
			if (i == 1)
				art = RotateCardArt(art, 90);
			else if (i == 3)
				art = RotateCardArt(art, 270);
			String ctrl_kind = ctrl ? (dynamic_cast<ImageCtrl*>(ctrl) ? "ImageCtrl" : "Ctrl") : "missing";
			Rect screen_rect = ctrl ? ctrl->GetScreenRect() : Rect();
			String rendered_card_key = GetRenderedCardKey(hand[j], i != 0);
			int image_key_index = i * 13 + j;
			if (ImageCtrl* img = dynamic_cast<ImageCtrl*>(ctrl)) {
				ASSERT(image_key_index >= 0 && image_key_index < human_card_image_keys.GetCount());
				if (human_card_image_keys[image_key_index] != rendered_card_key) {
					human_card_image_keys[image_key_index] = rendered_card_key;
					img->SetImage(art);
				}
			}
			if (ctrl)
				ctrl->Show();
			if (debug_render_trace) {
				String requested_file = i != 0 ? "back9.png" : hand[j].id + ".png";
				String resolved_path = ResolveHeartsCardArtPath("default", requested_file);
				String load_outcome = art.IsEmpty() ? (resolved_path.IsEmpty() ? "missing" : "empty") : "loaded";
				TraceRenderedCard(ctrl_name, ctrl_kind, hand[j].id, requested_file, resolved_path, load_outcome, art.GetSize(), screen_rect);
			}
		}
	}

	Vector<Rect> human_target_rects;
	for (int j = 0; j < state.players[0].GetCount(); j++) {
		if (Ctrl* ctrl = GetNamedCtrl(ui, Format("HumanCard%d", j))) {
			Rect rect = j < human_card_base_rects.GetCount() ? human_card_base_rects[j] : ctrl->GetRect();
			if (FindIndex(selected_cards, state.players[0][j]) >= 0)
				rect.top -= 16;
			human_target_rects.Add(rect);
		}
	}

	for (int j = state.players[0].GetCount(); j < 13; j++)
		if (Ctrl* ctrl = GetNamedCtrl(ui, Format("HumanCard%d", j)))
			ctrl->Hide();

	if (!human_target_rects.IsEmpty()) {
		bool same_target = human_card_anim_to.GetCount() == human_target_rects.GetCount();
		if (same_target) {
			for (int i = 0; i < human_target_rects.GetCount(); i++) {
				if (!RectsEqual(human_card_anim_to[i], human_target_rects[i])) {
					same_target = false;
					break;
				}
			}
		}
		if (!same_target) {
			StartHumanCardAnimation(human_target_rects);
		}
		else if (!human_card_animating) {
			ApplyHumanCardAnimation(1.0);
		}
	}

	for (int i = 0; i < trick_slots; i++) {
		String ctrl_name = Format("TrickCard%d", i);
		Ctrl* ctrl = GetNamedCtrl(ui, ctrl_name);
		if (i >= state.trick.GetCount()) {
			if (ctrl)
				ctrl->Hide();
			continue;
		}
		Image art = LoadCardImage(state.trick[i].card, false);
		String ctrl_kind = ctrl ? (dynamic_cast<ImageCtrl*>(ctrl) ? "ImageCtrl" : "Ctrl") : "missing";
		Rect screen_rect = ctrl ? ctrl->GetScreenRect() : Rect();
		String rendered_card_key = GetRenderedCardKey(state.trick[i].card, false);
		if (ImageCtrl* img = dynamic_cast<ImageCtrl*>(ctrl)) {
			ASSERT(i >= 0 && i < trick_card_image_keys.GetCount());
			if (trick_card_image_keys[i] != rendered_card_key) {
				trick_card_image_keys[i] = rendered_card_key;
				img->SetImage(art);
			}
		}
		if (ctrl)
			ctrl->Show();
		if (debug_render_trace) {
			String requested_file = state.trick[i].card.id + ".png";
			String resolved_path = ResolveHeartsCardArtPath("default", requested_file);
			String load_outcome = art.IsEmpty() ? (resolved_path.IsEmpty() ? "missing" : "empty") : "loaded";
			TraceRenderedCard(ctrl_name, ctrl_kind, state.trick[i].card.id, requested_file, resolved_path, load_outcome, art.GetSize(), screen_rect);
		}
	}

	bool show_pass = (state.phase == "PASSING" && pending_pass_player < 0);
	if (Ctrl* ctrl = GetNamedCtrl(ui, "PassButton")) {
		if (show_pass) ctrl->Show(); else ctrl->Hide();
		ctrl->Enable(selected_cards.GetCount() == 3);
	}
	if (Ctrl* ctrl = GetNamedCtrl(ui, "ClearButton")) {
		if (show_pass) ctrl->Show(); else ctrl->Hide();
		ctrl->Enable(selected_cards.GetCount() > 0);
	}
	if (Ctrl* ctrl = GetNamedCtrl(ui, "NewGameButton"))
		if (!started || state.game_over) ctrl->Show(); else ctrl->Hide();
}

void HeartsCtrl::StartHumanCardAnimation(const Vector<Rect>& target)
{
	human_card_anim_from.Clear();
	human_card_anim_to.Clear();
	human_card_anim_to.Reserve(target.GetCount());
	for (const Rect& rect : target)
		human_card_anim_to.Add(rect);
	human_card_anim_from.Reserve(target.GetCount());
	for (int i = 0; i < target.GetCount(); i++) {
		if (Ctrl* ctrl = GetNamedCtrl(ui, Format("HumanCard%d", i)))
			human_card_anim_from.Add(ctrl->GetRect());
		else
			human_card_anim_from.Add(target[i]);
	}
	human_card_anim_progress = 0.0;
	human_card_animating = true;
	SetTimeCallback(16, THISBACK(AnimateStep), 4);
}

void HeartsCtrl::ApplyHumanCardAnimation(double t)
{
	for (int i = 0; i < human_card_anim_to.GetCount(); i++) {
		if (Ctrl* ctrl = GetNamedCtrl(ui, Format("HumanCard%d", i))) {
			ctrl->SetRect(LerpRect(human_card_anim_from[i], human_card_anim_to[i], t));
			ctrl->Show();
		}
	}
}

void HeartsCtrl::AnimateStep()
{
	if (!human_card_animating)
		return;

	human_card_anim_progress = min(1.0, human_card_anim_progress + 0.16);
	ApplyHumanCardAnimation(human_card_anim_progress);
	if (human_card_anim_progress >= 1.0) {
		human_card_animating = false;
		KillTimeCallback(4);
		ApplyHumanCardAnimation(1.0);
	}
	else {
		SetTimeCallback(16, THISBACK(AnimateStep), 4);
	}
}

void HeartsCtrl::DumpLayout() {
	Layout();
	Cout() << "Hearts layout dump\n";
	Cout() << "window=" << GetRect() << " size=" << GetSize() << "\n";
	LOG("Hearts layout dump");
	LOG(Format("window=%s size=%s", AsString(GetRect()), AsString(GetSize())));

	ArrayMap<String, Ctrl>& ctrls = ui.GetCtrls();
	for (int i = 0; i < ctrls.GetCount(); i++) {
		Ctrl& ctrl = ctrls[i];
		Cout() << Format("%-16s rect=%s\n", ctrls.GetKey(i), AsString(ctrl.GetRect()));
		LOG(Format("%s rect=%s", ctrls.GetKey(i), AsString(ctrl.GetRect())));
	}
}

Image HeartsCtrl::RenderSnapshot()
{
	Layout();
	ImageDraw draw(GetSize());
	DrawCtrl(draw);
	return draw;
}

Image HeartsCtrl::MouseEvent(int event, Point p, int zdelta, dword keyflags)
{
	String line = Format("HEARTS_MOUSE mouse event=%s p=(%d,%d) z=%d flags=%x started=%d phase=%s selected=%d",
	                     TraceMouseEventName(event), p.x, p.y, zdelta, (int)keyflags,
	                     started ? 1 : 0, state.phase, selected_cards.GetCount());
	Cout() << line << "\n";
	LOG(line);
	return Ctrl::MouseEvent(event, p, zdelta, keyflags);
}

void HeartsCtrl::LeftDown(Point p, dword flags) {
	String line = Format("HEARTS_MOUSE leftdown p=(%d,%d) flags=%x started=%d phase=%s",
	                     p.x, p.y, (int)flags, started ? 1 : 0, state.phase);
	Cout() << line << "\n";
	LOG(line);
	if (!started || state.game_over)
		return;

	if (state.phase == "PASSING" || (state.phase == "PLAYING" && state.turn == 0)) {
		for (int i = state.players[0].GetCount() - 1; i >= 0; i--) {
			String ctrl_name = Format("HumanCard%d", i);
			if (Ctrl* ctrl = GetNamedCtrl(ui, ctrl_name)) {
				if (ctrl->GetRect().Contains(p)) {
					OnCardClick(state.players[0][i]);
					break;
				}
			}
		}
	}
}

void HeartsCtrl::ChildMouseEvent(Ctrl *child, int event, Point p, int zdelta, dword keyflags)
{
	String child_name = FindCtrlName(ui, child);
	String line = Format("HEARTS_MOUSE child event=%s child=%s p=(%d,%d) z=%d flags=%x started=%d phase=%s",
	                     TraceMouseEventName(event), child_name, p.x, p.y, zdelta, (int)keyflags,
	                     started ? 1 : 0, state.phase);
	Cout() << line << "\n";
	LOG(line);

	if (event == LEFTDOWN && started && !state.game_over) {
		for (int i = 0; i < state.players[0].GetCount(); i++) {
			Ctrl *card = GetNamedCtrl(ui, Format("HumanCard%d", i));
			if (card && card == child) {
				OnCardClick(state.players[0][i]);
				return;
			}
		}
	}
	if (!child || child == &ui)
		Ctrl::ChildMouseEvent(child, event, p, zdelta, keyflags);
}

void HeartsCtrl::Layout() {
	ui.SizePos();
	ui.Layout();

	Size card_size = Size(84, 118);
	if (Ctrl* sample = GetNamedCtrl(ui, "HumanCard0"))
		card_size = sample->GetRect().GetSize();

	Vector<Rect> human_layout = MakeRowLayout(GetSize(), state.players[0].GetCount(), card_size, 24, false);
	human_card_base_rects.SetCount(0);
	for (int j = 0; j < human_layout.GetCount(); j++) {
		human_card_base_rects.Add(human_layout[j]);
		if (Ctrl* ctrl = GetNamedCtrl(ui, Format("HumanCard%d", j)))
			ctrl->SetRect(human_layout[j]);
	}

	Vector<Rect> north_layout = MakeRowLayout(GetSize(), state.players[2].GetCount(), card_size, 24, true);
	for (int j = 0; j < north_layout.GetCount(); j++)
		if (Ctrl* ctrl = GetNamedCtrl(ui, Format("NorthCard%d", j)))
			ctrl->SetRect(north_layout[j]);
}

void HeartsCtrl::Paint(Draw& w) {
	(void)w;
}

Image HeartsCtrl::LoadCardImage(const String& card_id, bool back, Size target) {
	String file_name = back ? "back9.png" : card_id + ".png";
	Image img = LoadCardArt(file_name, target, "default");
	if (debug_render_trace) {
		String resolved_path = ResolveHeartsCardArtPath("default", file_name);
		String load_outcome = img.IsEmpty() ? (resolved_path.IsEmpty() ? "missing" : "empty") : "loaded";
		TraceCardLoad(back ? "<back>" : card_id, file_name, resolved_path, load_outcome, img.GetSize());
	}
	return img;
}

Image HeartsCtrl::LoadCardImage(const Card& card, bool back, Size target) {
	return LoadCardImage(card.id, back, target);
}

String HeartsCtrl::GetRenderedCardKey(const Card& card, bool back) const
{
	return String(back ? "back:" : "front:") + card.id;
}

void HeartsCtrl::TraceRenderedCard(const String& ctrl_name, const String& ctrl_kind, const String& card_id, const String& file_name, const String& resolved_path, const String& load_outcome, const Size& image_size, const Rect& screen_rect) const
{
	String screen_rect_text = FormatScreenRect(screen_rect);
	String line = Format("HEARTS_RENDER ctrl=%s kind=%s card=%s file=%s resolved=%s load=%s image=%dx%d screen_rect=%s",
	                     ctrl_name, ctrl_kind, card_id, file_name,
	                     resolved_path.IsEmpty() ? "<missing>" : resolved_path,
	                     load_outcome, image_size.cx, image_size.cy, screen_rect_text);
	FileAppend out(AppendFileName(GetCurrentDirectory(), "tmp/HeartsTrace.txt"));
	out.Put(line + "\n");
}

END_UPP_NAMESPACE
