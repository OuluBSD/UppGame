#include <Poker/MultiGameGen.h>
#include <Poker/LocalEngineFactory.h>
#include <GameRules/GuiInterface.h>
#include <GameRules/Game.h>
#include <GameRules/PlayerData.h>
#include <GameRules/PlayerInterface.h>
#include <EditorCommon/ConfigFile.h>
#include <iostream>

namespace Upp {

class GenGui : public GuiInterface {
public:
	virtual void initGui(int speed) override {}
	virtual void refreshGameLabels(TexasRound state) const override {}
	virtual void nextRoundCleanGui() override {}
	virtual void logNewGameHandMsg(int gameID, int HandID) override {}
	virtual void flushLogAtGame(int gameID) override {}
	virtual void logNewBlindsSetsMsg(int sbSet, int bbSet, String sbName, String bbName) override {}
	virtual void flushLogAtHand() override {}
	virtual void dealHoleCards() override {}
	virtual void refreshPot() override {}
	virtual void refreshSet() override {}
	virtual void nextPlayerAnimation() override {}
	virtual void flipHolecardsAllIn() override {}
	virtual void logDealBoardCardsMsg(int roundID, int card1, int card2, int card3, int card4 = -1, int card5 = -1) override {}
	virtual void refreshGroupbox(int playerID, int type) override {}
	virtual void preflopAnimation1() override {}
	virtual void flopAnimation1() override {}
	virtual void turnAnimation1() override {}
	virtual void riverAnimation1() override {}
	virtual void postRiverAnimation1() override {}
	virtual void logPlayerActionMsg(String playName, int action, int setValue) override {}
	virtual void logFlipHoleCardsMsg(String playerName, int card1, int card2, int cardsValueInt = -1, String showHas = "shows") override {}
	virtual void logWinningHandMsg(String playerName, String handName, int amount) override {}
	virtual void dealBeRoCards(TexasRound state) override {}
	virtual void beRoAnimation2(TexasRound state) override {}
	virtual void meInAction() override {}
	virtual void postRiverRunAnimation1() override {}
	virtual void refreshCash() override {}
	virtual void refreshAction(int playerID, int action) override {}
	virtual void SignalNetClientError(int errorId, int osErrorCode) override {}
};

int RunPloGameplayGeneration(int num_hands, const String& output_path) {
	GenGui gui;
	auto factory = std::make_shared<LocalEngineFactory>();
	static class ConfigFile cfg(nullptr, false);
	cfg.writeConfigInt("AI_BACKEND", 0);
	
	PlayerDataList pdList;
	for (int i = 0; i < 6; i++) {
		auto pd = std::make_shared<PlayerData>(i + 1, i, PLAYER_TYPE_COMPUTER, PLAYER_RIGHTS_NORMAL, false);
		pd->SetName("Bot_" + AsString(i + 1));
		pd->SetStartCash(1000);
		pdList.push_back(pd);
	}
	
	GameData gData;
	gData.startMoney = 1000;
	gData.firstSmallBlind = 10;
	
	StartData sData;
	sData.numberOfPlayers = 6;
	sData.startDealerPlayerId = 1;
	
	Game game(&gui, factory, pdList, gData, sData, 1, nullptr, &cfg, GAME_TYPE_PLO);
	
	FileOut out(output_path);
	if (!out) return -1;
	
	Cout() << "Generating " << num_hands << " PLO hands to " << output_path << "...\n";
	
	for (int h = 0; h < num_hands; h++) {
		game.initHand();
		game.startHand();
		if (h % 100 == 0) Cout() << "Hand " << h << " done.\n";
	}
	
	return 0;
}

int RunTexasGameplayGeneration(int num_hands, const String& output_path) {
	GenGui gui;
	auto factory = std::make_shared<LocalEngineFactory>();
	static class ConfigFile cfg(nullptr, false);
	cfg.writeConfigInt("AI_BACKEND", 0);
	
	PlayerDataList pdList;
	for (int i = 0; i < 6; i++) {
		auto pd = std::make_shared<PlayerData>(i + 1, i, PLAYER_TYPE_COMPUTER, PLAYER_RIGHTS_NORMAL, false);
		pd->SetName("Bot_" + AsString(i + 1));
		pd->SetStartCash(1000);
		pdList.push_back(pd);
	}
	
	GameData gData;
	gData.startMoney = 1000;
	gData.firstSmallBlind = 10;
	
	StartData sData;
	sData.numberOfPlayers = 6;
	sData.startDealerPlayerId = 1;
	
	Game game(&gui, factory, pdList, gData, sData, 1, nullptr, &cfg, GAME_TYPE_NLTH);
	
	FileOut out(output_path);
	if (!out) return -1;
	
	Cout() << "Generating " << num_hands << " NLTH hands to " << output_path << "...\n";
	
	for (int h = 0; h < num_hands; h++) {
		game.initHand();
		game.startHand();
		if (h % 100 == 0) Cout() << "Hand " << h << " done.\n";
	}
	
	return 0;
}

}
