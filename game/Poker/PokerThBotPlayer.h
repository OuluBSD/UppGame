#ifndef _CardEngine_PokerThBotPlayer_h_
#define _CardEngine_PokerThBotPlayer_h_

#include <Poker/PokerLocalPlayer.h>

NAMESPACE_UPP

class PokerThBotPlayer : public PokerLocalPlayer
{
public:
	PokerThBotPlayer(class ConfigFile* c, int id, unsigned uniqueId, String name, String avatar, int sC, bool aS, bool sotS, int mB);
	virtual ~PokerThBotPlayer();

	virtual void setMyAction(PlayerAction action, bool human = false) override;
	virtual void action() override;

private:
	virtual void preflopEngine() override;
	virtual void flopEngine() override;
	virtual void turnEngine() override;
	virtual void riverEngine() override;
	
	void evaluation(int bet, int raise);
	void calcMyOdds();
	int flopCardsValue(int* cards);

	bool m_disableAllIn;
};

END_UPP_NAMESPACE

#endif
