#ifndef _CardEngine_BeroInterface_h_
#define _CardEngine_BeroInterface_h_

#include <GameRules/EngineDefs.h>
#include <GameRules/GameDefs.h>

NAMESPACE_UPP

class HandInterface;

class BeroInterface
{
public:
	virtual ~BeroInterface() {}

	virtual void start() = 0;
	virtual void reset() = 0;
	virtual void run() = 0;
	virtual void nextPlayer() = 0;
	virtual void postRiverRun() = 0;

	virtual TexasRound getMyBeRoID() const = 0;

	virtual void setCurrentPlayersTurnId(unsigned id) = 0;
	virtual unsigned getCurrentPlayersTurnId() const = 0;
	
	virtual void setCurrentPlayersTurnIt(PlayerListIterator it) = 0;
	virtual PlayerListIterator getCurrentPlayersTurnIt() const = 0;

	virtual void setSmallBlindPositionId(unsigned id) = 0;
	virtual unsigned getSmallBlindPositionId() const = 0;
	
	virtual void setBigBlindPositionId(unsigned id) = 0;
	virtual unsigned getBigBlindPositionId() const = 0;
	
	virtual void setHighestSet(int val) = 0;
	virtual int getHighestSet() const = 0;
	
	virtual void setMinimumRaise(int val) = 0;
	virtual int getMinimumRaise() const = 0;
	
	virtual void setFullBetRule(bool val) = 0;
	virtual bool getFullBetRule() const = 0;
	
	virtual void setHighestCardsValue(int val) = 0;
	virtual int getHighestCardsValue() const = 0;
	
	virtual bool getFirstRound() const = 0;
	virtual void setFirstRound(bool val) = 0;
	virtual void skipFirstRunGui() = 0;

	virtual void resetLoopCounters() = 0;
	virtual void recordAction(int playerID, PlayerAction action, int value) = 0;
	virtual const ActionVec& getActionHistory() const = 0;
};

END_UPP_NAMESPACE

#endif
