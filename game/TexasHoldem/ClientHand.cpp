#include <Poker/PokerPlayerInterface.h>
#include "ClientHand.h"
#include <GameRules/EngineFactory.h>
#include <GameRules/BoardInterface.h>
#include <GameRules/BeroInterface.h>
#include <GameRules/PlayerInterface.h>
#include <GameRules/GuiInterface.h>

NAMESPACE_UPP

ClientHand::ClientHand(std::shared_ptr<EngineFactory> f, GuiInterface* g, std::shared_ptr<BoardInterface> b, EngineLog* l, PlayerList sl, PlayerList apl, PlayerList rpl, int id, int sP, int dP, int sB, int sC, GameType gt)
	: myFactory(f), myGui(g), myBoard(b), myLog(l), seatsList(sl), activePlayerList(apl), runningPlayerList(rpl), myID(id), startQuantityPlayers(sP), dealerPosition(dP), bigBlindPosition(dP), currentRound(GAME_STATE_PREFLOP), roundBeforePostRiver(GAME_STATE_PREFLOP),
	  smallBlind(sB), startCash(sC), previousPlayerID(-1), allInCondition(0), cardsShown(false), my_game_type(gt)
{
	for (auto& p : *seatsList) {
		p->setHand(this);
		((PokerPlayerInterface&)*p).setMyCardsFlip(false, 0);
	}
	for (auto& p : *activePlayerList) {
		p->setMyRoundStartCash(p->getMyCash());
		((PokerPlayerInterface&)*p).setMyCardsValueInt(0);
		p->setMyButton(0);
		if (p->getMyUniqueID() == (unsigned)dealerPosition)
			p->setMyButton(1);
	}
	myBeRo = myFactory->createBeRo(this, dealerPosition, smallBlind);
}

ClientHand::~ClientHand() {}

void ClientHand::start() {}

PlayerList ClientHand::getSeatsList() const { Mutex::Lock lock(m_syncMutex); return seatsList; }
PlayerList ClientHand::getActivePlayerList() const { Mutex::Lock lock(m_syncMutex); return activePlayerList; }
PlayerList ClientHand::getRunningPlayerList() const { Mutex::Lock lock(m_syncMutex); return runningPlayerList; }

std::shared_ptr<BoardInterface> ClientHand::getBoard() const { return myBoard; }
std::shared_ptr<BeroInterface> ClientHand::getPreflop() const { return myBeRo[GAME_STATE_PREFLOP]; }
std::shared_ptr<BeroInterface> ClientHand::getFlop() const { return myBeRo[GAME_STATE_FLOP]; }
std::shared_ptr<BeroInterface> ClientHand::getTurn() const { return myBeRo[GAME_STATE_TURN]; }
std::shared_ptr<BeroInterface> ClientHand::getRiver() const { return myBeRo[GAME_STATE_RIVER]; }
std::shared_ptr<BeroInterface> ClientHand::getCurrentBeRo() const { return myBeRo[currentRound]; }
GuiInterface* ClientHand::getGuiInterface() const { return myGui; }
const Vector<int>& ClientHand::getCurrentTrickCards() const { return myCurrentTrickCards; }
int ClientHand::getHeartsTrickSuit() const { return myHeartsTrickSuit; }

void ClientHand::setMyID(int theValue) { Mutex::Lock lock(m_syncMutex); myID = theValue; }
int ClientHand::getMyID() const { Mutex::Lock lock(m_syncMutex); return myID; }
void ClientHand::setStartQuantityPlayers(int theValue) { Mutex::Lock lock(m_syncMutex); startQuantityPlayers = theValue; }
int ClientHand::getStartQuantityPlayers() const { Mutex::Lock lock(m_syncMutex); return startQuantityPlayers; }
void ClientHand::setCurrentRound(TexasRound theValue) { Mutex::Lock lock(m_syncMutex); currentRound = theValue; }
TexasRound ClientHand::getCurrentRound() const { Mutex::Lock lock(m_syncMutex); return currentRound; }
TexasRound ClientHand::getRoundBeforePostRiver() const { Mutex::Lock lock(m_syncMutex); return roundBeforePostRiver; }
void ClientHand::setDealerPosition(int theValue) { Mutex::Lock lock(m_syncMutex); dealerPosition = theValue; }
int ClientHand::getDealerPosition() const { Mutex::Lock lock(m_syncMutex); return dealerPosition; }
void ClientHand::setSmallBlind(int theValue) { Mutex::Lock lock(m_syncMutex); smallBlind = theValue; }
int ClientHand::getSmallBlind() const { Mutex::Lock lock(m_syncMutex); return smallBlind; }

void ClientHand::setAllInCondition(bool theValue)
{
	Mutex::Lock lock(m_syncMutex);
	allInCondition = theValue;
	myBoard->setAllInCondition(theValue);
}

bool ClientHand::getAllInCondition() const { Mutex::Lock lock(m_syncMutex); return allInCondition; }
void ClientHand::setStartCash(int theValue) { Mutex::Lock lock(m_syncMutex); startCash = theValue; }
int ClientHand::getStartCash() const { Mutex::Lock lock(m_syncMutex); return startCash; }
void ClientHand::setPreviousPlayerID(int theValue) { Mutex::Lock lock(m_syncMutex); previousPlayerID = theValue; }
int ClientHand::getPreviousPlayerID() const { Mutex::Lock lock(m_syncMutex); return previousPlayerID; }

void ClientHand::setLastActionPlayerID(unsigned theValue)
{
	Mutex::Lock lock(m_syncMutex);
	lastActionPlayerID = theValue;
	myBoard->setLastActionPlayerID(theValue);
}

unsigned ClientHand::getLastActionPlayerID() const { Mutex::Lock lock(m_syncMutex); return lastActionPlayerID; }
unsigned ClientHand::getBigBlindPositionId() const { Mutex::Lock lock(m_syncMutex); return bigBlindPosition; }
unsigned ClientHand::getSmallBlindPositionId() const { return 0; } // Stub
std::shared_ptr<PlayerInterface> ClientHand::getPlayerByUniqueId(unsigned id) const {
	Mutex::Lock lock(m_syncMutex);
	for (auto& p : *seatsList) {
		if (p && p->getMyUniqueID() == id)
			return p;
	}
	return {};
}
void ClientHand::setCardsShown(bool theValue) { Mutex::Lock lock(m_syncMutex); cardsShown = theValue; }
bool ClientHand::getCardsShown() const { Mutex::Lock lock(m_syncMutex); return cardsShown; }

void ClientHand::switchRounds()
{
	Mutex::Lock lock(m_syncMutex);
	for (int i = 0; i < (int)runningPlayerList->size(); ) {
		auto p = (*runningPlayerList)[i];
		if (p->getMyAction() == PLAYER_ACTION_FOLD || p->getMyAction() == PLAYER_ACTION_ALLIN) {
			runningPlayerList->erase(runningPlayerList->begin() + i);
			if (!runningPlayerList->empty()) {
				int prev = (i == 0) ? (int)runningPlayerList->size() - 1 : i - 1;
				getCurrentBeRo()->setCurrentPlayersTurnId((*runningPlayerList)[prev]->getMyUniqueID());
			}
		} else {
			i++;
		}
	}
}

void ClientHand::resetLoopCounters()
{
}

void ClientHand::setCurrentQuantityPlayers(int) {}
void ClientHand::setBettingRoundsPlayed(int) {}
int ClientHand::getBettingRoundsPlayed() const { return 0; }
int ClientHand::getCurrentQuantityPlayers() const { return (int)activePlayerList->size(); }

PlayerListIterator ClientHand::getSeatIt(unsigned id) const
{
	for (auto it = seatsList->begin(); it != seatsList->end(); ++it) if ((*it)->getMyUniqueID() == id) return it;
	return seatsList->end();
}

PlayerListIterator ClientHand::getActivePlayerIt(unsigned id) const
{
	for (auto it = activePlayerList->begin(); it != activePlayerList->end(); ++it) if ((*it)->getMyUniqueID() == id) return it;
	return activePlayerList->end();
}

PlayerListIterator ClientHand::getRunningPlayerIt(unsigned id) const
{
	for (auto it = runningPlayerList->begin(); it != runningPlayerList->end(); ++it) if ((*it)->getMyUniqueID() == id) return it;
	return runningPlayerList->end();
}

END_UPP_NAMESPACE
