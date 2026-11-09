#ifndef _GameCommon_Poker_PokerLocalPlayer_h_
#define _GameCommon_Poker_PokerLocalPlayer_h_

#include <GameRules/LocalPlayer.h>
#include <Poker/PokerPlayerInterface.h>

NAMESPACE_UPP

class PokerLocalPlayer : public LocalPlayer, public PokerPlayerInterface
{
public:
	PokerLocalPlayer(class ConfigFile* c, int id, unsigned uniqueId, PlayerType type, String name, String avatar, int sC, bool aS, bool sotS, int mB);
	virtual ~PokerLocalPlayer();

	virtual void setMyDude(int theValue) override { myDude = theValue; }
	virtual int getMyDude() const override { return myDude; }

	virtual void setMyDude4(int theValue) override { myDude4 = theValue; }
	virtual int getMyDude4() const override { return myDude4; }

	virtual void setMyCardsValueInt(int theValue) override { myCardsValueInt = theValue; }
	virtual int getMyCardsValueInt() const override { return myCardsValueInt; }

	virtual void setMyBestHandPosition(int* theValue) override;
	virtual void getMyBestHandPosition(int* theValue) const override;

	virtual void setMyOdds(double theValue) override { myOdds = theValue; }
	virtual double getMyOdds() const override { return myOdds; }

	virtual void setMyNiveau(int index, int theValue) override { if(index >= 0 && index < 3) myNiveau[index] = theValue; }
	virtual int getMyNiveau(int index) const override { return (index >= 0 && index < 3) ? myNiveau[index] : 0; }

	virtual void setMyCardsFlip(bool theValue, int state) override;
	virtual bool getMyCardsFlip() const override { return myCardsFlip; }

	virtual void setSBluff(int theValue) override { sBluff = theValue; }
	virtual int getSBluff() const override { return sBluff; }

	virtual void setSBluffStatus(bool theValue) override { sBluffStatus = theValue; }
	virtual bool getSBluffStatus() const override { return sBluffStatus; }

	virtual void setMyAverageSets(int index, int theValue) override { if(index >= 0 && index < 4) myAverageSets[index] = theValue; }
	virtual int getMyAverageSets(int index) const override { return (index >= 0 && index < 4) ? myAverageSets[index] : 0; }

	virtual void setMyAggressive(int index, bool theValue) override { if(index >= 0 && index < 7) myAggressive[index] = theValue; }
	virtual bool getMyAggressive(int index) const override { return (index >= 0 && index < 7) ? myAggressive[index] : false; }
	virtual int  getMyAggressiveSum() const override;

	// Engines (can be empty base)
	virtual void preflopEngine() {}
	virtual void flopEngine() {}
	virtual void turnEngine() {}
	virtual void riverEngine() {}

protected:
	int myDude;
	int myDude4;
	int myCardsValueInt;
	int myBestHandPosition[5];
	double myOdds;
	int myNiveau[3];
	bool logHoleCardsDone;
	bool myCardsFlip;

	int myAverageSets[4];
	bool myAggressive[7];

	int sBluff;
	bool sBluffStatus;
};

END_UPP_NAMESPACE

#endif
