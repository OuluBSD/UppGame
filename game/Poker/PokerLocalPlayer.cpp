#include <Poker/PokerLocalPlayer.h>
#include <GameRules/HandInterface.h>
#include <EditorCommon/Tools.h>

NAMESPACE_UPP

PokerLocalPlayer::PokerLocalPlayer(class ConfigFile *c, int id, unsigned uniqueId, PlayerType type, String name, String avatar, int sC, bool aS, bool sotS, int mB)
	: LocalPlayer(c, id, uniqueId, type, name, avatar, sC, aS, sotS, mB),
	  myDude(0), myDude4(0), myCardsValueInt(0), myOdds(-1.0), logHoleCardsDone(false), myCardsFlip(false),
	  sBluff(0), sBluffStatus(false)
{
	for(int i=0; i<3; i++) myNiveau[i] = 0;
	for(int i=0; i<5; i++) myBestHandPosition[i] = -1;
	for(int i=0; i<4; i++) myAverageSets[i] = 0;
	for(int i=0; i<7; i++) myAggressive[i] = false;

	Tools::GetRand(3, 5, 1, &myDude);
	Tools::GetRand(-5, 5, 1, &myDude4);
}

PokerLocalPlayer::~PokerLocalPlayer()
{
}

void PokerLocalPlayer::setMyBestHandPosition(int* theValue)
{
	for (int i = 0; i < 5; i++) myBestHandPosition[i] = theValue[i];
}

void PokerLocalPlayer::getMyBestHandPosition(int* theValue) const
{
	for (int i = 0; i < 5; i++) theValue[i] = myBestHandPosition[i];
}

void PokerLocalPlayer::setMyCardsFlip(bool theValue, int state)
{
	myCardsFlip = theValue;
}

int PokerLocalPlayer::getMyAggressiveSum() const
{
	int sum = 0;
	for (int i = 0; i < 7; i++) if (myAggressive[i]) sum++;
	return sum;
}

END_UPP_NAMESPACE
