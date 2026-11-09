#ifndef _CardEngine_BoardInterface_h_
#define _CardEngine_BoardInterface_h_

#include <GameRules/EngineDefs.h>
#include <GameRules/GameDefs.h>

NAMESPACE_UPP

class GuiInterface;
class EngineLog;

class BoardInterface
{
public:
	virtual ~BoardInterface() {}

	virtual void reset() = 0;
	virtual void setPlayerLists(PlayerList sl, PlayerList apl, PlayerList rpl) = 0;
	virtual void collectSets() = 0;
	virtual void collectPot() = 0;
	virtual void distributePot(unsigned dealerPosition) = 0;

	virtual void setMyCards(int* cards) = 0;
	virtual void getMyCards(int* cards) const = 0;

	virtual int getPot() const = 0;
	virtual const int* getMyCards() const = 0;
	virtual int getSets() const = 0;

	virtual void determinePlayerNeedToShowCards() = 0;
	virtual const Vector<unsigned>& getPlayerNeedToShowCards() const = 0;

	virtual void setAllInCondition(bool value) = 0;
	virtual void setLastActionPlayerID(unsigned id) = 0;
	
	virtual const Vector<unsigned>& getWinners() const = 0;

protected:
	PlayerList seatsList;
	PlayerList activePlayerList;
	PlayerList runningPlayerList;

	int myCards[5];
	int pot;
	int sets;
	bool allInCondition;
	unsigned lastActionPlayerID;

	Vector<unsigned> playerNeedToShowCards;
	Vector<unsigned> winners;
};

END_UPP_NAMESPACE

#endif
