#ifndef _CardEngine_EngineFactory_h_
#define _CardEngine_EngineFactory_h_

#include <GameRules/EngineDefs.h>
#include <GameRules/GameDefs.h>
#include <GameRules/PlayerData.h>

NAMESPACE_UPP

class HandInterface;
class BoardInterface;
class BeroInterface;
class PlayerInterface;
class ConfigFile;
class GuiInterface;
class EngineLog;

namespace omp { class HandEvaluator; }

class EngineFactory
{
public:
	virtual ~EngineFactory() {}

	virtual std::shared_ptr<HandInterface> createHand(std::shared_ptr<EngineFactory> f, GuiInterface *g, std::shared_ptr<BoardInterface> b, EngineLog *l, PlayerList sl, PlayerList apl, PlayerList rpl, int id, int sP, int dP, int sB, int sC, int seed = -1, GameType gt = GAME_TYPE_NLTH) = 0;

	virtual std::shared_ptr<BoardInterface> createBoard(GuiInterface *g, EngineLog *l) = 0;
	virtual std::vector<std::shared_ptr<BeroInterface>> createBeRo(HandInterface *hi, unsigned dP, int sB) = 0;
	virtual std::shared_ptr<PlayerInterface> createPlayer(class ConfigFile *c, int id, unsigned uniqueId, PlayerType type, String name, String avatar, int sC, bool aS, bool sotS, int mB) = 0;
	virtual omp::HandEvaluator* getEvaluator() = 0;
	};
END_UPP_NAMESPACE

#endif