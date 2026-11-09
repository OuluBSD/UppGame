#ifndef _CardEngine_ClientBero_h_
#define _CardEngine_ClientBero_h_

#include <GameRules/BeroInterface.h>

NAMESPACE_UPP

class HandInterface;

class ClientBero : public BeroInterface
{
public:
	ClientBero(HandInterface* hi, unsigned dP, int sB, TexasRound gS);
	virtual ~ClientBero();

	virtual void start() override;
	virtual void reset() override;
	virtual void run() override;
	virtual void nextPlayer() override;
	virtual void postRiverRun() override;

	virtual TexasRound getMyBeRoID() const override;

	virtual void setCurrentPlayersTurnId(unsigned id) override;
	virtual unsigned getCurrentPlayersTurnId() const override;
	
	virtual void setCurrentPlayersTurnIt(PlayerListIterator it) override;
	virtual PlayerListIterator getCurrentPlayersTurnIt() const override;

	virtual void setSmallBlindPositionId(unsigned id) override;
	virtual unsigned getSmallBlindPositionId() const override;
	
	virtual void setBigBlindPositionId(unsigned id) override;
	virtual unsigned getBigBlindPositionId() const override;
	
	virtual void setHighestSet(int val) override;
	virtual int getHighestSet() const override;
	
	virtual void setMinimumRaise(int val) override;
	virtual int getMinimumRaise() const override;
	
	virtual void setFullBetRule(bool val) override;
	virtual bool getFullBetRule() const override;
	
	virtual void setHighestCardsValue(int val) override;
	virtual int getHighestCardsValue() const override;
	
	virtual bool getFirstRound() const override;
	virtual void setFirstRound(bool val) override;
	virtual void skipFirstRunGui() override;

	virtual void resetLoopCounters() override {}
	virtual void recordAction(int playerID, PlayerAction action, int value) override;
	virtual const ActionVec& getActionHistory() const override { return actionHistory; }

private:
	mutable Mutex m_syncMutex;
	HandInterface *myHand;
	const TexasRound myBeRoID;
	unsigned dealerPosition;
	unsigned smallBlindPositionId;
	unsigned bigBlindPositionId;
	int smallBlind;
	int highestSet;
	int minimumRaise;
	bool fullBetRule;
	bool firstRound;
	PlayerListIterator currentPlayersTurnIt;
	unsigned currentPlayersTurnId;
	int highestCardsValue;
	ActionVec actionHistory;
};

END_UPP_NAMESPACE

#endif
