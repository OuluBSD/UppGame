#ifndef _game_Hearts_HeartsCtrl_h_
#define _game_Hearts_HeartsCtrl_h_

class HeartsCtrl;

struct VisualCard : public Moveable<VisualCard> {
	Card card;
	Rect rect;
};

struct AnimCard : public Moveable<AnimCard> {
	Card card;
	Point src;
	Point dst;
	double progress; // 0.0 to 1.0
	int angle;
	bool back;
};

class HeartsAnimationLayer : public Ctrl {
public:
	HeartsCtrl *owner = nullptr;

	typedef HeartsAnimationLayer CLASSNAME;
	virtual void Paint(Draw& w) override;
};

class HeartsCtrl : public Ctrl {
public:
	typedef HeartsCtrl CLASSNAME;
	friend class HeartsAnimationLayer;

	enum Difficulty {
		DIFFICULTY_EASY,
		DIFFICULTY_MEDIUM,
		DIFFICULTY_HARD
	};

	CardGameState state;
	Difficulty difficulty;

	// UI Buttons
	Form ui;
	HeartsAnimationLayer anim_layer;

	// Animation state
	Vector<AnimCard> anim_cards;
	bool animating;

	// Game loop settings
	Vector<Card> selected_cards;
	bool autoplay_enabled;
	bool started;
	int pending_pass_player;
	bool pass_animating;
	bool collecting_trick;
	int collecting_winner;
	int collecting_points;
	bool debug_render_trace;
	bool human_card_animating;
	double human_card_anim_progress;

	HeartsCtrl();
	void SetDebugRenderTrace(bool b = true) { debug_render_trace = b; }

	void StartGame();
	void HandleUiSignal(const String& path, const String& op, const String& action);
	void ResetGame();
	void RefreshUI();
	void UpdateHUD();
	void UpdateForm();
	void DumpLayout();
	Image RenderSnapshot();
	void ScheduleAiStep(int delay_ms);
	void AiStep();
	void StartTrickCollect();
	void FinishTrickCollect();
	void StartPassAnimation();
	void FinishPassAnimation();
	void NextRound();
	void FinishAutoplayIfNeeded();
	void ShowHighScores();
	void ShowAbout();

	void OnCardClick(const Card& card);
	void OnPassClick();
	void OnClearClick();

	// Layout and Paint
	virtual void Paint(Draw& w) override;
	virtual Image MouseEvent(int event, Point p, int zdelta, dword keyflags) override;
	virtual void LeftDown(Point p, dword flags) override;
	virtual void ChildMouseEvent(Ctrl *child, int event, Point p, int zdelta, dword keyflags) override;
	virtual void Layout() override;

private:
	Vector<VisualCard> human_card_rects; // Cached for hit testing
	Vector<Rect> human_card_base_rects;
	Vector<String> human_card_image_keys;
	Vector<String> trick_card_image_keys;
	Vector<Rect> human_card_anim_from;
	Vector<Rect> human_card_anim_to;

	void StartHumanCardAnimation(const Vector<Rect>& target);
	void ApplyHumanCardAnimation(double t);
	static bool RectsEqual(const Rect& a, const Rect& b);
	void AnimateStep();
	void AnimateOverlayStep();
	void AddCardAnimation(const Card& card, Point src, Point dst, int angle, bool back = false);
	Point GetHandCenter(int player_idx) const;
	Point GetTrickCenter(int player_idx) const;
	Rect GetCardRect(int player_idx, const Card& card) const;
	Point GetCardCenter(int player_idx, const Card& card) const;
	void PreparePassAnimation(int player_idx, const Vector<Card>& cards);
	bool WillCompletePassWith(int player_idx) const;
	bool HasAnimatingCard(const String& card_id) const;
	String GetRenderedCardKey(const Card& card, bool back) const;
	Image LoadCardImage(const String& card_id, bool back = false, Size target = Size(96, 136));
	Image LoadCardImage(const Card& card, bool back = false, Size target = Size(96, 136));
	void TraceRenderedCard(const String& ctrl_name, const String& ctrl_kind, const String& card_id, const String& file_name, const String& resolved_path, const String& load_outcome, const Size& image_size, const Rect& screen_rect) const;
	void PaintAnimations(Draw& w);
};

#endif
