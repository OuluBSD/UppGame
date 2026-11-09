#ifndef _CardEngine_GuiInterface_h_
#define _CardEngine_GuiInterface_h_

#include <GameRules/EngineDefs.h>
#include <GameRules/GameDefs.h>

NAMESPACE_UPP

class GuiInterface
{
public:
	virtual ~GuiInterface() {}

	virtual void initGui(int speed) = 0;
	virtual void refreshGameLabels(TexasRound state) const = 0;
	virtual void nextRoundCleanGui() = 0;
	virtual void logNewGameHandMsg(int gameID, int HandID) = 0;
	virtual void flushLogAtGame(int gameID) = 0;

	virtual void logNewBlindsSetsMsg(int sbSet, int bbSet, String sbName, String bbName) = 0;
	virtual void flushLogAtHand() = 0;
	virtual void dealHoleCards() = 0;
	virtual void refreshPot() = 0;
	virtual void refreshSet() = 0;
	virtual void nextPlayerAnimation() = 0;
	virtual void flipHolecardsAllIn() = 0;
	virtual void logDealBoardCardsMsg(int roundID, int card1, int card2, int card3, int card4 = -1, int card5 = -1) = 0;
	virtual void refreshGroupbox(int playerID, int type) = 0;
	virtual void preflopAnimation1() = 0;
	virtual void flopAnimation1() = 0;
	virtual void turnAnimation1() = 0;
	virtual void riverAnimation1() = 0;
	virtual void postRiverAnimation1() = 0;

	virtual void logPlayerActionMsg(String playName, int action, int setValue) = 0;
	virtual void logFlipHoleCardsMsg(String playerName, int card1, int card2, int cardsValueInt = -1, String showHas = "shows") = 0;
	virtual void logWinningHandMsg(String playerName, String handName, int amount) = 0;

	virtual void dealBeRoCards(TexasRound state) = 0;
	virtual void beRoAnimation2(TexasRound state) = 0;
	virtual void meInAction() = 0;
	virtual void postRiverRunAnimation1() = 0;
	virtual void refreshCash() = 0;
	virtual void refreshAction(int playerID, int action) = 0;
	virtual void SignalNetClientError(int errorId, int osErrorCode) = 0;
	virtual bool isTestMode() const { return false; }
	virtual bool isVerbose() const { return false; }
	virtual void SetGame(std::shared_ptr<class Game> game) {}
};

END_UPP_NAMESPACE

#endif
