#include "KuhnPoker.h"

#include <Form/Form.hpp>
#include <Ctrl/Xform3D/Xform3D.h>

NAMESPACE_UPP

static void AppendLine(String& text, const String& line)
{
	if (!text.IsEmpty())
		text << '\n';
	text << line;
}

static Card CardForIndex(int card)
{
	switch (card) {
	case KuhnPoker::CARD_J: return Card("spades", "jack");
	case KuhnPoker::CARD_Q: return Card("spades", "queen");
	default: return Card("spades", "king");
	}
}

static int CardRankValue(int card)
{
	switch (card) {
	case KuhnPoker::CARD_J: return 0;
	case KuhnPoker::CARD_Q: return 1;
	default: return 2;
	}
}

static Image LoadScaledCardImage(const String& filename)
{
	static VectorMap<String, Image> cache;
	int idx = cache.Find(filename);
	if (idx >= 0)
		return cache[idx];

	String path = ShareDirFile("imgs/cards/default/" + filename);
	Image img = StreamRaster::LoadFileAny(path);
	if (!img.IsEmpty())
		img = Rescale(img, Size(96, 136));
	cache.Add(filename, img);
	return img;
}

static Image LoadCardFallback(const String& label, bool back)
{
	ImageDraw draw(96, 136);
	draw.Alpha().DrawRect(0, 0, 96, 136, RGBAZero());
	draw.DrawRect(0, 0, 96, 136, back ? Color(30, 60, 140) : White());
	draw.DrawRect(0, 0, 96, 1, back ? Color(180, 200, 255) : Black());
	draw.DrawRect(0, 135, 96, 1, back ? Color(180, 200, 255) : Black());
	draw.DrawRect(0, 0, 1, 136, back ? Color(180, 200, 255) : Black());
	draw.DrawRect(95, 0, 1, 136, back ? Color(180, 200, 255) : Black());
	if (back) {
		draw.DrawRect(12, 12, 72, 112, Color(70, 100, 180));
		draw.DrawRect(18, 18, 60, 100, Color(110, 140, 220));
	}
	else {
		Font font = Arial(24).Bold();
		Size tsz = GetTextSize(label, font);
		draw.DrawText((96 - tsz.cx) / 2, (136 - tsz.cy) / 2, label, font, Black());
	}
	return draw;
}

static Image LoadCardArt(const String& id, const String& rank, bool back)
{
	if (back)
	{
		Image img = LoadScaledCardImage("back1.png");
		if (!img.IsEmpty())
			return img;
		return LoadCardFallback("", true);
	}

	Image img = LoadScaledCardImage(id + ".png");
	if (!img.IsEmpty())
		return img;
	return LoadCardFallback(ToUpper(rank), false);
}

static Image Faded(Image src, double alpha01)
{
	ImageBuffer ib(src);
	int mul = int(clamp(alpha01, 0.0, 1.0) * 255);
	for (int y = 0; y < ib.GetHeight(); y++) {
		RGBA *p = ib[y];
		for (int x = 0; x < ib.GetWidth(); x++, p++)
			p->a = byte((int(p->a) * mul) / 255);
	}
	return ib;
}

static void PaintShadow(Draw& w, Size sz, Rect dest, double alpha)
{
	DrawPainter dp(w, sz);
	dp.Clear(RGBAZero());
	RGBA c;
	c.r = 0;
	c.g = 0;
	c.b = 0;
	c.a = byte(clamp(alpha, 0.0, 1.0) * 90);
	dp.RoundedRectangle(dest.left + 6, dest.top + 8, dest.Width(), dest.Height(), 10).Fill(c);
}

static Image FlipFace(const String& id, const String& rank, double e, double& squeeze)
{
	if (e < 0.5) {
		squeeze = 1 - e * 2;
		return LoadCardArt(id, rank, true);
	}
	squeeze = (e - 0.5) * 2;
	return LoadCardArt(id, rank, false);
}

static double SqueezeToAngle(double squeeze)
{
	return acos(clamp(squeeze, 0.0, 1.0));
}

class KuhnPokerCardFlip : public Ctrl {
	typedef KuhnPokerCardFlip CLASSNAME;

	String card_id;
	String card_rank;
	bool   face_up = false;

public:
	KuhnPokerCardFlip()
	{
		Transparent();
	}

	void SetCard(const Card& card, bool reveal, bool animate = true)
	{
		card_id = card.id;
		card_rank = card.rank;

		if (reveal) {
			if (face_up && !IsTransitionRunning())
				return;
			face_up = true;
			SetTransitionMode(TRANSITION_FLIP_H);
			SetTransitionCurve(TRANSITION_EASE_OUT_CUBIC);
			SetTransitionDuration(350);
			if (animate)
				StartTransition();
			else
				StopTransition();
			Refresh();
			return;
		}

		face_up = false;
		StopTransition();
		Refresh();
	}

	virtual void Paint(Draw& w) override
	{
		Size sz = GetSize();
		if (sz.cx <= 0 || sz.cy <= 0)
			return;

		Rect dest = RectC(0, 0, sz.cx, sz.cy);
		if (!face_up && !IsTransitionRunning()) {
			Image back = LoadCardArt(card_id, card_rank, true);
			if (!back.IsEmpty())
				w.DrawImage(dest, back);
			return;
		}

		double e = IsTransitionRunning() ? GetTransitionValue() : 1.0;
		double squeeze = 0.0;
		Image shown = Faded(FlipFace(card_id, card_rank, e, squeeze), max(0.5, e));
		double angle = SqueezeToAngle(squeeze);
		Xform3D xf;
		xf.Set(0.0, angle, dest.GetSize());
		PaintShadow(w, sz, dest, e);
		DrawWarped3D(w, shown, xf, 10, dest.CenterPoint());
	}
};

KuhnPokerTable::KuhnPokerTable()
{
	Transparent();
}

void KuhnPokerTable::SetGame(KuhnPoker* poker)
{
	this->poker = poker;
	Refresh();
}

Card KuhnPokerTable::ToCard(int card)
{
	return CardForIndex(card);
}

Image KuhnPokerTable::LoadCardImage(const Card& card, bool back) const
{
	return LoadCardArt(card.id, card.rank, back);
}

void KuhnPokerTable::Paint(Draw& w)
{
	Size sz = GetSize();
	w.DrawRect(sz, Color(20, 90, 45));
	w.DrawRect(0, 0, sz.cx, 8, Color(96, 68, 24));
	w.DrawRect(0, sz.cy - 8, sz.cx, 8, Color(96, 68, 24));
	w.DrawRect(0, 0, 8, sz.cy, Color(96, 68, 24));
	w.DrawRect(sz.cx - 8, 0, 8, sz.cy, Color(96, 68, 24));

	w.DrawText(20, 16, "KUHN POKER", Arial(24).Bold(), Color(255, 220, 120));
	w.DrawText(20, 46, "A compact form-based card table", Arial(14), White());

	Rect table = RectC(30, 80, max(1, sz.cx - 60), max(1, sz.cy - 110));
	w.DrawRect(table, Color(18, 110, 58));
	w.DrawRect(table.left, table.top, table.Width(), 2, Color(225, 205, 130));
	w.DrawRect(table.left, table.bottom - 2, table.Width(), 2, Color(225, 205, 130));
	w.DrawRect(table.left, table.top, 2, table.Height(), Color(225, 205, 130));
	w.DrawRect(table.right - 2, table.top, 2, table.Height(), Color(225, 205, 130));

	if (!poker) {
		w.DrawText(table.left + 20, table.top + 20, "No game loaded", Arial(16).Bold(), White());
		return;
	}

	String phase_text;
	switch (poker->phase) {
	case KuhnPoker::PHASE_NOT_STARTED: phase_text = "Not started"; break;
	case KuhnPoker::PHASE_HUMAN_OPEN: phase_text = "Human to act"; break;
	case KuhnPoker::PHASE_HUMAN_FACING_BET: phase_text = "Human facing a bet"; break;
	case KuhnPoker::PHASE_SHOWDOWN: phase_text = "Showdown"; break;
	case KuhnPoker::PHASE_FOLD: phase_text = "Folded"; break;
	default: phase_text = "Unknown"; break;
	}

	w.DrawText(table.left + 20, table.top + 16, phase_text, Arial(16).Bold(), White());

	Point center = table.CenterPoint();
	Rect pot = RectC(center.x - 45, table.top + 80, 90, 60);
	w.DrawRect(pot, Color(90, 40, 20));
	w.DrawRect(pot.left, pot.top, pot.Width(), 1, Color(255, 220, 120));
	w.DrawRect(pot.left, pot.bottom - 1, pot.Width(), 1, Color(255, 220, 120));
	w.DrawRect(pot.left, pot.top, 1, pot.Height(), Color(255, 220, 120));
	w.DrawRect(pot.right - 1, pot.top, 1, pot.Height(), Color(255, 220, 120));
	w.DrawText(pot.left + 12, pot.top + 12, "Pot", Arial(12).Bold(), White());
	w.DrawText(pot.left + 10, pot.top + 28, AsString(poker->pot), Arial(18).Bold(), Color(255, 230, 140));

	Rect ai_rect = RectC(center.x - 48, table.top + 30, 96, 136);
	Rect human_rect = RectC(center.x - 48, table.bottom - 166, 96, 136);
	Image human_img = LoadCardImage(ToCard(poker->human_card), false);
	w.DrawRect(ai_rect, Color(24, 68, 34));
	w.DrawRect(ai_rect.left, ai_rect.top, ai_rect.Width(), 1, Color(225, 205, 130));
	w.DrawRect(ai_rect.left, ai_rect.bottom - 1, ai_rect.Width(), 1, Color(225, 205, 130));
	w.DrawRect(ai_rect.left, ai_rect.top, 1, ai_rect.Height(), Color(225, 205, 130));
	w.DrawRect(ai_rect.right - 1, ai_rect.top, 1, ai_rect.Height(), Color(225, 205, 130));
	w.DrawImage(human_rect, human_img);

	w.DrawText(ai_rect.left - 10, ai_rect.top - 22, "AI", Arial(14).Bold(), White());
	w.DrawText(human_rect.left - 20, human_rect.bottom + 6, "Human", Arial(14).Bold(), White());

	String chips = Format("Human chips: %d    AI chips: %d", poker->human_chips, poker->ai_chips);
	w.DrawText(table.left + 20, table.bottom - 34, chips, Arial(14).Bold(), White());

	String summary = poker->log;
	if (summary.IsEmpty())
		summary = "Press New Hand to start.";
	Vector<String> lines = Split(summary, '\n');
	int y = table.top + 240;
	for (const String& line : lines) {
		if (line.IsEmpty())
			continue;
		w.DrawText(table.left + 20, y, line, Arial(13), White());
		y += 18;
	}
}

void KuhnPoker::Ante()
{
	if (human_chips <= 0 || ai_chips <= 0)
		return;
	human_chips--;
	ai_chips--;
	pot = 2;
}

void KuhnPoker::NewHand()
{
	if (human_chips <= 0 || ai_chips <= 0) {
		phase = PHASE_NOT_STARTED;
		winner = WIN_NONE;
		pot = 0;
		log = "Game over: one player is out of chips.";
		return;
	}

	Vector<int> deck;
	deck.Add(CARD_J);
	deck.Add(CARD_Q);
	deck.Add(CARD_K);
	for (int i = deck.GetCount() - 1; i > 0; --i)
		Swap(deck[i], deck[Random(i + 1)]);
	human_card = deck[0];
	ai_card = deck[1];
	pot = 0;
	winner = WIN_NONE;
	phase = PHASE_NOT_STARTED;
	log.Clear();
	Ante();
	phase = PHASE_HUMAN_OPEN;
	log = Format("New hand. Human: %s, AI: hidden.", CardName(human_card));
}

String KuhnPoker::CardName(int card)
{
	switch (card) {
	case CARD_J: return "Jack";
	case CARD_Q: return "Queen";
	default: return "King";
	}
}

void KuhnPoker::Resolve(Winner w, const String& reason)
{
	winner = w;
	if (winner == WIN_HUMAN)
		human_chips += pot;
	else if (winner == WIN_AI)
		ai_chips += pot;
	AppendLine(log, reason);
	AppendLine(log, Format("Winner: %s.", winner == WIN_HUMAN ? "Human" : "AI"));
}

void KuhnPoker::Showdown()
{
	phase = PHASE_SHOWDOWN;
	if (CardRankValue(human_card) > CardRankValue(ai_card))
		Resolve(WIN_HUMAN, "Showdown: Human wins with the higher card.");
	else
		Resolve(WIN_AI, "Showdown: AI wins with the higher card.");
}

void KuhnPoker::AiActAfterHumanCheck()
{
	if (ai_card == CARD_J) {
		AppendLine(log, "AI checks.");
		Showdown();
		return;
	}

	ai_chips--;
	pot++;
	phase = PHASE_HUMAN_FACING_BET;
	AppendLine(log, Format("AI bets with %s.", CardName(ai_card)));
}

void KuhnPoker::HumanCheck()
{
	if (phase != PHASE_HUMAN_OPEN)
		return;
	AppendLine(log, "Human checks.");
	AiActAfterHumanCheck();
}

void KuhnPoker::HumanBet()
{
	if (phase != PHASE_HUMAN_OPEN || human_chips <= 0)
		return;
	human_chips--;
	pot++;
	AppendLine(log, Format("Human bets with %s.", CardName(human_card)));

	if (ai_card == CARD_J) {
		phase = PHASE_FOLD;
		Resolve(WIN_HUMAN, "AI folds.");
		return;
	}

	ai_chips--;
	pot++;
	AppendLine(log, "AI calls.");
	Showdown();
}

void KuhnPoker::HumanCall()
{
	if (phase != PHASE_HUMAN_FACING_BET || human_chips <= 0)
		return;
	human_chips--;
	pot++;
	AppendLine(log, "Human calls.");
	Showdown();
}

void KuhnPoker::HumanFold()
{
	if (phase != PHASE_HUMAN_FACING_BET)
		return;
	phase = PHASE_FOLD;
	Resolve(WIN_AI, "Human folds.");
}

static void UpdateButtonState(Form& form, const String& name, bool enabled)
{
	if (Ctrl* ctrl = form.GetCtrl(name))
		ctrl->Enable(enabled);
}

static void UpdateButtonState(Form& form, const char* name, bool enabled)
{
	UpdateButtonState(form, String(name), enabled);
}

static void UpdateStatus(Form& form, const KuhnPoker& game)
{
	if (Ctrl* ctrl = form.GetCtrl("StatusLabel"))
		if (Label* label = dynamic_cast<Label*>(ctrl))
			label->SetLabel(Format("Phase: %s | Human: %d | AI: %d | Pot: %d",
			                       game.phase == KuhnPoker::PHASE_NOT_STARTED ? "Not started" :
			                       game.phase == KuhnPoker::PHASE_HUMAN_OPEN ? "Human to act" :
			                       game.phase == KuhnPoker::PHASE_HUMAN_FACING_BET ? "Facing bet" :
			                       game.phase == KuhnPoker::PHASE_SHOWDOWN ? "Showdown" : "Folded",
			                       game.human_chips, game.ai_chips, game.pot));

	if (Ctrl* ctrl = form.GetCtrl("LogLabel"))
		if (Label* label = dynamic_cast<Label*>(ctrl))
			label->SetLabel(game.log);

	UpdateButtonState(form, "NewHandButton", game.human_chips > 0 && game.ai_chips > 0);
	UpdateButtonState(form, "CheckButton", game.phase == KuhnPoker::PHASE_HUMAN_OPEN);
	UpdateButtonState(form, "BetButton", game.phase == KuhnPoker::PHASE_HUMAN_OPEN && game.human_chips > 0);
	UpdateButtonState(form, "CallButton", game.phase == KuhnPoker::PHASE_HUMAN_FACING_BET && game.human_chips > 0);
	UpdateButtonState(form, "FoldButton", game.phase == KuhnPoker::PHASE_HUMAN_FACING_BET);
}

static Form* GetBoardForm(Form& form)
{
	if (Ctrl* ctrl = form.GetCtrl("BoardForm"))
		return dynamic_cast<Form*>(ctrl);
	return nullptr;
}

static void UpdateBoardLabel(Form& form, const String& name, const String& text)
{
	if (Form* board = GetBoardForm(form))
		if (Ctrl* ctrl = board->GetCtrl(name))
			if (Label* label = dynamic_cast<Label*>(ctrl))
				label->SetLabel(text);
}

static void UpdateBoardImage(Form& form, const String& name, const Image& img)
{
	if (Form* board = GetBoardForm(form))
		if (Ctrl* ctrl = board->GetCtrl(name))
			if (ImageCtrl* image = dynamic_cast<ImageCtrl*>(ctrl))
				image->SetImage(img);
}

static void UpdateBoard(Form& form, const KuhnPoker& game)
{
	UpdateBoardLabel(form, "BoardPhaseLabel", Format("Phase: %s",
		game.phase == KuhnPoker::PHASE_NOT_STARTED ? "Not started" :
		game.phase == KuhnPoker::PHASE_HUMAN_OPEN ? "Human to act" :
		game.phase == KuhnPoker::PHASE_HUMAN_FACING_BET ? "Facing bet" :
		game.phase == KuhnPoker::PHASE_SHOWDOWN ? "Showdown" : "Folded"));
	UpdateBoardLabel(form, "BoardPotValue", AsString(game.pot));
	UpdateBoardLabel(form, "BoardChipsLabel", Format("Human chips: %d    AI chips: %d",
		game.human_chips, game.ai_chips));

	UpdateBoardImage(form, "AICard",
		LoadCardArt(CardForIndex(game.ai_card).id, CardForIndex(game.ai_card).rank,
			game.phase != KuhnPoker::PHASE_SHOWDOWN && game.phase != KuhnPoker::PHASE_FOLD));
	UpdateBoardImage(form, "HumanCard",
		LoadCardArt(CardForIndex(game.human_card).id, CardForIndex(game.human_card).rank, false));
}

static String ResolveFormPath()
{
	Vector<String> candidates;
	candidates.Add(AppendFileName(GetCurrentDirectory(), "game/KuhnPoker/KuhnPoker.form"));
	candidates.Add(AppendFileName(GetFileDirectory(GetExeFilePath()), "../game/KuhnPoker/KuhnPoker.form"));
	candidates.Add(ConfigFile("KuhnPoker.form"));
	for (const String& candidate : candidates) {
		if (FileExists(candidate))
			return candidate;
	}
	return ConfigFile("KuhnPoker.form");
}

END_UPP_NAMESPACE

using namespace Upp;

GUI_APP_MAIN
{
	String form_path = ResolveFormPath();
	FormWindow window;
	if (!window.Load(form_path)) {
		Exclamation("Could not load form: " + DeQtf(form_path));
		return;
	}

	window.Layout("Default");
	Form& form = window.GetForm();

	KuhnPoker game;
	game.NewHand();

	auto sync_views = [&] {
		UpdateStatus(form, game);
		UpdateBoard(form, game);
	};

	auto refresh = [&] {
		sync_views();
	};

	form.SignalHandler << [&](const String&, const String& op, const String& action) {
		if (op != "OnAction")
			return;
		if (action == "NewHand") {
			game.NewHand();
			refresh();
		}
		else if (action == "Check") {
			game.HumanCheck();
			refresh();
		}
		else if (action == "Bet") {
			game.HumanBet();
			refresh();
		}
		else if (action == "Call") {
			game.HumanCall();
			refresh();
		}
		else if (action == "Fold") {
			game.HumanFold();
			refresh();
		}
	};

	refresh();
	window.Run();
}
