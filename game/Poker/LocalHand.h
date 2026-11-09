#ifndef _CardEngine_LocalHand_h_
#define _CardEngine_LocalHand_h_

#include <map>
#include <GameRules/HandInterface.h>
#include <Poker/PokerHandInterface.h>

NAMESPACE_UPP

class EngineFactory;

class LocalHand : public HandInterface, public PokerHandInterface
{
public:
	LocalHand(std::shared_ptr<EngineFactory> f, GuiInterface* g, std::shared_ptr<BoardInterface> b, EngineLog* l, PlayerList sl, PlayerList apl, PlayerList rpl, int id, int sP, unsigned dP, int sB, int sC, int seed = -1, GameType gt = GAME_TYPE_NLTH);
	virtual ~LocalHand();

	virtual void start() override;

	virtual PlayerList getSeatsList() const override { return seatsList; }
	virtual PlayerList getActivePlayerList() const override { return activePlayerList; }
	virtual PlayerList getRunningPlayerList() const override { return runningPlayerList; }

	virtual std::shared_ptr<BoardInterface> getBoard() const override { return myBoard; }
	virtual std::shared_ptr<BeroInterface> getPreflop() const override { return getBeRo(GAME_STATE_PREFLOP); }
	virtual std::shared_ptr<BeroInterface> getFlop() const override { return getBeRo(GAME_STATE_FLOP); }
	virtual std::shared_ptr<BeroInterface> getTurn() const override { return getBeRo(GAME_STATE_TURN); }
	virtual std::shared_ptr<BeroInterface> getRiver() const override { return getBeRo(GAME_STATE_RIVER); }
	virtual GuiInterface* getGuiInterface() const override { return myGui; }
	virtual std::shared_ptr<BeroInterface> getCurrentBeRo() const override { return getBeRo(currentRound); }

	virtual EngineLog* getLog() const override { return myLog; }
	virtual bool       isVerbose() const override;

	virtual GameType getGameType() const override { return my_game_type; }
	virtual int      getHandSize() const override {
		if (my_game_type == GAME_TYPE_PLO) return 4;
		if (my_game_type == GAME_TYPE_PLO5) return 5;
		if (my_game_type == GAME_TYPE_HEARTS) return 13;
		return 2;
	}
	virtual const Vector<int>& getCurrentTrickCards() const override { return current_trick_cards; }
	int              getHeartsTrickSuit() const override { return hearts_trick_suit; }
	HeartsRound      getHeartsRound() const { return hearts_round; }
	int              getHeartsTrickCount() const { return hearts_trick_count; }

	virtual void setMyID(int theValue) override { myID = theValue; }
	virtual int getMyID() const override { return myID; }

	virtual void setCurrentQuantityPlayers(int theValue) override {} // Stub
	virtual int getCurrentQuantityPlayers() const override { return (int)activePlayerList->size(); }

	virtual void setStartQuantityPlayers(int theValue) override { startQuantityPlayers = theValue; }
	virtual int getStartQuantityPlayers() const override { return startQuantityPlayers; }

	virtual void setCurrentRound(TexasRound theValue) override;
	virtual TexasRound getCurrentRound() const override { return currentRound; }
	virtual TexasRound getRoundBeforePostRiver() const override { return roundBeforePostRiver; }

	virtual void setDealerPosition(int theValue) override { dealerPosition = (unsigned)theValue; }
	virtual int getDealerPosition() const override { return (int)dealerPosition; }

	virtual void setSmallBlind(int theValue) override { smallBlind = theValue; }
	virtual int getSmallBlind() const override { return smallBlind; }

	virtual void setAllInCondition(bool theValue) override { allInCondition = theValue; }
	virtual bool getAllInCondition() const override { return allInCondition; }

	virtual void setStartCash(int theValue) override { startCash = theValue; }
	virtual int getStartCash() const override { return startCash; }

	virtual void setBettingRoundsPlayed(int theValue) override {} // Stub
	virtual int getBettingRoundsPlayed() const override { return 0; }

	virtual void setPreviousPlayerID(int theValue) override { previousPlayerID = theValue; }
	virtual int getPreviousPlayerID() const override { return previousPlayerID; }

	virtual void setLastActionPlayerID ( unsigned theValue ) override;
	virtual unsigned getLastActionPlayerID() const override { return lastActionPlayerID; }
	virtual unsigned getBigBlindPositionId() const override { return bigBlindPosition; }
	virtual unsigned getSmallBlindPositionId() const override { return smallBlindPosition; }

	virtual std::shared_ptr<PlayerInterface> getPlayerByUniqueId(unsigned id) const override;

	virtual void setCardsShown(bool theValue) override { cardsShown = theValue; }
	virtual bool getCardsShown() const override { return cardsShown; }

	void assignButtons();
	void setBlinds();

	virtual void resetLoopCounters() override;
	virtual void switchRounds() override;
	void         switchHeartsRounds();
	bool         validateHeartsPlay(int player_id, int card);
	void         playHeartsCard(int player_id, int card);
	void         setGame(class Game* g) { m_game = g; }

protected:
	virtual PlayerListIterator getSeatIt(unsigned id) const override;
	virtual PlayerListIterator getActivePlayerIt(unsigned id) const override;
	virtual PlayerListIterator getRunningPlayerIt(unsigned id) const override;

private:
	std::shared_ptr<BeroInterface> getBeRo(TexasRound state) const {
		int q = myBeRo.Find((int)state);
		return q >= 0 ? myBeRo[q] : nullptr;
	}

	std::shared_ptr<EngineFactory> myFactory;
	GuiInterface *myGui;
	std::shared_ptr<BoardInterface> myBoard;
	EngineLog *myLog;

	PlayerList seatsList;
	PlayerList activePlayerList;
	PlayerList runningPlayerList;

	ArrayMap<int, std::shared_ptr<BeroInterface>> myBeRo;

	int myID;
	int startQuantityPlayers;
	unsigned dealerPosition;
	unsigned smallBlindPosition;
	unsigned bigBlindPosition;
	TexasRound currentRound;
	TexasRound roundBeforePostRiver;
	int smallBlind;
	int startCash;

	int previousPlayerID;
	unsigned lastActionPlayerID;

	bool allInCondition;
	bool cardsShown;
	int switchRoundsLoopCounter;
	GameType my_game_type;
	
	HeartsRound hearts_round;
	int         hearts_trick_count;
	Vector<int> current_trick_cards;
	int         trick_lead_player;
	int         hearts_scores[MAX_NUMBER_OF_PLAYERS];
	bool        hearts_broken;
	int         hearts_trick_suit;
	int         hearts_passing_direction; // 0: Left, 1: Right, 2: Across, 3: Hold
	bool        hearts_switching;
	class Game* m_game = nullptr;
};

END_UPP_NAMESPACE

#endif
