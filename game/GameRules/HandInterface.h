#ifndef _CardEngine_HandInterface_h_
#define _CardEngine_HandInterface_h_

#include <GameRules/EngineDefs.h>
#include <GameRules/GameDefs.h>

NAMESPACE_UPP

class BoardInterface;
class BeroInterface;
class GuiInterface;
class EngineLog;

class HandInterface
{
public:
	virtual ~HandInterface() {}

	virtual void start() = 0;

	virtual PlayerList getSeatsList() const = 0;
	virtual PlayerList getActivePlayerList() const = 0;
	virtual PlayerList getRunningPlayerList() const = 0;

	virtual std::shared_ptr<BoardInterface> getBoard() const = 0;
	virtual GuiInterface* getGuiInterface() const = 0;
	virtual std::shared_ptr<BeroInterface> getCurrentBeRo() const = 0;
	virtual EngineLog* getLog() const = 0;
	virtual bool       isVerbose() const = 0;

	virtual GameType getGameType() const = 0;
	virtual int      getHandSize() const = 0;
	virtual const Vector<int>& getCurrentTrickCards() const = 0;
	virtual int      getHeartsTrickSuit() const = 0;

	virtual void setMyID(int theValue) = 0;
	virtual int getMyID() const = 0;

	virtual void setCurrentQuantityPlayers(int theValue) = 0;
	virtual int getCurrentQuantityPlayers() const = 0;

	virtual void setStartQuantityPlayers(int theValue) = 0;
	virtual int getStartQuantityPlayers() const = 0;

	virtual void setCurrentRound(TexasRound theValue) = 0;
	virtual TexasRound getCurrentRound() const = 0;

	virtual void setDealerPosition(int theValue) = 0;
	virtual int getDealerPosition() const = 0;

	virtual void setAllInCondition(bool theValue) = 0;
	virtual bool getAllInCondition() const = 0;

	virtual void setStartCash(int theValue) = 0;
	virtual int getStartCash() const = 0;

	virtual void setBettingRoundsPlayed(int theValue) = 0;
	virtual int getBettingRoundsPlayed() const = 0;

	virtual void setPreviousPlayerID(int theValue) = 0;
	virtual int getPreviousPlayerID() const = 0;

	virtual void setLastActionPlayerID( unsigned theValue ) = 0;
	virtual unsigned getLastActionPlayerID() const = 0;

	virtual std::shared_ptr<PlayerInterface> getPlayerByUniqueId(unsigned id) const = 0;

	virtual void setCardsShown(bool theValue) = 0;
	virtual bool getCardsShown() const = 0;

	virtual void resetLoopCounters() = 0;
	virtual void switchRounds() = 0;

	virtual PlayerListIterator getSeatIt(unsigned id) const = 0;
	virtual PlayerListIterator getActivePlayerIt(unsigned id) const = 0;
	virtual PlayerListIterator getRunningPlayerIt(unsigned id) const = 0;

protected:
	friend class Game;
};

END_UPP_NAMESPACE

#endif
