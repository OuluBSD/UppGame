#ifndef _GameCommon_Rules_LocalPlayer_h_
#define _GameCommon_Rules_LocalPlayer_h_

#include <GameRules/PlayerInterface.h>

NAMESPACE_UPP

class ConfigFile;
class HandInterface;

class LocalPlayer : public PlayerInterface
{
public:
	LocalPlayer(class ConfigFile* c, int id, unsigned uniqueId, PlayerType type, String name, String avatar, int sC, bool aS, bool sotS, int mB);
	virtual ~LocalPlayer();

	virtual void action() override {}

	virtual void setHand(HandInterface* hand) override { currentHand = hand; }
	virtual HandInterface* getHand() const override { return currentHand; }

	virtual void setMyID(int id) override { myID = id; }
	virtual int  getMyID() const override { return myID; }

	virtual unsigned getMyUniqueID() const override { return myUniqueID; }
	virtual PlayerType getMyType() const override { return myType; }
	virtual String getMyName() const override { return myName; }
	virtual void setMyName(String name) override { myName = name; }
	virtual String getMyAvatar() const override { return myAvatar; }
	virtual void setMyGuid(const String& guid) override { myGuid = guid; }
	virtual String getMyGuid() const override { return myGuid; }

	virtual void setMyActiveStatus(bool theValue) override { myActiveStatus = theValue; }
	virtual bool getMyActiveStatus() const override { return myActiveStatus; }

	virtual void setMyStayOnTableStatus(bool theValue) override { myStayOnTableStatus = theValue; }
	virtual bool getMyStayOnTableStatus() const override { return myStayOnTableStatus; }

	virtual void setMyCards(const int* theValue, int count) override;
	virtual Vector<int> getMyCards() const override;
	virtual void getMyCards(int& card1, int& card2) const override;

	virtual void setMyTurn(bool theValue) override { myTurn = theValue; }
	virtual bool getMyTurn() const override { return myTurn; }

	virtual void setMyCash(int theValue) override { myCash = theValue; }
	virtual int getMyCash() const override { return myCash; }

	virtual void setMySet(int theValue) override;
	virtual int getMySet() const override { return mySet; }
	virtual void setMySetNull() override { mySet = 0; myLastRelativeSet = 0; }

	virtual void setMyLastRelativeSet(int theValue) override { myLastRelativeSet = theValue; }
	virtual int getMyLastRelativeSet() const override { return myLastRelativeSet; }

	virtual void setMyAction(PlayerAction theValue, bool human = false) override;
	virtual PlayerAction getMyAction() const override { return myAction; }

	virtual void setMyButton(int theValue) override { myButton = theValue; }
	virtual int getMyButton() const override { return myButton; }

	virtual void setMyRoundStartCash(int theValue) override { myRoundStartCash = theValue; }
	virtual int getMyRoundStartCash() const override { return myRoundStartCash; }

	virtual void setLastMoneyWon(int theValue) override { lastMoneyWon = theValue; }
	virtual int getLastMoneyWon() const override { return lastMoneyWon; }

	virtual void setMyActionTimeoutCounter(unsigned count) override { m_actionTimeoutCounter = count; }
	virtual unsigned getMyActionTimeoutCounter() const override { return m_actionTimeoutCounter; }

	virtual void setIsSessionActive(bool active) override { m_isSessionActive = active; }
	virtual bool isSessionActive() const override { return m_isSessionActive; }

	virtual void setIsKicked(bool kicked) override { m_isKicked = kicked; }
	virtual bool isKicked() const override { return m_isKicked; }

	virtual void setIsMuted(bool muted) override { m_isMuted = muted; }
	virtual bool isMuted() const override { return m_isMuted; }

	virtual bool checkIfINeedToShowCards() override;

	virtual void markRemoteAction() override;
	virtual unsigned getTimeSecSinceLastRemoteAction() const override;

protected:
	class ConfigFile* myConfig;
	HandInterface* currentHand;

	int myID;
	unsigned myUniqueID;
	PlayerType myType;
	String myName;
	String myAvatar;
	String myGuid;

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
	int myRoundStartCash;
	int lastMoneyWon;

	unsigned m_actionTimeoutCounter;
	bool m_isSessionActive;
	bool m_isKicked;
	bool m_isMuted;
	TimeStop m_lastRemoteActionTimer;
};

END_UPP_NAMESPACE

#endif
