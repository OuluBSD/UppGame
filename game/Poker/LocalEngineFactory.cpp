#include <Poker/PokerHandInterface.h>
#include <Poker/PokerPlayerInterface.h>
#include <Poker/LocalEngineFactory.h>
#include <Poker/LocalHand.h>
#include <Poker/LocalBoard.h>
#include <Poker/LocalBero.h>
#include <GameRules/LocalPlayer.h>
#include <Poker/PokerLocalPlayer.h>
#include <CardEvaluator/CardEvaluator.h>
#include <Poker/RandomBotPlayer.h>
#include <Poker/PokerThBotPlayer.h>
#include <EditorCommon/ConfigFile.h>

NAMESPACE_UPP

std::shared_ptr<HandInterface> LocalEngineFactory::createHand(std::shared_ptr<EngineFactory> f, GuiInterface *g, std::shared_ptr<BoardInterface> b, EngineLog *l, PlayerList sl, PlayerList apl, PlayerList rpl, int id, int sP, int dP, int sB, int sC, int seed, GameType gt)
{
	return std::make_shared<LocalHand>(f, g, b, l, sl, apl, rpl, id, sP, (unsigned)dP, sB, sC, seed, gt);
}

std::shared_ptr<BoardInterface> LocalEngineFactory::createBoard(GuiInterface *g, EngineLog *l)
{
	return std::make_shared<LocalBoard>(g, l);
}

std::vector<std::shared_ptr<BeroInterface>> LocalEngineFactory::createBeRo(HandInterface *hi, unsigned dP, int sB)
{
	std::vector<std::shared_ptr<BeroInterface>> v;
	v.push_back(std::make_shared<LocalBero>(hi, dP, sB, GAME_STATE_PREFLOP));
	v.push_back(std::make_shared<LocalBero>(hi, dP, sB, GAME_STATE_FLOP));
	v.push_back(std::make_shared<LocalBero>(hi, dP, sB, GAME_STATE_TURN));
	v.push_back(std::make_shared<LocalBero>(hi, dP, sB, GAME_STATE_RIVER));
	return v;
}

std::shared_ptr<PlayerInterface> LocalEngineFactory::createPlayer(class ConfigFile *c, int id, unsigned uniqueId, PlayerType type, String name, String avatar, int sC, bool aS, bool sotS, int mB)
{
	if (type == PLAYER_TYPE_COMPUTER) {
		int backend = c ? c->readConfigInt("AI_BACKEND") : 0;
		if (sotS) { // Primary agent under test
			if (backend == 0) // AI_BACKEND_POKERTH
				return std::make_shared<PokerThBotPlayer>(c, id, uniqueId, name, avatar, sC, aS, sotS, mB);
			else if (backend == 3) // AI_BACKEND_RANDOM
				return std::make_shared<RandomBotPlayer>(c, id, uniqueId, name, avatar, sC, aS, sotS, mB);
			
			// Fallback: default to PokerThBotPlayer
			return std::make_shared<PokerThBotPlayer>(c, id, uniqueId, name, avatar, sC, aS, sotS, mB);
		} else {
			// Normal bots should always be PokerTH for performance testing baseline
			return std::make_shared<PokerThBotPlayer>(c, id, uniqueId, name, avatar, sC, aS, sotS, mB);
		}
	}
	return std::make_shared<PokerLocalPlayer>(c, id, uniqueId, type, name, avatar, sC, aS, sotS, mB);
}

omp::HandEvaluator* LocalEngineFactory::getEvaluator()
{
	static omp::HandEvaluator evaluator;
	return &evaluator;
}

END_UPP_NAMESPACE
