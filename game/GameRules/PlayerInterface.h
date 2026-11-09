#ifndef _CardEngine_PlayerInterface_h_
#define _CardEngine_PlayerInterface_h_

#include <GameRules/EngineDefs.h>
#include <GameRules/PlayerData.h>

NAMESPACE_UPP

class HandInterface;

class PlayerInterface
{
public:
	virtual ~PlayerInterface() {}

	virtual void action() = 0;

	virtual void setHand(HandInterface* hand) = 0;
	virtual HandInterface* getHand() const = 0;

	virtual void setMyID(int id) = 0;
	virtual int  getMyID() const = 0;

	virtual unsigned getMyUniqueID() const = 0;
	virtual PlayerType getMyType() const = 0;
	virtual String getMyName() const = 0;
	virtual void setMyName(String name) = 0;
	virtual String getMyAvatar() const = 0;
	virtual void setMyGuid(const String& guid) = 0;
	virtual String getMyGuid() const = 0;

	virtual void setMyActiveStatus(bool theValue) = 0;
	virtual bool getMyActiveStatus() const = 0;

	virtual void setMyStayOnTableStatus(bool theValue) = 0;
	virtual bool getMyStayOnTableStatus() const = 0;

	virtual void setMyCards(const int* theValue, int count) = 0;
	virtual Vector<int> getMyCards() const = 0;
	virtual void getMyCards(int& card1, int& card2) const = 0; // Legacy support for NLTH

	virtual void setMyTurn(bool theValue) = 0;
	virtual bool getMyTurn() const = 0;

	virtual void setMyCash(int theValue) = 0;
	virtual int getMyCash() const = 0;

	virtual void setMySet(int theValue) = 0;
	virtual int getMySet() const = 0;
	virtual void setMySetNull() = 0;

	virtual void setMyLastRelativeSet(int theValue) = 0;
	virtual int getMyLastRelativeSet() const = 0;

	virtual void setMyAction(PlayerAction theValue, bool human = false) = 0;
	virtual PlayerAction getMyAction() const = 0;

	virtual void setMyButton(int theValue) = 0;
	virtual int getMyButton() const = 0;

	virtual void setMyRoundStartCash(int theValue) = 0;
	virtual int getMyRoundStartCash() const = 0;

	virtual void setLastMoneyWon(int theValue) = 0;
	virtual int getLastMoneyWon() const = 0;

	virtual void setMyActionTimeoutCounter(unsigned count) = 0;
	virtual unsigned getMyActionTimeoutCounter() const = 0;

	virtual void setIsSessionActive(bool active) = 0;
	virtual bool isSessionActive() const = 0;

	virtual void setIsKicked(bool kicked) = 0;
	virtual bool isKicked() const = 0;

	virtual void setIsMuted(bool muted) = 0;
	virtual bool isMuted() const = 0;

	virtual bool checkIfINeedToShowCards() = 0;

	virtual void markRemoteAction() = 0;
	virtual unsigned getTimeSecSinceLastRemoteAction() const = 0;
};

END_UPP_NAMESPACE

#endif
