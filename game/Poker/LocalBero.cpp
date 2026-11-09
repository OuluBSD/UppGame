#include <Poker/PokerHandInterface.h>
#include <Poker/PokerPlayerInterface.h>
#include <Poker/LocalBero.h>
#include <GameRules/HandInterface.h>
#include <GameRules/BoardInterface.h>
#include <GameRules/PlayerInterface.h>
#include <GameRules/GuiInterface.h>
#include <EditorCommon/Tools.h>
#include <iostream>

NAMESPACE_UPP

LocalBero::LocalBero(HandInterface *hi, unsigned dP, int sB, TexasRound id)
	: myHand(hi), myBeRoID(id), dealerPosition(dP), smallBlindPositionId(0), bigBlindPositionId(0), smallBlind(sB), highestSet(0), minimumRaise(2 * sB), fullBetRule(false), firstRun(true), firstRunGui(true), firstRound(true), currentPlayersTurnId(0), firstRoundLastPlayersTurnId(0), logBoardCardsDone(false), nextPlayerLoopCounter(0)
{
	const auto* poker_hand = dynamic_cast<const PokerHandInterface*>(myHand);
	if (myBeRoID != GAME_STATE_PREFLOP) {
		firstRound = false;
		minimumRaise = 2 * sB;
	} else {
		highestSet = 2 * sB;
	}
	
	// Pre-calculate starting player
	if (myBeRoID == GAME_STATE_PREFLOP) {
		unsigned bbId = poker_hand ? poker_hand->getBigBlindPositionId() : dealerPosition;
		auto it = myHand->getActivePlayerIt(bbId);
		if (it != myHand->getActivePlayerList()->end()) {
			++it;
			if (it == myHand->getActivePlayerList()->end()) it = myHand->getActivePlayerList()->begin();
			currentPlayersTurnId = (*it)->getMyUniqueID();
		}
	} else {
		auto it = myHand->getActivePlayerIt(dealerPosition);
		if (it != myHand->getActivePlayerList()->end()) {
			++it;
			if (it == myHand->getActivePlayerIt(dealerPosition)) {
				// Special case for heads up? No, just follow the list
			}
			if (it == myHand->getActivePlayerList()->end()) it = myHand->getActivePlayerList()->begin();
			currentPlayersTurnId = (*it)->getMyUniqueID();
		}
	}
}

LocalBero::~LocalBero() {}



void LocalBero::start() {}

void LocalBero::reset()
{
	highestSet = 0;
	minimumRaise = 2 * smallBlind;
	fullBetRule = false;
	firstRun = true;
	firstRunGui = true;
	firstRound = true;
	if (myBeRoID != GAME_STATE_PREFLOP) {
		firstRound = false;
	} else {
		highestSet = 2 * smallBlind;
	}
	logBoardCardsDone = false;
	nextPlayerLoopCounter = 0;
	actionHistory.Clear();
}

int LocalBero::getHighestCardsValue() const { return 0; }



void LocalBero::recordAction(int playerID, PlayerAction action, int value)



{



	if (action == PLAYER_ACTION_FOLD) actionHistory.Add(0);



	else if (action == PLAYER_ACTION_CHECK || action == PLAYER_ACTION_CALL) actionHistory.Add(1);



	else if (action == PLAYER_ACTION_BET || action == PLAYER_ACTION_RAISE || action == PLAYER_ACTION_ALLIN) {



		double pot = 0;



		if (myHand && myHand->getBoard())



			pot = (double)myHand->getBoard()->getPot();



		



		if (action == PLAYER_ACTION_ALLIN && value <= highestSet) {







			actionHistory.Add(1); // Call all-in



		} else {



			// In recordAction, 'value' is the total committed in this round (mySet)



			// A raise/bet size is the difference from previous high bet



			int incremental = value - highestSet;



			if (pot > 0) {



				double rel = (double)incremental / pot;



				if (rel > 0.85) actionHistory.Add(3); // POT



				else if (rel > 0.35) actionHistory.Add(2); // 0.5 POT



				else actionHistory.Add(2); // Default



			} else {



				actionHistory.Add(2);



			}



		}



	}



}





void LocalBero::nextPlayer()
{
	PlayerList rpl = myHand->getRunningPlayerList();

	for(auto it = rpl->begin(); it != rpl->end(); ) {
		if((*it)->getMyAction() == PLAYER_ACTION_FOLD || (*it)->getMyAction() == PLAYER_ACTION_ALLIN) {
			it = rpl->erase(it);
		} else ++it;
	}

	if (rpl->empty()) {
		myHand->switchRounds();
		return;
	}
	if (rpl->size() == 1) {
		auto only = rpl->begin();
		// If a lone non-allin player is still below highest set, they must still respond
		// to an outstanding all-in/raise instead of auto-ending the round.
		if ((*only)->getMyAction() != PLAYER_ACTION_FOLD &&
			(*only)->getMyAction() != PLAYER_ACTION_ALLIN &&
			(*only)->getMySet() < highestSet &&
			(*only)->getMyCash() > 0) {
			currentPlayersTurnId = (*only)->getMyUniqueID();
		}
		else {
			myHand->switchRounds();
			return;
		}
	}

	if (firstRun) {
		if (myBeRoID == GAME_STATE_PREFLOP) {
			const auto* poker_hand = dynamic_cast<const PokerHandInterface*>(myHand);
			unsigned bbId = poker_hand ? poker_hand->getBigBlindPositionId() : dealerPosition;
			auto it = myHand->getRunningPlayerIt(bbId);
			if (it == rpl->end())
				it = rpl->begin();
			else {
				++it;
				if (it == rpl->end()) it = rpl->begin();
			}
			currentPlayersTurnId = (*it)->getMyUniqueID();
		}
		else {
			auto it = myHand->getRunningPlayerIt(dealerPosition);
			if (it == rpl->end())
				it = rpl->begin();
			else {
				++it;
				if (it == rpl->end()) it = rpl->begin();
			}
			currentPlayersTurnId = (*it)->getMyUniqueID();
		}
		firstRun = false;
	}

	auto round_complete = [&]() -> bool {
		const int highest = highestSet;
		for (auto& p : *rpl) {
			if (p->getMySet() != highest)
				return false;
			PlayerAction a = p->getMyAction();
			if (a == PLAYER_ACTION_NONE || a == PLAYER_ACTION_SMALL_BLIND || a == PLAYER_ACTION_BIG_BLIND)
				return false;
		}
		return true;
	};
	if (round_complete()) {
		currentPlayersTurnId = (unsigned)-1;
		myHand->switchRounds();
		return;
	}

	auto currentPlayerIt = myHand->getRunningPlayerIt(currentPlayersTurnId);
	if (currentPlayerIt == rpl->end()) {
		currentPlayersTurnId = (*rpl->begin())->getMyUniqueID();
		currentPlayerIt = rpl->begin();
	}

	// Human turn is input-driven: keep turn id on human until action is actually set.
	if ((*currentPlayerIt)->getMyType() == PLAYER_TYPE_HUMAN) {
		PlayerAction a = (*currentPlayerIt)->getMyAction();
		// Human must act if they have no action yet, or if highest bet increased above their current set.
		bool waiting_for_input = (a == PLAYER_ACTION_NONE || a == PLAYER_ACTION_SMALL_BLIND || a == PLAYER_ACTION_BIG_BLIND ||
		                          (*currentPlayerIt)->getMySet() < highestSet);
		if (waiting_for_input) {
			for (auto& p : *myHand->getActivePlayerList())
				p->setMyTurn(false);
			(*currentPlayerIt)->setMyTurn(true);
			if (myHand->getGuiInterface())
				myHand->getGuiInterface()->meInAction();
			return;
		}
		++currentPlayerIt;
		if (currentPlayerIt == rpl->end())
			currentPlayerIt = rpl->begin();
	}

	if (myHand && myHand->getGuiInterface() && myHand->getGuiInterface()->isVerbose()) {
		std::cout << "DEBUG: [" << (int)myBeRoID << "] Action turn: " << (*currentPlayerIt)->getMyName().ToStd() << " (UID " << currentPlayersTurnId << ") Set=" << (*currentPlayerIt)->getMySet() << " Action=" << (int)(*currentPlayerIt)->getMyAction() << std::endl;
	}

	for (auto& p : *myHand->getActivePlayerList())
		p->setMyTurn(false);
	(*currentPlayerIt)->setMyTurn(true);

	auto nextIt = currentPlayerIt;
	++nextIt;
	if (nextIt == rpl->end())
		nextIt = rpl->begin();
	currentPlayersTurnId = (*nextIt)->getMyUniqueID();

	if (myHand && myHand->getGuiInterface() && myHand->getGuiInterface()->isVerbose()) {
		std::cout << "DEBUG: [" << (int)myBeRoID << "] Calling action() for " << (*currentPlayerIt)->getMyName().ToStd() << std::endl;
	}

	if ((*currentPlayerIt)->getMyType() == PLAYER_TYPE_HUMAN) {
		if (myHand->getGuiInterface())
			myHand->getGuiInterface()->meInAction();
		return;
	}
	(*currentPlayerIt)->action();
}

void LocalBero::run()
{
	nextPlayer();
}

void LocalBeroPreflop::run()
{
	nextPlayer();
}

LocalBeroPreflop::LocalBeroPreflop(HandInterface* hi, unsigned dP, int sB)
	: LocalBero(hi, dP, sB, GAME_STATE_PREFLOP)
{
}

LocalBeroPostRiver::LocalBeroPostRiver(HandInterface* hi, unsigned dP, int sB)
	: LocalBero(hi, dP, sB, GAME_STATE_POST_RIVER), highestCardsValue(0)
{
}

void LocalBeroPostRiver::postRiverRun()
{
	getMyHand()->switchRounds();
}

END_UPP_NAMESPACE
