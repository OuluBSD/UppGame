#include "ClientBero.h"

NAMESPACE_UPP

ClientBero::ClientBero(HandInterface* hi, unsigned dP, int sB, TexasRound gS)
	: myHand(hi), myBeRoID(gS), dealerPosition(dP), smallBlindPositionId(0), bigBlindPositionId(0), smallBlind(sB), highestSet(0), minimumRaise(2*sB),
	  fullBetRule(false), firstRound(true), currentPlayersTurnId(0), highestCardsValue(0)
{
}

ClientBero::~ClientBero() {}

void ClientBero::start() {}
void ClientBero::reset() {}
void ClientBero::run() {}
void ClientBero::nextPlayer() {}
void ClientBero::postRiverRun() {}

TexasRound ClientBero::getMyBeRoID() const { return myBeRoID; }

void ClientBero::setCurrentPlayersTurnId(unsigned id) { Mutex::Lock lock(m_syncMutex); currentPlayersTurnId = id; }
unsigned ClientBero::getCurrentPlayersTurnId() const { Mutex::Lock lock(m_syncMutex); return currentPlayersTurnId; }

void ClientBero::setCurrentPlayersTurnIt(PlayerListIterator it) { Mutex::Lock lock(m_syncMutex); currentPlayersTurnIt = it; }
PlayerListIterator ClientBero::getCurrentPlayersTurnIt() const { Mutex::Lock lock(m_syncMutex); return currentPlayersTurnIt; }

void ClientBero::setSmallBlindPositionId(unsigned id) { Mutex::Lock lock(m_syncMutex); smallBlindPositionId = id; }
unsigned ClientBero::getSmallBlindPositionId() const { Mutex::Lock lock(m_syncMutex); return smallBlindPositionId; }

void ClientBero::setBigBlindPositionId(unsigned id) { Mutex::Lock lock(m_syncMutex); bigBlindPositionId = id; }
unsigned ClientBero::getBigBlindPositionId() const { Mutex::Lock lock(m_syncMutex); return bigBlindPositionId; }

void ClientBero::setHighestSet(int val) { Mutex::Lock lock(m_syncMutex); highestSet = val; }
int ClientBero::getHighestSet() const { Mutex::Lock lock(m_syncMutex); return highestSet; }

void ClientBero::setMinimumRaise(int val) { Mutex::Lock lock(m_syncMutex); minimumRaise = val; }
int ClientBero::getMinimumRaise() const { Mutex::Lock lock(m_syncMutex); return minimumRaise; }

void ClientBero::setFullBetRule(bool val) { Mutex::Lock lock(m_syncMutex); fullBetRule = val; }
bool ClientBero::getFullBetRule() const { Mutex::Lock lock(m_syncMutex); return fullBetRule; }

void ClientBero::setHighestCardsValue(int val) { Mutex::Lock lock(m_syncMutex); highestCardsValue = val; }
int ClientBero::getHighestCardsValue() const { Mutex::Lock lock(m_syncMutex); return highestCardsValue; }

bool ClientBero::getFirstRound() const { Mutex::Lock lock(m_syncMutex); return firstRound; }
void ClientBero::setFirstRound(bool val) { Mutex::Lock lock(m_syncMutex); firstRound = val; }
void ClientBero::skipFirstRunGui() {}

void ClientBero::recordAction(int playerID, PlayerAction action, int value)
{
	Mutex::Lock lock(m_syncMutex);
	if (action == PLAYER_ACTION_FOLD) actionHistory.Add(0);
	else if (action == PLAYER_ACTION_CHECK || action == PLAYER_ACTION_CALL) actionHistory.Add(1);
	else if (action == PLAYER_ACTION_BET || action == PLAYER_ACTION_RAISE || action == PLAYER_ACTION_ALLIN) {
		actionHistory.Add(2); 
	}
}

END_UPP_NAMESPACE
