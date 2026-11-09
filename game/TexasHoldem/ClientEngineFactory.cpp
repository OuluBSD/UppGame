#include "ClientEngineFactory.h"
#include "ClientHand.h"
#include "ClientBoard.h"
#include "ClientPlayer.h"
#include "ClientBero.h"

NAMESPACE_UPP

ClientEngineFactory::ClientEngineFactory() {}
ClientEngineFactory::~ClientEngineFactory() {}

std::shared_ptr<HandInterface> ClientEngineFactory::createHand(std::shared_ptr<EngineFactory> f, GuiInterface *g, std::shared_ptr<BoardInterface> b, EngineLog *l, PlayerList sl, PlayerList apl, PlayerList rpl, int id, int sP, int dP, int sB, int sC, int seed, GameType gt)
{
	return std::make_shared<ClientHand>(f, g, b, l, sl, apl, rpl, id, sP, dP, sB, sC, gt);
}

std::shared_ptr<BoardInterface> ClientEngineFactory::createBoard(GuiInterface *g, EngineLog *l)
{
	return std::make_shared<ClientBoard>();
}

std::shared_ptr<PlayerInterface> ClientEngineFactory::createPlayer(class ConfigFile *c, int id, unsigned uniqueId, PlayerType type, String name, String avatar, int sC, bool aS, bool sotS, int mB)
{
	return std::make_shared<ClientPlayer>(c, id, uniqueId, type, name, avatar, sC, aS, sotS, mB);
}

std::vector<std::shared_ptr<BeroInterface>> ClientEngineFactory::createBeRo(HandInterface *hi, unsigned dP, int sB)
{
	std::vector<std::shared_ptr<BeroInterface>> v;
	v.push_back(std::make_shared<ClientBero>(hi, dP, sB, GAME_STATE_PREFLOP));
	v.push_back(std::make_shared<ClientBero>(hi, dP, sB, GAME_STATE_FLOP));
	v.push_back(std::make_shared<ClientBero>(hi, dP, sB, GAME_STATE_TURN));
	v.push_back(std::make_shared<ClientBero>(hi, dP, sB, GAME_STATE_RIVER));
	v.push_back(std::make_shared<ClientBero>(hi, dP, sB, GAME_STATE_POST_RIVER));
	return v;
}

END_UPP_NAMESPACE
