#ifndef _GameCommon_Poker_PokerHandInterface_h_
#define _GameCommon_Poker_PokerHandInterface_h_

#include <GameRules/HandInterface.h>

NAMESPACE_UPP

class PokerHandInterface
{
public:
	virtual ~PokerHandInterface() {}

	virtual std::shared_ptr<BeroInterface> getPreflop() const = 0;
	virtual std::shared_ptr<BeroInterface> getFlop() const = 0;
	virtual std::shared_ptr<BeroInterface> getTurn() const = 0;
	virtual std::shared_ptr<BeroInterface> getRiver() const = 0;
	
	virtual TexasRound getRoundBeforePostRiver() const = 0;

	virtual void setSmallBlind(int theValue) = 0;
	virtual int getSmallBlind() const = 0;

	virtual unsigned getBigBlindPositionId() const = 0;
	virtual unsigned getSmallBlindPositionId() const = 0;
};

END_UPP_NAMESPACE

#endif
