#include <Poker/RandomBotPlayer.h>
#include <GameRules/HandInterface.h>
#include <EditorCommon/Tools.h>
#include <GameRules/BeroInterface.h>
#include <GameRules/GuiInterface.h>

NAMESPACE_UPP

RandomBotPlayer::RandomBotPlayer(class ConfigFile *c, int id, unsigned uniqueId, String name, String avatar, int sC, bool aS, bool sotS, int mB)
	: PokerLocalPlayer(c, id, uniqueId, PLAYER_TYPE_COMPUTER, name, avatar, sC, aS, sotS, mB)
{
}

RandomBotPlayer::~RandomBotPlayer()
{
}

void RandomBotPlayer::setMyAction(PlayerAction action, bool human) {
	LocalPlayer::setMyAction(action, human);
}

void RandomBotPlayer::action()
{
	if (!currentHand) { LocalPlayer::action(); return; }

	int highestSet = currentHand->getCurrentBeRo()->getHighestSet();
	int callValue = highestSet - mySet;
	int minRaise = currentHand->getCurrentBeRo()->getMinimumRaise();
	
	// Very simple decision logic (Random/Legal)
	int r; Tools::GetRand(0, 100, 1, &r);
	if (callValue == 0) {
		if (r < 20) { // Bet
			int bet = minRaise;
			if (bet > myCash) bet = myCash;
			setMySet(bet);
			if (mySet > currentHand->getCurrentBeRo()->getHighestSet())
				currentHand->getCurrentBeRo()->setHighestSet(mySet);
			setMyAction(bet == myCash ? PLAYER_ACTION_ALLIN : PLAYER_ACTION_BET);
		} else { // Check
			setMyAction(PLAYER_ACTION_CHECK);
		}
	} else {
		if (r < 10 && myCash > callValue) { // Raise
			int raiseInc = callValue + minRaise;
			if (raiseInc > myCash) raiseInc = myCash;
			setMySet(raiseInc);
			if (mySet > currentHand->getCurrentBeRo()->getHighestSet())
				currentHand->getCurrentBeRo()->setHighestSet(mySet);
			setMyAction(myCash == 0 ? PLAYER_ACTION_ALLIN : PLAYER_ACTION_RAISE);
		} else if (r < 85) { // Call
			int callInc = callValue;
			if (callInc > myCash) callInc = myCash;
			setMySet(callInc);
			setMyAction(myCash == 0 ? PLAYER_ACTION_ALLIN : PLAYER_ACTION_CALL);
		} else { // Fold
			setMyAction(PLAYER_ACTION_FOLD);
		}
	}
	
	myTurn = false;
	currentHand->setPreviousPlayerID(myID);
	// Log using the total committed for the round for bets/raises
	int logValue = (myAction == PLAYER_ACTION_BET || myAction == PLAYER_ACTION_RAISE) ? mySet : myLastRelativeSet;

	// Always notify GuiInterface (it might be a TestGui or a real GUI)
	if (currentHand->getGuiInterface()) {
		currentHand->getGuiInterface()->logPlayerActionMsg(myName, (int)myAction, logValue);
		currentHand->getGuiInterface()->nextPlayerAnimation();
	}
}

END_UPP_NAMESPACE
