#ifndef _game_CardGame_GameState_h_
#define _game_CardGame_GameState_h_

struct TrickItem : public Moveable<TrickItem> {
	int player_index;
	Card card;

	TrickItem();
	TrickItem(int p_idx, const Card& c);
};

struct PlayResult {
	bool success;
	String message;

	PlayResult(bool s, const String& m);
};

class CardGameState {
public:
	Vector<Card> players[4];
	int scores[4];
	int round_scores[4];
	Vector<TrickItem> trick;
	bool hearts_broken;
	int turn;
	String leading_suit;
	int round_number;
	String phase; // "PASSING", "PLAYING", "ROUND_END", "GAME_OVER"
	Vector<Card> passed_cards[4];
	int last_trick_winner;
	int last_trick_points;
	int last_round_scores[4];
	int last_round_moon_shooter;
	bool game_over;
	bool trick_pending;
	int pending_trick_winner;
	int pending_trick_points;

	Event<const String&> LogCallback;

	CardGameState();

	void Log(const String& msg);
	void Deal();
	bool SelectPass(int player_index, const Vector<Card>& cards);
	void ExecutePass();
	void StartPlayPhase();
	PlayResult ValidatePlay(int player_index, const Card& card) const;
	PlayResult PlayCard(int player_index, const Card& card);
	int GetTrickResult(int& points_out) const;
	void ResolveTrick();
	void ResolveRound();
	void BeginNextRound();
};

#endif
