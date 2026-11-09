#ifndef _CardEngine_GameTable_h_
#define _CardEngine_GameTable_h_

#include <CtrlLib/CtrlLib.h>
#include <Form/Form.hpp>
#include <memory>
#include <GameRules/GuiInterface.h>
#include <EditorCommon/GameTheme.h>
#include "ScreenGameState.h"
#include <EditorCommon/Scripting.h>

NAMESPACE_UPP

class BoardCtrl : public Ctrl {
public:
	Event<Draw&> WhenPaint;
	BoardCtrl() { Transparent(); }
	virtual void Paint(Draw& w) override {
		WhenPaint(w);
	}
};

class PlayerBgCtrl : public Ctrl {
public:
	bool isWinner = false;
	virtual void Paint(Draw& w) override {
		Size sz = GetSize();
		w.DrawRect(sz, Black());
		Color c = isWinner ? Color(0, 255, 0) : Color(0, 120, 0);
		for(int y = 0; y < sz.cy; y += 2)
			w.DrawRect(0, y, sz.cx, 1, c);
		DrawFrame(w, 0, 0, sz.cx, sz.cy, isWinner ? Color(0, 255, 0) : Color(0, 180, 0));
	}
};

class ActionButton : public Button {
public:
	Image img, img_h, img_p;
	String shortcut;
	ActionButton() { Transparent(); }
	virtual void Paint(Draw& w) override {
		Image i = img;
		if (IsPush()) i = img_p;
		else if (HasMouse()) i = img_h;
		if (i.IsEmpty()) { Button::Paint(w); return; }
		
		Size sz = GetSize();
		if (i.GetSize() == sz)
			w.DrawImage(0, 0, i);
		else
			w.DrawImage(0, 0, Rescale(i, sz.cx, sz.cy));
		
		String lbl = GetLabel();
		Font font = StdFont().Bold();
		Vector<String> lines = Split(lbl, '\n');
		int lineH = font.GetLineHeight();
		int totalH = (int)lines.GetCount() * lineH;
		int y = (sz.cy - totalH) / 2;
		for(const String& s : lines) {
			Size tsz = GetTextSize(s, font);
			w.DrawText((sz.cx - tsz.cx) / 2, y, s, font, White());
			y += lineH;
		}
		
		if (!shortcut.IsEmpty()) {
			Font sfont = StdFont(10).Bold();
			Size stsz = GetTextSize(shortcut, sfont);
			w.DrawText(sz.cx - stsz.cx - 4, sz.cy - stsz.cy - 2, shortcut, sfont, Gray());
		}
	}
};

class ScaledImageCtrl : public Ctrl {
public:
	// M05-05 (task 0123), Objective 3: FIT (default, today's only behavior)
	// uniform-scales by min(sz/isz) on each axis, centered, letterboxed
	// (whole image always visible, empty space on the shorter axis). ZOOM/
	// cover instead uniform-scales by max(sz/isz), centered, and is CLIPPED
	// to the control's own bounds (slot filled edge-to-edge on both axes,
	// the image's edges may be cropped, never letterboxed). Every EXISTING
	// caller keeps the default FIT mode -- only an explicit SetMode(ZOOM)
	// call opts a control into the new behavior.
	enum Mode { FIT, ZOOM };

	Image img;
	Mode mode = FIT;
	ScaledImageCtrl() { Transparent(); }
	void SetImage(const Image& i) { img = i; Refresh(); }
	void SetMode(Mode m) { mode = m; Refresh(); }
	virtual void Paint(Draw& w) override {
		Size sz = GetSize();
		if (!img.IsEmpty()) {
			Size isz = img.GetSize();
			if (isz.cx > 0 && isz.cy > 0) {
				double r = mode == ZOOM
					? fmax((double)sz.cx / isz.cx, (double)sz.cy / isz.cy)
					: fmin((double)sz.cx / isz.cx, (double)sz.cy / isz.cy);
				int w1 = (int)(isz.cx * r);
				int h1 = (int)(isz.cy * r);
				bool clip = mode == ZOOM && (w1 > sz.cx || h1 > sz.cy);
				if (clip) {
					// Clip(), not Clipoff() -- Clipoff() also re-bases the
					// draw origin to the clip rect's own top-left (see
					// GameTable.cpp's RenderToImage DrawChild lambda for the
					// concrete bug that bit -- harmless here only because
					// this rect already starts at (0,0), but Clip() is the
					// semantically correct call: intersect, don't re-offset).
					w.Clip(0, 0, sz.cx, sz.cy);
				}
				if (w1 == isz.cx && h1 == isz.cy)
					w.DrawImage((sz.cx - w1) / 2, (sz.cy - h1) / 2, img);
				else
					w.DrawImage((sz.cx - w1) / 2, (sz.cy - h1) / 2, Rescale(img, w1, h1));
				if (clip)
					w.End();
			}
		}
	}
};

class PlayerCtrl : public ParentCtrl {
public:
	PlayerBgCtrl bg;
	ScaledImageCtrl label_Avatar;
	Label label_PlayerName;
	Label textLabel_Cash;
	Label textLabel_Set;
	ScaledImageCtrl pixmapLabel_carda;
	ScaledImageCtrl pixmapLabel_cardb;
	ScaledImageCtrl pixmapLabel_cards;
	ScaledImageCtrl textLabel_Button;
	ScaledImageCtrl chipStack_Bet;
	ScaledImageCtrl actionPic;
	StaticRect label_Timeout;

	PlayerCtrl();
	virtual void Layout() override;
};

class PokerChatCtrl : public ParentCtrl {
public:
	DocEdit chatHistory;
	EditField chatInput;
	Button btnSend;

	PokerChatCtrl();
	virtual void Layout() override;
};

class ProbabilityCtrl : public Ctrl {
public:
	struct Item {
		String title;
		double prob;
	};
	Array<Item> items;
	
	ProbabilityCtrl() {
		const char* titles[] = {
			"Royal Flush", "Straight Flush", "Four of a Kind", "Full House",
			"Flush", "Straight", "Three of a Kind", "Two Pair", "One Pair", "Highest Card"
		};
		for(const char* t : titles) {
			Item& it = items.Add();
			it.title = t;
			it.prob = 0.0;
		}
	}
	
	virtual void Paint(Draw& w) override {
		Size sz = GetSize();
		w.DrawRect(sz, Color(0, 60, 0));
		int count = (int)items.GetCount();
		if (count == 0) return;
		int h = sz.cy / count;
		for(int i = 0; i < count; i++) {
			int y = i * h;
			w.DrawText(5, y + (h - 15) / 2, items[i].title, StdFont().Bold(), White());
			int barMaxW = sz.cx - 150;
			int barW = (int)(barMaxW * items[i].prob);
			if (barW > 0) {
				for(int x = 0; x < barW; x++) {
					Color c = Blend(Color(0, 0, 255), Color(255, 255, 0), (int)(255 * x / barMaxW));
					w.DrawRect(100 + x, y + 5, 1, h - 10, c);
				}
			}
			w.DrawText(sz.cx - 45, y + (h - 15) / 2, Format("%.1f%%", items[i].prob * 100), StdFont().Bold(), White());
		}
	}
};

class GameTable : public TopWindow, public GuiInterface {
public:
	typedef GameTable CLASSNAME;
	GameTable(const String& provider = String());
	virtual ~GameTable();

	void SetGame(std::shared_ptr<class Game> game) override;

	// GuiInterface implementation
	virtual void initGui(int speed) override;
	virtual void refreshGameLabels(TexasRound state) const override;
	virtual void nextRoundCleanGui() override;
	virtual void logNewGameHandMsg(int gameID, int HandID) override;
	virtual void flushLogAtGame(int gameID) override;
	virtual void logNewBlindsSetsMsg(int sbSet, int bbSet, String sbName, String bbName) override;
	virtual void flushLogAtHand() override;
	virtual void dealHoleCards() override;
	virtual void refreshPot() override;
	virtual void refreshSet() override;
	virtual void nextPlayerAnimation() override;
	virtual void flipHolecardsAllIn() override;
	virtual void logDealBoardCardsMsg(int roundID, int card1, int card2, int card3, int card4 = -1, int card5 = -1) override;
	virtual void refreshGroupbox(int playerID, int type) override;
	virtual void preflopAnimation1() override;
	virtual void flopAnimation1() override;
	virtual void turnAnimation1() override;
	virtual void riverAnimation1() override;
	virtual void postRiverAnimation1() override;
	virtual void logPlayerActionMsg(String playName, int action, int setValue) override;
	virtual void logFlipHoleCardsMsg(String playerName, int card1, int card2, int cardsValueInt = -1, String showHas = "shows") override;
	virtual void logWinningHandMsg(String playerName, String handName, int amount) override;
	virtual void dealBeRoCards(TexasRound state) override;
	virtual void beRoAnimation2(TexasRound state) override;
	virtual void meInAction() override;
	virtual void postRiverRunAnimation1() override;
	virtual void refreshCash() override;
	virtual void refreshAction(int playerID, int action) override;
	virtual void SignalNetClientError(int errorId, int osErrorCode) override;

	void SetRemoteState(const ScreenGameState& state);

	void PaintBoard(Draw& w);
	void RenderToImage(Draw& w);
	void RenderNormalSnapshot(Draw& w);
	bool DumpSnapshot(const String& path, bool image_mode);
	void DumpLayoutRects(Stream& out) const;
	void DumpGameState(Stream& out) const;
	void LoadTheme(const String& themeName);
	void SetProjectContext(const String& project_name, const String& platform_name = "texas-holdem");
	void UpdateProbabilities();
	int GetActionDelayMs() const;
	bool IsActionFlowPaused() const;
	void SetScriptAutomationEnabled(bool enabled);
	// Recorder-only opt-in (task 0130): when enabled together with script
	// automation, postRiverAnimation1() defers the next-hand deal instead of
	// dealing it synchronously in the same call stack, so the recorder can
	// capture a frame showing the showdown reveal before the next hand is
	// dealt. Default false -> every existing consumer keeps today's behavior.
	void SetDeferNextHandTransition(bool enabled);
	bool HasPendingNextHand() const;
	void ResolvePendingNextHand();

	virtual void Layout() override;
	virtual void Paint(Draw& w) override;
	virtual void Timer();

private:
	class PlayersProxy {
	public:
		PlayersProxy() = default;
		explicit PlayersProxy(GameTable* owner) : owner(owner) {}
		void Bind(GameTable* table) { owner = table; }
		PlayerCtrl& operator[](int index) const;
	private:
		GameTable* owner = nullptr;
	};

	void MainMenu(Bar& bar);
	void ViewMenu(Bar& bar);
	void ShowMenu(Bar& bar);
	void SettingsMenu(Bar& bar);
	void ActionsMenu(Bar& bar);

	public: void OnViewNormal();
	void OnViewImage();
	void OnSaveScreenshot();

	void ToggleTab(TabCtrl& tab, Ctrl& pane, int index);
	void OnSettings();

	void OnToggleFullScreen();
	void OnToggleChat();
	void OnToggleHands();
	void OnToggleLog();
	void OnToggleAway();
	void OnToggleProbs();
	void OnSpeedChange();
	void OnNextPlayer();
	void OnNextHand();
	void OnPause();
	void AppendLog(const String& s);

	void OnFold();
	void OnCheckCall();
	void OnBetRaise();
	void OnAllIn();
	void OnPot33();
	void OnPot50();
	void OnPot100();
	void HideActionButtons();
	void UpdateActionLabels();
	void ApplyProjectThemeMetadata(const String& project_name, const String& platform_name);
	void BuildThemeGameState(GameState& st) const;
	void BindFormControls();
	static String ResolveFormPath(const String& provider = String());
	static String ProviderToLayoutProfile(const String& provider);
	template <class T> T& CtrlAs(const String& name) const;

	bool m_imageMode = false;
	bool m_scriptAutomationEnabled = false;
	bool m_deferNextHandTransition = false; // task 0130: recorder-only opt-in
	bool m_nextHandPending = false;         // task 0130: a deferred next-hand deal is queued
	GameScript m_script;
	String m_scriptProject = "default";
	String m_scriptPlatform = "texas-holdem";
	ThemeFile m_loadedTheme;
	String m_themeProfile = "default";
	Vector<String> m_gameLog;

	bool m_isPaused = false;
	bool m_isFullScreen = false;
	std::shared_ptr<class Game> m_game;
	MenuBar menu;

	Form m_form;
	PlayersProxy players;
	BoardCtrl* m_board = nullptr;
	Label* m_lblPotTitle = nullptr;
	Label* m_lblPotTotal = nullptr;
	Label* m_lblPotBets = nullptr;
	Label* m_lblTurnTitle = nullptr;
	Label* m_lblGameInfo = nullptr;
	Label* m_lblHandInfo = nullptr;
	TabCtrl* m_tabLeft = nullptr;
	TabCtrl* m_tabRight = nullptr;
	ParentCtrl* m_humanButtons = nullptr;
	ActionButton* m_btnFold = nullptr;
	ActionButton* m_btnCheckCall = nullptr;
	ActionButton* m_btnBetRaise = nullptr;
	ActionButton* m_btnAllIn = nullptr;
	ActionButton* m_btnPot33 = nullptr;
	ActionButton* m_btnPot50 = nullptr;
	ActionButton* m_btnPot100 = nullptr;
	EditInt* m_editBet = nullptr;
	SliderCtrl* m_sliderBet = nullptr;
	Label* m_lblSpeed = nullptr;
	SliderCtrl* m_sliderSpeed = nullptr;
	Button* m_btnPause = nullptr;
	PokerChatCtrl* m_chatCtrl = nullptr;
	StaticRect* m_handsImgPane = nullptr;
	ImageCtrl* m_handsImg = nullptr;
	ProbabilityCtrl* m_probCtrl = nullptr;
	RichTextView* m_engineLogView = nullptr;
	RichTextView* m_awayLog = nullptr;
	PlayerCtrl* m_player[10] = {};
	int boardCards[5];

	Image tableImg;
	Image puckDealer, puckSB, puckBB;
	Image avatarDefault;
	Image cardHolderFlop, cardHolderTurn, cardHolderRiver;
	Image actionPics[10];
	String currentCardTheme;
	String currentTableTheme;
	int m_potTotalAmount = 0;

	Image GetCardImage(int card);
	Image GetTransparentCard(int card, int alpha);
	Image ApplyInterlacedTransparency(const Image& img);
	Image GetPuckImage(int role);
	Image GetChipStackImage(int amount);
	static void DrawChipDiscs(Draw& w, int cx, int baseY, int amount, Draw* alphaW = nullptr);

public:
	BoardCtrl& Board() const;
	Label& LblPotTitle() const;
	Label& LblPotTotal() const;
	Label& LblPotBets() const;
	Label& LblTurnTitle() const;
	Label& LblGameInfo() const;
	Label& LblHandInfo() const;
	TabCtrl& TabLeft() const;
	TabCtrl& TabRight() const;
	ParentCtrl& HumanButtons() const;
	ActionButton& BtnFold() const;
	ActionButton& BtnCheckCall() const;
	ActionButton& BtnBetRaise() const;
	ActionButton& BtnAllIn() const;
	ActionButton& BtnPot33() const;
	ActionButton& BtnPot50() const;
	ActionButton& BtnPot100() const;
	EditInt& EditBet() const;
	SliderCtrl& SliderBet() const;
	Label& LblSpeed() const;
	SliderCtrl& SliderSpeed() const;
	Button& BtnPause() const;
	PokerChatCtrl& ChatCtrl() const;
	StaticRect& HandsImgPane() const;
	ImageCtrl& HandsImg() const;
	ProbabilityCtrl& ProbCtrl() const;
	RichTextView& EngineLogView() const;
	RichTextView& AwayLog() const;
	PlayerCtrl& PlayerAt(int index) const;
};

END_UPP_NAMESPACE

#endif
