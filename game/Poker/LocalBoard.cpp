#include <Poker/PokerHandInterface.h>
#include <Poker/PokerPlayerInterface.h>
#include <Poker/LocalBoard.h>
#include <GameRules/PlayerInterface.h>
#include <GameRules/GuiInterface.h>
#include <GameRules/Exception.h>
#include <algorithm>

NAMESPACE_UPP

namespace {

static const PokerPlayerInterface* GetPokerPlayer_Board(const PlayerInterface* player)
{
	return dynamic_cast<const PokerPlayerInterface*>(player);
}

static PokerPlayerInterface* GetPokerPlayer_Board(PlayerInterface* player)
{
	return dynamic_cast<PokerPlayerInterface*>(player);
}

}

LocalBoard::LocalBoard(GuiInterface *g, EngineLog *l)
	: myGui(g), myLog(l)
{
	reset();
}

LocalBoard::~LocalBoard() {}

void LocalBoard::reset()
{
	pot = 0;
	sets = 0;
	allInCondition = false;
	lastActionPlayerID = 0;
	for (int i = 0; i < 5; i++) myCards[i] = 0;
	playerNeedToShowCards.Clear();
	winners.Clear();
}

void LocalBoard::setPlayerLists(PlayerList sl, PlayerList apl, PlayerList rpl)
{
	seatsList = sl;
	activePlayerList = apl;
	runningPlayerList = rpl;
}

void LocalBoard::collectSets()
{
	sets = 0;
	if (seatsList) {
		for (auto& player : *seatsList) sets += player->getMySet();
	}
}

void LocalBoard::collectPot()
{
	pot += sets;
	sets = 0;
	if (seatsList) {
		for (auto& player : *seatsList) player->setMySetNull();
	}
}

void LocalBoard::distributePot(unsigned dealerPosition)
{
	winners.Clear();
	if (!seatsList) return;

	if (myGui && myGui->isVerbose()) {
		Cout() << "DEBUG: distributePot starting. Total Pot=" << pot << " DealerPos=" << dealerPosition << "\n";
		Cout().Flush();
	}

	std::vector<unsigned> playerSets;
	for (auto& player : *seatsList) {
		// total committed in this hand
		unsigned committed = (unsigned)(player->getMyRoundStartCash() - player->getMyCash());
		playerSets.push_back(committed);
		if (myGui && myGui->isVerbose() && committed > 0) {
			Cout() << "DEBUG:   Player " << player->getMyName() << " total committed=" << (int)committed << "\n";
			Cout().Flush();
		}
		player->setLastMoneyWon(0);
	}

	// Work on a copy of player contributions to calculate slices
	std::vector<unsigned> remainingSets = playerSets;
	
	while (true) {
		// Find the smallest non-zero contribution to define the next slice
		unsigned sliceHeight = 0;
		int contributors = 0;
		for (unsigned val : remainingSets) {
			if (val > 0) {
				if (sliceHeight == 0 || val < sliceHeight) sliceHeight = val;
				contributors++;
			}
		}
		
		if (sliceHeight == 0) break; // No more money to distribute

		unsigned sliceAmount = 0;
		std::vector<int> eligiblePlayerIndices;
		
		for (size_t i = 0; i < remainingSets.size(); i++) {
			if (remainingSets[i] > 0) {
				sliceAmount += sliceHeight;
				remainingSets[i] -= sliceHeight;
				
				// Player is eligible for this slice if they didn't fold
				if ((*seatsList)[i]->getMyAction() != PLAYER_ACTION_FOLD) {
					eligiblePlayerIndices.push_back((int)i);
				}
			}
		}

		if (myGui && myGui->isVerbose()) {
			Cout() << "DEBUG:   Slice Height=" << (int)sliceHeight << " Total Amount=" << (int)sliceAmount << " Eligible Players=" << (int)eligiblePlayerIndices.size() << "\n";
			Cout().Flush();
		}

		if (!eligiblePlayerIndices.empty()) {
			// Find winner(s) among eligible players
			int highestCardsValue = -1;
			for (int idx : eligiblePlayerIndices) {
				const auto* poker_player = GetPokerPlayer_Board((*seatsList)[idx].get());
				int val = poker_player ? poker_player->getMyCardsValueInt() : -1;
				if (val > highestCardsValue) highestCardsValue = val;
			}

			std::vector<int> sliceWinners;
			for (int idx : eligiblePlayerIndices) {
				const auto* poker_player = GetPokerPlayer_Board((*seatsList)[idx].get());
				if (poker_player && poker_player->getMyCardsValueInt() == highestCardsValue) {
					sliceWinners.push_back(idx);
				}
			}

			unsigned winAmount = sliceAmount / (unsigned)sliceWinners.size();
			unsigned mod = sliceAmount % (unsigned)sliceWinners.size();

			if (myGui && myGui->isVerbose()) {
				Cout() << "DEBUG:     Highest Value=" << highestCardsValue << " Winners=" << (int)sliceWinners.size() << " winAmount=" << (int)winAmount << "\n";
				Cout().Flush();
			}

			// Distribute winAmount
			for (int idx : sliceWinners) {
				auto& winner = (*seatsList)[idx];
				winner->setMyCash(winner->getMyCash() + (int)winAmount);
				winner->setLastMoneyWon(winner->getLastMoneyWon() + (int)winAmount);
				winners.Add(winner->getMyUniqueID());
			}
			
			// Distribute mod (remainder) starting from the player after dealer
			if (mod > 0) {
				auto it = seatsList->begin();
				for (; it != seatsList->end(); ++it) if ((*it)->getMyUniqueID() == dealerPosition) break;
				
				for (unsigned m = 0; m < mod; m++) {
					bool found = false;
					for (size_t k = 0; k < seatsList->size(); k++) {
						++it; if (it == seatsList->end()) it = seatsList->begin();
						// Check if this player is one of the slice winners
						for (int winnerIdx : sliceWinners) {
							if ((*seatsList)[winnerIdx]->getMyUniqueID() == (*it)->getMyUniqueID()) {
								(*it)->setMyCash((*it)->getMyCash() + 1);
								(*it)->setLastMoneyWon((*it)->getLastMoneyWon() + 1);
								found = true;
								break;
							}
						}
						if (found) break;
					}
				}
			}
		} else {
			// This slice has no eligible winners (everyone folded). 
			// In theory, the last folder should have taken the pot earlier, 
			// but if we are here, we give it to the dealer or just discard?
			// Standard rule: Pot goes to the last player who didn't fold.
			// If somehow everyone folded, we just log it.
			if (myGui && myGui->isVerbose()) {
				Cout() << "DEBUG:     NO ELIGIBLE WINNERS for this slice!\n";
				Cout().Flush();
			}
		}
	}

	Sort(winners);
	for(int i = 0; i < winners.GetCount() - 1; i++) if(winners[i] == winners[i+1]) { winners.Remove(i); i--; }
}

void LocalBoard::determinePlayerNeedToShowCards()
{
	playerNeedToShowCards.Clear();
	if (!activePlayerList) return;
	bool all_allin_or_folded = true;
	for (auto& player : *activePlayerList) {
		PlayerAction a = player->getMyAction();
		if (a != PLAYER_ACTION_FOLD && a != PLAYER_ACTION_ALLIN) {
			all_allin_or_folded = false;
			break;
		}
	}
	if (all_allin_or_folded) {
		for (auto& player : *activePlayerList)
			if (player->getMyAction() != PLAYER_ACTION_FOLD)
				playerNeedToShowCards.Add(player->getMyUniqueID());
		Sort(playerNeedToShowCards);
		for(int i = 0; i < playerNeedToShowCards.GetCount() - 1; i++) if(playerNeedToShowCards[i] == playerNeedToShowCards[i+1]) { playerNeedToShowCards.Remove(i); i--; }
		return;
	}
	if (allInCondition) {
		for (auto& player : *activePlayerList) if (player->getMyAction() != PLAYER_ACTION_FOLD) playerNeedToShowCards.Add(player->getMyUniqueID());
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
			int last_value = 0;
			if (const auto* poker_player = GetPokerPlayer_Board(lastActionPlayerIt->get()))
				last_value = poker_player->getMyCardsValueInt();
			level.push_back({last_value, (*lastActionPlayerIt)->getMyRoundStartCash() - (*lastActionPlayerIt)->getMyCash()});
			auto it = lastActionPlayerIt; ++it;
			for (size_t i = 0; i < activePlayerList->size(); i++) {
				if (it == activePlayerList->end()) it = activePlayerList->begin();
				if ((*it)->getMyAction() != PLAYER_ACTION_FOLD) {
					const auto* poker_player = GetPokerPlayer_Board(it->get());
					int cards_value = poker_player ? poker_player->getMyCardsValueInt() : 0;
					bool show = false;
					for (size_t j = 0; j < level.size(); ++j) {
						if (cards_value > level[j].first) {
							if (j == level.size() - 1) { show = true; level.push_back({cards_value, (*it)->getMyRoundStartCash() - (*it)->getMyCash()}); break; }
						} else if (cards_value == level[j].first) {
							if (j == level.size() - 1 || (*it)->getMyRoundStartCash() - (*it)->getMyCash() > level[j+1].second) {
								show = true; if ((*it)->getMyRoundStartCash() - (*it)->getMyCash() > level[j].second) level[j].second = (*it)->getMyRoundStartCash() - (*it)->getMyCash();
							}
							break;
						} else if ((*it)->getMyRoundStartCash() - (*it)->getMyCash() > level[j].second) {
							show = true; level.insert(level.begin() + j, {cards_value, (*it)->getMyRoundStartCash() - (*it)->getMyCash()}); break;
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

void LocalBoard::setMyCards(int* cards) { for(int i=0; i<5; i++) myCards[i] = cards[i]; }
void LocalBoard::getMyCards(int* cards) const { for(int i=0; i<5; i++) cards[i] = myCards[i]; }

const Vector<unsigned>& LocalBoard::getPlayerNeedToShowCards() const { return playerNeedToShowCards; }
const Vector<unsigned>& LocalBoard::getWinners() const { return winners; }
void LocalBoard::setAllInCondition(bool value) { allInCondition = value; }
void LocalBoard::setLastActionPlayerID(unsigned id) { lastActionPlayerID = id; }

END_UPP_NAMESPACE
