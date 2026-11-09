#include <Poker/PokerHandInterface.h>
#include <Poker/PokerPlayerInterface.h>
#include <Poker/LocalHand.h>
#include <GameRules/Game.h>
#include <iostream>
#include <EditorCommon/Tools.h>
#include <Poker/CardsValue.h>
#include <GameRules/EngineFactory.h>
#include <GameRules/GuiInterface.h>
#include <GameRules/EngineLog.h>
#include <GameRules/BoardInterface.h>
#include <GameRules/PlayerInterface.h>
#include <GameRules/BeroInterface.h>
#include <GameRules/Exception.h>
#include <GameRules/LocalPlayer.h>

NAMESPACE_UPP

namespace {

static PokerPlayerInterface* GetPokerPlayer_Hand(PlayerInterface* player)
{
	return dynamic_cast<PokerPlayerInterface*>(player);
}

static const PokerPlayerInterface* GetPokerPlayer_Hand(const PlayerInterface* player)
{
	return dynamic_cast<const PokerPlayerInterface*>(player);
}

}

LocalHand::LocalHand(std::shared_ptr<EngineFactory> f, GuiInterface *g, std::shared_ptr<BoardInterface> b, EngineLog *l, PlayerList sl, PlayerList apl, PlayerList rpl, int id, int sP, unsigned dP, int sB, int sC, int seed, GameType gt)
	: myFactory(f), myGui(g), myBoard(b), myLog(l), seatsList(sl), activePlayerList(apl), runningPlayerList(rpl), myID(id), startQuantityPlayers(sP), dealerPosition(dP), smallBlindPosition(dP), bigBlindPosition(dP), currentRound(GAME_STATE_PREFLOP), roundBeforePostRiver(GAME_STATE_PREFLOP), smallBlind(sB), startCash(sC), previousPlayerID(-1), lastActionPlayerID(0), allInCondition(false), cardsShown(false), my_game_type(gt)
{
	hearts_switching = false;
	for (int i = 0; i < MAX_NUMBER_OF_PLAYERS; i++) hearts_scores[i] = 0;
	for (int i = 0; i < (int)activePlayerList->size(); i++) {
		hearts_scores[i] = 100 - (*activePlayerList)[i]->getMyCash();
	}
	hearts_broken = false;
	hearts_trick_suit = -1;
	hearts_trick_count = 0;
	trick_lead_player = 0;
	hearts_passing_direction = myID % 4;
	
	myBoard->reset();
	switchRoundsLoopCounter = 0;
	for(auto& player : *seatsList) {
		player->setHand(this);
		if (auto* poker_player = GetPokerPlayer_Hand(player.get()))
			poker_player->setMyCardsFlip(false, 0);
	}

	const int NumCards = 52;
	int cardsArray[NumCards];
	for (int i = 0; i < NumCards; i++) cardsArray[i] = i;

	if (seed >= 0) {
		Tools::SeedRandom(seed);
	}
	Tools::ShuffleArrayNonDeterministic(cardsArray, NumCards);

	int hand_size = getHandSize();
	int tempBoardArray[5];
	int poker_offset = (my_game_type == GAME_TYPE_HEARTS) ? 0 : 5;
	
	if (my_game_type != GAME_TYPE_HEARTS) {
		for(int i = 0; i < 5; i++) tempBoardArray[i] = cardsArray[i];
		myBoard->setMyCards(tempBoardArray);
	}

	int k = 0;
	for(auto& player : *activePlayerList) {
		int tempPlayerArray[13];
		for(int j = 0; j < hand_size; j++) tempPlayerArray[j] = cardsArray[hand_size*k+j+poker_offset];
		player->setMyCards(tempPlayerArray, hand_size);
		
		int bestHandPos[5] = { -1, -1, -1, -1, -1 };
		if (auto* poker_player = GetPokerPlayer_Hand(player.get())) {
			poker_player->setMyCardsValueInt(0);
			poker_player->setMyBestHandPosition(bestHandPos);
		}
		player->setMyRoundStartCash(player->getMyCash());
		k++;
	}

	auto beroVec = myFactory->createBeRo(this, dealerPosition, smallBlind);
	for (auto& b : beroVec) myBeRo.Add((int)b->getMyBeRoID(), b);
	currentRound = GAME_STATE_PREFLOP;
	}
LocalHand::~LocalHand() {}

void LocalHand::resetLoopCounters() {
	switchRoundsLoopCounter = 0;
	for (int i = 0; i < myBeRo.GetCount(); i++) myBeRo[i]->resetLoopCounters();
}

void LocalHand::start() {
	if (my_game_type == GAME_TYPE_HEARTS) {
		hearts_round = HEARTS_STATE_PASSING;
		hearts_broken = false;
		hearts_trick_suit = -1;
		hearts_trick_count = 0;
		current_trick_cards.Clear();
		
		const int NumCards = 52;
		int cardsArray[NumCards];
		for (int i = 0; i < NumCards; i++) cardsArray[i] = i;
		Tools::ShuffleArrayNonDeterministic(cardsArray, NumCards);
		
		int k = 0;
		for(auto& player : *activePlayerList) {
			int tempPlayerArray[13];
			for(int j = 0; j < 13; j++) tempPlayerArray[j] = cardsArray[13*k+j];
			player->setMyCards(tempPlayerArray, 13);
			player->setMyAction(PLAYER_ACTION_NONE);
			k++;
		}
	} else {
		assignButtons();
		setBlinds();
	}

	myGui->nextRoundCleanGui();
	resetLoopCounters();
	
	if (my_game_type != GAME_TYPE_HEARTS) {
		auto bero = getCurrentBeRo();
		auto it_sB = getActivePlayerIt(smallBlindPosition);
		auto it_bB = getActivePlayerIt(bigBlindPosition);
		if(it_sB != activePlayerList->end() && it_bB != activePlayerList->end()) {
			myGui->logNewBlindsSetsMsg((*it_sB)->getMySet(), (*it_bB)->getMySet(), (*it_sB)->getMyName(), (*it_bB)->getMyName());
		}
		myGui->dealHoleCards();
		myBoard->collectSets();
		myGui->refreshPot();
		bero->setSmallBlindPositionId(smallBlindPosition);
		bero->setBigBlindPositionId(bigBlindPosition);
		getCurrentBeRo()->nextPlayer();
	} else {
		switchHeartsRounds();
	}
}

void LocalHand::setCurrentRound(TexasRound theValue) {
	currentRound = theValue;
	if (myLog) myLog->setCurrentRound(currentRound);
}

void LocalHand::assignButtons() {
	for (auto& player : *seatsList) player->setMyButton(GBUTTON_NONE);
	auto it = getSeatIt(dealerPosition);
	if(it == seatsList->end()) throw LocalException(__FILE__, __LINE__, ERR_SEAT_NOT_FOUND);
	(*it)->setMyButton(GBUTTON_DEALER);
	auto nextActive = [&](auto it) {
		auto n = it;
		while (true) {
			++n; if (n == seatsList->end()) n = seatsList->begin();
			if (getActivePlayerIt((*n)->getMyUniqueID()) != activePlayerList->end()) return n;
		}
	};
	auto it_sB = nextActive(it);
	smallBlindPosition = (*it_sB)->getMyUniqueID();
	if (activePlayerList->size() > 2) (*it_sB)->setMyButton(GBUTTON_SMALL_BLIND);
	auto it_bB = nextActive(it_sB);
	bigBlindPosition = (*it_bB)->getMyUniqueID();
	(*it_bB)->setMyButton(GBUTTON_BIG_BLIND);
}

void LocalHand::setBlinds() {
	auto it_sB = getActivePlayerIt(smallBlindPosition);
	auto it_bB = getActivePlayerIt(bigBlindPosition);
	if (it_sB != activePlayerList->end()) {
		int val = min((*it_sB)->getMyCash(), smallBlind);
		(*it_sB)->setMySet(val);
		(*it_sB)->setMyAction(PLAYER_ACTION_SMALL_BLIND);
	}
	if (it_bB != activePlayerList->end()) {
		int val = min((*it_bB)->getMyCash(), smallBlind * 2);
		(*it_bB)->setMySet(val);
		(*it_bB)->setMyAction(PLAYER_ACTION_BIG_BLIND);
	}
}

void LocalHand::switchRounds() {
	if (my_game_type == GAME_TYPE_HEARTS) { switchHeartsRounds(); return; }
	for(auto it = runningPlayerList->begin(); it != runningPlayerList->end(); ) {
		if((*it)->getMyAction() == PLAYER_ACTION_FOLD || (*it)->getMyAction() == PLAYER_ACTION_ALLIN) it = runningPlayerList->erase(it);
		else ++it;
	}
	if (runningPlayerList->size() <= 1 || allInCondition) currentRound = GAME_STATE_POST_RIVER;
	else {
		if (currentRound < GAME_STATE_RIVER) {
			// End of betting round: move current round commitments into pot and reset round sets.
			myBoard->collectPot();
			myGui->refreshPot();
			setCurrentRound((TexasRound)(currentRound + 1));
			int bc[5] = { -1, -1, -1, -1, -1 };
			myBoard->getMyCards(bc);
			if (currentRound == GAME_STATE_FLOP)
				myGui->logDealBoardCardsMsg((int)currentRound, bc[0], bc[1], bc[2], -1, -1);
			else if (currentRound == GAME_STATE_TURN)
				myGui->logDealBoardCardsMsg((int)currentRound, bc[0], bc[1], bc[2], bc[3], -1);
			else if (currentRound == GAME_STATE_RIVER)
				myGui->logDealBoardCardsMsg((int)currentRound, bc[0], bc[1], bc[2], bc[3], bc[4]);
			for (auto& p : *activePlayerList) {
				PlayerAction a = p->getMyAction();
				if (a != PLAYER_ACTION_ALLIN && a != PLAYER_ACTION_FOLD)
					p->setMyAction(PLAYER_ACTION_NONE);
			}
			getCurrentBeRo()->nextPlayer();
		} else currentRound = GAME_STATE_POST_RIVER;
	}
	if (currentRound == GAME_STATE_POST_RIVER) {
		// Final betting round commitments must be moved into pot and cleared from players.
		// Otherwise next hand can inherit stale per-round sets (causing wrong blind logs/turn loops).
		myBoard->collectPot();

		// Evaluate hand strengths for all non-folded players before showdown distribution.
		int board_cards[5] = { -1, -1, -1, -1, -1 };
		myBoard->getMyCards(board_cards);
		for (auto& p : *activePlayerList) {
			if (p->getMyAction() == PLAYER_ACTION_FOLD)
				continue;
			
			Vector<int> hole = p->getMyCards();
			if (hole.IsEmpty()) continue;
			
			int v = 0;
			if (my_game_type == GAME_TYPE_PLO) {
				std::array<unsigned, 4> h;
				std::array<unsigned, 5> b;
				for(int i=0; i<4; i++) h[i] = (i < hole.GetCount() ? hole[i] : 0);
				for(int i=0; i<5; i++) b[i] = (board_cards[i] >= 0 ? board_cards[i] : 0);
				v = myFactory->getEvaluator()->evaluateOmaha(h, b);
			} else if (my_game_type == GAME_TYPE_PLO5) {
				std::array<unsigned, 5> h;
				std::array<unsigned, 5> b;
				for(int i=0; i<5; i++) h[i] = (i < hole.GetCount() ? hole[i] : 0);
				for(int i=0; i<5; i++) b[i] = (board_cards[i] >= 0 ? board_cards[i] : 0);
				v = myFactory->getEvaluator()->evaluateOmaha5(h, b);
			} else {
				int cards7[7] = { hole[0], hole[1], board_cards[0], board_cards[1], board_cards[2], board_cards[3], board_cards[4] };
				int best_pos[5] = { -1, -1, -1, -1, -1 };
				v = CardsValue::cardsValue(cards7, best_pos);
				if (auto* poker_player = GetPokerPlayer_Hand(p.get()))
					poker_player->setMyBestHandPosition(best_pos);
			}
			if (auto* poker_player = GetPokerPlayer_Hand(p.get()))
				poker_player->setMyCardsValueInt(v);
		}

		myBoard->determinePlayerNeedToShowCards();
		for (unsigned uid : myBoard->getPlayerNeedToShowCards()) {
			auto it = getActivePlayerIt(uid);
			if (it == activePlayerList->end())
				continue;
			int c1 = -1, c2 = -1;
			(*it)->getMyCards(c1, c2);
			if (c1 < 0 || c2 < 0)
				continue;
			int cards_value = 0;
			if (const auto* poker_player = GetPokerPlayer_Hand(it->get()))
				cards_value = poker_player->getMyCardsValueInt();
			myGui->logFlipHoleCardsMsg((*it)->getMyName(), c1, c2, cards_value, "shows");
		}
		myBoard->distributePot(dealerPosition);
		for (auto& p : *activePlayerList) {
			int won = p->getLastMoneyWon();
			if (won <= 0)
				continue;
			int cards_value = 0;
			if (const auto* poker_player = GetPokerPlayer_Hand(p.get()))
				cards_value = poker_player->getMyCardsValueInt();
			String hand = CardsValue::determineHandName(cards_value, activePlayerList);
			myGui->logWinningHandMsg(p->getMyName(), hand, won);
		}
		myGui->refreshCash();
		myGui->refreshPot();
		myGui->postRiverAnimation1();
	}
}

void LocalHand::switchHeartsRounds() {
	if (hearts_switching) return;
	hearts_switching = true;
	int master_lc = 0;
	while (hearts_round != HEARTS_STATE_POST_GAME && master_lc++ < 2000) {
		if (hearts_round == HEARTS_STATE_PASSING) {
			bool all_passed = true;
			for (auto& p : *activePlayerList) if (p->getMyAction() != PLAYER_ACTION_PASS_CARDS) all_passed = false;
			if (all_passed) {
				if (hearts_passing_direction != 3) {
					int offset = 1;
					if (hearts_passing_direction == 1) offset = (int)activePlayerList->size() - 1;
					else if (hearts_passing_direction == 2) offset = (int)activePlayerList->size() / 2;
					Vector<Vector<int>> passed_cards;
					passed_cards.SetCount(activePlayerList->size());
					for (int i = 0; i < (int)activePlayerList->size(); i++) {
						Vector<int> hand = (*activePlayerList)[i]->getMyCards();
						for (int j = 0; j < 3; j++) { passed_cards[i].Add(hand[0]); hand.Remove(0); }
						(*activePlayerList)[i]->setMyCards(hand.begin(), hand.GetCount());
					}
					for (int i = 0; i < (int)activePlayerList->size(); i++) {
						int target = (i + offset) % (int)activePlayerList->size();
						Vector<int> hand = (*activePlayerList)[target]->getMyCards();
						for (int c : passed_cards[i]) hand.Add(c);
						(*activePlayerList)[target]->setMyCards(hand.begin(), hand.GetCount());
					}
				}
				hearts_round = HEARTS_STATE_TRICKS;
				hearts_trick_count = 0;
				trick_lead_player = 0;
				for (int i = 0; i < (int)activePlayerList->size(); i++) {
					Vector<int> hand = (*activePlayerList)[i]->getMyCards();
					for (int c : hand) if (c == ((2 << 2) | 3)) { trick_lead_player = i; break; }
				}
				for (auto& p : *activePlayerList) { p->setMyAction(PLAYER_ACTION_NONE); p->setMyTurn(false); }
			} else {
				for (int i = 0; i < (int)activePlayerList->size(); i++) {
					auto p = (*activePlayerList)[i];
					if (p->getMyAction() != PLAYER_ACTION_PASS_CARDS) {
						p->setMyTurn(true); p->action(); p->setMyTurn(false); break;
					}
				}
			}
		} else if (hearts_round == HEARTS_STATE_TRICKS) {
			if (current_trick_cards.GetCount() == (int)activePlayerList->size()) {
				int num_p = (int)activePlayerList->size();
				std::vector<unsigned> trick;
				for (int i = 0; i < num_p; i++) trick.push_back((unsigned)current_trick_cards[i]);
				int lead_suit = current_trick_cards[0] & 3;
				int local_winner_idx = myFactory->getEvaluator()->evaluateHeartsTrick(trick, lead_suit);
				int winner_id = (trick_lead_player + local_winner_idx) % num_p;
				int trick_points = 0;
				for (int c : current_trick_cards) {
					int suit = c & 3; int rank = c >> 2;
					if (suit == 2) { trick_points++; hearts_broken = true; }
					if (suit == 0 && rank == 10) { trick_points += 13; hearts_broken = true; }
				}
				hearts_scores[winner_id] += trick_points;
				for (int i = 0; i < (int)activePlayerList->size(); i++) (*activePlayerList)[i]->setMyCash(100 - hearts_scores[i]);
				trick_lead_player = winner_id;
				current_trick_cards.Clear();
				hearts_trick_suit = -1;
				hearts_trick_count++;
				if (hearts_trick_count == 13) {
					hearts_round = HEARTS_STATE_POST_GAME;
					for (int i = 0; i < (int)activePlayerList->size(); i++) {
						if (hearts_scores[i] == 26) {
							for (int j = 0; j < (int)activePlayerList->size(); j++) hearts_scores[j] = (i == j ? 0 : 26);
							break;
						}
					}
					String msg = "Hearts Hand Finished. Scores: ";
					for (int i = 0; i < (int)activePlayerList->size(); i++) msg << (*activePlayerList)[i]->getMyName() << "=" << hearts_scores[i] << " ";
					Upp::Cout() << msg << "\n"; Upp::Cout().Flush();
				}
			} else {
				int next_idx = (trick_lead_player + current_trick_cards.GetCount()) % (int)activePlayerList->size();
				auto p = (*activePlayerList)[next_idx];
				int before = current_trick_cards.GetCount();
				p->setMyTurn(true); p->action(); p->setMyTurn(false);
				if (current_trick_cards.GetCount() <= before) break; 
			}
		}
	}
	hearts_switching = false;
}

bool LocalHand::validateHeartsPlay(int player_id, int card) {
	auto it = getActivePlayerIt(player_id);
	if (it == activePlayerList->end()) return false;
	Vector<int> hand = (*it)->getMyCards();
	int suit = card & 3; int rank = card >> 2;
	if (hearts_trick_suit != -1 && suit != hearts_trick_suit) {
		for (int c : hand) if ((c & 3) == hearts_trick_suit) return false;
	}
	if (hearts_trick_suit == -1 && suit == 2 && !hearts_broken) {
		bool has_other = false;
		for (int c : hand) if ((c & 3) != 2) { has_other = true; break; }
		if (has_other) return false;
	}
	if (hearts_trick_count == 0) {
		bool is_point = (suit == 2 || (suit == 0 && rank == 10));
		if (is_point) {
			bool has_non_p = false;
			for (int c : hand) {
				int cs = c & 3; int cr = c >> 2;
				if (!(cs == 2 || (cs == 0 && cr == 10))) { has_non_p = true; break; }
			}
			if (has_non_p) return false;
		}
	}
	return true;
}

void LocalHand::playHeartsCard(int player_id, int card) {
	if (!validateHeartsPlay(player_id, card)) return;
	auto it = getActivePlayerIt(player_id);
	if (it == activePlayerList->end()) return;
	Vector<int> hand = (*it)->getMyCards();
	for (int i = 0; i < hand.GetCount(); i++) {
		if (hand[i] == card) { hand.Remove(i); (*it)->setMyCards(hand.begin(), hand.GetCount()); break; }
	}
	current_trick_cards.Add(card);
	if (current_trick_cards.GetCount() == 1) hearts_trick_suit = card & 3;
	(*it)->setMyTurn(false);
}

std::shared_ptr<PlayerInterface> LocalHand::getPlayerByUniqueId(unsigned id) const {
	for(auto& p : *seatsList) if(p->getMyUniqueID() == id) return p;
	return nullptr;
}

PlayerListIterator LocalHand::getSeatIt(unsigned id) const { for(auto it = seatsList->begin(); it != seatsList->end(); ++it) if((*it)->getMyUniqueID() == id) return it; return seatsList->end(); }
PlayerListIterator LocalHand::getActivePlayerIt(unsigned id) const { for(auto it = activePlayerList->begin(); it != activePlayerList->end(); ++it) if((*it)->getMyUniqueID() == id) return it; return activePlayerList->end(); }
PlayerListIterator LocalHand::getRunningPlayerIt(unsigned id) const { for(auto it = runningPlayerList->begin(); it != runningPlayerList->end(); ++it) if((*it)->getMyUniqueID() == id) return it; return runningPlayerList->end(); }
void LocalHand::setLastActionPlayerID(unsigned theValue) { lastActionPlayerID = theValue; myBoard->setLastActionPlayerID(theValue); }

bool LocalHand::isVerbose() const {
	return m_game ? m_game->isVerbose() : false;
}

END_UPP_NAMESPACE
