#ifndef _CardEngine_ClientEngineFactory_h_
#define _CardEngine_ClientEngineFactory_h_

#include <GameRules/EngineFactory.h>

NAMESPACE_UPP

class ClientEngineFactory : public EngineFactory
{
public:
	ClientEngineFactory();
	virtual ~ClientEngineFactory();

	virtual std::shared_ptr<HandInterface> createHand(std::shared_ptr<EngineFactory> f, GuiInterface *g, std::shared_ptr<BoardInterface> b, EngineLog *l, PlayerList sl, PlayerList apl, PlayerList rpl, int id, int sP, int dP, int sB, int sC, int seed = -1, GameType gt = GAME_TYPE_NLTH) override;
	virtual std::shared_ptr<BoardInterface> createBoard(GuiInterface *g, EngineLog *l) override;
	virtual std::vector<std::shared_ptr<BeroInterface>> createBeRo(HandInterface *hi, unsigned dP, int sB) override;
	virtual std::shared_ptr<PlayerInterface> createPlayer(class ConfigFile *c, int id, unsigned uniqueId, PlayerType type, String name, String avatar, int sC, bool aS, bool sotS, int mB) override;
};

END_UPP_NAMESPACE

#endif
