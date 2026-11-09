#include <GameRules/LocalPlayer.h>
#include <GameRules/HandInterface.h>
#include <GameRules/GuiInterface.h>

NAMESPACE_UPP

LocalPlayer::LocalPlayer(class ConfigFile *c, int id, unsigned uniqueId, PlayerType type, String name, String avatar, int sC, bool aS, bool sotS, int mB)
	: myConfig(c), currentHand(nullptr), myID(id), myUniqueID(uniqueId), myType(type), myName(name), myAvatar(avatar),
	  myCash(sC), mySet(0), myLastRelativeSet(0), myAction(PLAYER_ACTION_NONE),
	  myButton(mB), myActiveStatus(aS), myStayOnTableStatus(sotS), myTurn(false), myRoundStartCash(0), lastMoneyWon(0),
	  m_actionTimeoutCounter(0), m_isSessionActive(false), m_isKicked(false), m_isMuted(false)
{
	for(int i=0; i<13; i++) myCards[i] = -1;
	myCardsCount = 0;
	m_lastRemoteActionTimer.Reset();
}

LocalPlayer::~LocalPlayer()
{
}

void LocalPlayer::setMyCards(const int* theValue, int count)
{
	myCardsCount = min(count, 13);
	for(int i=0; i < myCardsCount; i++) myCards[i] = theValue[i];
	for(int i=myCardsCount; i < 13; i++) myCards[i] = -1;
}

Vector<int> LocalPlayer::getMyCards() const
{
	Vector<int> v;
	for(int i=0; i < myCardsCount; i++) v.Add(myCards[i]);
	return v;
}

void LocalPlayer::getMyCards(int& card1, int& card2) const
{
	card1 = myCardsCount > 0 ? myCards[0] : -1;
	card2 = myCardsCount > 1 ? myCards[1] : -1;
}

void LocalPlayer::setMySet(int theValue)
{
	if (theValue <= 0) {
		myLastRelativeSet = 0;
		return;
	}
	int inc = min(theValue, myCash);
	myLastRelativeSet = inc;
	myCash -= inc;
	mySet += inc;
}

void LocalPlayer::setMyAction(PlayerAction theValue, bool human)
{
	if (!currentHand) return;
	myAction = theValue;
	currentHand->getGuiInterface()->refreshAction(myID, (int)myAction);
}

bool LocalPlayer::checkIfINeedToShowCards()
{
	return false;
}

void LocalPlayer::markRemoteAction()
{
	m_lastRemoteActionTimer.Reset();
}

unsigned LocalPlayer::getTimeSecSinceLastRemoteAction() const
{
	return (unsigned)(m_lastRemoteActionTimer.Elapsed() / 1000);
}

END_UPP_NAMESPACE
