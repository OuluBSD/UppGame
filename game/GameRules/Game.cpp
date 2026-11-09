#include <GameRules/Game.h>
#include <Poker/LocalHand.h>
#include <GameRules/EngineFactory.h>
#include <GameRules/GuiInterface.h>
#include <GameRules/EngineLog.h>
#include <GameRules/HandInterface.h>
#include <GameRules/BoardInterface.h>
#include <GameRules/PlayerInterface.h>
#include <GameRules/BeroInterface.h>
#include <GameRules/Exception.h>

NAMESPACE_UPP

Game::Game(GuiInterface* gui, std::shared_ptr<EngineFactory> factory,
                   const PlayerDataList &playerDataList, const GameData &gameData,
                   const StartData &startData, int gameId, EngineLog* log, class ConfigFile* config, GameType gt)
        : myFactory(factory), myGui(gui), myLog(log), myConfig(config), startQuantityPlayers(startData.numberOfPlayers),
          startCash(gameData.startMoney), startSmallBlind(gameData.firstSmallBlind),
          myGameID(gameId), currentSmallBlind(gameData.firstSmallBlind), currentHandID(0), dealerPosition(0),
          lastHandBlindsRaised(1), lastTimeBlindsRaised(0), myGameData(gameData),
          m_gameOver(false), my_game_type(gt)
{
	for (int b : myGameData.manualBlindsList) blindsList.Add(b);
	dealerPosition = startData.startDealerPlayerId;

	bool foundDealer = false;
	if (my_game_type == GAME_TYPE_HEARTS) {
		foundDealer = true; // Hearts lead is determined by 2 of clubs
	} else {
		for (const auto& pd : playerDataList) if (pd->GetUniqueId() == dealerPosition) { foundDealer = true; break; }
	}
	if (!foundDealer) throw LocalException(__FILE__, __LINE__, ERR_DEALER_NOT_FOUND);

	currentBoard = myFactory->createBoard(myGui, myLog);

	seatsList = std::make_shared<std::vector<std::shared_ptr<PlayerInterface>>>();
	activePlayerList = std::make_shared<std::vector<std::shared_ptr<PlayerInterface>>>();
	runningPlayerList = std::make_shared<std::vector<std::shared_ptr<PlayerInterface>>>();

	for(int i = 0; i < MAX_NUMBER_OF_PLAYERS; i++) {
		String myName, myAvatarFile, myGuid;
		unsigned uniqueId = 0;
		PlayerType type = PLAYER_TYPE_COMPUTER;
		int myStartCash = startCash;
		bool myStayOnTableStatus = false;

		if (i < (int)playerDataList.size()) {
			const auto& pd = playerDataList[i];
			uniqueId = pd->GetUniqueId();
			type = pd->GetType();
			myName = pd->GetName();
			myAvatarFile = pd->GetAvatarFile();
			myGuid = pd->GetGuid();
			if (pd->GetStartCash() >= 0) myStartCash = pd->GetStartCash();
			myStayOnTableStatus = (type == PLAYER_TYPE_HUMAN || pd->IsGameAdmin());
		}

		std::shared_ptr<PlayerInterface> tmpPlayer = myFactory->createPlayer(myConfig, i, uniqueId, type, myName, myAvatarFile, myStartCash, startQuantityPlayers > i, myStayOnTableStatus, 0);
		tmpPlayer->setIsSessionActive(true);
		tmpPlayer->setMyGuid(myGuid);

		int unDealt[13];
		for (int j = 0; j < 13; j++) unDealt[j] = -2;
		tmpPlayer->setMyCards(unDealt, 13);

		seatsList->push_back(tmpPlayer);
		if(startQuantityPlayers > i) activePlayerList->push_back(tmpPlayer);

		myGui->refreshGroupbox(i, 0);
	}
	*runningPlayerList = *activePlayerList;
	currentBoard->setPlayerLists(seatsList, activePlayerList, runningPlayerList);
	if(myLog) myLog->logNewGameMsg(myGameID, startCash, startSmallBlind, (int)dealerPosition, seatsList);
	blindsTimer.Reset();
}
Game::~Game() {}

std::shared_ptr<HandInterface> Game::getCurrentHand() { return currentHand; }
const std::shared_ptr<HandInterface> Game::getCurrentHand() const { return currentHand; }

void Game::initHand()
{
	currentHandID++;
	raiseBlinds();
	for(auto& player : *seatsList) {
		bool should_be_active = player->isSessionActive() &&
		                       player->getMyID() < startQuantityPlayers &&
		                       player->getMyCash() > 0;
		player->setMyActiveStatus(should_be_active);
		player->setMyAction(PLAYER_ACTION_NONE);
		player->setMySetNull();
		player->setMyRoundStartCash(player->getMyCash());
	}
	for(auto it = activePlayerList->begin(); it != activePlayerList->end(); ) {
		if((*it)->getMyCash() == 0) {
			(*it)->setMyActiveStatus(false);
			it = activePlayerList->erase(it);
		} else ++it;
	}
	if (activePlayerList->size() <= 1) {
		m_gameOver = true;
		currentHand = nullptr;
		return;
	}
	m_gameOver = false;
	runningPlayerList->clear();
	*runningPlayerList = *activePlayerList;
	int seed = (m_baseSeed >= 0) ? (m_baseSeed + currentHandID) : -1;
	currentHand = myFactory->createHand(myFactory, myGui, currentBoard, myLog, seatsList, activePlayerList, runningPlayerList, currentHandID, startQuantityPlayers, dealerPosition, currentSmallBlind, startCash, seed, my_game_type);
	if (currentHand) {
		LocalHand* lh = dynamic_cast<LocalHand*>(currentHand.get());
		if (lh) lh->setGame(this);
	}

	for (int i = 0; i < 10; i++) myGui->refreshGroupbox(i, 0);

	if (my_game_type != GAME_TYPE_HEARTS) {
		PlayerListIterator dealerPositionIt = currentHand->getSeatIt(dealerPosition);
		if(dealerPositionIt == seatsList->end()) {
			throw LocalException(__FILE__, __LINE__, ERR_SEAT_NOT_FOUND);
		}

		bool nextDealerFound = false;
		for(size_t i = 0; i < seatsList->size(); i++) {
			++dealerPositionIt;
			if(dealerPositionIt == seatsList->end()) dealerPositionIt = seatsList->begin();
			if(currentHand->getActivePlayerIt((*dealerPositionIt)->getMyUniqueID()) != activePlayerList->end()) {
				nextDealerFound = true;
				dealerPosition = (*dealerPositionIt)->getMyUniqueID();
				break;
			}
		}
		if(!nextDealerFound) {
			throw LocalException(__FILE__, __LINE__, ERR_NEXT_DEALER_NOT_FOUND);
		}
	}
}

void Game::startHand()
{
	if (!currentHand)
		return;
	myGui->nextRoundCleanGui();
	myGui->logNewGameHandMsg(myGameID, currentHandID);
	myGui->flushLogAtGame(myGameID);
	currentHand->start();
}

std::shared_ptr<PlayerInterface> Game::getPlayerByUniqueId(unsigned id)
{
	for (auto& player : *seatsList) if (player->getMyUniqueID() == id) return player;
	return nullptr;
}

std::shared_ptr<PlayerInterface> Game::getPlayerByNumber(int number)
{
	for (auto& player : *seatsList) if (player->getMyID() == number) return player;
	return nullptr;
}

std::shared_ptr<PlayerInterface> Game::getCurrentPlayer()
{
	auto player = getPlayerByUniqueId(getCurrentHand()->getCurrentBeRo()->getCurrentPlayersTurnId());
	if (!player) throw LocalException(__FILE__, __LINE__, ERR_CURRENT_PLAYER_NOT_FOUND);
	return player;
}

std::shared_ptr<PlayerInterface> Game::getPlayerByName(const String &name)
{
	for (auto& player : *seatsList) if (player->getMyName() == name) return player;
	return nullptr;
}

void Game::raiseBlinds()
{
	bool raise = false;
	if (myGameData.raiseIntervalMode == RAISE_ON_HANDNUMBER) {
		if (lastHandBlindsRaised + myGameData.raiseSmallBlindEveryHandsValue <= currentHandID) {
			raise = true; lastHandBlindsRaised = currentHandID;
		}
	} else if (lastTimeBlindsRaised + myGameData.raiseSmallBlindEveryMinutesValue <= blindsTimer.Elapsed() / 60000) {
		raise = true; lastTimeBlindsRaised = (int)(blindsTimer.Elapsed() / 60000);
	}
	
	if (raise) {
		if (myGameData.raiseMode == DOUBLE_BLINDS) currentSmallBlind *= 2;
		else if(!blindsList.IsEmpty()) { currentSmallBlind = blindsList[0]; blindsList.Remove(0); }
		else if (myGameData.afterManualBlindsMode == AFTERMB_DOUBLE_BLINDS) currentSmallBlind *= 2;
		else if(myGameData.afterManualBlindsMode == AFTERMB_RAISE_ABOUT) currentSmallBlind += myGameData.afterMBAlwaysRaiseValue;
		currentSmallBlind = min(currentSmallBlind, startQuantityPlayers * startCash / 2);
	}
}

void Game::AddPlayer(std::shared_ptr<PlayerInterface> player) {
	if (!player) return;
	for (auto& p : *seatsList) {
		if (p->getMyUniqueID() == 0) {
			p = player;
			p->setIsSessionActive(true);
			activePlayerList->push_back(p);
			return;
		}
	}
}

void Game::RemovePlayer(unsigned uniqueId) {
	auto it = std::remove_if(activePlayerList->begin(), activePlayerList->end(), [&](auto p) { return p->getMyUniqueID() == uniqueId; });
	activePlayerList->erase(it, activePlayerList->end());

	it = std::remove_if(runningPlayerList->begin(), runningPlayerList->end(), [&](auto p) { return p->getMyUniqueID() == uniqueId; });
	runningPlayerList->erase(it, runningPlayerList->end());

	for (auto& p : *seatsList) {
		if (p->getMyUniqueID() == uniqueId) {
			p = myFactory->createPlayer(myConfig, p->getMyID(), 0, PLAYER_TYPE_COMPUTER, "", "", 0, false, false, 0);
			break;
		}
	}
}

void Game::resetHand() {
	currentHand = nullptr;
}

END_UPP_NAMESPACE
