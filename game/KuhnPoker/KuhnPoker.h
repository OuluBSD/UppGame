#ifndef _game_KuhnPoker_KuhnPoker_h_
#define _game_KuhnPoker_KuhnPoker_h_

#include <CtrlLib/CtrlLib.h>
#include <CardGame/CardGame.h>

NAMESPACE_UPP

// Kuhn Poker: the smallest nontrivial poker game (Kuhn, 1950) -- a 3-card
// deck (Jack/Queen/King), 2 players, 1 card each, a single betting round.
// This package acts as the form-based table example that later card-game UI
// ports can mirror.
class KuhnPoker {
public:
	enum { CARD_J, CARD_Q, CARD_K };
	enum Winner { WIN_NONE = -1, WIN_HUMAN = 0, WIN_AI = 1 };

	// Which actions are legal right now (mutually exclusive: either
	// Check/Bet are both offered, or Call/Fold are, never a mix).
	enum Phase {
		PHASE_NOT_STARTED, // call NewHand() to deal
		PHASE_HUMAN_OPEN,  // human to act, no bet yet: Check or Bet
		PHASE_HUMAN_FACING_BET, // AI bet after human's check: Call or Fold
		PHASE_SHOWDOWN,    // both cards revealed, hand resolved by rank
		PHASE_FOLD,        // someone folded, hand resolved without showdown
	};

	int    human_card = CARD_J;
	int    ai_card = CARD_Q;
	int    pot = 0;
	int    human_chips = 20;
	int    ai_chips = 20;
	Phase  phase = PHASE_NOT_STARTED;
	Winner winner = WIN_NONE;
	String log; // human-readable narration of the last hand's action sequence

	void NewHand();

	bool CanCheck() const { return phase == PHASE_HUMAN_OPEN; }
	bool CanBet() const   { return phase == PHASE_HUMAN_OPEN; }
	bool CanCall() const  { return phase == PHASE_HUMAN_FACING_BET; }
	bool CanFold() const  { return phase == PHASE_HUMAN_FACING_BET; }

	void HumanCheck();
	void HumanBet();
	void HumanCall();
	void HumanFold();

	static String CardName(int card);

	bool IsOver() const { return human_chips <= 0 || ai_chips <= 0; }

private:
	void Ante();
	void AiActAfterHumanCheck();
	void Showdown();
	void Resolve(Winner w, const String& reason);
};

class KuhnPokerTable : public Ctrl {
public:
	typedef KuhnPokerTable CLASSNAME;

	KuhnPokerTable();

	void SetGame(KuhnPoker* poker);

	virtual void Paint(Draw& w) override;

private:
	KuhnPoker* poker = nullptr;

	Image LoadCardImage(const Card& card, bool back) const;
	static Card ToCard(int card);
};

END_UPP_NAMESPACE

#endif
