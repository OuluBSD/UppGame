#ifndef _CardEngine_ClientHand_h_
#define _CardEngine_ClientHand_h_

#include <GameRules/HandInterface.h>
#include <Poker/PokerHandInterface.h>
#include <Poker/PokerHandInterface.h>
#include <memory>
#include <vector>

NAMESPACE_UPP

class EngineFactory;
class GuiInterface;
class BoardInterface;
class EngineLog;
class BeroInterface;

class ClientHand : public HandInterface, public PokerHandInterface
{
public:
	ClientHand(std::shared_ptr<EngineFactory> f, GuiInterface* gui, std::shared_ptr<BoardInterface> board, EngineLog* log, PlayerList seats, PlayerList active, PlayerList running, int id, int startPlayers, int dealer, int sb, int startCash, GameType gt = GAME_TYPE_NLTH);
	virtual ~ClientHand();

	virtual void start() override;

	virtual PlayerList getSeatsList() const override;
	virtual PlayerList getActivePlayerList() const override;
	virtual PlayerList getRunningPlayerList() const override;

	virtual std::shared_ptr<BoardInterface> getBoard() const override;
	virtual std::shared_ptr<BeroInterface> getPreflop() const override;
	virtual std::shared_ptr<BeroInterface> getFlop() const override;
	virtual std::shared_ptr<BeroInterface> getTurn() const override;
	virtual std::shared_ptr<BeroInterface> getRiver() const override;
	virtual GuiInterface* getGuiInterface() const override;
	virtual std::shared_ptr<BeroInterface> getCurrentBeRo() const override;

	virtual EngineLog* getLog() const override { return myLog; }
	virtual bool isVerbose() const override { return false; }
	virtual GameType getGameType() const override { return my_game_type; }
	virtual int getHandSize() const override {
		if (my_game_type == GAME_TYPE_PLO) return 4;
		if (my_game_type == GAME_TYPE_HEARTS) return 13;
		return 2;
	}
	virtual const Vector<int>& getCurrentTrickCards() const override;
	virtual int getHeartsTrickSuit() const override;

	virtual void setMyID(int theValue) override;
	virtual int getMyID() const override;

	virtual void setCurrentQuantityPlayers(int theValue) override;
	virtual int getCurrentQuantityPlayers() const override;

	virtual void setStartQuantityPlayers(int theValue) override;
	virtual int getStartQuantityPlayers() const override;

	virtual void setCurrentRound(TexasRound theValue) override;
	virtual TexasRound getCurrentRound() const override;
	virtual TexasRound getRoundBeforePostRiver() const override;

	virtual void setDealerPosition(int theValue) override;
	virtual int getDealerPosition() const override;

	virtual void setSmallBlind(int theValue) override;
	virtual int getSmallBlind() const override;

	virtual void setAllInCondition(bool theValue) override;
	virtual bool getAllInCondition() const override;

	virtual void setStartCash(int theValue) override;
	virtual int getStartCash() const override;

	virtual void setBettingRoundsPlayed(int theValue) override;
	virtual int getBettingRoundsPlayed() const override;

	virtual void setPreviousPlayerID(int theValue) override;
	virtual int getPreviousPlayerID() const override;

	virtual void setLastActionPlayerID(unsigned theValue) override;
	virtual unsigned getLastActionPlayerID() const override;
	virtual unsigned getBigBlindPositionId() const override;
	virtual unsigned getSmallBlindPositionId() const override;
	virtual std::shared_ptr<PlayerInterface> getPlayerByUniqueId(unsigned id) const override;

	virtual void setCardsShown(bool theValue) override;
	virtual bool getCardsShown() const override;

	virtual void resetLoopCounters() override;
	virtual void switchRounds() override;

	virtual PlayerListIterator getSeatIt(unsigned id) const override;
	virtual PlayerListIterator getActivePlayerIt(unsigned id) const override;
	virtual PlayerListIterator getRunningPlayerIt(unsigned id) const override;

private:
	mutable Mutex m_syncMutex;

	std::shared_ptr<EngineFactory> myFactory;
	GuiInterface *myGui;
	std::shared_ptr<BoardInterface> myBoard;
	EngineLog *myLog;

	PlayerList seatsList;
	PlayerList activePlayerList;
	PlayerList runningPlayerList;

	std::vector<std::shared_ptr<BeroInterface>> myBeRo;

	int myID;
	int startQuantityPlayers;
	unsigned dealerPosition;
	unsigned bigBlindPosition;
	TexasRound currentRound;
	TexasRound roundBeforePostRiver;
	int smallBlind;
	int startCash;

	int previousPlayerID;
	unsigned lastActionPlayerID;

	bool allInCondition;
	bool cardsShown;
	GameType my_game_type;
	Vector<int> myCurrentTrickCards;
	int myHeartsTrickSuit = -1;
};

END_UPP_NAMESPACE

#endif
