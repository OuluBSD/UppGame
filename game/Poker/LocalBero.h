#ifndef _CardEngine_LocalBero_h_
#define _CardEngine_LocalBero_h_

#include <GameRules/BeroInterface.h>

NAMESPACE_UPP

class HandInterface;

class LocalBero : public BeroInterface
{
public:
	LocalBero(HandInterface* hi, unsigned dP, int sB, TexasRound gS);
	virtual ~LocalBero();

	virtual void start() override;
	virtual void reset() override;

	virtual TexasRound getMyBeRoID() const override { return myBeRoID; }

	virtual int getHighestCardsValue() const override;
	virtual void setHighestCardsValue(int /*theValue*/) override { }

	virtual void setMinimumRaise ( int theValue ) override { minimumRaise = theValue; }
	virtual int getMinimumRaise() const override { return minimumRaise; }

	virtual void setFullBetRule ( bool theValue ) override { fullBetRule = theValue; }
	virtual bool getFullBetRule() const override { return fullBetRule; }

	virtual void skipFirstRunGui() override { firstRunGui = false; }

	virtual void resetLoopCounters() override { nextPlayerLoopCounter = 0; }
	virtual void nextPlayer() override;
	virtual void run() override;

	virtual void postRiverRun() override {}

	virtual bool getFirstRound() const override { return firstRound; }
	virtual void setFirstRound(bool theValue) override { firstRound = theValue; }

	virtual void recordAction(int playerID, PlayerAction action, int value) override;
	virtual const ActionVec& getActionHistory() const override { return actionHistory; }

	virtual void setCurrentPlayersTurnId(unsigned theValue) override { currentPlayersTurnId = theValue; }
	virtual unsigned getCurrentPlayersTurnId() const override { return currentPlayersTurnId; }

	virtual void setCurrentPlayersTurnIt(PlayerListIterator theValue) override { currentPlayersTurnIt = theValue; }
	virtual PlayerListIterator getCurrentPlayersTurnIt() const override { return currentPlayersTurnIt; }

	virtual void setSmallBlindPositionId(unsigned theValue) override { smallBlindPositionId = theValue; }
	virtual unsigned getSmallBlindPositionId() const override { return smallBlindPositionId; }

	virtual void setBigBlindPositionId(unsigned theValue) override { bigBlindPositionId = theValue; }
	virtual unsigned getBigBlindPositionId() const override { return bigBlindPositionId; }

	virtual void setHighestSet(int theValue) override { highestSet = theValue; }
	virtual int getHighestSet() const override { return highestSet; }

protected:
	HandInterface* getMyHand() const { return myHand; }
	unsigned getDealerPosition() const { return dealerPosition; }
	
	void setFirstRun(bool theValue) { firstRun = theValue; }
	bool getFirstRun() const { return firstRun; }

	void setFirstRoundLastPlayersTurnId(unsigned theValue) { firstRoundLastPlayersTurnId = theValue; }
	unsigned getFirstRoundLastPlayersTurnId() const { return firstRoundLastPlayersTurnId; }

	int getSmallBlind() const { return smallBlind; }

private:
	HandInterface* myHand;
	const TexasRound myBeRoID;
	unsigned dealerPosition;
	unsigned smallBlindPositionId;
	unsigned bigBlindPositionId;
	int smallBlind;
	int highestSet;
	int minimumRaise;
	bool fullBetRule;
	bool firstRun;
	bool firstRunGui;
	bool firstRound;
	PlayerListIterator currentPlayersTurnIt;
	unsigned currentPlayersTurnId;
	unsigned firstRoundLastPlayersTurnId;
	bool logBoardCardsDone;
	int nextPlayerLoopCounter;
	ActionVec actionHistory;
};

class LocalBeroPreflop : public LocalBero
{
public:
	LocalBeroPreflop(HandInterface* hi, unsigned dP, int sB);
	virtual void run() override;
};

class LocalBeroFlop : public LocalBero
{
public:
	LocalBeroFlop(HandInterface* hi, unsigned dP, int sB) : LocalBero(hi, dP, sB, GAME_STATE_FLOP) {}
};

class LocalBeroTurn : public LocalBero
{
public:
	LocalBeroTurn(HandInterface* hi, unsigned dP, int sB) : LocalBero(hi, dP, sB, GAME_STATE_TURN) {}
};

class LocalBeroRiver : public LocalBero
{
public:
	LocalBeroRiver(HandInterface* hi, unsigned dP, int sB) : LocalBero(hi, dP, sB, GAME_STATE_RIVER) {}
};

class LocalBeroPostRiver : public LocalBero
{
public:
	LocalBeroPostRiver(HandInterface* hi, unsigned dP, int sB);
	virtual void run() override {}
	virtual void postRiverRun() override;
private:
	int highestCardsValue;
};

END_UPP_NAMESPACE

#endif
