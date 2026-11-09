#include "ClientPlayer.h"
#include <GameRules/HandInterface.h>
#include <GameRules/BoardInterface.h>
#include <GameRules/GuiInterface.h>
#include <EditorCommon/ConfigFile.h>

NAMESPACE_UPP

ClientPlayer::ClientPlayer(class ConfigFile *c, int id, unsigned uniqueId, PlayerType type, String name, String avatar, int sC, bool aS, bool sotS, int mB)
	: myConfig(c), currentHand(nullptr), myID(id), myUniqueID(uniqueId), myType(type),
	  myName(name), myAvatar(avatar), myDude(0), myDude4(0), myCardsValueInt(0), myOdds(-1.0), logHoleCardsDone(false), myCash(sC), mySet(0), myLastRelativeSet(0),
	  myAction(PLAYER_ACTION_NONE), myButton(mB), myActiveStatus(aS), myStayOnTableStatus(sotS), myTurn(false), myCardsFlip(false), myRoundStartCash(0),
	  lastMoneyWon(0), sBluff(0), sBluffStatus(false), m_isSessionActive(false), m_isKicked(false), m_isMuted(false), m_actionTimeoutCounter(0)
{
	for(int i=0; i<5; i++) myBestHandPosition[i] = 0;
	myNiveau[0] = myNiveau[1] = myNiveau[2] = 0;
	for (int i = 0; i < 13; i++) myCards[i] = -1;
	myCardsCount = 0;
	for(int i=0; i<4; i++) myAverageSets[i] = 0;
	for(int i=0; i<7; i++) myAggressive[i] = false;
}

ClientPlayer::~ClientPlayer() {}

void ClientPlayer::setHand(HandInterface* br) { Mutex::Lock lock(m_syncMutex); currentHand = br; }
HandInterface* ClientPlayer::getHand() const { Mutex::Lock lock(m_syncMutex); return currentHand; }
void ClientPlayer::setMyID(int id) { Mutex::Lock lock(m_syncMutex); myID = id; }
int ClientPlayer::getMyID() const { return myID; }
void ClientPlayer::setMyUniqueID(unsigned newId) { Mutex::Lock lock(m_syncMutex); myUniqueID = newId; }
unsigned ClientPlayer::getMyUniqueID() const { Mutex::Lock lock(m_syncMutex); return myUniqueID; }
void ClientPlayer::setMyGuid(const String &theValue) { Mutex::Lock lock(m_syncMutex); myGuid = theValue; }
String ClientPlayer::getMyGuid() const { Mutex::Lock lock(m_syncMutex); return myGuid; }
PlayerType ClientPlayer::getMyType() const { Mutex::Lock lock(m_syncMutex); return myType; }
void ClientPlayer::setMyDude(int theValue) { Mutex::Lock lock(m_syncMutex); myDude = theValue; }
int ClientPlayer::getMyDude() const { Mutex::Lock lock(m_syncMutex); return myDude; }
void ClientPlayer::setMyDude4(int theValue) { Mutex::Lock lock(m_syncMutex); myDude4 = theValue; }
int ClientPlayer::getMyDude4() const { Mutex::Lock lock(m_syncMutex); return myDude4; }
void ClientPlayer::setMyName(String theValue) { Mutex::Lock lock(m_syncMutex); myName = theValue; }
String ClientPlayer::getMyName() const { Mutex::Lock lock(m_syncMutex); return myName; }
void ClientPlayer::setMyAvatar(const String& theValue) { Mutex::Lock lock(m_syncMutex); myAvatar = theValue; }
String ClientPlayer::getMyAvatar() const { Mutex::Lock lock(m_syncMutex); return myAvatar; }
void ClientPlayer::setMyCash(int theValue) { Mutex::Lock lock(m_syncMutex); myCash = theValue; }
int ClientPlayer::getMyCash() const { Mutex::Lock lock(m_syncMutex); return myCash; }
void ClientPlayer::setMySet(int theValue) { Mutex::Lock lock(m_syncMutex); myLastRelativeSet = theValue; }
void ClientPlayer::setMySetAbsolute(int theValue) { Mutex::Lock lock(m_syncMutex); mySet = theValue; }
void ClientPlayer::setMySetNull() { Mutex::Lock lock(m_syncMutex); mySet = 0; myLastRelativeSet = 0; }
int ClientPlayer::getMySet() const { Mutex::Lock lock(m_syncMutex); return mySet; }
void ClientPlayer::setMyLastRelativeSet(int theValue) { Mutex::Lock lock(m_syncMutex); myLastRelativeSet = theValue; }
int ClientPlayer::getMyLastRelativeSet() const { Mutex::Lock lock(m_syncMutex); return myLastRelativeSet; }
void ClientPlayer::setMyAction(PlayerAction theValue, bool human) { Mutex::Lock lock(m_syncMutex); myAction = theValue; }
PlayerAction ClientPlayer::getMyAction() const { Mutex::Lock lock(m_syncMutex); return myAction; }
void ClientPlayer::setMyButton(int theValue) { Mutex::Lock lock(m_syncMutex); myButton = theValue; }
int ClientPlayer::getMyButton() const { Mutex::Lock lock(m_syncMutex); return myButton; }
void ClientPlayer::setMyActiveStatus(bool theValue) { Mutex::Lock lock(m_syncMutex); myActiveStatus = theValue; }
bool ClientPlayer::getMyActiveStatus() const { Mutex::Lock lock(m_syncMutex); return myActiveStatus; }
void ClientPlayer::setMyStayOnTableStatus(bool theValue) { Mutex::Lock lock(m_syncMutex); myStayOnTableStatus = theValue; }
bool ClientPlayer::getMyStayOnTableStatus() const { Mutex::Lock lock(m_syncMutex); return myStayOnTableStatus; }

void ClientPlayer::setMyCards(const int* theValue, int count)
{
	Mutex::Lock lock(m_syncMutex);
	myCardsCount = min(count, 13);
	for (int i = 0; i < myCardsCount; i++) myCards[i] = theValue[i];
	for (int i = myCardsCount; i < 13; i++) myCards[i] = -1;
}

Vector<int> ClientPlayer::getMyCards() const
{
	Mutex::Lock lock(m_syncMutex);
	Vector<int> v;
	for (int i = 0; i < myCardsCount; i++) v.Add(myCards[i]);
	return v;
}

void ClientPlayer::getMyCards(int& card1, int& card2) const
{
	Mutex::Lock lock(m_syncMutex);
	card1 = myCardsCount > 0 ? myCards[0] : -1;
	card2 = myCardsCount > 1 ? myCards[1] : -1;
}

void ClientPlayer::setMyTurn(bool theValue) { Mutex::Lock lock(m_syncMutex); myTurn = theValue; }
bool ClientPlayer::getMyTurn() const { Mutex::Lock lock(m_syncMutex); return myTurn; }

void ClientPlayer::setMyCardsFlip(bool theValue, int state)
{
	Mutex::Lock lock(m_syncMutex);
	myCardsFlip = theValue;
	if (myCardsFlip && currentHand && currentHand->getGuiInterface()) {
		switch(state) {
		case 1: currentHand->getGuiInterface()->logFlipHoleCardsMsg(myName, myCards[0], myCards[1], myCardsValueInt); break;
		case 2: currentHand->getGuiInterface()->logFlipHoleCardsMsg(myName, myCards[0], myCards[1]); break;
		case 3: currentHand->getGuiInterface()->logFlipHoleCardsMsg(myName, myCards[0], myCards[1], myCardsValueInt, "has"); break;
		}
	}
}

bool ClientPlayer::getMyCardsFlip() const { Mutex::Lock lock(m_syncMutex); return myCardsFlip; }
void ClientPlayer::setMyCardsValueInt(int theValue) { Mutex::Lock lock(m_syncMutex); myCardsValueInt = theValue; }
int ClientPlayer::getMyCardsValueInt() const { Mutex::Lock lock(m_syncMutex); return myCardsValueInt; }
void ClientPlayer::setMyOdds(double theValue) { Mutex::Lock lock(m_syncMutex); myOdds = theValue; }
double ClientPlayer::getMyOdds() const { Mutex::Lock lock(m_syncMutex); return myOdds; }
void ClientPlayer::setMyNiveau(int index, int theValue) { Mutex::Lock lock(m_syncMutex); if (index >= 0 && index < 3) myNiveau[index] = theValue; }
int ClientPlayer::getMyNiveau(int index) const { Mutex::Lock lock(m_syncMutex); return (index >= 0 && index < 3) ? myNiveau[index] : 0; }
void ClientPlayer::setLogHoleCardsDone(bool theValue) { Mutex::Lock lock(m_syncMutex); logHoleCardsDone = theValue; }
bool ClientPlayer::getLogHoleCardsDone() const { Mutex::Lock lock(m_syncMutex); return logHoleCardsDone; }

void ClientPlayer::setMyBestHandPosition(int* theValue)
{
	Mutex::Lock lock(m_syncMutex);
	for(int i=0; i<5; i++) myBestHandPosition[i] = theValue[i];
}

void ClientPlayer::getMyBestHandPosition(int* theValue) const
{
	Mutex::Lock lock(m_syncMutex);
	for(int i=0; i<5; i++) theValue[i] = myBestHandPosition[i];
}

void ClientPlayer::setMyRoundStartCash(int theValue) { Mutex::Lock lock(m_syncMutex); myRoundStartCash = theValue; }
int ClientPlayer::getMyRoundStartCash() const { Mutex::Lock lock(m_syncMutex); return myRoundStartCash; }
void ClientPlayer::setLastMoneyWon ( int theValue ) { Mutex::Lock lock(m_syncMutex); lastMoneyWon = theValue; }
int ClientPlayer::getLastMoneyWon() const { Mutex::Lock lock(m_syncMutex); return lastMoneyWon; }

void ClientPlayer::setMyAverageSets(int index, int theValue) { Mutex::Lock lock(m_syncMutex); if (index >= 0 && index < 4) myAverageSets[index] = theValue; }
int ClientPlayer::getMyAverageSets(int index) const { Mutex::Lock lock(m_syncMutex); return (index >= 0 && index < 4) ? myAverageSets[index] : 0; }

void ClientPlayer::setMyAggressive(int index, bool theValue) { Mutex::Lock lock(m_syncMutex); if (index >= 0 && index < 7) myAggressive[index] = theValue; }
bool ClientPlayer::getMyAggressive(int index) const { Mutex::Lock lock(m_syncMutex); return (index >= 0 && index < 7) ? myAggressive[index] : false; }
int ClientPlayer::getMyAggressiveSum() const {
	Mutex::Lock lock(m_syncMutex);
	int sum = 0;
	for (int i = 0; i < 7; i++) sum += myAggressive[i] ? 1 : 0;
	return sum;
}

void ClientPlayer::setMyActionTimeoutCounter(unsigned count) { Mutex::Lock lock(m_syncMutex); m_actionTimeoutCounter = count; }
unsigned ClientPlayer::getMyActionTimeoutCounter() const { Mutex::Lock lock(m_syncMutex); return m_actionTimeoutCounter; }

void ClientPlayer::setSBluff(int theValue) { Mutex::Lock lock(m_syncMutex); sBluff = theValue; }
int ClientPlayer::getSBluff() const { Mutex::Lock lock(m_syncMutex); return sBluff; }
void ClientPlayer::setSBluffStatus(bool theValue) { Mutex::Lock lock(m_syncMutex); sBluffStatus = theValue; }
bool ClientPlayer::getSBluffStatus() const { Mutex::Lock lock(m_syncMutex); return sBluffStatus; }

void ClientPlayer::action() {}
int ClientPlayer::checkMyAction(int targetAction, int targetBet, int highestSet, int minimumRaise, int smallBlind) { return 0; }
void ClientPlayer::preflopEngine() {}
void ClientPlayer::flopEngine() {}
void ClientPlayer::turnEngine() {}
void ClientPlayer::riverEngine() {}
void ClientPlayer::preflopEngine3() {}
void ClientPlayer::flopEngine3() {}
void ClientPlayer::turnEngine3() {}
void ClientPlayer::riverEngine3() {}
int ClientPlayer::preflopCardsValue(int*) { return 0; }
int ClientPlayer::flopCardsValue(int*) { return 0; }
int ClientPlayer::turnCardsValue(int*) { return 0; }
void ClientPlayer::readFile() {}
void ClientPlayer::evaluation(int, int) {}

void ClientPlayer::setIsSessionActive(bool active) { Mutex::Lock lock(m_syncMutex); m_isSessionActive = active; }
bool ClientPlayer::isSessionActive() const { Mutex::Lock lock(m_syncMutex); return m_isSessionActive; }
void ClientPlayer::setIsKicked(bool kicked) { Mutex::Lock lock(m_syncMutex); m_isKicked = kicked; }
bool ClientPlayer::isKicked() const { Mutex::Lock lock(m_syncMutex); return m_isKicked; }
void ClientPlayer::setIsMuted(bool muted) { Mutex::Lock lock(m_syncMutex); m_isMuted = muted; }
bool ClientPlayer::isMuted() const { Mutex::Lock lock(m_syncMutex); return m_isMuted; }

bool ClientPlayer::checkIfINeedToShowCards()
{
	Mutex::Lock lock(m_syncMutex);
	if (!currentHand || !currentHand->getBoard()) return false;
	Vector<unsigned> playerNeedToShowCardsList;
	playerNeedToShowCardsList <<= currentHand->getBoard()->getPlayerNeedToShowCards();
	for(unsigned id : playerNeedToShowCardsList) if(id == myUniqueID) return true;
	return false;
}

END_UPP_NAMESPACE
