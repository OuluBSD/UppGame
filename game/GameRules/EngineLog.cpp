#include <GameRules/EngineLog.h>
#include <GameRules/PlayerInterface.h>
#include <iostream>

NAMESPACE_UPP

EngineLog::EngineLog(class ConfigFile *c) : myConfig(c), uniqueGameID(0), currentHandID(0), currentRound(GAME_STATE_PREFLOP) {}
EngineLog::~EngineLog() {}

void EngineLog::init() {}
void EngineLog::logNewGameMsg(int gameID, int startCash, int startSmallBlind, unsigned dealerPosition, PlayerList seatsList) { uniqueGameID++; }
void EngineLog::logNewHandMsg(int handID, unsigned dealerPosition, int smallBlind, unsigned smallBlindPosition, int bigBlind, unsigned bigBlindPosition, PlayerList seatsList)
{
	currentHandID = handID;
	currentRound = GAME_STATE_PREFLOP;
}

void EngineLog::logPlayerAction(String playerName, PlayerActionLog action, int amount) {}
void EngineLog::logPlayerAction(int seat, PlayerActionLog action, int amount) {}

PlayerActionLog EngineLog::transformPlayerActionLog(PlayerAction action)
{
    switch(action) {
    case PLAYER_ACTION_FOLD: return LOG_ACTION_FOLD;
    case PLAYER_ACTION_CHECK: return LOG_ACTION_CHECK;
    case PLAYER_ACTION_CALL: return LOG_ACTION_CALL;
    case PLAYER_ACTION_BET:
    case PLAYER_ACTION_RAISE: return LOG_ACTION_BET;
    case PLAYER_ACTION_ALLIN: return LOG_ACTION_ALL_IN;
    default: return LOG_ACTION_NONE;
    }
}

void EngineLog::logBoardCards(int boardCards[5]) {}
void EngineLog::logHoleCardsHandName(PlayerList activePlayerList) {}
void EngineLog::logHoleCardsHandName(PlayerList activePlayerList, std::shared_ptr<PlayerInterface> player, bool forceExecLog) {}
void EngineLog::logHandWinner(PlayerList activePlayerList, int highestCardsValue, const Vector<unsigned>& winners) {}
void EngineLog::logGameWinner(PlayerList activePlayerList) {}
void EngineLog::logPlayerSitsOut(PlayerList activePlayerList) {}
void EngineLog::logAfterHand() {}
void EngineLog::logAfterGame() {}
void EngineLog::flushLog() {}
void EngineLog::logChatMsg(String playName, String msg) {
	std::cout << "CHAT [" << playName.ToStd() << "]: " << msg.ToStd() << std::endl;
}
void EngineLog::exec_transaction() {}

END_UPP_NAMESPACE
