#include "GameTable.h"
#include "ClientThread.h"
#include "NetPacket.h"
#include "TexasHoldemLogicState.h"
#include <TexasHoldemProviderCatalog/TexasHoldemProviderCatalog.h>
#include <Poker/CardsValue.h>
#include <EditorCommon/Tools.h>
#include <EditorCommon/EditorCommon.h>
#include <EditorCommon/GameTheme.h>
#include <EditorCommon/GameThemeRender.h>
#include <Poker/TexasRenderer.h>
#include <Poker/TableLayoutProfile.h>
#include <GameRules/Game.h>
#include <GameRules/HandInterface.h>
#include <GameRules/BoardInterface.h>
#include <GameRules/BeroInterface.h>
#include <GameRules/PlayerInterface.h>
#include "SettingsWindow.h"
#include <EditorCommon/ConfigFile.h>
#include <plugin/png/png.h>
#include <cmath>
#include <CardRender/CardRender.h>

NAMESPACE_UPP

extern std::shared_ptr<ClientThread> g_clientThread;

static void RegisterGameTableFormFactories()
{
	static bool done = false;
	if (done)
		return;
	done = true;

	RegisterFormCtrlType<BoardCtrl>("BoardCtrl");
	RegisterFormCtrlType<PlayerCtrl>("PlayerCtrl");
	RegisterFormCtrlType<ActionButton>("ActionButton");
	RegisterFormCtrlType<ScaledImageCtrl>("ScaledImageCtrl");
	RegisterFormCtrlType<PokerChatCtrl>("PokerChatCtrl");
	RegisterFormCtrlType<ProbabilityCtrl>("ProbabilityCtrl");
	RegisterFormCtrlType<ParentCtrl>("ParentCtrl");
	RegisterFormCtrlType<SliderCtrl>("SliderCtrl");
	RegisterFormCtrlType<RichTextView>("RichTextView");
}

String GameTable::ResolveFormPath(const String& provider)
{
	const TexasHoldemProviderInfo *info = TexasHoldemFindProvider(provider);
	String form_name = info ? info->form_file : String("GameTable.form");
	Vector<String> candidates;
	candidates.Add(AppendFileName(GetCurrentDirectory(), "game/TexasHoldem/" + form_name));
	candidates.Add(AppendFileName(GetFileDirectory(GetExeFilePath()), "../game/TexasHoldem/" + form_name));
	candidates.Add(ConfigFile(form_name));
	for (const String& candidate : candidates) {
		if (FileExists(candidate))
			return candidate;
	}
	return ConfigFile(form_name);
}

String GameTable::ProviderToLayoutProfile(const String& provider)
{
	const TexasHoldemProviderInfo *info = TexasHoldemFindProvider(provider);
	return info ? info->table_profile : String("texas-holdem-classic");
}

template <class T>
T& GameTable::CtrlAs(const String& name) const
{
	Form& form = const_cast<Form&>(m_form);
	Ctrl* ctrl = form.GetCtrl(name);
	ASSERT(ctrl);
	T* out = dynamic_cast<T*>(ctrl);
	ASSERT(out);
	return *out;
}

PlayerCtrl& GameTable::PlayersProxy::operator[](int index) const
{
	ASSERT(owner);
	return owner->PlayerAt(index);
}

BoardCtrl& GameTable::Board() const { return CtrlAs<BoardCtrl>("board"); }
Label& GameTable::LblPotTitle() const { return CtrlAs<Label>("lblPotTitle"); }
Label& GameTable::LblPotTotal() const { return CtrlAs<Label>("lblPotTotal"); }
Label& GameTable::LblPotBets() const { return CtrlAs<Label>("lblPotBets"); }
Label& GameTable::LblTurnTitle() const { return CtrlAs<Label>("lblTurnTitle"); }
Label& GameTable::LblGameInfo() const { return CtrlAs<Label>("lblGameInfo"); }
Label& GameTable::LblHandInfo() const { return CtrlAs<Label>("lblHandInfo"); }
TabCtrl& GameTable::TabLeft() const { return CtrlAs<TabCtrl>("tabLeft"); }
TabCtrl& GameTable::TabRight() const { return CtrlAs<TabCtrl>("tabRight"); }
ParentCtrl& GameTable::HumanButtons() const { return CtrlAs<ParentCtrl>("humanButtons"); }
ActionButton& GameTable::BtnFold() const { return CtrlAs<ActionButton>("btnFold"); }
ActionButton& GameTable::BtnCheckCall() const { return CtrlAs<ActionButton>("btnCheckCall"); }
ActionButton& GameTable::BtnBetRaise() const { return CtrlAs<ActionButton>("btnBetRaise"); }
ActionButton& GameTable::BtnAllIn() const { return CtrlAs<ActionButton>("btnAllIn"); }
ActionButton& GameTable::BtnPot33() const { return CtrlAs<ActionButton>("btnPot33"); }
ActionButton& GameTable::BtnPot50() const { return CtrlAs<ActionButton>("btnPot50"); }
ActionButton& GameTable::BtnPot100() const { return CtrlAs<ActionButton>("btnPot100"); }
EditInt& GameTable::EditBet() const { return CtrlAs<EditInt>("editBet"); }
SliderCtrl& GameTable::SliderBet() const { return CtrlAs<SliderCtrl>("sliderBet"); }
Label& GameTable::LblSpeed() const { return CtrlAs<Label>("lblSpeed"); }
SliderCtrl& GameTable::SliderSpeed() const { return CtrlAs<SliderCtrl>("sliderSpeed"); }
Button& GameTable::BtnPause() const { return CtrlAs<Button>("btnPause"); }
PokerChatCtrl& GameTable::ChatCtrl() const { return CtrlAs<PokerChatCtrl>("chatCtrl"); }
StaticRect& GameTable::HandsImgPane() const { return CtrlAs<StaticRect>("handsImgPane"); }
ImageCtrl& GameTable::HandsImg() const { return CtrlAs<ImageCtrl>("handsImg"); }
ProbabilityCtrl& GameTable::ProbCtrl() const { return CtrlAs<ProbabilityCtrl>("probCtrl"); }
RichTextView& GameTable::EngineLogView() const { return CtrlAs<RichTextView>("engineLogView"); }
RichTextView& GameTable::AwayLog() const { return CtrlAs<RichTextView>("awayLog"); }
PlayerCtrl& GameTable::PlayerAt(int index) const
{
	ASSERT(index >= 0 && index < 10);
	ASSERT(m_player[index]);
	return *m_player[index];
}

void GameTable::BindFormControls()
{
	m_board = &Board();
	m_lblPotTitle = &LblPotTitle();
	m_lblPotTotal = &LblPotTotal();
	m_lblPotBets = &LblPotBets();
	m_lblTurnTitle = &LblTurnTitle();
	m_lblGameInfo = &LblGameInfo();
	m_lblHandInfo = &LblHandInfo();
	m_tabLeft = &TabLeft();
	m_tabRight = &TabRight();
	m_humanButtons = &HumanButtons();
	m_btnFold = &BtnFold();
	m_btnCheckCall = &BtnCheckCall();
	m_btnBetRaise = &BtnBetRaise();
	m_btnAllIn = &BtnAllIn();
	m_btnPot33 = &BtnPot33();
	m_btnPot50 = &BtnPot50();
	m_btnPot100 = &BtnPot100();
	m_editBet = &EditBet();
	m_sliderBet = &SliderBet();
	m_lblSpeed = &LblSpeed();
	m_sliderSpeed = &SliderSpeed();
	m_btnPause = &BtnPause();
	m_chatCtrl = &ChatCtrl();
	m_handsImgPane = &HandsImgPane();
	m_handsImg = &HandsImg();
	m_probCtrl = &ProbCtrl();
	m_engineLogView = &EngineLogView();
	m_awayLog = &AwayLog();
	for (int i = 0; i < 10; i++)
		m_player[i] = &CtrlAs<PlayerCtrl>(String("player") + AsString(i));
}

#define board Board()
#define lblPotTitle LblPotTitle()
#define lblPotTotal LblPotTotal()
#define lblPotBets LblPotBets()
#define lblTurnTitle LblTurnTitle()
#define lblGameInfo LblGameInfo()
#define lblHandInfo LblHandInfo()
#define tabLeft TabLeft()
#define tabRight TabRight()
#define humanButtons HumanButtons()
#define btnFold BtnFold()
#define btnCheckCall BtnCheckCall()
#define btnBetRaise BtnBetRaise()
#define btnAllIn BtnAllIn()
#define btnPot33 BtnPot33()
#define btnPot50 BtnPot50()
#define btnPot100 BtnPot100()
#define editBet EditBet()
#define sliderBet SliderBet()
#define lblSpeed LblSpeed()
#define sliderSpeed SliderSpeed()
#define btnPause BtnPause()
#define chatCtrl ChatCtrl()
#define handsImgPane HandsImgPane()
#define handsImg HandsImg()
#define probCtrl ProbCtrl()
#define engineLogView EngineLogView()
#define awayLog AwayLog()

static Image SwapRBForPng(const Image& src)
{
	if (src.IsEmpty())
		return src;
	Image tmp = src;
	ImageBuffer ib(tmp);
	for (int y = 0; y < ib.GetHeight(); y++) {
		RGBA* row = ib[y];
		for (int x = 0; x < ib.GetWidth(); x++) {
			byte t = row[x].r;
			row[x].r = row[x].b;
			row[x].b = t;
		}
	}
	return Image(ib);
}

static bool SaveSnapshotPng(const String& path, const Image& src)
{
	Image out = src;
#ifdef flagX11
	// X11 screenshot/export path stores channels in opposite order for PNG output.
	// Keep explicit swap here so saved artifacts match on-screen colors.
	out = SwapRBForPng(out);
#endif
	RealizeDirectory(GetFileFolder(path));
	return PNGEncoder().SaveFile(path, out);
}

PokerChatCtrl::PokerChatCtrl()
{
	Add(chatHistory);
	Add(chatInput);
	Add(btnSend);
	chatHistory.SetReadOnly();
	btnSend.SetLabel(t_("Send"));
}

void PokerChatCtrl::Layout()
{
	Size sz = GetSize();
	int btnW = 60;
	int editH = 20;
	chatHistory.SetRect(0, 0, sz.cx, sz.cy - editH - 2);
	chatInput.SetRect(0, sz.cy - editH, sz.cx - btnW - 2, editH);
	btnSend.SetRect(sz.cx - btnW, sz.cy - editH, btnW, editH);
}

Image GameTable::ApplyInterlacedTransparency(const Image& img)
{
	Image m = img;
	ImageBuffer ib(m);
	for(int y = 0; y < ib.GetHeight(); y++) {
		if (y % 2 != 0) {
			for(int x = 0; x < ib.GetWidth(); x++)
				ib[y][x] = RGBAZero();
		}
	}
	return ib;
}

Image GameTable::GetCardImage(int card)
{
	static String cachedTheme;
	static VectorMap<int, Image> cache;
	if (cachedTheme != currentCardTheme) {
		cache.Clear();
		cachedTheme = currentCardTheme;
	}
	if (cache.Find(card) >= 0) return cache.Get(card);

	String filename;
	if (card < 0) {
		filename = "back9.png";
	} else {
		static const char* suits[] = { "clubs", "diamonds", "hearts", "spades" };
		static const char* ranks[] = { "2", "3", "4", "5", "6", "7", "8", "9", "10", "jack", "queen", "king", "ace" };
		if (card < 52) {
			filename = String(suits[card / 13]) + "_" + ranks[card % 13] + ".png";
		}
	}

	Image img = LoadCardArt(filename, Size(48, 76), currentCardTheme);
	if (!img.IsEmpty()) {
		// already loaded at the target size by the shared helper
	}
	else {
		ImageDraw iw(48, 76);
		iw.Alpha().DrawRect(0, 0, 48, 76, RGBAZero());
		iw.DrawRect(0, 0, 48, 76, (card < 0 && card != -2) ? Color(0, 0, 150) : White());
		DrawFrame(iw, 0, 0, 48, 76, Black());
		if (card >= 0) {
			Font f = SansSerif(14).Bold();
			Size tsz = GetTextSize(AsString(card), f);
			iw.DrawText((48 - tsz.cx) / 2, (76 - tsz.cy) / 2, AsString(card), f, Black());
		} else if (card == -2) {
			iw.DrawRect(4, 4, 40, 68, Color(0, 0, 150));
		}
		img = iw;
	}
	cache.Add(card, img);
	return img;
}

Image GameTable::GetTransparentCard(int card, int alpha)
{
	return ApplyInterlacedTransparency(GetCardImage(card));
}

// Chip-color/disc-count logic shared by the cached per-player bet-stack
// Image (GetChipStackImage, below) and the direct pot-stack Draw call
// (RenderToImage) -- factored out so both paths agree exactly on what an
// amount looks like. Standard poker chip color tiers, picked by the highest
// denomination that "fits" the amount (white < 5, red < 25, green < 100,
// black < 500, purple >= 500); disc count mirrors the tier index (1-4),
// capped at 4 regardless of amount so the stack never grows unboundedly.
void GameTable::DrawChipDiscs(Draw& w, int cx, int baseY, int amount, Draw* alphaW)
{
	if (amount <= 0) return;

	struct ChipTier { int minAmount; Color fill; Color rim; };
	static const ChipTier tiers[] = {
		{ 0,   Color(245, 245, 245), Color(110, 110, 110) }, // white  ( < 5 )
		{ 5,   Color(210, 30, 30),   Color(255, 255, 255) }, // red    ( 5-24 )
		{ 25,  Color(30, 190, 90),   Color(255, 255, 255) }, // green  ( 25-99 ) -- kept clearly
		                                                      // brighter/cooler than the board felt
		                                                      // fallback fill (Color(0,100,0),
		                                                      // PaintBoard) so it doesn't blend in.
		{ 100, Color(20, 20, 20),    Color(220, 220, 220) }, // black  ( 100-499 )
		{ 500, Color(150, 30, 190),  Color(255, 215, 0) },   // purple ( >= 500 )
	};
	const int tierCount = (int)(sizeof(tiers) / sizeof(tiers[0]));
	int tierIdx = 0;
	for (int i = 0; i < tierCount; i++)
		if (amount >= tiers[i].minAmount)
			tierIdx = i;
	int discCount = min(tierIdx + 1, 4);
	Color fill = tiers[tierIdx].fill;
	Color rim = tiers[tierIdx].rim;

	const int discW = 20, discH = 8, discStep = 6;
	for (int i = 0; i < discCount; i++) {
		int y = baseY - discH - i * discStep;
		int x = cx - discW / 2;
		// When rendering into a cached ImageDraw (GetChipStackImage), the RGB
		// and alpha planes are two entirely separate off-screen bitmaps in
		// this codebase's Win32 ImageDraw backend (uppsrc/CtrlCore/ImageWin32.cpp:
		// ImageDraw::Get() copies the final alpha strictly from the alpha
		// plane's own drawing, never implicitly from whatever the RGB plane
		// painted) -- so the disc must be explicitly stamped as opaque on the
		// alpha plane too, matching this same geometry, or it silently
		// composites as 100% transparent (invisible) despite correct RGB
		// content. Not needed when drawing directly onto a live screen Draw&
		// (the pot stack, no separate alpha plane involved) -- alphaW is null
		// there.
		if (alphaW)
			alphaW->DrawEllipse(x, y, discW, discH, White(), 2, White());
		w.DrawEllipse(x, y, discW, discH, fill, 2, Black());
		// Thin edge-stripe pattern: a couple of short contrasting rects
		// around the rim, standing in for the sculpted edge-spots on real
		// poker chips (simple, not photorealistic per the task spec).
		w.DrawRect(x + 2, y + discH / 2 - 1, 4, 2, rim);
		w.DrawRect(x + discW - 6, y + discH / 2 - 1, 4, 2, rim);
	}
}

Image GameTable::GetChipStackImage(int amount)
{
	if (amount <= 0) return Image();

	static String cachedTheme;
	static VectorMap<int, Image> cache;
	if (cachedTheme != currentTableTheme) {
		cache.Clear();
		cachedTheme = currentTableTheme;
	}

	// The rendered visual (color + disc count) depends only on which
	// denomination tier the amount falls into -- the same bucketing
	// DrawChipDiscs uses -- so cache by tier index (0..4), not by the exact
	// bet amount, keeping the cache bounded regardless of how many distinct
	// bet sizes a long session sees (mirrors GetCardImage/GetPuckImage's
	// small-finite-key-space caching idiom).
	static const int tierThresholds[] = { 0, 5, 25, 100, 500 };
	int tierIdx = 0;
	for (int i = 0; i < 5; i++)
		if (amount >= tierThresholds[i])
			tierIdx = i;

	int idx = cache.Find(tierIdx);
	if (idx >= 0) return cache[idx];

	// Themed-load-first: try <dataDir>/gfx/gui/table/<theme>/chip_<N>.png for
	// the tier's representative denomination (chip_1/5/25/100/500.png).
	String dataDir = Tools::GetDataDir();
	String dir = AppendFileName(dataDir, "gfx/gui/table/" + currentTableTheme);
	int filenameDenom = tierThresholds[tierIdx] == 0 ? 1 : tierThresholds[tierIdx];
	Image img = StreamRaster::LoadFileAny(AppendFileName(dir, Format("chip_%d.png", filenameDenom)));
	if (img.IsEmpty()) {
		const int cw = 28, ch = 34;
		ImageDraw iw(cw, ch);
		iw.Alpha().DrawRect(0, 0, cw, ch, RGBAZero());
		DrawChipDiscs(iw, cw / 2, ch - 4, amount, &iw.Alpha());
		img = iw;
	}
	cache.Add(tierIdx, img);
	return img;
}

Image GameTable::GetPuckImage(int role)
{
	static String cachedTheme;
	static VectorMap<int, Image> cache;
	if (cachedTheme != currentTableTheme) {
		cache.Clear();
		cachedTheme = currentTableTheme;
	}
	int idx = cache.Find(role);
	if (idx >= 0) return cache[idx];

	// M05-03 (task 0121): the full "what image represents puck role N under
	// theme T" logic (themed-file-load tiers + procedural fallback) now
	// lives in TexasHoldemGetPuckReferenceImage (game/TexasHoldem/
	// TexasHoldemLogicState.h/.cpp), callable without a live GameTable
	// instance so reference/VisualStateLogicCompare can compute the exact
	// same reference images. This method keeps only its existing per-
	// instance cache; rendered output is unchanged (verified byte-for-byte
	// against a recorded frame - see task 0121's evidence).
	Image img = TexasHoldemGetPuckReferenceImage(role, currentTableTheme);
	cache.Add(role, img);
	return img;
}

PlayerCtrl::PlayerCtrl()
{
	Transparent();
	Add(bg);
	Add(label_Avatar);
	Add(pixmapLabel_carda);
	Add(pixmapLabel_cardb);
	Add(pixmapLabel_cards);
	Add(label_PlayerName);
	Add(textLabel_Cash);
	Add(textLabel_Set);
	Add(chipStack_Bet);
	Add(textLabel_Button);
	Add(actionPic);
	Add(label_Timeout);

	pixmapLabel_cards.Transparent();
	pixmapLabel_cards.Hide();
	label_PlayerName.Transparent();
	textLabel_Cash.Transparent();
	textLabel_Set.Transparent();
	label_Timeout.Transparent();

	label_PlayerName.SetInk(White());
	textLabel_Cash.SetInk(Yellow());
	textLabel_Set.SetInk(Yellow());
	label_Timeout.Color(SColorFace());

	// M05-05 (task 0123), Objective 3: chipStack_Bet's slot is a short, wide
	// strip (40x12 in the 190x142 reference grid) while GetChipStackImage's
	// procedural chip-stack graphic is a tall, narrow image (28x34) -- the
	// opposite aspect ratio. The default "fit"/letterbox mode scales that
	// image down to the SHORTER available dimension, so it renders as a tiny
	// ~10px-wide sliver with large empty margins on both sides. "zoom"/cover
	// mode instead fills the slot's width edge-to-edge (cropping the top/
	// bottom of the chip-stack image, itself just a repeated vertical stack
	// of discs, so a cropped view still reads as "a stack of chips") -- a
	// concretely better fit for this specific graphic. Every other
	// ScaledImageCtrl in this class (avatar, hole cards, puck) keeps the
	// default "fit" mode unchanged.
	chipStack_Bet.SetMode(ScaledImageCtrl::ZOOM);
}

void PlayerCtrl::Layout()
{
	Size sz = GetSize();
	double fx = (double)sz.cx / 190.0;
	double fy = (double)sz.cy / 142.0;

	bg.SetRect(0, 0, sz.cx, sz.cy);
	label_Avatar.SetRect((int)(10 * fx), (int)(10 * fy), max(1, (int)(60 * fx)), max(1, (int)(60 * fy)));
	label_PlayerName.SetRect((int)(10 * fx), (int)(75 * fy), max(1, (int)(170 * fx)), max(1, (int)(15 * fy)));
	textLabel_Cash.SetRect((int)(10 * fx), (int)(90 * fy), max(1, (int)(170 * fx)), max(1, (int)(15 * fy)));
	
	pixmapLabel_carda.SetRect((int)(80 * fx), (int)(10 * fy), max(1, (int)(48 * fx)), max(1, (int)(60 * fy)));
	pixmapLabel_cardb.SetRect((int)(110 * fx), (int)(10 * fy), max(1, (int)(48 * fx)), max(1, (int)(60 * fy)));
	pixmapLabel_cards.SetRect((int)(80 * fx), (int)(10 * fy), max(1, (int)(78 * fx)), max(1, (int)(60 * fy)));
	
	// M05-05 (task 0123): SIZE is locked to a single uniform scale factor
	// (fmin(fx,fy)) so the puck rect is always square regardless of the
	// owning PlayerCtrl box's own aspect ratio -- defense-in-depth, holds
	// even if a future form redesign reintroduces non-190:142-proportioned
	// boxes. POSITION keeps using the independent fx/fy (unaffected).
	double f_puck = fmin(fx, fy);
	textLabel_Button.SetRect((int)(140 * fx), (int)(75 * fy), max(1, (int)(32 * f_puck)), max(1, (int)(32 * f_puck)));
	// bet-amount text ("$N") keeps its existing rect but narrowed a bit so the
	// new chip-stack graphic can sit beside it, in the small free strip to the
	// right (x=150..180) that was previously unused in this 190x142 base grid
	// (below the dealer/blind puck at x=140..172,y=75..107, above actionPic
	// at y=120..140) -- additive, textLabel_Set itself is unchanged otherwise.
	textLabel_Set.SetRect((int)(10 * fx), (int)(105 * fy), max(1, (int)(128 * fx)), max(1, (int)(15 * fy)));
	chipStack_Bet.SetRect((int)(140 * fx), (int)(108 * fy), max(1, (int)(40 * fx)), max(1, (int)(12 * fy)));

	actionPic.SetRect((int)(10 * fx), (int)(120 * fy), max(1, (int)(170 * fx)), max(1, (int)(20 * fy)));
	label_Timeout.SetRect((int)(10 * fx), (int)(70 * fy), max(1, (int)(60 * fx)), max(1, (int)(5 * fy)));
}

GameTable::GameTable(const String& provider)
{
	players.Bind(this);
	Sizeable().Zoomable();
	Title(t_("PKR Game Table"));
	AddFrame(menu);
	menu.Set(callback(this, &GameTable::MainMenu));

	RegisterGameTableFormFactories();
	Add(m_form.SizePos());
	String form_path = ResolveFormPath(provider);
	bool form_loaded = m_form.Load(form_path);
	bool layout_ok = false;
	if (form_loaded)
		layout_ok = m_form.Layout("GameTable");
	if (form_loaded && layout_ok)
		BindFormControls();
	else {
		LOG("Could not load GameTable form: " + form_path);
		ASSERT_(false, "Could not load GameTable form");
	}
	HumanButtons().Transparent();
	HumanButtons().Add(BtnFold());
	HumanButtons().Add(BtnCheckCall());
	HumanButtons().Add(BtnBetRaise());
	HumanButtons().Add(BtnAllIn());
	HumanButtons().Add(BtnPot33());
	HumanButtons().Add(BtnPot50());
	HumanButtons().Add(BtnPot100());
	HumanButtons().Add(EditBet());
	HumanButtons().Add(SliderBet());

	TabLeft().Add(ChatCtrl().SizePos(), t_("Chat"));
	HandsImgPane().Background(Color(0, 60, 0));
	HandsImgPane().Add(HandsImg().SizePos());
	TabLeft().Add(HandsImgPane().SizePos(), t_("Hands"));

	TabRight().Add(EngineLogView().SizePos(), t_("Log"));
	TabRight().Add(ProbCtrl().SizePos(), t_("Probabilities"));
	TabRight().Add(AwayLog().SizePos(), t_("Away"));

	LoadTheme("default");

	SliderBet().WhenAction = [this] {
		EditBet().SetData(SliderBet().GetData());
		UpdateActionLabels();
	};
	EditBet().WhenAction = [this] {
		SliderBet().SetData(EditBet().GetData());
		UpdateActionLabels();
	};

	BtnFold().WhenAction = callback(this, &GameTable::OnFold);
	BtnCheckCall().WhenAction = callback(this, &GameTable::OnCheckCall);
	BtnBetRaise().WhenAction = callback(this, &GameTable::OnBetRaise);
	BtnAllIn().WhenAction = callback(this, &GameTable::OnAllIn);

	BtnFold().SetLabel(t_("Fold")); BtnFold().shortcut = "F1";
	BtnCheckCall().SetLabel(t_("Check")); BtnCheckCall().shortcut = "F2";
	BtnBetRaise().SetLabel(t_("Raise")); BtnBetRaise().shortcut = "F3";
	BtnAllIn().SetLabel(t_("All-In")); BtnAllIn().shortcut = "F4";

	BtnPot33().WhenAction = callback(this, &GameTable::OnPot33);
	BtnPot50().WhenAction = callback(this, &GameTable::OnPot50);
	BtnPot100().WhenAction = callback(this, &GameTable::OnPot100);

	BtnPot33().SetLabel("33%");
	BtnPot50().SetLabel("50%");
	BtnPot100().SetLabel("100%");

	HumanButtons().Hide();

	m_script.action_proxy = [this](int act) {
		switch(act) {
			case 0: OnFold(); break;
			case 1: OnCheckCall(); break;
			case 2: OnBetRaise(); break;
			case 3: OnAllIn(); break;
		}
	};
	SetProjectContext("default", "texas-holdem");
	TexasTableLayout::SetProfile(ProviderToLayoutProfile(provider));

	SetTimeCallback(-500, [this] { Timer(); });

	ChatCtrl().btnSend << [this] {
		String msg = ChatCtrl().chatInput.GetData();
		if (!msg.IsEmpty() && m_game && m_game->getPlayerByNumber(0)) {
			ChatCtrl().chatHistory.Append("[" + m_game->getPlayerByNumber(0)->getMyName() + "] " + msg + "\n");
			ChatCtrl().chatInput.SetData("");
		}
	};

	SColorPaper_Write(Color(0, 60, 0));
	SColorText_Write(White());
	SColorFace_Write(Color(0, 60, 0));

	LblPotTitle().SetInk(Color(255, 255, 120)).SetAlign(ALIGN_CENTER).SetFont(StdFont().Bold());
	LblPotTitle() = t_("Pot");
	LblTurnTitle().SetInk(Color(255, 255, 120)).SetAlign(ALIGN_CENTER).SetFont(StdFont().Bold());
	LblPotTotal().SetInk(Yellow()).SetAlign(ALIGN_CENTER);
	LblPotBets().SetInk(Yellow()).SetAlign(ALIGN_CENTER);
	LblGameInfo().SetInk(Yellow()).SetAlign(ALIGN_CENTER);
	LblHandInfo().SetInk(Yellow()).SetAlign(ALIGN_CENTER);

	LblSpeed() = t_("Speed: 4");
	SliderSpeed().Range(10).SetData(4);
	SliderSpeed().WhenAction = callback(this, &GameTable::OnSpeedChange);
	BtnPause().WhenAction = callback(this, &GameTable::OnPause);
	BtnPause().SetLabel(t_("Pause"));

	Board().WhenPaint = [this](Draw& w) { PaintBoard(w); };

	for (int i = 0; i < 5; i++) boardCards[i] = -1;
}

GameTable::~GameTable() {}

void GameTable::Layout()
{
	TopWindow::Layout();
	m_form.RefreshLayoutDeep();
	Refresh();
}

void GameTable::SetGame(std::shared_ptr<class Game> game)
{
	m_game = game;
	if (m_game && m_game->getConfig()) {
		String theme = m_game->getConfig()->readConfigString("TableTheme");
		if (theme.IsEmpty()) theme = "default";
		LoadTheme(theme);
	}
}

void GameTable::MainMenu(Bar& bar)
{
	bar.Add(t_("Actions"), callback(this, &GameTable::ActionsMenu));
	bar.Add(t_("View"), callback(this, &GameTable::ViewMenu));
	bar.Add(t_("Show"), callback(this, &GameTable::ShowMenu));
	bar.Add(t_("Settings"), callback(this, &GameTable::SettingsMenu));
}

void GameTable::ViewMenu(Bar& bar)
{
	bar.Add(t_("Normal View"), callback(this, &GameTable::OnViewNormal)).Key(K_F7);
	bar.Add(t_("Image View"), callback(this, &GameTable::OnViewImage)).Key(K_F8);
	bar.Separator();
	bar.Add(t_("Save Screenshot"), callback(this, &GameTable::OnSaveScreenshot)).Key(K_F9);
}

void GameTable::OnSaveScreenshot()
{
	String path = AppendFileName(GetCurrentDirectory(), "tmp/texas_holdem_screenshot.png");
	if (DumpSnapshot(path, true))
		AppendLog("Screenshot saved to " + path);
}

bool GameTable::DumpSnapshot(const String& path, bool image_mode)
{
	ImageDraw id(GetSize());
	if (image_mode)
		RenderToImage(id);
	else
		RenderNormalSnapshot(id);
	Image img = id;
	return SaveSnapshotPng(path, img);
}

void GameTable::DumpLayoutRects(Stream& out) const
{
	out << "window " << GetRect() << " size=" << GetSize() << "\n";
	out << "form " << m_form.GetRect() << " size=" << m_form.GetSize()
	    << " scale_mode=" << (int)m_form.GetScaleMode() << "\n";
	out << "board " << Board().GetRect() << "\n";
	for(int i = 0; i < 10; i++)
		out << "player" << i << " " << players[i].GetRect() << " rel=" << players[i].IsRelPos() << "\n";
	out << "pot_title " << LblPotTitle().GetRect() << "\n";
	out << "pot_total " << LblPotTotal().GetRect() << "\n";
	out << "pot_bets " << LblPotBets().GetRect() << "\n";
	out << "turn_title " << LblTurnTitle().GetRect() << "\n";
	out << "game_info " << LblGameInfo().GetRect() << "\n";
	out << "hand_info " << LblHandInfo().GetRect() << "\n";
	out << "tab_left " << TabLeft().GetRect() << "\n";
	out << "tab_right " << TabRight().GetRect() << "\n";
	out << "human_buttons " << HumanButtons().GetRect() << "\n";
	out << "btn_all_in " << BtnAllIn().GetRect() << " rel=" << BtnAllIn().IsRelPos() << "\n";
	out << "edit_bet " << EditBet().GetRect() << " rel=" << EditBet().IsRelPos() << "\n";
	out << "slider_bet " << SliderBet().GetRect() << " rel=" << SliderBet().IsRelPos() << "\n";
	out << "btn_bet_raise " << BtnBetRaise().GetRect() << " rel=" << BtnBetRaise().IsRelPos() << "\n";
	out << "btn_check_call " << BtnCheckCall().GetRect() << " rel=" << BtnCheckCall().IsRelPos() << "\n";
	out << "btn_fold " << BtnFold().GetRect() << " rel=" << BtnFold().IsRelPos() << "\n";
	out << "speed_label " << LblSpeed().GetRect() << "\n";
	out << "speed_slider " << SliderSpeed().GetRect() << "\n";
	out << "pause_button " << BtnPause().GetRect() << "\n";
	for (int i = 0; i < 5; i++)
		out << "board_card" << i << " " << TexasTableLayout::BoardCardRect(Board().GetSize(), i) << "\n";
	auto Intersects = [](Rect a, Rect b) {
		return max(a.left, b.left) < min(a.right, b.right) &&
		       max(a.top, b.top) < min(a.bottom, b.bottom);
	};
	Rect pot_total = LblPotTotal().GetRect();
	Rect pot_bets = LblPotBets().GetRect();
	Rect turn_title = LblTurnTitle().GetRect();
	Rect game_info = LblGameInfo().GetRect();
	Rect hand_info = LblHandInfo().GetRect();
	Rect board_cards = TexasTableLayout::BoardCardRect(Board().GetSize(), 0);
	for(int i = 1; i < 5; i++)
		board_cards |= TexasTableLayout::BoardCardRect(Board().GetSize(), i);
	out << "layout_check pot_title_total_overlap=" << (Intersects(LblPotTitle().GetRect(), pot_total) ? 1 : 0)
	    << " pot_bets_hand_overlap=" << (Intersects(pot_bets, hand_info) ? 1 : 0)
	    << " turn_game_overlap=" << (Intersects(turn_title, game_info) ? 1 : 0) << "\n";
	out << "layout_check pot_total_board_cards_overlap=" << (Intersects(pot_total, board_cards) ? 1 : 0)
	    << " pot_title_board_cards_overlap=" << (Intersects(LblPotTitle().GetRect(), board_cards) ? 1 : 0)
	    << " pot_bets_board_cards_overlap=" << (Intersects(pot_bets, board_cards) ? 1 : 0) << "\n";
	int visible_players = 0;
	bool players_uniform = true;
	Size player_size(0, 0);
	bool pot_total_player_overlap = false;
	bool pot_title_player_overlap = false;
	for(int i = 0; i < 10; i++) {
		Rect pr = players[i].GetRect();
		if(pr.GetWidth() <= 1 || pr.GetHeight() <= 1)
			continue;
		visible_players++;
		if(player_size.cx == 0 && player_size.cy == 0)
			player_size = pr.GetSize();
		else
			players_uniform = players_uniform && pr.GetSize() == player_size;
		pot_total_player_overlap = pot_total_player_overlap || Intersects(pr, pot_total);
		pot_title_player_overlap = pot_title_player_overlap || Intersects(pr, LblPotTitle().GetRect());
		out << "layout_check player" << i
		    << "_board_cards_overlap=" << (Intersects(pr, board_cards) ? 1 : 0)
		    << " player_size=" << pr.GetSize() << "\n";
	}
	out << "layout_check visible_players=" << visible_players
	    << " players_uniform=" << (players_uniform ? 1 : 0)
	    << " pot_total_player_overlap=" << (pot_total_player_overlap ? 1 : 0)
	    << " pot_title_player_overlap=" << (pot_title_player_overlap ? 1 : 0)
	    << " uniform_size=" << player_size
	    << " board_cards_bounds=" << board_cards << "\n";
	out.Flush();
}

void GameTable::DumpGameState(Stream& out) const
{
	out << "game_state has_game=" << (m_game ? 1 : 0)
	    << " image_mode=" << (m_imageMode ? 1 : 0)
	    << " table_shown=" << (IsShown() ? 1 : 0)
	    << " size=" << GetSize() << "\n";
	if (!m_game) {
		out.Flush();
		return;
	}

	auto hand = m_game->getCurrentHand();
	auto bero = hand ? hand->getCurrentBeRo() : nullptr;
	out << "game ids game=" << m_game->getMyGameID()
	    << " hand=" << m_game->getCurrentHandID()
	    << " over=" << (m_game->isGameOver() ? 1 : 0)
	    << " players=" << m_game->getStartQuantityPlayers()
	    << " has_hand=" << (hand ? 1 : 0)
	    << " has_bero=" << (bero ? 1 : 0);
	if (bero)
		out << " turn_uid=" << bero->getCurrentPlayersTurnId();
	out << "\n";

	out << "board_cards";
	for (int i = 0; i < 5; i++)
		out << " " << boardCards[i];
	out << "\n";
	out << "labels pot_total_shown=" << (LblPotTotal().IsShown() ? 1 : 0)
	    << " pot_bets_shown=" << (LblPotBets().IsShown() ? 1 : 0)
	    << " turn_shown=" << (LblTurnTitle().IsShown() ? 1 : 0)
	    << " game_shown=" << (LblGameInfo().IsShown() ? 1 : 0)
	    << " hand_shown=" << (LblHandInfo().IsShown() ? 1 : 0) << "\n";
	out << "human_buttons shown=" << (HumanButtons().IsShown() ? 1 : 0)
	    << " rect=" << HumanButtons().GetRect() << "\n";

	int player_count = min(10, m_game->getStartQuantityPlayers());
	for (int i = 0; i < player_count; i++) {
		auto player = m_game->getPlayerByNumber(i);
		out << "player" << i << " exists=" << (player ? 1 : 0)
		    << " ctrl_shown=" << (players[i].IsShown() ? 1 : 0)
		    << " rect=" << players[i].GetRect();
		if (player) {
			int card1 = -2;
			int card2 = -2;
			player->getMyCards(card1, card2);
			out << " uid=" << player->getMyUniqueID()
			    << " active=" << (player->getMyActiveStatus() ? 1 : 0)
			    << " cash=" << player->getMyCash()
			    << " set=" << player->getMySet()
			    << " action=" << (int)player->getMyAction()
			    << " cards=" << card1 << "," << card2
			    << " carda_shown=" << (players[i].pixmapLabel_carda.IsShown() ? 1 : 0)
			    << " cardb_shown=" << (players[i].pixmapLabel_cardb.IsShown() ? 1 : 0)
			    << " cards_shown=" << (players[i].pixmapLabel_cards.IsShown() ? 1 : 0);
		}
		out << "\n";
	}
	out.Flush();
}

void GameTable::OnViewNormal()
{
	if (!m_imageMode) return;
	m_imageMode = false;
	
	// Show all standard UI controls
	board.Show();
	lblPotTitle.Show(); lblPotTotal.Show(); lblPotBets.Show();
	lblTurnTitle.Show(); lblGameInfo.Show(); lblHandInfo.Show();
	tabLeft.Show(); tabRight.Show();
	// humanButtons visibility depends on game state, but let's just refresh layout
	lblSpeed.Show(); sliderSpeed.Show(); btnPause.Show();
	for(int i = 0; i < 10; i++) players[i].Show();
	
	bool hero_turn = false;
	if (m_game && m_game->getCurrentHand()) {
		auto bero = m_game->getCurrentHand()->getCurrentBeRo();
		auto hero = m_game->getPlayerByNumber(0);
		if (bero && hero) {
			int tid = (int)bero->getCurrentPlayersTurnId();
			hero_turn = tid >= 0 && (unsigned)tid == hero->getMyUniqueID();
		}
	}
	if (hero_turn) humanButtons.Show();
	else humanButtons.Hide();

	UpdateActionLabels();
	Refresh();
}

void GameTable::OnViewImage()
{
	if (m_imageMode) return;
	m_imageMode = true;
	
	// Hide all standard UI controls
	board.Hide();
	lblPotTitle.Hide(); lblPotTotal.Hide(); lblPotBets.Hide();
	lblTurnTitle.Hide(); lblGameInfo.Hide(); lblHandInfo.Hide();
	tabLeft.Hide(); tabRight.Hide();
	humanButtons.Hide();
	lblSpeed.Hide(); sliderSpeed.Hide(); btnPause.Hide();
	for(int i = 0; i < 10; i++) players[i].Hide();
	
	Refresh();
}

void GameTable::ShowMenu(Bar& bar)
{
	bar.Add(t_("Full-screen"), callback(this, &GameTable::OnToggleFullScreen)).Key(K_CTRL_F);
	bar.Separator();
	bar.Add(t_("Show/hide chat window"), callback(this, &GameTable::OnToggleChat)).Key(K_CTRL_T);
	bar.Add(t_("Show/hide hands window"), callback(this, &GameTable::OnToggleHands)).Key(K_CTRL_H);
	bar.Add(t_("Show/hide log window"), callback(this, &GameTable::OnToggleLog)).Key(K_CTRL_L);
	bar.Add(t_("Show/hide away window"), callback(this, &GameTable::OnToggleAway)).Key(K_CTRL_A);
	bar.Add(t_("Show/hide probability window"), callback(this, &GameTable::OnToggleProbs));
	bar.Separator();
	bar.Add(t_("Exit"), callback(this, &GameTable::Close));
}

void GameTable::SettingsMenu(Bar& bar)
{
	bar.Add(t_("PKR settings"), callback(this, &GameTable::OnSettings));
}

void GameTable::ActionsMenu(Bar& bar)
{
	bar.Add(t_("Fold"), callback(this, &GameTable::OnFold)).Key(K_F1);
	bar.Add(t_("Check/Call"), callback(this, &GameTable::OnCheckCall)).Key(K_F2);
	bar.Add(t_("Bet/Raise"), callback(this, &GameTable::OnBetRaise)).Key(K_F3);
	bar.Add(t_("All-In"), callback(this, &GameTable::OnAllIn)).Key(K_F4);
}

void GameTable::ToggleTab(TabCtrl& tab, Ctrl& parent, int index)
{
	if (tab.Get() == index && tab.IsVisible()) {
		tab.Hide();
	} else {
		tab.Set(index);
		tab.Show();
	}
}

void GameTable::OnToggleFullScreen()
{
	FullScreen(!IsFullScreen());
}

void GameTable::OnToggleChat() { ToggleTab(tabLeft, board, 0); }
void GameTable::OnToggleHands() { ToggleTab(tabLeft, board, 1); }
void GameTable::OnToggleLog() { ToggleTab(tabRight, board, 0); }
void GameTable::OnToggleAway() { ToggleTab(tabRight, board, 2); }
void GameTable::OnToggleProbs() { ToggleTab(tabRight, board, 1); }

void GameTable::OnSpeedChange()
{
	lblSpeed.SetLabel(Format(t_("Speed: %d"), (int)sliderSpeed.GetData()));
}

int GameTable::GetActionDelayMs() const
{
	int speed = (int)sliderSpeed.GetData();
	return max(0, (10 - speed) * 200);
}

bool GameTable::IsActionFlowPaused() const
{
	return m_isPaused;
}

void GameTable::SetScriptAutomationEnabled(bool enabled)
{
	m_scriptAutomationEnabled = enabled;
}

void GameTable::SetDeferNextHandTransition(bool enabled)
{
	m_deferNextHandTransition = enabled;
	if (!enabled)
		m_nextHandPending = false;
}

bool GameTable::HasPendingNextHand() const
{
	return m_nextHandPending;
}

void GameTable::ResolvePendingNextHand()
{
	if (!m_nextHandPending)
		return;
	m_nextHandPending = false;
	// Deal the next hand now -- the deferred half of the split transition (task 0130).
	OnNextHand();
}

void GameTable::SetProjectContext(const String& project_name, const String& platform_name)
{
	m_scriptProject = TrimBoth(project_name);
	m_scriptPlatform = TrimBoth(platform_name);
	if (m_scriptProject.IsEmpty())
		m_scriptProject = "default";
	if (m_scriptPlatform.IsEmpty())
		m_scriptPlatform = "texas-holdem";

	String code = LoadProjectCommonCode(m_scriptProject, String());
	String platform_code = LoadPlatformCode(m_scriptProject, m_scriptPlatform, String());
	if (!code.IsEmpty() && !platform_code.IsEmpty())
		code << "\n";
	code << platform_code;

	bool loaded = false;
	if (!code.IsEmpty())
		loaded = m_script.Load(code);
	if (!loaded) {
		String fallback = AppendFileName(AppendFileName(GetCurrentDirectory(), "gamescreen/default/platforms"), "texas-holdem.py");
		m_script.LoadFile(fallback);
		m_scriptPlatform = "texas-holdem";
	}
	ApplyProjectThemeMetadata(m_scriptProject, m_scriptPlatform);
}

void GameTable::ApplyProjectThemeMetadata(const String& project_name, const String& platform_name)
{
	String theme_path = GetProjectThemePath(project_name, platform_name);
	m_loadedTheme.objects.Clear();
	m_loadedTheme.settings.Clear();
	m_themeProfile = "default";
	TexasTableLayout::SetProfile(ResolveThemeLayoutProfile(m_loadedTheme, platform_name));
	if (!FileExists(theme_path))
		return;

	try {
		LoadFromJson(m_loadedTheme, LoadFile(theme_path));
	}
	catch (...) {
		m_loadedTheme.objects.Clear();
		m_loadedTheme.settings.Clear();
		return;
	}

	m_themeProfile = ResolveThemeProfile(m_loadedTheme);
	TexasTableLayout::SetProfile(ResolveThemeLayoutProfile(m_loadedTheme, platform_name));

	for (const ThemeObject& o : m_loadedTheme.objects) {
		if (!ThemeObjectVisibleForProfile(o, m_themeProfile))
			continue;
		String t = ToLower(TrimBoth(o.type));
		if (t == "background") {
			String p = ResolveThemeAssetPath(project_name, ThemeGetPropString(o.props, "asset"));
			Image bg = StreamRaster::LoadFileAny(p);
			if (!bg.IsEmpty())
				tableImg = bg;
		}
		else if (t == "card-theme") {
			String theme = ThemeGetPropString(o.props, "theme");
			if (!theme.IsEmpty())
				currentCardTheme = theme;
		}
	}
}

void GameTable::BuildThemeGameState(GameState& st) const
{
	st = GameState();
	if (!m_game)
		return;
	std::shared_ptr<HandInterface> hand = m_game->getCurrentHand();
	if (!hand)
		return;
	std::shared_ptr<BoardInterface> tableBoard = hand->getBoard();
	if (tableBoard) {
		st.pot = tableBoard->getPot();
		const int* cards = tableBoard->getMyCards();
		for (int i = 0; i < 5; i++) {
			if (cards[i] >= 0)
				st.community_cards.Add(cards[i]);
		}
	}
	for (int i = 0; i < m_game->getStartQuantityPlayers(); i++) {
		std::shared_ptr<PlayerInterface> p = m_game->getPlayerByNumber(i);
		if (!p)
			continue;
		GameState::Player& gp = st.players.Add();
		gp.name = p->getMyName();
		gp.stack = p->getMyCash();
		gp.bet = p->getMySet();
		gp.is_active = p->getMyActiveStatus();
		gp.is_dealer = p->getMyButton() == GBUTTON_DEALER;
		gp.is_folded = p->getMyAction() == PLAYER_ACTION_FOLD;
		gp.is_allin = p->getMyAction() == PLAYER_ACTION_ALLIN;
		st.stacks.Add(gp.stack);
	}
	switch (hand->getCurrentRound()) {
		case GAME_STATE_PREFLOP: st.round = ROUND_PREFLOP; break;
		case GAME_STATE_FLOP: st.round = ROUND_FLOP; break;
		case GAME_STATE_TURN: st.round = ROUND_TURN; break;
		case GAME_STATE_RIVER: st.round = ROUND_RIVER; break;
		case GAME_STATE_POST_RIVER: st.round = ROUND_SHOWDOWN; break;
		default: st.round = ROUND_NONE; break;
	}
	st.my_turn = false;
	std::shared_ptr<PlayerInterface> hero = m_game->getPlayerByNumber(0);
	if (hero)
		st.my_turn = hero->getMyTurn();
	st.active_platform = m_scriptPlatform;
}

void GameTable::OnPause()
{
	m_isPaused = !m_isPaused;
	btnPause.SetLabel(m_isPaused ? t_("Play") : t_("Pause"));
}

void GameTable::OnSettings()
{
	if (!m_game || !m_game->getConfig()) return;
	SettingsWindow dlg;
	dlg.Init(*m_game->getConfig());
	if (dlg.Run() == IDOK) {
		String theme = m_game->getConfig()->readConfigString("TableTheme");
		if (theme.IsEmpty()) theme = "default";
		LoadTheme(theme);
		Refresh();
	}
}

void GameTable::UpdateActionLabels()
{
	if (!m_game)
		return;
	auto hand = m_game->getCurrentHand();
	auto p = m_game->getPlayerByNumber(0);
	if (!hand || !p)
		return;
	auto bero = hand->getCurrentBeRo();
	if (!bero)
		return;

	int highestSet = bero->getHighestSet();
	int mySet = p->getMySet();
	int callValue = highestSet - mySet;
	if (callValue > p->getMyCash()) callValue = p->getMyCash();
	if (callValue < 0) callValue = 0;

	btnCheckCall.SetLabel(callValue > 0 ? String(t_("Call\n")) + Format("$%d", callValue) : String(t_("Check\n")));
	
	int currentBet = (int)editBet.GetData();
	int raiseAmount = currentBet - mySet;
	btnBetRaise.SetLabel(callValue > 0 ? String(t_("Raise\n")) + Format("$%d", raiseAmount) : String(t_("Bet\n")) + Format("$%d", raiseAmount));
}

void GameTable::OnFold()
{
	if (m_game) {
		auto hand = m_game->getCurrentHand();
		auto p = m_game->getPlayerByNumber(0);
		if (!p) {
			HideActionButtons();
			return;
		}
		if (hand)
			hand->setPreviousPlayerID(p->getMyID());
		p->setMyAction(PLAYER_ACTION_FOLD, true);
		logPlayerActionMsg(p->getMyName(), PLAYER_ACTION_FOLD, 0);
		nextPlayerAnimation();
	}
	HideActionButtons();
}

void GameTable::HideActionButtons()
{
	humanButtons.Hide();
}

void GameTable::OnCheckCall()
{
	if (m_game) {
		auto hand = m_game->getCurrentHand();
		auto p = m_game->getPlayerByNumber(0);
		auto bero = hand ? hand->getCurrentBeRo() : nullptr;
		if (!p || !bero) {
			HideActionButtons();
			return;
		}
		int highestSet = bero->getHighestSet();
		int mySetBefore = p->getMySet();
		int callValue = highestSet - mySetBefore;
		if (callValue > p->getMyCash()) callValue = p->getMyCash();
		if (callValue < 0) callValue = 0;
		p->setMySet(callValue);
		PlayerAction act;
		if (callValue > 0 && p->getMyCash() == 0) act = PLAYER_ACTION_ALLIN;
		else act = highestSet > mySetBefore ? PLAYER_ACTION_CALL : PLAYER_ACTION_CHECK;
		hand->setPreviousPlayerID(p->getMyID());
		p->setMyAction(act, true);
		logPlayerActionMsg(p->getMyName(), (int)act, callValue);
		nextPlayerAnimation();
	}
	HideActionButtons();
}

void GameTable::OnBetRaise()
{
	if (m_game) {
		auto hand = m_game->getCurrentHand();
		auto p = m_game->getPlayerByNumber(0);
		auto bero = hand ? hand->getCurrentBeRo() : nullptr;
		if (!p || !bero) {
			HideActionButtons();
			return;
		}
		int val = (int)editBet.GetData();
		int mySetBefore = p->getMySet();
		int raiseVal = val - mySetBefore;
		p->setMySet(raiseVal);
		if (p->getMySet() > bero->getHighestSet())
			bero->setHighestSet(p->getMySet());
		hand->setPreviousPlayerID(p->getMyID());
		PlayerAction act = (p->getMyCash() == 0) ? PLAYER_ACTION_ALLIN : PLAYER_ACTION_RAISE;
		p->setMyAction(act, true);
		logPlayerActionMsg(p->getMyName(), act, p->getMySet());
		nextPlayerAnimation();
	}
	HideActionButtons();
}

void GameTable::OnAllIn()
{
	if (m_game) {
		auto hand = m_game->getCurrentHand();
		auto p = m_game->getPlayerByNumber(0);
		auto bero = hand ? hand->getCurrentBeRo() : nullptr;
		if (!p || !bero) {
			HideActionButtons();
			return;
		}
		p->setMySet(p->getMyCash());
		if (p->getMySet() > bero->getHighestSet())
			bero->setHighestSet(p->getMySet());
		hand->setPreviousPlayerID(p->getMyID());
		p->setMyAction(PLAYER_ACTION_ALLIN, true);
		logPlayerActionMsg(p->getMyName(), PLAYER_ACTION_ALLIN, p->getMySet());
		nextPlayerAnimation();
	}
	HideActionButtons();
}

void GameTable::OnPot33()  {
	if (!m_game) return;
	auto hand = m_game->getCurrentHand();
	auto bero = hand ? hand->getCurrentBeRo() : nullptr;
	auto p = m_game->getPlayerByNumber(0);
	if (!bero || !p) return;
	int call = bero->getHighestSet() - p->getMySet();
	int min_raise = max(1, bero->getMinimumRaise());
	int min_total = p->getMySet() + call + min_raise;
	int max_opp_cash = 0;
	for (int i = 0; i < 10; i++) {
		auto q = m_game->getPlayerByNumber(i);
		if (!q || !q->getMyActiveStatus())
			continue;
		if (q->getMyID() != p->getMyID() && q->getMyAction() != PLAYER_ACTION_FOLD)
			max_opp_cash = max(max_opp_cash, q->getMyCash());
	}
	int effective_stack = min(p->getMyCash(), max_opp_cash);
	int target_total = p->getMySet() + max(call, effective_stack * 33 / 100);
	if (target_total > p->getMySet() && target_total < min_total)
		target_total = min_total;
	target_total = min(target_total, p->getMySet() + max(0, effective_stack));
	editBet.SetData(target_total);
	UpdateActionLabels();
}
void GameTable::OnPot50()  {
	if (!m_game) return;
	auto hand = m_game->getCurrentHand();
	auto bero = hand ? hand->getCurrentBeRo() : nullptr;
	auto p = m_game->getPlayerByNumber(0);
	if (!bero || !p) return;
	int call = bero->getHighestSet() - p->getMySet();
	int min_raise = max(1, bero->getMinimumRaise());
	int min_total = p->getMySet() + call + min_raise;
	int max_opp_cash = 0;
	for (int i = 0; i < 10; i++) {
		auto q = m_game->getPlayerByNumber(i);
		if (!q || !q->getMyActiveStatus())
			continue;
		if (q->getMyID() != p->getMyID() && q->getMyAction() != PLAYER_ACTION_FOLD)
			max_opp_cash = max(max_opp_cash, q->getMyCash());
	}
	int effective_stack = min(p->getMyCash(), max_opp_cash);
	int target_total = p->getMySet() + max(call, effective_stack * 50 / 100);
	if (target_total > p->getMySet() && target_total < min_total)
		target_total = min_total;
	target_total = min(target_total, p->getMySet() + max(0, effective_stack));
	editBet.SetData(target_total);
	UpdateActionLabels();
}
void GameTable::OnPot100() {
	if (!m_game) return;
	auto hand = m_game->getCurrentHand();
	auto bero = hand ? hand->getCurrentBeRo() : nullptr;
	auto p = m_game->getPlayerByNumber(0);
	if (!bero || !p) return;
	int call = bero->getHighestSet() - p->getMySet();
	int min_raise = max(1, bero->getMinimumRaise());
	int min_total = p->getMySet() + call + min_raise;
	int max_opp_cash = 0;
	for (int i = 0; i < 10; i++) {
		auto q = m_game->getPlayerByNumber(i);
		if (!q || !q->getMyActiveStatus())
			continue;
		if (q->getMyID() != p->getMyID() && q->getMyAction() != PLAYER_ACTION_FOLD)
			max_opp_cash = max(max_opp_cash, q->getMyCash());
	}
	int effective_stack = min(p->getMyCash(), max_opp_cash);
	int target_total = p->getMySet() + max(call, effective_stack);
	if (effective_stack > 0) target_total = p->getMySet() + effective_stack;
	if (target_total > p->getMySet() && target_total < min_total)
		target_total = min_total;
	target_total = min(target_total, p->getMySet() + max(0, effective_stack));
	editBet.SetData(target_total);
	UpdateActionLabels();
}

void GameTable::LoadTheme(const String& themeName)
{
	currentTableTheme = themeName;
	String dataDir = Tools::GetDataDir();
	String dir = AppendFileName(dataDir, "gfx/gui/table/" + themeName);
	Image img = StreamRaster::LoadFileAny(AppendFileName(dir, "table.png"));
	if (!img.IsEmpty())
		tableImg = img;
	puckDealer = StreamRaster::LoadFileAny(AppendFileName(dir, "dealerPuck.png"));
	puckSB = StreamRaster::LoadFileAny(AppendFileName(dir, "smallblindPuck.png"));
	puckBB = StreamRaster::LoadFileAny(AppendFileName(dir, "bigblindPuck.png"));
	avatarDefault = StreamRaster::LoadFileAny(AppendFileName(dir, "genereticAvatar.png"));
	cardHolderFlop = StreamRaster::LoadFileAny(AppendFileName(dir, "cardholder_flop.png"));
	cardHolderTurn = StreamRaster::LoadFileAny(AppendFileName(dir, "cardholder_turn.png"));
	cardHolderRiver = StreamRaster::LoadFileAny(AppendFileName(dir, "cardholder_river.png"));
	handsImg.SetImage(StreamRaster::LoadFileAny(AppendFileName(dir, "handranking.png")));
	
	String actionDir = AppendFileName(dataDir, "gfx/gui/misc/actionpics");
	actionPics[PLAYER_ACTION_CHECK] = StreamRaster::LoadFileAny(AppendFileName(actionDir, "action_check.png"));
	actionPics[PLAYER_ACTION_CALL] = StreamRaster::LoadFileAny(AppendFileName(actionDir, "action_call.png"));
	actionPics[PLAYER_ACTION_BET] = StreamRaster::LoadFileAny(AppendFileName(actionDir, "action_bet.png"));
	actionPics[PLAYER_ACTION_RAISE] = StreamRaster::LoadFileAny(AppendFileName(actionDir, "action_raise.png"));
	actionPics[PLAYER_ACTION_FOLD] = StreamRaster::LoadFileAny(AppendFileName(actionDir, "action_fold.png"));
	actionPics[PLAYER_ACTION_ALLIN] = StreamRaster::LoadFileAny(AppendFileName(actionDir, "action_allin.png"));
	actionPics[9] = StreamRaster::LoadFileAny(AppendFileName(actionDir, "action_winner.png"));
	
	if (m_game && m_game->getConfig()) {
		currentCardTheme = m_game->getConfig()->readConfigString("CardTheme");
	}
	if (currentCardTheme.IsEmpty()) currentCardTheme = "default_800x480";

	auto LoadBtn = [&](ActionButton& b, String color) {
		b.img = StreamRaster::LoadFileAny(AppendFileName(dir, "playeraction_" + color + ".png"));
		b.img_h = StreamRaster::LoadFileAny(AppendFileName(dir, "playeraction_" + color + "_hover.png"));
		b.img_p = StreamRaster::LoadFileAny(AppendFileName(dir, "playeraction_" + color + "_checked.png"));
	};
	
	LoadBtn(btnCheckCall, "blue");
	LoadBtn(btnBetRaise, "green");
	LoadBtn(btnFold, "red");
	LoadBtn(btnAllIn, "orange");
	LoadBtn(btnPot33, "green");
	LoadBtn(btnPot50, "green");
	LoadBtn(btnPot100, "green");
}

void GameTable::PaintBoard(Draw& w)
{
	Size sz = board.GetSize();
	auto DrawStretched = [&](int x, int y, int cx, int cy, const Image& img) {
		if (img.IsEmpty()) return;
		if (img.GetWidth() == cx && img.GetHeight() == cy)
			w.DrawImage(x, y, img);
		else
			w.DrawImage(x, y, Rescale(img, cx, cy));
	};
	auto DrawFitted = [&](int x, int y, int cx, int cy, const Image& img) {
		if (img.IsEmpty()) return;
		Image fitted = FitCardArt(img, Size(cx, cy));
		w.DrawImage(x + (cx - fitted.GetWidth()) / 2, y + (cy - fitted.GetHeight()) / 2, fitted);
	};
	if (!tableImg.IsEmpty())
		DrawStretched(0, 0, sz.cx, sz.cy, tableImg);
	else {
		w.DrawRect(sz, Color(0, 100, 0));
		w.DrawEllipse(sz.cx / 10, sz.cy / 10, 8 * sz.cx / 10, 8 * sz.cy / 10, Color(0, 80, 0), 2, Gray());
	}
	
	double fx = (double)sz.cx / 1024.0;
	double fy = (double)sz.cy / 648.0;
	
	for (int i = 0; i < 5; i++) {
		Rect br = TexasTableLayout::BoardCardRect(sz, i);
		int x = br.left;
		int y = br.top;
		int w1 = br.GetWidth();
		int h1 = br.GetHeight();
		if (boardCards[i] >= 0) {
			Image img = GetCardImage(boardCards[i]);
			DrawFitted(x, y, w1, h1, img);
		} else {
			Image holder;
			if (i < 3) holder = cardHolderFlop;
			else if (i == 3) holder = cardHolderTurn;
			else holder = cardHolderRiver;
			if (!holder.IsEmpty())
				DrawFitted(x, y, w1, h1, holder);
			else
				w.DrawRect(x, y, w1, h1, Color(0, 80, 0));
		}
	}
}

void GameTable::RenderToImage(Draw& w)
{
	Size sz = GetSize();
	w.DrawRect(sz, Black()); // Clear background
	
	// 1. Draw Board (background + community cards)
	PaintBoard(w);
	
	// 2. Draw Pot Labels
	auto DrawLabel = [&](Label& lbl, Color ink = White()) {
		Rect r = lbl.GetRect();
		Image img = lbl.GetImage();
		if (!img.IsEmpty())
			w.DrawImage(r.left, r.top, img);
		String txt = lbl.GetText();
		if (!txt.IsEmpty())
			w.DrawText(r.left, r.top, txt, lbl.GetFont(), ink);
	};
	DrawLabel(lblPotTitle);
	DrawLabel(lblPotTotal, Yellow());
	DrawLabel(lblPotBets, Yellow());
	DrawLabel(lblTurnTitle);
	DrawLabel(lblGameInfo);
	DrawLabel(lblHandInfo);
	
	// 3. Draw Players
	for (int i = 0; i < 10; i++) {
		PlayerCtrl& p = players[i];
		
		Rect pr = p.GetRect();
		// Draw Player background manually or via child
		// Replicating PlayerBgCtrl::Paint
		bool winner = p.bg.isWinner;
		Color bc = winner ? Color(0, 255, 0) : Color(0, 120, 0);
		w.DrawRect(pr, Black());
		for(int y = 0; y < pr.GetHeight(); y += 2)
			w.DrawRect(pr.left, pr.top + y, pr.GetWidth(), 1, bc);
		DrawFrame(w, pr, winner ? Color(0, 255, 0) : Color(0, 180, 0));
		
		// Draw Player Children
		auto DrawChild = [&](Ctrl& c, Color ink = White()) {
			Rect cr = c.GetRect();
			Rect abs_cr = cr + pr.TopLeft();
			if (dynamic_cast<Label*>(&c)) {
				Label& l = (Label&)c;
				Image limg = l.GetImage();
				if (!limg.IsEmpty())
					w.DrawImage(abs_cr.left, abs_cr.top, limg);
				String txt = l.GetText();
				if (!txt.IsEmpty())
					w.DrawText(abs_cr.left, abs_cr.top, txt, l.GetFont(), ink);
			} else if (dynamic_cast<ScaledImageCtrl*>(&c)) {
				ScaledImageCtrl& si = (ScaledImageCtrl&)c;
				if (!si.img.IsEmpty()) {
					Size isz = si.img.GetSize();
					// M05-05 (task 0123): this is a SEPARATE, hand-rolled
					// re-implementation of ScaledImageCtrl::Paint()'s scaling
					// (GameTable.h) used only by this offscreen/image-mode
					// renderer (RenderToImage, used by --record-session and
					// --dump-render-image -- i.e. every recorded/dumped PNG
					// this whole M05 pipeline inspects) -- it does NOT go
					// through ScaledImageCtrl::Paint() at all, so it must
					// mirror `si.mode` explicitly here too, or SetMode(ZOOM)
					// would be silently dead for every offscreen render even
					// though it works for the live interactive window.
					double r = si.mode == ScaledImageCtrl::ZOOM
						? fmax((double)cr.GetWidth() / isz.cx, (double)cr.GetHeight() / isz.cy)
						: fmin((double)cr.GetWidth() / isz.cx, (double)cr.GetHeight() / isz.cy);
					int w1 = (int)(isz.cx * r);
					int h1 = (int)(isz.cy * r);
					Image out = (w1 == isz.cx && h1 == isz.cy) ? si.img : Rescale(si.img, w1, h1);
					int dx = abs_cr.left + (cr.GetWidth() - w1) / 2;
					int dy = abs_cr.top + (cr.GetHeight() - h1) / 2;
					bool clip = si.mode == ScaledImageCtrl::ZOOM && (w1 > cr.GetWidth() || h1 > cr.GetHeight());
					if (clip) {
						// Clip() (NOT Clipoff() -- Clipoff() ALSO re-bases the
						// draw origin to the clip rect's own top-left, per
						// SDraw::ClipoffOp/DrawOpWin32.cpp, which would make
						// the subsequent DrawImage()'s already-ABSOLUTE dx/dy
						// draw miles outside the (now relocated) origin) just
						// intersects the clip region, leaving coordinates as-is.
						w.Clip(abs_cr);
					}
					w.DrawImage(dx, dy, out);
					if (clip)
						w.End();
				}
			}
		};
		
		DrawChild(p.label_PlayerName);
		DrawChild(p.textLabel_Cash, Yellow());
		DrawChild(p.textLabel_Set, Yellow());
		DrawChild(p.chipStack_Bet);
		DrawChild(p.pixmapLabel_carda);
		DrawChild(p.pixmapLabel_cardb);
		DrawChild(p.pixmapLabel_cards);
		DrawChild(p.textLabel_Button, Yellow());
		DrawChild(p.actionPic);
	}

	// 3b. Objective C: pot chip stack -- a direct procedural Draw call (no
	// new Ctrl, no .form change; m_lblPotTotal is .form-resolved and .form
	// files are not touched/depended-on structurally here), reusing the
	// exact same DrawChipDiscs tier/disc-count logic as the per-player bet
	// stack (B), positioned just above the pot total label's actual rect.
	// Drawn AFTER the player loop (not alongside the other pot labels in
	// step 2 above) because in this table's actual layout (GameTable_PS_6p
	// .form) one seat's PlayerCtrl rect visually overlaps the screen area
	// directly above the pot label -- drawing the chip stack before the
	// player loop let that seat's opaque background paint over and hide it;
	// drawing it last guarantees it stays on top, matching how the seat's
	// own children already draw on top of that seat's own background.
	if (m_potTotalAmount > 0) {
		Rect potRect = lblPotTotal.GetRect();
		DrawChipDiscs(w, potRect.left + potRect.GetWidth() / 2, potRect.top - 6, m_potTotalAmount);
	}

	// 4. Draw Game Log (12 lines)
	Font logFont = StdFont(12).Bold();
	int logY = sz.cy - 12 * (logFont.GetLineHeight() + 2) - 10;
	for (int i = 0; i < m_gameLog.GetCount(); i++) {
		Size tsz = GetTextSize(m_gameLog[i], logFont);
		w.DrawText(sz.cx - tsz.cx - 12, logY, m_gameLog[i], logFont, White());
		logY += logFont.GetLineHeight() + 2;
	}
}

void GameTable::RenderNormalSnapshot(Draw& w)
{
	RenderToImage(w);

	Font panel_font = StdFont(12).Bold();
	auto draw_panel = [&](const Rect& r, const String& title) {
		w.DrawRect(r, Color(35, 35, 35));
		DrawFrame(w, r, Color(90, 90, 90));
		if (!title.IsEmpty())
			w.DrawText(r.left + 6, r.top + 5, title, panel_font, White());
	};
	auto draw_button = [&](ActionButton& b) {
		Rect r = b.GetRect();
		Rect ar = r + humanButtons.GetRect().TopLeft();
		w.DrawRect(ar, Color(50, 90, 140));
		DrawFrame(w, ar, Color(140, 170, 220));
		Vector<String> lines = Split(AsString(b.GetLabel()), '\n');
		int y = ar.top + 4;
		for (const String& s : lines) {
			if (!s.IsEmpty())
				w.DrawText(ar.left + 6, y, s, panel_font, White());
			y += panel_font.GetLineHeight();
		}
	};

	draw_panel(tabLeft.GetRect(), "Tabs");
	draw_panel(tabRight.GetRect(), "Panels");
	draw_panel(humanButtons.GetRect(), "Actions");

	draw_button(btnAllIn);
	draw_button(btnPot33);
	draw_button(btnPot50);
	draw_button(btnPot100);
	draw_button(btnBetRaise);
	draw_button(btnCheckCall);
	draw_button(btnFold);

	Rect er = editBet.GetRect() + humanButtons.GetRect().TopLeft();
	w.DrawRect(er, White());
	DrawFrame(w, er, Color(80, 80, 80));
	w.DrawText(er.left + 4, er.top + 4, AsString((int)editBet.GetData()), panel_font, Black());

	Rect sb = sliderBet.GetRect() + humanButtons.GetRect().TopLeft();
	w.DrawRect(sb, Color(70, 70, 70));
	DrawFrame(w, sb, Color(120, 120, 120));
	Rect ss = sliderSpeed.GetRect();
	w.DrawRect(ss, Color(70, 70, 70));
	DrawFrame(w, ss, Color(120, 120, 120));
	w.DrawText(lblSpeed.GetRect().left, lblSpeed.GetRect().top, lblSpeed.GetText(), panel_font, White());

	Rect pr = btnPause.GetRect();
	w.DrawRect(pr, Color(90, 90, 90));
	DrawFrame(w, pr, Color(160, 160, 160));
	w.DrawText(pr.left + 6, pr.top + 4, btnPause.GetLabel(), panel_font, White());
}

void GameTable::Paint(Draw& w)
{
	if (m_imageMode) {
		RenderToImage(w);
	} else {
		TopWindow::Paint(w);
	}
}

void GameTable::Timer()
{
	if (!m_scriptAutomationEnabled) return;
	if (!m_game || !m_script.IsLoaded()) return;
	
	// 1. Render to Image
	ImageDraw id(GetSize());
	RenderToImage(id);
	Image img = id;
	
	// 2. Run Script
	PyValue instance = m_script.RunCommonMain(img);
	if (instance.IsNone()) return;
	
	Vector<Rect> rects;
	Vector<double> scores;
	m_script.RunPlatformMain(m_scriptPlatform, instance, img, PyValue::None(), rects, scores);
}

void GameTable::initGui(int speed)
{
	for (int i = 0; i < 10; i++) refreshGroupbox(i, 0);
	refreshPot();
	refreshSet();
}

void GameTable::refreshGameLabels(TexasRound state) const {}
void GameTable::nextRoundCleanGui()
{
	for (int i = 0; i < 5; i++) boardCards[i] = -1;
	board.Refresh();
	for (int i = 0; i < 10; i++) {
		players[i].textLabel_Set.SetLabel("");
		players[i].chipStack_Bet.SetImage(Null);
		players[i].actionPic.SetImage(Null);
	}
}

void GameTable::AppendLog(const String& s)
{
	engineLogView.SetQTF(engineLogView.GetQTF() + DeQtf(s) + "&");
	engineLogView.ScrollEnd();
	
	m_gameLog.Add(s);
	while (m_gameLog.GetCount() > 12)
		m_gameLog.Remove(0);
	if (m_imageMode)
		Refresh();
}

void GameTable::logNewGameHandMsg(int gameID, int HandID)
{
	AppendLog(Format("## Game: %d | Hand: %d ##", gameID, HandID));
}

void GameTable::flushLogAtGame(int gameID) {}
void GameTable::logNewBlindsSetsMsg(int sbSet, int bbSet, String sbName, String bbName)
{
	AppendLog(Format("## Blinds: %d / %d ##", sbSet, bbSet));
	AppendLog(Format("%s posts small blind ($%d)", sbName, sbSet));
	AppendLog(Format("%s posts big blind ($%d)", bbName, bbSet));
}

void GameTable::flushLogAtHand() {}
void GameTable::dealHoleCards()
{
	for (int i = 0; i < 10; i++) refreshGroupbox(i, 0);
}

void GameTable::refreshPot()
{
	if (m_game && m_game->getCurrentHand() && m_game->getCurrentHand()->getBoard()) {
		int total = 0;
		int round = 0;
		for (int i = 0; i < 10; i++) {
			auto p = m_game->getPlayerByNumber(i);
			if (!p || !p->getMyActiveStatus())
				continue;
			total += max(0, p->getMyRoundStartCash() - p->getMyCash());
			round += max(0, p->getMySet());
		}
		m_potTotalAmount = total;
		lblPotTotal.SetLabel(Format(t_("Total: $%d"), total));
		lblPotBets.SetLabel(Format(t_("Round: $%d"), round));
		String phase = "Pre-Flop";
		switch (m_game->getCurrentHand()->getCurrentRound()) {
			case GAME_STATE_FLOP: phase = "Flop"; break;
			case GAME_STATE_TURN: phase = "Turn"; break;
			case GAME_STATE_RIVER: phase = "River"; break;
			case GAME_STATE_POST_RIVER: phase = "Showdown"; break;
			default: break;
		}
		lblTurnTitle.SetLabel(phase);
		lblGameInfo.SetLabel(Format("Game %d", m_game->getMyGameID()));
		lblHandInfo.SetLabel(Format("Hand %d", m_game->getCurrentHandID()));
		UpdateProbabilities();
	}
	if (m_imageMode)
		Refresh();
}

void GameTable::refreshSet()
{
	for (int i = 0; i < 10; i++) refreshGroupbox(i, 0);
	if (m_imageMode)
		Refresh();
}

void GameTable::nextPlayerAnimation()
{
	if (m_scriptAutomationEnabled) {
		// Automated/scripted driving (recorder --record-session/--step-actions,
		// --local-game-script): resolve the pending turn-advance synchronously
		// and deterministically instead of scheduling it via SetTimeCallback/
		// PostCallback, which race against real wall-clock time and the
		// recorder's Ctrl::ProcessEvents() pump (non-deterministic across
		// runs). Precedent for gating on this flag: GameTable::Timer()
		// (GameTable.cpp:1290). Real interactive GUI play always has
		// m_scriptAutomationEnabled == false and is unaffected below.
		OnNextPlayer();
		return;
	}
	if (m_isPaused) {
		SetTimeCallback(200, callback(this, &GameTable::nextPlayerAnimation));
		return;
	}
	if (m_game) {
		int delay = GetActionDelayMs(); // 0 to 2 seconds delay
		if (delay > 0)
			SetTimeCallback(delay, callback(this, &GameTable::OnNextPlayer));
		else
			PostCallback(callback(this, &GameTable::OnNextPlayer));
	}
}

void GameTable::OnNextPlayer()
{
	if (!m_game)
		return;
	auto hand = m_game->getCurrentHand();
	if (!hand)
		return;
	auto bero = hand->getCurrentBeRo();
	if (bero)
		bero->nextPlayer();
}

void GameTable::flipHolecardsAllIn() {}
static String CardIdToString(int card)
{
	if (card < 0 || card >= 52) return "";
	static const char* rnk[] = { "2", "3", "4", "5", "6", "7", "8", "9", "T", "J", "Q", "K", "A" };
	static const char* sut[] = { "c", "d", "h", "s" };
	return String(rnk[card % 13]) + sut[card / 13];
}

void GameTable::logDealBoardCardsMsg(int roundID, int card1, int card2, int card3, int card4, int card5)
{
	boardCards[0] = card1;
	boardCards[1] = card2;
	boardCards[2] = card3;
	boardCards[3] = card4;
	boardCards[4] = card5;
	board.Refresh();
	
	String cardsStr = "";
	if (card1 >= 0) {
		cardsStr = " [" + CardIdToString(card1);
		if (card2 >= 0) cardsStr += " " + CardIdToString(card2);
		if (card3 >= 0) cardsStr += " " + CardIdToString(card3);
		if (card4 >= 0) cardsStr += " " + CardIdToString(card4);
		if (card5 >= 0) cardsStr += " " + CardIdToString(card5);
		cardsStr += "]";
	}
	
	String rName = "Preflop";
	switch(roundID) {
		case GAME_STATE_FLOP: rName = "Flop"; break;
		case GAME_STATE_TURN: rName = "Turn"; break;
		case GAME_STATE_RIVER: rName = "River"; break;
	}
	lblTurnTitle.SetLabel(rName);
	if (m_game) {
		lblGameInfo.SetLabel(Format("Game %d", m_game->getMyGameID()));
		lblHandInfo.SetLabel(Format("Hand %d", m_game->getCurrentHandID()));
	}
	AppendLog("*** " + ToUpper(rName) + " ***" + cardsStr);
}

void GameTable::refreshGroupbox(int playerID, int type)
{
	if (playerID >= 0 && playerID < 10 && m_game) {
		auto player = m_game->getPlayerByNumber(playerID);
		if (player && player->getMyActiveStatus()) {
			players[playerID].label_PlayerName.Show();
			players[playerID].textLabel_Cash.Show();
			players[playerID].textLabel_Set.Show();
			players[playerID].label_Avatar.Show();
			players[playerID].textLabel_Button.Show();
			players[playerID].actionPic.Show();
			players[playerID].label_PlayerName.SetLabel(player->getMyName());
			players[playerID].textLabel_Cash.SetLabel(Format("$%d", player->getMyCash()));
			
			int pSet = player->getMySet();
			if (pSet > 0) {
				players[playerID].textLabel_Set.SetLabel(Format("$%d", pSet));
				players[playerID].chipStack_Bet.SetImage(GetChipStackImage(pSet));
			} else {
				players[playerID].textLabel_Set.SetLabel("");
				players[playerID].chipStack_Bet.SetImage(Null);
			}
			
			PlayerAction act = player->getMyAction();
			
			bool isWinner = false;
			if (m_game && m_game->getCurrentHand() && m_game->getCurrentHand()->getBoard()) {
				const Vector<unsigned>& winners = m_game->getCurrentHand()->getBoard()->getWinners();
				for(unsigned id : winners) {
					if (id == player->getMyUniqueID()) {
						isWinner = true;
						break;
					}
				}
			}
			players[playerID].bg.isWinner = isWinner;
			players[playerID].bg.Refresh();

			Image aPic;
			if (isWinner) aPic = actionPics[9];
			else if ((int)act >= 0 && (int)act < 10) aPic = actionPics[(int)act];
			players[playerID].actionPic.SetImage(aPic);
			
			int h1, h2; player->getMyCards(h1, h2);
			if (h1 != -2) {
				bool showdown = (m_game->getCurrentHand() && m_game->getCurrentHand()->getCurrentRound() == GAME_STATE_POST_RIVER);
				bool folded = (player->getMyAction() == PLAYER_ACTION_FOLD);
				
				if (playerID == 0 || m_game->isGameOver() || (showdown && !folded)) {
					if (folded) {
						ImageDraw iw(72, 76);
						iw.Alpha().DrawRect(0, 0, 72, 76, RGBAZero());
						iw.DrawImage(0, 0, GetCardImage(h1));
						iw.DrawImage(26, 0, GetCardImage(h2));
						players[playerID].pixmapLabel_cards.SetImage(ApplyInterlacedTransparency(iw));
						players[playerID].pixmapLabel_cards.Show();
						players[playerID].pixmapLabel_carda.Hide();
						players[playerID].pixmapLabel_cardb.Hide();
					} else {
						players[playerID].pixmapLabel_carda.SetImage(GetCardImage(h1));
						players[playerID].pixmapLabel_cardb.SetImage(GetCardImage(h2));
						players[playerID].pixmapLabel_carda.Show();
						players[playerID].pixmapLabel_cardb.Show();
						players[playerID].pixmapLabel_cards.Hide();
					}
				} else {
					if (folded) {
						ImageDraw iw(72, 76);
						iw.Alpha().DrawRect(0, 0, 72, 76, RGBAZero());
						iw.DrawImage(0, 0, GetCardImage(-1));
						iw.DrawImage(26, 0, GetCardImage(-1));
						players[playerID].pixmapLabel_cards.SetImage(ApplyInterlacedTransparency(iw));
						players[playerID].pixmapLabel_cards.Show();
						players[playerID].pixmapLabel_carda.Hide();
						players[playerID].pixmapLabel_cardb.Hide();
					} else {
						players[playerID].pixmapLabel_carda.SetImage(GetCardImage(-1));
						players[playerID].pixmapLabel_cardb.SetImage(GetCardImage(-1));
						players[playerID].pixmapLabel_carda.Show();
						players[playerID].pixmapLabel_cardb.Show();
						players[playerID].pixmapLabel_cards.Hide();
					}
				}
			} else {
				players[playerID].pixmapLabel_carda.SetImage(Null);
				players[playerID].pixmapLabel_cardb.SetImage(Null);
				players[playerID].pixmapLabel_cards.SetImage(Null);
			}

			String avatarFile = player->getMyAvatar();
			if (!avatarFile.IsEmpty()) {
				players[playerID].label_Avatar.SetImage(StreamRaster::LoadFileAny(avatarFile));
			} else {
				if (!avatarDefault.IsEmpty()) players[playerID].label_Avatar.SetImage(avatarDefault);
				else players[playerID].label_Avatar.SetImage(CtrlImg::exclamation());
			}
			
			String dataDir = Tools::GetDataDir();
			if (player->getMyButton() == GBUTTON_DEALER) {
				Image img = puckDealer;
				if (img.IsEmpty()) img = StreamRaster::LoadFileAny(AppendFileName(dataDir, "gfx/dealer.png"));
				if (img.IsEmpty()) img = GetPuckImage(0); // procedural fallback tier (Objective A)
				players[playerID].textLabel_Button.SetImage(img);
			} else if (player->getMyButton() == GBUTTON_SMALL_BLIND) {
				Image img = puckSB;
				if (img.IsEmpty()) img = StreamRaster::LoadFileAny(AppendFileName(dataDir, "gfx/small_blind.png"));
				if (img.IsEmpty()) img = GetPuckImage(1);
				players[playerID].textLabel_Button.SetImage(img);
			} else if (player->getMyButton() == GBUTTON_BIG_BLIND) {
				Image img = puckBB;
				if (img.IsEmpty()) img = StreamRaster::LoadFileAny(AppendFileName(dataDir, "gfx/big_blind.png"));
				if (img.IsEmpty()) img = GetPuckImage(2);
				players[playerID].textLabel_Button.SetImage(img);
			} else {
				players[playerID].textLabel_Button.SetImage(Null);
			}

		} else {
			players[playerID].label_PlayerName.SetLabel("");
			players[playerID].textLabel_Cash.SetLabel("");
			players[playerID].textLabel_Set.SetLabel("");
			players[playerID].chipStack_Bet.SetImage(Null);
			players[playerID].label_PlayerName.Hide();
			players[playerID].textLabel_Cash.Hide();
			players[playerID].textLabel_Set.Hide();
			players[playerID].bg.isWinner = false;
			players[playerID].bg.Refresh();
			players[playerID].pixmapLabel_carda.SetImage(Null);
			players[playerID].pixmapLabel_cardb.SetImage(Null);
			players[playerID].pixmapLabel_cards.SetImage(Null);
			players[playerID].pixmapLabel_carda.Hide();
			players[playerID].pixmapLabel_cardb.Hide();
			players[playerID].pixmapLabel_cards.Hide();
			players[playerID].label_Avatar.SetImage(Null);
			players[playerID].label_Avatar.Hide();
			players[playerID].textLabel_Button.SetImage(Null);
			players[playerID].textLabel_Button.Hide();
			players[playerID].actionPic.SetImage(Null);
			players[playerID].actionPic.Hide();
		}
	}
	if (m_imageMode)
		Refresh();
}

void GameTable::preflopAnimation1() {}
void GameTable::flopAnimation1() {}
void GameTable::turnAnimation1() {}
void GameTable::riverAnimation1() {}
void GameTable::postRiverAnimation1()
{
	if (m_scriptAutomationEnabled) {
		// Automated/scripted driving (recorder --record-session/--step-actions,
		// --local-game-script): resolve the pending next-hand transition
		// synchronously and deterministically instead of scheduling it via
		// SetTimeCallback, which races against real wall-clock time and the
		// recorder's Ctrl::ProcessEvents() pump (non-deterministic across
		// runs). Same precedent/pattern as GameTable::nextPlayerAnimation()
		// above and GameTable::Timer() (GameTable.cpp:1290). Real interactive
		// GUI play always has m_scriptAutomationEnabled == false and is
		// unaffected below.
		if (m_deferNextHandTransition) {
			// task 0130 recorder-only opt-in: do NOT deal the next hand here.
			// The showdown reveal was just pushed into the GUI (refreshCash()
			// in LocalHand::switchRounds, immediately before this call). Set a
			// pending flag and return so the recorder's step loop can capture a
			// frame with the reveal visible, then resolve the deal on a later
			// step via ResolvePendingNextHand(). Splits one atomic transition
			// into two recorder-visible steps.
			m_nextHandPending = true;
			return;
		}
		OnNextHand();
		return;
	}
	if (m_isPaused) {
		SetTimeCallback(500, callback(this, &GameTable::postRiverAnimation1));
		return;
	}
	int speed = (int)sliderSpeed.GetData();
	int delay = (10 - speed) * 500 + 2000; // Delay to show winners
	SetTimeCallback(delay, callback(this, &GameTable::OnNextHand));
}

void GameTable::OnNextHand()
{
	if (m_game) {
		if (m_game->isGameOver()) {
			// Under script automation (recorder --record-session/--step-actions,
			// --local-game-script) this can now be reached synchronously, deep
			// inside the same call stack as the step that triggered it (see
			// postRiverAnimation1() above) -- e.g. if the recursive cascade of
			// automated hands actually eliminates every player but one before
			// the driving --frames budget is exhausted. PromptOK() is a modal,
			// blocking dialog with no automated dismissal path; calling it here
			// would hang a headless/automated session forever waiting for a
			// user click that will never come. Real interactive GUI play
			// (m_scriptAutomationEnabled == false) keeps the exact original
			// behavior -- the player still sees the "Game Over!" dialog.
			if (!m_scriptAutomationEnabled)
				PromptOK(t_("Game Over!"));
			return;
		}
		m_game->initHand();
		m_game->startHand();
	}
}

void GameTable::logPlayerActionMsg(String playName, int action, int setValue) {
	String actStr;
	switch(action) {
		case PLAYER_ACTION_FOLD: actStr = "folds."; break;
		case PLAYER_ACTION_CALL: actStr = Format("calls $%d.", setValue); break;
		case PLAYER_ACTION_CHECK: actStr = "checks."; break;
		case PLAYER_ACTION_BET: actStr = Format("bets $%d.", setValue); break;
		case PLAYER_ACTION_RAISE: actStr = Format("raises to $%d.", setValue); break;
		case PLAYER_ACTION_ALLIN: actStr = "is all-in."; break;
		case PLAYER_ACTION_SMALL_BLIND: actStr = Format("posts small blind ($%d)", setValue); break;
		case PLAYER_ACTION_BIG_BLIND: actStr = Format("posts big blind ($%d)", setValue); break;
		default: actStr = AsString(action); break;
	}
	AppendLog("[" + playName + "] " + actStr);
}

void GameTable::logFlipHoleCardsMsg(String playerName, int card1, int card2, int cardsValueInt, String showHas)
{
	String handName = CardsValue::determineHandName(cardsValueInt, nullptr);
	AppendLog("[" + playerName + "] " + showHas + " " + handName);
}

void GameTable::logWinningHandMsg(String playerName, String handName, int amount)
{
	AppendLog(Format("[%s] wins $%d with %s", playerName, amount, handName));
}

void GameTable::UpdateProbabilities()
{
	for(int i = 0; i < probCtrl.items.GetCount(); i++) {
		if (i == 0) probCtrl.items[i].prob = 0.0; // Royal Flush 0%
		else probCtrl.items[i].prob = (double)Random(50) / 100.0;
	}
	probCtrl.Refresh();
}

void GameTable::dealBeRoCards(TexasRound state) {}
void GameTable::beRoAnimation2(TexasRound state) {}

void GameTable::meInAction()
{
	if (m_imageMode) {
		humanButtons.Hide();
		return;
	}
	humanButtons.Show();
	
	if (m_game && m_game->getCurrentHand()) {
		auto p = m_game->getPlayerByNumber(0);
		auto bero = m_game->getCurrentHand()->getCurrentBeRo();
		if (!p || !bero)
			return;
		
		int highestSet = bero->getHighestSet();
		int mySet = p->getMySet();
		int callValue = highestSet - mySet;
		int minRaise = bero->getMinimumRaise();
		
		int minBet = callValue + minRaise;
		int maxBet = p->getMyCash();
		
		if (minBet > maxBet) minBet = maxBet;
		// SliderCtrl crashes on zero span (max == min) on some paths/backends.
		// Keep a non-zero slider range and disable raise controls when no raise is possible.
		int sliderMin = minBet;
		int sliderMax = max(maxBet, minBet + 1);
		sliderBet.MinMax(sliderMin, sliderMax);
		sliderBet.SetData(minBet);
		editBet.MinMax(minBet, maxBet);
		editBet.SetData(minBet);
		bool canRaise = maxBet > minBet;
		sliderBet.Enable(canRaise);
		editBet.Enable(canRaise);
		btnBetRaise.Enable(canRaise);
		btnPot33.Enable(canRaise);
		btnPot50.Enable(canRaise);
		btnPot100.Enable(canRaise);
		
		UpdateActionLabels();
	}
}

void GameTable::postRiverRunAnimation1() {}
void GameTable::refreshCash()
{
	for (int i = 0; i < 10; i++) refreshGroupbox(i, 0);
}
void GameTable::refreshAction(int playerID, int action)
{
	refreshGroupbox(playerID, 0);
}
void GameTable::SignalNetClientError(int errorId, int osErrorCode)
{
	PromptOK(Format("Network Error: %d (OS: %d)", errorId, osErrorCode));
}

void GameTable::SetRemoteState(const ScreenGameState& state)
{
	if (!state.found) return;
	
	m_potTotalAmount = state.pot;
	lblPotTotal.SetLabel(Format(t_("Total: $%d"), state.pot));
	
	for (int i = 0; i < 10; i++) {
		if (i < state.stacks.GetCount()) {
			players[i].textLabel_Cash.SetLabel(Format("$%d", state.stacks[i]));
		}
	}
	
	// Update board cards
	for (int i = 0; i < 5; i++) {
		boardCards[i] = (i < state.community_cards.GetCount()) ? state.community_cards[i] : -2;
	}
	
	Refresh();
}

END_UPP_NAMESPACE
