#include <Poker/PokerPlayerInterface.h>
#include "ClientBoard.h"
#include <GameRules/PlayerInterface.h>

NAMESPACE_UPP

ClientBoard::ClientBoard()
{
	reset();
}

ClientBoard::~ClientBoard() {}

void ClientBoard::reset()
{
	Mutex::Lock lock(m_syncMutex);
	pot = 0;
	sets = 0;
	allInCondition = false;
	lastActionPlayerID = 0;
	for (int i = 0; i < 5; i++) myCards[i] = 0;
	winners.Clear();
	playerNeedToShowCards.Clear();
}

void ClientBoard::setPlayerLists(PlayerList sl, PlayerList apl, PlayerList rpl)
{
	Mutex::Lock lock(m_syncMutex);
	seatsList = sl;
	activePlayerList = apl;
	runningPlayerList = rpl;
}

void ClientBoard::setMyCards(int* cards)
{
	Mutex::Lock lock(m_syncMutex);
	for (int i = 0; i < 5; i++) myCards[i] = cards[i];
}

void ClientBoard::getMyCards(int* cards) const
{
	Mutex::Lock lock(m_syncMutex);
	for (int i = 0; i < 5; i++) cards[i] = myCards[i];
}

const int* ClientBoard::getMyCards() const
{
	Mutex::Lock lock(m_syncMutex);
	return myCards;
}

int ClientBoard::getPot() const { Mutex::Lock lock(m_syncMutex); return pot; }
void ClientBoard::setPot(int val) { Mutex::Lock lock(m_syncMutex); pot = val; }
int ClientBoard::getSets() const { Mutex::Lock lock(m_syncMutex); return sets; }
void ClientBoard::setSets(int val) { Mutex::Lock lock(m_syncMutex); sets = val; }

void ClientBoard::setAllInCondition(bool value) { Mutex::Lock lock(m_syncMutex); allInCondition = value; }
void ClientBoard::setLastActionPlayerID(unsigned id) { Mutex::Lock lock(m_syncMutex); lastActionPlayerID = id; }

void ClientBoard::collectSets()
{
	Mutex::Lock lock(m_syncMutex);
	sets = 0;
	for (auto& p : *seatsList) sets += p->getMySet();
}

void ClientBoard::collectPot()
{
	Mutex::Lock lock(m_syncMutex);
	sets = 0;
	for (auto& p : *seatsList) sets += p->getMySet();
	pot += sets;
	sets = 0;
	for (auto& p : *seatsList) p->setMySetNull();
}

void ClientBoard::distributePot(unsigned) {}

void ClientBoard::determinePlayerNeedToShowCards()
{
	Mutex::Lock lock(m_syncMutex);
	playerNeedToShowCards.Clear();
	if (allInCondition) {
		for (auto& p : *activePlayerList) if (p->getMyAction() != PLAYER_ACTION_FOLD) playerNeedToShowCards.Add(p->getMyUniqueID());
	} else {
		auto lastActionPlayerIt = activePlayerList->end();
		for (auto it = activePlayerList->begin(); it != activePlayerList->end(); ++it)
			if ((*it)->getMyUniqueID() == lastActionPlayerID && (*it)->getMyAction() != PLAYER_ACTION_FOLD) { lastActionPlayerIt = it; break; }
		if (lastActionPlayerIt == activePlayerList->end())
			for (auto it = activePlayerList->begin(); it != activePlayerList->end(); ++it)
				if ((*it)->getMyAction() != PLAYER_ACTION_FOLD) { lastActionPlayerIt = it; break; }

		if (lastActionPlayerIt != activePlayerList->end()) {
			playerNeedToShowCards.Add((*lastActionPlayerIt)->getMyUniqueID());
			std::vector<std::pair<int, int>> level;
			level.push_back({((PokerPlayerInterface&)**lastActionPlayerIt).getMyCardsValueInt(), (*lastActionPlayerIt)->getMyRoundStartCash() - (*lastActionPlayerIt)->getMyCash()});
			auto it = lastActionPlayerIt; ++it;
			for (size_t i = 0; i < activePlayerList->size(); i++) {
				if (it == activePlayerList->end()) it = activePlayerList->begin();
				if ((*it)->getMyAction() != PLAYER_ACTION_FOLD) {
					bool show = false;
					for (size_t j = 0; j < (int)level.size(); ++j) {
						if (((PokerPlayerInterface&)**it).getMyCardsValueInt() > level[j].first) {
							if (j == (int)level.size() - 1) { show = true; level.push_back({((PokerPlayerInterface&)**it).getMyCardsValueInt(), (*it)->getMyRoundStartCash() - (*it)->getMyCash()}); break; }
						} else if (((PokerPlayerInterface&)**it).getMyCardsValueInt() == level[j].first) {
							if (j == (int)level.size() - 1 || (*it)->getMyRoundStartCash() - (*it)->getMyCash() > level[j+1].second) {
								show = true; if ((*it)->getMyRoundStartCash() - (*it)->getMyCash() > level[j].second) level[j].second = (*it)->getMyRoundStartCash() - (*it)->getMyCash();
							}
							break;
						} else if ((*it)->getMyRoundStartCash() - (*it)->getMyCash() > level[j].second) {
							show = true; level.insert(level.begin() + j, {((PokerPlayerInterface&)**it).getMyCardsValueInt(), (*it)->getMyRoundStartCash() - (*it)->getMyCash()}); break;
						}
					}
					if (show) playerNeedToShowCards.Add((*it)->getMyUniqueID());
				}
				++it;
			}
		}
	}
	Sort(playerNeedToShowCards);
	for(int i = 0; i < playerNeedToShowCards.GetCount() - 1; i++) if(playerNeedToShowCards[i] == playerNeedToShowCards[i+1]) { playerNeedToShowCards.Remove(i); i--; }
}

const Vector<unsigned>& ClientBoard::getWinners() const { Mutex::Lock lock(m_syncMutex); return winners; }
void ClientBoard::setWinners(const Vector<unsigned>& w) { Mutex::Lock lock(m_syncMutex); winners <<= w; }
const Vector<unsigned>& ClientBoard::getPlayerNeedToShowCards() const { Mutex::Lock lock(m_syncMutex); return playerNeedToShowCards; }
void ClientBoard::setPlayerNeedToShowCards(const Vector<unsigned>& p) { Mutex::Lock lock(m_syncMutex); playerNeedToShowCards <<= p; }

END_UPP_NAMESPACE
