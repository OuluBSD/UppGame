#ifndef _CardEngine_LocalBoard_h_
#define _CardEngine_LocalBoard_h_

#include <GameRules/BoardInterface.h>

NAMESPACE_UPP

class LocalBoard : public BoardInterface
{
public:
	LocalBoard(GuiInterface *g, EngineLog *l);
	virtual ~LocalBoard();

	virtual void reset() override;
	virtual void setPlayerLists(PlayerList sl, PlayerList apl, PlayerList rpl) override;
	virtual void collectSets() override;
	virtual void collectPot() override;
	virtual void distributePot(unsigned dealerPosition) override;

	virtual void setMyCards(int* cards) override;
	virtual void getMyCards(int* cards) const override;

	virtual int getPot() const override { return pot; }
	virtual const int* getMyCards() const override { return myCards; }
	virtual int getSets() const override { return sets; }

	virtual void determinePlayerNeedToShowCards() override;
	virtual const Vector<unsigned>& getPlayerNeedToShowCards() const override;

	virtual void setAllInCondition(bool value) override;
	virtual void setLastActionPlayerID(unsigned id) override;

	virtual const Vector<unsigned>& getWinners() const override;
	void setPot(int p) { pot = p; }

private:
	GuiInterface *myGui;
	EngineLog *myLog;
};

END_UPP_NAMESPACE

#endif