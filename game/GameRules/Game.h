#ifndef _CardEngine_Game_h_
#define _CardEngine_Game_h_

#include <GameRules/EngineDefs.h>
#include <GameRules/GameDefs.h>
#include <GameRules/GameData.h>
#include <GameRules/PlayerData.h>

NAMESPACE_UPP

class GuiInterface;
class EngineLog;
class HandInterface;
class BoardInterface;
class EngineFactory;

class Game
{
public:
	Game(GuiInterface *gui, std::shared_ptr<EngineFactory> factory,
		 const PlayerDataList &playerDataList, const GameData &gameData,
		 const StartData &startData, int gameId, EngineLog *myLog, class ConfigFile *config, GameType gt = GAME_TYPE_NLTH);

	~Game();

	void initHand();
	void startHand();

	std::shared_ptr<HandInterface> getCurrentHand();
	const std::shared_ptr<HandInterface> getCurrentHand() const;

	PlayerList getSeatsList() const { return seatsList; }
	PlayerList getActivePlayerList() const { return activePlayerList; }
	PlayerList getRunningPlayerList() const { return runningPlayerList; }

	void setStartQuantityPlayers(int theValue) { startQuantityPlayers = theValue; }
	int getStartQuantityPlayers() const { return startQuantityPlayers; }

	void setStartSmallBlind(int theValue) { startSmallBlind = theValue; }
	int getStartSmallBlind() const { return startSmallBlind; }

	void setStartCash(int theValue) { startCash = theValue; }
	int getStartCash() const { return startCash; }

	int getMyGameID() const { return myGameID; }

	void setCurrentSmallBlind(int theValue) { currentSmallBlind = theValue; }
	int getCurrentSmallBlind() const { return currentSmallBlind; }

	void setCurrentHandID(int theValue) { currentHandID = theValue; }
	int getCurrentHandID() const { return currentHandID; }

	unsigned getDealerPosition() const { return dealerPosition; }
	bool isGameOver() const { return m_gameOver; }

	void replaceDealer(unsigned oldDealer, unsigned newDealer)
	{
		if (dealerPosition == oldDealer)
			dealerPosition = newDealer;
	}

	std::shared_ptr<PlayerInterface> getPlayerByUniqueId(unsigned id);
	std::shared_ptr<PlayerInterface> getPlayerByNumber(int number);
	std::shared_ptr<PlayerInterface> getPlayerByName(const String &name);
	std::shared_ptr<PlayerInterface> getCurrentPlayer();

	void AddPlayer(std::shared_ptr<PlayerInterface> player);
	void RemovePlayer(unsigned uniqueId);
	void resetHand();

	class ConfigFile* getConfig() const { return myConfig; }

	void raiseBlinds();
	void SetBaseSeed(int seed) { m_baseSeed = seed; }
	
	void SetGameType(GameType gt) { my_game_type = gt; }
	GameType GetGameType() const { return my_game_type; }
	
	void setVerbose(bool v) { m_verbose = v; }
	bool isVerbose() const { return m_verbose; }

private:
	std::shared_ptr<EngineFactory> myFactory;

	GuiInterface *myGui;
	EngineLog *myLog;
	class ConfigFile *myConfig;
	std::shared_ptr<HandInterface> currentHand;
	std::shared_ptr<BoardInterface> currentBoard;

	PlayerList seatsList;
	PlayerList activePlayerList;
	PlayerList runningPlayerList;

	int startQuantityPlayers;
	int startCash;
	int startSmallBlind;
	int myGameID;

	int currentSmallBlind;
	int currentHandID;
	unsigned dealerPosition;
	int lastHandBlindsRaised;
	int lastTimeBlindsRaised;
	const GameData myGameData;
	Vector<int> blindsList;

	TimeStop blindsTimer;
	bool m_gameOver;
	int m_baseSeed = -1;
	GameType my_game_type;
	bool m_verbose = false;
};

END_UPP_NAMESPACE

#endif
