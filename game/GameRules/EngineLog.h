#ifndef _CardEngine_EngineLog_h_
#define _CardEngine_EngineLog_h_

#include <GameRules/EngineDefs.h>
#include <GameRules/GameDefs.h>
#ifdef True
#undef True
#endif
#ifdef False
#undef False
#endif
#include <Sql/Sql.h>

NAMESPACE_UPP

class ConfigFile;

class EngineLog
{
public:
    EngineLog(class ConfigFile *c);
    ~EngineLog();

    void init();
    void logNewGameMsg(int gameID, int startCash, int startSmallBlind, unsigned dealerPosition, PlayerList seatsList);
    void logNewHandMsg(int handID, unsigned dealerPosition, int smallBlind, unsigned smallBlindPosition, int bigBlind, unsigned bigBlindPosition, PlayerList seatsList);
    void logPlayerAction(String playerName, PlayerActionLog action, int amount = 0);
    void logPlayerAction(int seat, PlayerActionLog action, int amount = 0);
    PlayerActionLog transformPlayerActionLog(PlayerAction action);
    void logBoardCards(int boardCards[5]);
    void logHoleCardsHandName(PlayerList activePlayerList);
    void logHoleCardsHandName(PlayerList activePlayerList, std::shared_ptr<PlayerInterface> player, bool forceExecLog = false);
    void logHandWinner(PlayerList activePlayerList, int highestCardsValue, const Vector<unsigned>& winners);
    void logGameWinner(PlayerList activePlayerList);
    void logPlayerSitsOut(PlayerList activePlayerList);
    void logAfterHand();
    void logAfterGame();
    void flushLog();
    void logChatMsg(String playName, String msg);

    void setCurrentRound(TexasRound theValue) { currentRound = theValue; }

    String getMySqliteLogFileName() { return mySqliteLogFileName; }

private:
    void exec_transaction();

    String mySqliteLogFileName;
    class ConfigFile *myConfig;
    int uniqueGameID;
    int currentHandID;
    TexasRound currentRound;
    String sql;
    Index<String> loggedSitsOut;
};

END_UPP_NAMESPACE

#endif
