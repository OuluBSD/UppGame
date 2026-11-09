#ifndef _CardEngine_ClientBoard_h_
#define _CardEngine_ClientBoard_h_

#include <GameRules/BoardInterface.h>

NAMESPACE_UPP

class ClientBoard : public BoardInterface
{
public:
	ClientBoard();
	virtual ~ClientBoard();

	virtual void reset() override;
	virtual void setPlayerLists(PlayerList sl, PlayerList apl, PlayerList rpl) override;

	virtual void setMyCards(int* cards) override;
	virtual void getMyCards(int* cards) const override;
	virtual const int* getMyCards() const override;

	virtual int getPot() const override;
	virtual void setPot(int val);
	virtual int getSets() const override;
	virtual void setSets(int val);

	virtual void setAllInCondition(bool value) override;
	virtual void setLastActionPlayerID(unsigned id) override;

	virtual void collectSets() override;
	virtual void collectPot() override;

	virtual void distributePot(unsigned dealerPosition) override;
	virtual void determinePlayerNeedToShowCards() override;

	virtual const Vector<unsigned>& getWinners() const override;
	virtual void setWinners(const Vector<unsigned>& winners);

	virtual const Vector<unsigned>& getPlayerNeedToShowCards() const override;
	virtual void setPlayerNeedToShowCards(const Vector<unsigned>& playerNeedToShowCards);

private:
	mutable Mutex m_syncMutex;
};

END_UPP_NAMESPACE

#endif
