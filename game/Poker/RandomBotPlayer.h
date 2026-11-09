#ifndef _CardEngine_RandomBotPlayer_h_
#define _CardEngine_RandomBotPlayer_h_

#include <Poker/PokerLocalPlayer.h>

NAMESPACE_UPP

class RandomBotPlayer : public PokerLocalPlayer
{
public:
	RandomBotPlayer(class ConfigFile* c, int id, unsigned uniqueId, String name, String avatar, int sC, bool aS, bool sotS, int mB);
	virtual ~RandomBotPlayer();

	virtual void action() override;
	virtual void setMyAction(PlayerAction action, bool human = false) override;

private:
	virtual void preflopEngine() override {}
	virtual void flopEngine() override {}
	virtual void turnEngine() override {}
	virtual void riverEngine() override {}
};

END_UPP_NAMESPACE

#endif
