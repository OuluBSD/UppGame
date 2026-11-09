#ifndef _GameCommon_Poker_PokerPlayerInterface_h_
#define _GameCommon_Poker_PokerPlayerInterface_h_

#include <Core/Core.h>

NAMESPACE_UPP

class PokerPlayerInterface
{
public:
	virtual ~PokerPlayerInterface() {}

	virtual void setMyDude(int theValue) = 0;
	virtual int getMyDude() const = 0;

	virtual void setMyDude4(int theValue) = 0;
	virtual int getMyDude4() const = 0;

	virtual void setMyCardsValueInt(int theValue) = 0;
	virtual int getMyCardsValueInt() const = 0;

	virtual void setMyBestHandPosition(int* theValue) = 0;
	virtual void getMyBestHandPosition(int* theValue) const = 0;

	virtual void setMyOdds(double theValue) = 0;
	virtual double getMyOdds() const = 0;

	virtual void setMyNiveau(int index, int theValue) = 0;
	virtual int getMyNiveau(int index) const = 0;

	virtual void setMyCardsFlip(bool theValue, int state) = 0;
	virtual bool getMyCardsFlip() const = 0;

	virtual void setSBluff(int theValue) = 0;
	virtual int getSBluff() const = 0;

	virtual void setSBluffStatus(bool theValue) = 0;
	virtual bool getSBluffStatus() const = 0;

	virtual void setMyAverageSets(int index, int theValue) = 0;
	virtual int getMyAverageSets(int index) const = 0;

	virtual void setMyAggressive(int index, bool theValue) = 0;
	virtual bool getMyAggressive(int index) const = 0;
	virtual int  getMyAggressiveSum() const = 0;
};

END_UPP_NAMESPACE

#endif
