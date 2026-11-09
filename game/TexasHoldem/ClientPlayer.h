#ifndef _CardEngine_ClientPlayer_h_
#define _CardEngine_ClientPlayer_h_

#include <GameRules/PlayerInterface.h>
#include <Poker/PokerPlayerInterface.h>
#include <memory>

NAMESPACE_UPP

class ClientPlayer : public PlayerInterface, public PokerPlayerInterface
{
public:
	ClientPlayer(class ConfigFile* config, int id, unsigned uniqueId, PlayerType type, String name, String avatar, int sC, bool aS, bool sotS, int mB);
	virtual ~ClientPlayer();

	virtual void setHand(HandInterface *) override;
	virtual HandInterface* getHand() const override;

	virtual void setMyID(int id) override;
	virtual int getMyID() const override;
	void setMyUniqueID(unsigned newId);
	virtual unsigned getMyUniqueID() const override;
	virtual void setMyGuid(const String &theValue) override;
	virtual String getMyGuid() const override;
	virtual PlayerType getMyType() const override;

	virtual void setMyDude(int theValue) override;
	virtual int getMyDude() const override;

	virtual void setMyDude4(int theValue) override;
	virtual int getMyDude4() const override;

	virtual void setMyName(String theValue) override;
	virtual String getMyName() const override;

	void setMyAvatar(const String& theValue);
	virtual String getMyAvatar() const override;

	virtual void setMyCash(int theValue) override;
	virtual int getMyCash() const override;

	virtual void setMySet(int theValue) override;
	void setMySetAbsolute(int theValue);
	virtual void setMySetNull() override;
	virtual int getMySet() const override;
	virtual void setMyLastRelativeSet(int theValue) override;
	virtual int getMyLastRelativeSet() const override;

	virtual void setMyAction(PlayerAction theValue, bool human) override;
	virtual PlayerAction getMyAction() const override;

	virtual void setMyButton(int theValue) override;
	virtual int getMyButton() const override;

	virtual void setMyActiveStatus(bool theValue) override;
	virtual bool getMyActiveStatus() const override;

	virtual void setMyStayOnTableStatus(bool theValue) override;
	virtual bool getMyStayOnTableStatus() const override;

	virtual void setMyCards(const int* theValue, int count) override;
	virtual Vector<int> getMyCards() const override;
	virtual void getMyCards(int& card1, int& card2) const override;

	virtual void setMyTurn(bool theValue) override;
	virtual bool getMyTurn() const override;

	virtual void setMyCardsFlip(bool theValue, int state) override;
	virtual bool getMyCardsFlip() const override;

	virtual void setMyCardsValueInt(int theValue) override;
	virtual int getMyCardsValueInt() const override;
	virtual void setMyOdds(double theValue) override;
	virtual double getMyOdds() const override;
	virtual void setMyNiveau(int index, int theValue) override;
	virtual int getMyNiveau(int index) const override;

	void setLogHoleCardsDone(bool theValue);
	bool getLogHoleCardsDone() const;

	virtual void setMyBestHandPosition(int* theValue) override;
	virtual void getMyBestHandPosition(int* theValue) const override;

	virtual void setMyRoundStartCash(int theValue) override;
	virtual int getMyRoundStartCash() const override;

	virtual void setLastMoneyWon ( int theValue ) override;
	virtual int getLastMoneyWon() const override;

	virtual void setMyAverageSets(int index, int theValue) override;
	virtual int getMyAverageSets(int index) const override;

	virtual void setMyAggressive(int index, bool theValue) override;
	virtual bool getMyAggressive(int index) const override;
	virtual int getMyAggressiveSum() const override;

	virtual void setMyActionTimeoutCounter(unsigned count) override;
	virtual unsigned getMyActionTimeoutCounter() const override;

	virtual void setSBluff (int theValue) override;
	virtual int getSBluff() const override;

	virtual void setSBluffStatus (bool theValue) override;
	virtual bool getSBluffStatus() const override;

	virtual void action() override;
	int checkMyAction(int targetAction, int targetBet, int highestSet, int minimumRaise, int smallBlind);

	virtual void preflopEngine();
	virtual void flopEngine();
	virtual void turnEngine();
	virtual void riverEngine();

	void preflopEngine3();
	void flopEngine3();
	void turnEngine3();
	void riverEngine3();

	int preflopCardsValue(int*);
	int flopCardsValue(int*);
	int turnCardsValue(int*);

	void readFile();

	void evaluation(int, int);

	virtual void setIsSessionActive(bool active) override;
	virtual bool isSessionActive() const override;
	virtual void setIsKicked(bool kicked) override;
	virtual bool isKicked() const override;
	virtual void setIsMuted(bool muted) override;
	virtual bool isMuted() const override;

	virtual bool checkIfINeedToShowCards() override;

	virtual void markRemoteAction() override {}
	virtual unsigned getTimeSecSinceLastRemoteAction() const override { return 0; }

private:
	mutable Mutex m_syncMutex;

	class ConfigFile *myConfig;
	HandInterface *currentHand;

	int myID;
	unsigned myUniqueID;
	String myGuid;
	const PlayerType myType;
	String myName;
	String myAvatar;
	int myDude;
	int myDude4;

	int myCardsValueInt;
	int myBestHandPosition[5];
	double myOdds;
	int myNiveau[3];
	bool logHoleCardsDone;

	int myCards[13];
	int myCardsCount;
	int myCash;
	int mySet;
	int myLastRelativeSet;
	PlayerAction myAction;
	int myButton;
	bool myActiveStatus;
	bool myStayOnTableStatus;
	bool myTurn;
	bool myCardsFlip;
	int myRoundStartCash;
	int lastMoneyWon;

	int myAverageSets[4];
	bool myAggressive[7];

	int sBluff;
	bool sBluffStatus;

	bool m_isSessionActive;
	bool m_isKicked;
	bool m_isMuted;
	unsigned m_actionTimeoutCounter;
};

END_UPP_NAMESPACE

#endif
