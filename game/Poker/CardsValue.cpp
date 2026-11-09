#include <Poker/PokerPlayerInterface.h>
#include <Poker/CardsValue.h>
#include <GameRules/ArrayData.h>
#include <EditorCommon/Tools.h>
#include <GameRules/PlayerInterface.h>
#include <CardEvaluator/CardEvaluator.h>

namespace Upp {

using namespace omp;

static HandEvaluator& GetHandEvaluator()
{
	static HandEvaluator eval;
	return eval;
}

static uint64_t PkrToOmpCard(int pkrCard)
{
	if (pkrCard < 0 || pkrCard >= 52) return 0;
	int rank = pkrCard % 13;
	int suit = pkrCard / 13;
	return 4 * rank + suit;
}

int CardsValue::holeCardsClass(int one, int two)
{
	if ((one - 1) % 13 < (two - 1) % 13) {
		Swap(one, two);
	}

	if ((one - 1) % 13 == (two - 1) % 13) {
		if ((one - 1) % 13 + 2 > 10) return 10;
		else {
			switch ((one - 1) % 13 + 2) {
			case 10: return 9;
			case 9:  return 8;
			case 8:  return 7;
			case 7:  return 6;
			default: return 5;
			}
		}
	}
	switch ((one - 1) % 13 + 2) {
	// Ace
	case 14: {
		if ((one - 1) / 13 == (two - 1) / 13) {
			switch ((one - 1) % 13 - (two - 1) % 13) {
			case 1:  return 10;
			case 2:  return 9;
			case 3:  return 9;
			case 4:  return 8;
			default: return 7;
			}
		} else {
			switch ((one - 1) % 13 - (two - 1) % 13) {
			case 1:  return 9;
			case 2:  return 8;
			case 3:  return 7;
			case 4:  return 7;
			default: return 4;
			}
		}
	}
	break;
	// King
	case 13: {
		if ((one - 1) / 13 == (two - 1) / 13) {
			switch ((one - 1) % 13 - (two - 1) % 13) {
			case 1:  return 9;
			case 2:  return 8;
			case 3:  return 8;
			case 4:  return 6;
			default: return 5;
			}
		} else {
			switch ((one - 1) % 13 - (two - 1) % 13) {
			case 1:  return 7;
			case 2:  return 6;
			case 3:  return 6;
			default: return 4;
			}
		}
	}
	break;
	// Queen
	case 12: {
		if ((one - 1) / 13 == (two - 1) / 13) {
			switch ((one - 1) % 13 - (two - 1) % 13) {
			case 1:  return 8;
			case 2:  return 7;
			case 3:  return 6;
			case 4:  return 5;
			default: return 4;
			}
		} else {
			switch ((one - 1) % 13 - (two - 1) % 13) {
			case 1:  return 6;
			case 2:  return 6;
			case 3:  return 4;
			default: return 3;
			}
		}
	}
	break;
	// Jack
	case 11: {
		if ((one - 1) / 13 == (two - 1) / 13) {
			switch ((one - 1) % 13 - (two - 1) % 13) {
			case 1:  return 7;
			case 2:  return 6;
			case 3:  return 5;
			case 4:  return 4;
			default: return 3;
			}
		} else {
			switch ((one - 1) % 13 - (two - 1) % 13) {
			case 1:  return 6;
			case 2:  return 5;
			case 3:  return 4;
			default: return 2;
			}
		}
	}
	break;
	// 10
	case 10: {
		if ((one - 1) / 13 == (two - 1) / 13) {
			switch ((one - 1) % 13 - (two - 1) % 13) {
			case 1:  return 6;
			case 2:  return 5;
			default: return 2;
			}
		} else {
			switch ((one - 1) % 13 - (two - 1) % 13) {
			case 1:  return 5;
			case 2:  return 4;
			default: return 1;
			}
		}
	}
	break;
	// Rest
	default: {
		if ((one - 1) % 13 - (two - 1) % 13 <= 2) {
			if ((one - 1) / 13 == (two - 1) / 13) return 5;
			else return 3;
		} else {
			if ((one - 1) % 13 - (two - 1) % 13 == 3) return 2;
			else return 1;
		}
	}
	}
	return 0;
}

int CardsValue::holeCardsToIntCode(int* cards)
{
	if (cards[0] % 13 == cards[1] % 13) {
		return ((cards[0] % 13) * 1000 + (cards[0] % 13) * 10);
	} else {
		if (cards[0] % 13 < cards[1] % 13) {
			if (cards[0] / 13 == cards[1] / 13) {
				return ((cards[0] % 13) * 1000 + (cards[1] % 13) * 10 + 1);
			} else {
				return ((cards[0] % 13) * 1000 + (cards[1] % 13) * 10);
			}
		} else {
			if (cards[0] / 13 == cards[1] / 13) {
				return ((cards[1] % 13) * 1000 + (cards[0] % 13) * 10 + 1);
			} else {
				return ((cards[1] % 13) * 1000 + (cards[0] % 13) * 10);
			}
		}
	}
}

int CardsValue::cardsValue(int* cards, int* position)
{
	int array[7][3];
	int j1, j2, j3, j4, j5, k1, k2;

	for (j1 = 0; j1 < 7; j1++) {
		array[j1][0] = cards[j1] / 13;
		array[j1][1] = cards[j1] % 13;
		array[j1][2] = j1;
	}

	for (k1 = 0; k1 < 7; k1++) {
		for (k2 = k1 + 1; k2 < 7; k2++) {
			if (array[k1][0] < array[k2][0]) {
				for (int i = 0; i < 3; i++) Swap(array[k1][i], array[k2][i]);
			}
		}
	}

	for (k1 = 0; k1 < 7; k1++) {
		for (k2 = k1 + 1; k2 < 7; k2++) {
			if (array[k1][0] == array[k2][0] && array[k1][1] < array[k2][1]) {
				for (int i = 0; i < 3; i++) Swap(array[k1][i], array[k2][i]);
			}
		}
	}

	for (j1 = 0; j1 < 3; j1++) {
		if (array[j1][0] == array[j1 + 1][0] && array[j1][0] == array[j1 + 2][0] && array[j1][0] == array[j1 + 3][0] && array[j1][0] == array[j1 + 4][0]) {
			if (array[j1][1] - 1 == array[j1 + 1][1] && array[j1 + 1][1] - 1 == array[j1 + 2][1] && array[j1 + 2][1] - 1 == array[j1 + 3][1] && array[j1 + 3][1] - 1 == array[j1 + 4][1]) {
				if (array[j1][1] == 12) {
					if (position) {
						for (j2 = 0; j2 < 5; j2++) position[j2] = array[j1 + j2][2];
					}
					return 900000000;
				} else {
					if (position) {
						for (j2 = 0; j2 < 5; j2++) position[j2] = array[j1 + j2][2];
					}
					return 800000000 + array[j1][1] * 1000000;
				}
			}
		}
	}

	for (j1 = 0; j1 < 3; j1++) {
		if (array[j1][0] == array[j1 + 1][0] && array[j1][0] == array[j1 + 2][0] && array[j1][0] == array[j1 + 3][0] && array[j1][0] == array[j1 + 4][0]) {
			for (j2 = j1 + 1; j2 < 4; j2++) {
				if (array[j1][1] - 9 == array[j2][1] && array[j2][1] - 1 == array[j2 + 1][1] && array[j2 + 1][1] - 1 == array[j2 + 2][1] && array[j2 + 2][1] - 1 == array[j2 + 3][1] && array[j1][0] == array[j2 + 2][0] && array[j1][0] == array[j2 + 3][0]) {
					if (position) {
						position[0] = array[j1][2];
						for (j3 = 0; j3 < 4; j3++) position[j3 + 1] = array[j2 + j3][2];
					}
					return 800000000 + 3 * 1000000;
				}
			}
		}
	}

	for (j1 = 0; j1 < 3; j1++) {
		if (array[j1][0] == array[j1 + 1][0] && array[j1][0] == array[j1 + 2][0] && array[j1][0] == array[j1 + 3][0] && array[j1][0] == array[j1 + 4][0]) {
			if (position) {
				for (j2 = 0; j2 < 5; j2++) position[j2] = array[j1 + j2][2];
			}
			return 500000000 + array[j1][1] * 1000000 + array[j1 + 1][1] * 10000 + array[j1 + 2][1] * 100 + array[j1 + 3][1] * 10 + array[j1 + 4][1];
		}
	}

	for (k1 = 0; k1 < 7; k1++) {
		for (k2 = k1 + 1; k2 < 7; k2++) {
			if (array[k1][1] < array[k2][1]) {
				for (int i = 0; i < 3; i++) Swap(array[k1][i], array[k2][i]);
			}
		}
	}

	for (k1 = 0; k1 < 7; k1++) {
		for (k2 = k1 + 1; k2 < 7; k2++) {
			if (array[k1][1] == array[k2][1] && array[k1][2] < array[k2][2]) {
				for (int i = 0; i < 3; i++) Swap(array[k1][i], array[k2][i]);
			}
		}
	}

	for (j1 = 0; j1 < 4; j1++) {
		if (array[j1][1] == array[j1 + 1][1] && array[j1][1] == array[j1 + 2][1] && array[j1][1] == array[j1 + 3][1]) {
			if (j1 == 0) {
				if (position) {
					for (j2 = 0; j2 < 5; j2++) position[j2] = array[j2][2];
				}
				return 700000000 + array[j1][1] * 1000000 + array[j1 + 4][1] * 10000;
			} else {
				if (position) {
					for (j2 = 0; j2 < 4; j2++) position[j2] = array[j1 + j2][2];
					position[4] = array[0][2];
				}
				return 700000000 + array[j1][1] * 1000000 + array[0][1] * 10000;
			}
		}
	}

	int drei, zwei;

	for (j1 = 0; j1 < 7; j1++) {
		for (j2 = j1 + 1; j2 < 7; j2++) {
			for (j3 = j2 + 1; j3 < 7; j3++) {
				for (j4 = j3 + 1; j4 < 7; j4++) {
					for (j5 = j4 + 1; j5 < 7; j5++) {
						if (array[j1][1] - 1 == array[j2][1] && array[j2][1] - 1 == array[j3][1] && array[j3][1] - 1 == array[j4][1] && array[j4][1] - 1 == array[j5][1]) {
							if (position) {
								position[0] = array[j1][2]; position[1] = array[j2][2]; position[2] = array[j3][2]; position[3] = array[j4][2]; position[4] = array[j5][2];
							}
							return 400000000 + array[j1][1] * 1000000;
						}
						if ((array[j1][1] == array[j2][1] && array[j1][1] == array[j3][1] && array[j4][1] == array[j5][1]) || (array[j3][1] == array[j4][1] && array[j3][1] == array[j5][1] && array[j1][1] == array[j2][1])) {
							if (position) {
								position[0] = array[j1][2]; position[1] = array[j2][2]; position[2] = array[j3][2]; position[3] = array[j4][2]; position[4] = array[j5][2];
							}
							if (array[j3][1] == array[j1][1]) {
								drei = array[j1][1]; zwei = array[j4][1];
							} else {
								drei = array[j4][1]; zwei = array[j1][1];
							}
							return 600000000 + drei * 1000000 + zwei * 10000;
						}
					}
				}
			}
		}
	}

	for (j1 = 0; j1 < 7; j1++) {
		for (j2 = j1 + 1; j2 < 7; j2++) {
			for (j3 = j2 + 1; j3 < 7; j3++) {
				for (j4 = j3 + 1; j4 < 7; j4++) {
					for (j5 = j4 + 1; j5 < 7; j5++) {
						if (array[j1][1] - 9 == array[j2][1] && array[j2][1] - 1 == array[j3][1] && array[j3][1] - 1 == array[j4][1] && array[j4][1] - 1 == array[j5][1]) {
							if (position) {
								position[0] = array[j1][2]; position[1] = array[j2][2]; position[2] = array[j3][2]; position[3] = array[j4][2]; position[4] = array[j5][2];
							}
							return 400000000 + array[j2][1] * 1000000;
						}
					}
				}
			}
		}
	}

	for (j1 = 0; j1 < 5; j1++) {
		if (array[j1][1] == array[j1 + 1][1] && array[j1][1] == array[j1 + 2][1]) {
			if (j1 == 0) {
				if (position) {
					for (j2 = 0; j2 < 5; j2++) position[j2] = array[j2][2];
				}
				return 300000000 + array[j1][1] * 1000000 + array[j1 + 3][1] * 10000 + array[j1 + 4][1] * 100;
			} else if (j1 == 1) {
				if (position) {
					for (j2 = 0; j2 < 5; j2++) position[j2] = array[j2][2];
				}
				return 300000000 + array[j1][1] * 1000000 + array[j1 - 1][1] * 10000 + array[j1 + 3][1] * 100;
			} else {
				if (position) {
					for (j2 = 0; j2 < 3; j2++) position[j2] = array[j1 + j2][2];
					position[3] = array[0][2]; position[4] = array[1][2];
				}
				return 300000000 + array[j1][1] * 1000000 + array[0][1] * 10000 + array[1][1] * 100;
			}
		}
	}

	for (j1 = 0; j1 < 4; j1++) {
		for (j2 = j1 + 2; j2 < 6; j2++) {
			if (array[j1][1] == array[j1 + 1][1] && array[j2][1] == array[j2 + 1][1]) {
				if (j1 == 0) {
					if (j2 == 2) {
						if (position) {
							position[0] = array[j1][2]; position[1] = array[j1+1][2]; position[2] = array[j2][2]; position[3] = array[j2+1][2]; position[4] = array[j2+2][2];
						}
						return 200000000 + array[j1][1] * 1000000 + array[j2][1] * 10000 + array[j2 + 2][1] * 100;
					} else {
						if (position) {
							position[0] = array[j1][2]; position[1] = array[j1+1][2]; position[2] = array[j2][2]; position[3] = array[j2+1][2]; position[4] = array[j1+2][2];
						}
						return 200000000 + array[j1][1] * 1000000 + array[j2][1] * 10000 + array[j1 + 2][1] * 100;
					}
				} else {
					if (position) {
						position[0] = array[j1][2]; position[1] = array[j1+1][2]; position[2] = array[j2][2]; position[3] = array[j2+1][2]; position[4] = array[0][2];
					}
					return 200000000 + array[j1][1] * 1000000 + array[j2][1] * 10000 + array[0][1] * 100;
				}
			}
		}
	}

	for (j1 = 0; j1 < 6; j1++) {
		if (array[j1][1] == array[j1 + 1][1]) {
			if (j1 == 0) {
				if (position) for (j2 = 0; j2 < 5; j2++) position[j2] = array[j2][2];
				return 100000000 + array[j1][1] * 1000000 + array[j1 + 2][1] * 10000 + array[j1 + 3][1] * 100 + array[j1 + 4][1];
			} else if (j1 == 1) {
				if (position) for (j2 = 0; j2 < 5; j2++) position[j2] = array[j2][2];
				return 100000000 + array[j1][1] * 1000000 + array[j1 - 1][1] * 10000 + array[j1 + 2][1] * 100 + array[j1 + 3][1];
			} else if (j1 == 2) {
				if (position) for (j2 = 0; j2 < 5; j2++) position[j2] = array[j2][2];
				return 100000000 + array[j1][1] * 1000000 + array[j1 - 2][1] * 10000 + array[j1 - 1][1] * 100 + array[j1 + 2][1];
			} else {
				if (position) {
					for (j2 = 0; j2 < 2; j2++) position[j2] = array[j1 + j2][2];
					position[2] = array[0][2]; position[3] = array[1][2]; position[4] = array[2][2];
				}
				return 100000000 + array[j1][1] * 1000000 + array[0][1] * 10000 + array[1][1] * 100 + array[2][1];
			}
		}
	}

	if (position) {
		for (j2 = 0; j2 < 5; j2++) position[j2] = array[j2][2];
	}
	return array[0][1] * 1000000 + array[1][1] * 10000 + array[2][1] * 100 + array[3][1] * 10 + array[4][1];
}

Vector<Vector<int>> CardsValue::calcCardsChance(TexasRound beRoID, int* playerCards, int* boardCards)
{
	int i, j;
	Vector<Vector<int>> chance;
	chance.SetCount(2);
	chance[0].SetCount(10, 0);
	chance[1].SetCount(10, 0);

	const HandEvaluator& evaluator = GetHandEvaluator();
	omp::Hand hand = omp::Hand::empty();
	hand += omp::Hand((unsigned)PkrToOmpCard(playerCards[0]));
	hand += omp::Hand((unsigned)PkrToOmpCard(playerCards[1]));

	int sum = 0;

	switch (beRoID) {
	case GAME_STATE_PREFLOP:
		chance = ArrayData::getHandChancePreflop(holeCardsToIntCode(playerCards));
		break;
	case GAME_STATE_FLOP:
		hand += omp::Hand((unsigned)PkrToOmpCard(boardCards[0]));
		hand += omp::Hand((unsigned)PkrToOmpCard(boardCards[1]));
		hand += omp::Hand((unsigned)PkrToOmpCard(boardCards[2]));
		for (i = 0; i < 51; i++) {
			if (i != playerCards[0] && i != playerCards[1] && i != boardCards[0] && i != boardCards[1] && i != boardCards[2]) {
				omp::Hand h1 = hand + omp::Hand((unsigned)PkrToOmpCard(i));
				for (j = i + 1; j < 52; j++) {
					if (j != playerCards[0] && j != playerCards[1] && j != boardCards[0] && j != boardCards[1] && j != boardCards[2] && j != i) {
						omp::Hand h2 = h1 + omp::Hand((unsigned)PkrToOmpCard(j));
						uint16_t val = evaluator.evaluate(h2);
						int cat = val / 4096;
						if (cat >= 1 && cat <= 9) {
							if (cat == 9 && (val & 0xFFF) == 0x00C) {
								chance[0][9]++;
							} else {
								chance[0][cat - 1]++;
							}
						}
						sum++;
					}
				}
			}
		}
		for (i = 0; i < 10; i++) {
			if (chance[0][i] > 0) chance[1][i] = 1;
			chance[0][i] = (int)(((double)chance[0][i] / (double)sum) * 100.0 + 0.5);
		}
		break;
	case GAME_STATE_TURN:
		hand += omp::Hand((unsigned)PkrToOmpCard(boardCards[0]));
		hand += omp::Hand((unsigned)PkrToOmpCard(boardCards[1]));
		hand += omp::Hand((unsigned)PkrToOmpCard(boardCards[2]));
		hand += omp::Hand((unsigned)PkrToOmpCard(boardCards[3]));
		for (i = 0; i < 52; i++) {
			if (i != playerCards[0] && i != playerCards[1] && i != boardCards[0] && i != boardCards[1] && i != boardCards[2] && i != boardCards[3]) {
				omp::Hand h1 = hand + omp::Hand((unsigned)PkrToOmpCard(i));
				uint16_t val = evaluator.evaluate(h1);
				int cat = val / 4096;
				if (cat >= 1 && cat <= 9) {
					if (cat == 9 && (val & 0xFFF) == 0x00C) {
						chance[0][9]++;
					} else {
						chance[0][cat - 1]++;
					}
				}
				sum++;
			}
		}
		for (i = 0; i < 10; i++) {
			if (chance[0][i] > 0) chance[1][i] = 1;
			chance[0][i] = (int)(((double)chance[0][i] / (double)sum) * 100.0 + 0.5);
		}
		break;
	case GAME_STATE_RIVER:
		hand += omp::Hand((unsigned)PkrToOmpCard(boardCards[0]));
		hand += omp::Hand((unsigned)PkrToOmpCard(boardCards[1]));
		hand += omp::Hand((unsigned)PkrToOmpCard(boardCards[2]));
		hand += omp::Hand((unsigned)PkrToOmpCard(boardCards[3]));
		hand += omp::Hand((unsigned)PkrToOmpCard(boardCards[4]));
		{
			uint16_t val = evaluator.evaluate(hand);
			int cat = val / 4096;
			if (cat >= 1 && cat <= 9) {
				int pkrCat = cat - 1;
				if (cat == 9 && (val & 0xFFF) == 0x00C) pkrCat = 9;
				chance[0][pkrCat] = 100;
				chance[1][pkrCat] = 1;
			}
		}
		break;
	default:
		break;
	}
	return chance;
}

String CardsValue::determineHandName(int myCardsValueInt, PlayerList activePlayerList)
{
	Vector<int> shownCardsValueInt;
	Vector<int> sameHandCardsValueInt;
	bool different = false;
	bool equal = false;

	if (activePlayerList) {
		for (int i = 0; i < (int)activePlayerList->size(); i++) {
			const auto& p = (*activePlayerList)[i];
			if (p->getMyAction() != PLAYER_ACTION_FOLD) {
				if (const auto* poker_player = dynamic_cast<const PokerPlayerInterface*>(p.get()))
					shownCardsValueInt.Add(poker_player->getMyCardsValueInt());
			}
		}
	}

	for (int i = 0; i < shownCardsValueInt.GetCount(); i++) {
		if (shownCardsValueInt[i] == myCardsValueInt) {
			shownCardsValueInt.Remove(i);
			break;
		}
	}

	Vector<String> cardString = translateCardsValueCode(myCardsValueInt);
	int csIdx = 0;
	String handName;

	switch (myCardsValueInt / 100000000) {
	case 9:
		if (csIdx < cardString.GetCount()) handName = cardString[csIdx++];
		break;
	case 8:
		if (csIdx + 1 < cardString.GetCount()) { handName = cardString[csIdx++]; handName += cardString[csIdx++]; }
		break;
	case 7:
		if (csIdx + 1 < cardString.GetCount()) { handName = cardString[csIdx++]; handName += cardString[csIdx++]; }
		for (int val : shownCardsValueInt) {
			if ((val / 1000000) == (myCardsValueInt / 1000000)) sameHandCardsValueInt.Add(val);
		}
		if (!sameHandCardsValueInt.IsEmpty()) {
			for (int i = 0; i < sameHandCardsValueInt.GetCount(); ) {
				if ((sameHandCardsValueInt[i] / 10000) == (myCardsValueInt / 10000)) {
					equal = true; i++;
				} else {
					different = true; sameHandCardsValueInt.Remove(i);
				}
			}
			if (different && csIdx < cardString.GetCount()) handName += ", fifth card " + cardString[csIdx++];
		}
		break;
	case 6:
		if (csIdx + 2 < cardString.GetCount()) { handName = cardString[csIdx++]; handName += cardString[csIdx++]; handName += cardString[csIdx++]; }
		break;
	case 5:
		if (csIdx + 1 < cardString.GetCount()) { handName = cardString[csIdx++]; handName += cardString[csIdx++]; }
		for (int val : shownCardsValueInt) {
			if ((val / 1000000) == (myCardsValueInt / 1000000)) sameHandCardsValueInt.Add(val);
		}
		if (!sameHandCardsValueInt.IsEmpty()) {
			for (int i = 0; i < sameHandCardsValueInt.GetCount(); ) {
				if ((sameHandCardsValueInt[i] / 10000) == (myCardsValueInt / 10000)) {
					equal = true; i++;
				} else {
					different = true; sameHandCardsValueInt.Remove(i);
				}
			}
			if (different && csIdx < cardString.GetCount()) handName += ", second card " + cardString[csIdx++];
			if (equal) {
				different = false; equal = false; csIdx++;
				for (int i = 0; i < sameHandCardsValueInt.GetCount(); ) {
					if ((sameHandCardsValueInt[i] / 100) == (myCardsValueInt / 100)) {
						equal = true; i++;
					} else {
						different = true; sameHandCardsValueInt.Remove(i);
					}
				}
				if (different && csIdx < cardString.GetCount()) handName += ", third card " + cardString[csIdx];
				if (equal) {
					different = false; equal = false;
					for (int i = 0; i < sameHandCardsValueInt.GetCount(); ) {
						if ((sameHandCardsValueInt[i] / 10) == (myCardsValueInt / 10)) {
							equal = true; i++;
						} else {
							different = true; sameHandCardsValueInt.Remove(i);
						}
					}
					if (different && csIdx + 1 < cardString.GetCount()) handName += ", fourth card " + cardString[++csIdx];
					if (equal) {
						different = false;
						for (int val : sameHandCardsValueInt) {
							if (val != myCardsValueInt) different = true;
						}
						if (different && csIdx + 1 < cardString.GetCount()) handName += ", fifth card " + cardString[++csIdx];
					}
				}
			}
		}
		break;
	case 4:
		if (csIdx + 1 < cardString.GetCount()) { handName = cardString[csIdx++]; handName += cardString[csIdx++]; }
		break;
	case 3:
		if (csIdx + 1 < cardString.GetCount()) { handName = cardString[csIdx++]; handName += cardString[csIdx++]; }
		for (int val : shownCardsValueInt) {
			if ((val / 1000000) == (myCardsValueInt / 1000000)) sameHandCardsValueInt.Add(val);
		}
		if (!sameHandCardsValueInt.IsEmpty()) {
			for (int i = 0; i < sameHandCardsValueInt.GetCount(); ) {
				if ((sameHandCardsValueInt[i] / 10000) == (myCardsValueInt / 10000)) {
					equal = true; i++;
				} else {
					different = true; sameHandCardsValueInt.Remove(i);
				}
			}
			if (different && csIdx + 1 < cardString.GetCount()) handName += ", fourth card " + cardString[++csIdx];
			if (equal) {
				different = false;
				for (int val : sameHandCardsValueInt) {
					if ((val / 100) != (myCardsValueInt / 100)) different = true;
				}
				if (different && csIdx + 1 < cardString.GetCount()) handName += ", fifth card " + cardString[++csIdx];
			}
		}
		break;
	case 2:
		if (csIdx + 2 < cardString.GetCount()) { handName = cardString[csIdx++]; handName += cardString[csIdx++]; handName += cardString[csIdx++]; }
		for (int val : shownCardsValueInt) {
			if ((val / 10000) == (myCardsValueInt / 10000)) sameHandCardsValueInt.Add(val);
		}
		if (!sameHandCardsValueInt.IsEmpty()) {
			for (int val : sameHandCardsValueInt) {
				if ((val / 100) != (myCardsValueInt / 100)) different = true;
			}
			if (different && csIdx + 1 < cardString.GetCount()) handName += ", fifth card " + cardString[++csIdx];
		}
		break;
	case 1:
		if (csIdx + 1 < cardString.GetCount()) { handName = cardString[csIdx++]; handName += cardString[csIdx++]; }
		for (int val : shownCardsValueInt) {
			if ((val / 1000000) == (myCardsValueInt / 1000000)) sameHandCardsValueInt.Add(val);
		}
		if (!sameHandCardsValueInt.IsEmpty()) {
			for (int i = 0; i < sameHandCardsValueInt.GetCount(); ) {
				if ((sameHandCardsValueInt[i] / 10000) == (myCardsValueInt / 10000)) {
					equal = true; i++;
				} else {
					different = true; sameHandCardsValueInt.Remove(i);
				}
			}
			if (different && csIdx + 1 < cardString.GetCount()) handName += ", third card " + cardString[++csIdx];
			if (equal) {
				different = false; equal = false;
				for (int i = 0; i < sameHandCardsValueInt.GetCount(); ) {
					if ((sameHandCardsValueInt[i] / 100) == (myCardsValueInt / 100)) {
						equal = true; i++;
					} else {
						different = true; sameHandCardsValueInt.Remove(i);
					}
				}
				if (different && csIdx + 1 < cardString.GetCount()) handName += ", fourth card " + cardString[++csIdx];
				if (equal) {
					different = false;
					for (int val : sameHandCardsValueInt) {
						if (val != myCardsValueInt) different = true;
					}
					if (different && csIdx + 1 < cardString.GetCount()) handName += ", fifth card " + cardString[++csIdx];
				}
			}
		}
		break;
	case 0:
		if (csIdx + 1 < cardString.GetCount()) { handName = cardString[csIdx++]; handName += cardString[csIdx++]; }
		for (int val : shownCardsValueInt) {
			if ((val / 1000000) == (myCardsValueInt / 1000000)) sameHandCardsValueInt.Add(val);
		}
		if (!sameHandCardsValueInt.IsEmpty()) {
			for (int i = 0; i < sameHandCardsValueInt.GetCount(); ) {
				if ((sameHandCardsValueInt[i] / 10000) == (myCardsValueInt / 10000)) {
					equal = true; i++;
				} else {
					different = true; sameHandCardsValueInt.Remove(i);
				}
			}
			if (different && csIdx + 1 < cardString.GetCount()) handName += ", second card " + cardString[++csIdx];
			if (equal) {
				different = false; equal = false;
				for (int i = 0; i < sameHandCardsValueInt.GetCount(); ) {
					if ((sameHandCardsValueInt[i] / 100) == (myCardsValueInt / 100)) {
						equal = true; i++;
					} else {
						different = true; sameHandCardsValueInt.Remove(i);
					}
				}
				if (different && csIdx + 1 < cardString.GetCount()) handName += ", third card " + cardString[++csIdx];
				if (equal) {
					different = false; equal = false;
					for (int i = 0; i < sameHandCardsValueInt.GetCount(); ) {
						if ((sameHandCardsValueInt[i] / 10) == (myCardsValueInt / 10)) {
							equal = true; i++;
						} else {
							different = true; sameHandCardsValueInt.Remove(i);
						}
					}
					if (different && csIdx + 1 < cardString.GetCount()) handName += ", fourth card " + cardString[++csIdx];
					if (equal) {
						different = false;
						for (int val : sameHandCardsValueInt) {
							if (val != myCardsValueInt) different = true;
						}
						if (different && csIdx + 1 < cardString.GetCount()) handName += ", fifth card " + cardString[++csIdx];
					}
				}
			}
		}
		break;
	default:
		break;
	}
	return handName;
}

Vector<String> CardsValue::translateCardsValueCode(int cvc)
{
	Vector<String> res;
	int f = cvc / 100000000;
	int s = cvc / 1000000 - f * 100;
	int t = cvc / 10000 - f * 10000 - s * 100;
	int frth = cvc / 100 - f * 1000000 - s * 10000 - t * 100;
	int fifth = cvc - f * 100000000 - s * 1000000 - t * 10000 - frth * 100;
	int fA = cvc / 10 - f * 10000000 - s * 100000 - t * 1000 - frth * 10;
	int fB = cvc - f * 100000000 - s * 1000000 - t * 10000 - frth * 100 - fA * 10;

	static const char* rnk[] = { "Deuce", "Three", "Four", "Five", "Six", "Seven", "Eight", "Nine", "Ten", "Jack", "Queen", "King", "Ace" };
	static const char* rnks[] = { "Deuces", "Threes", "Fours", "Fives", "Sixes", "Sevens", "Eights", "Nines", "Tens", "Jacks", "Queens", "Kings", "Aces" };

	auto GetRank = [&](int v) { return (v >= 0 && v <= 12) ? String(rnk[v]) : "ERROR"; };
	auto GetRanks = [&](int v) { return (v >= 0 && v <= 12) ? String(rnks[v]) : "ERROR"; };

	switch (f) {
	case 9: res.Add("Royal Flush"); break;
	case 8: res.Add("Straight Flush, "); res.Add(GetRank(s) + " high"); break;
	case 7: res.Add("Four of a Kind, "); res.Add(GetRanks(s)); res.Add(GetRank(t)); break;
	case 6: res.Add("Full House, "); res.Add(GetRanks(s) + " full of "); res.Add(GetRanks(t)); break;
	case 5: res.Add("Flush, "); res.Add(GetRank(s) + " high"); res.Add(GetRank(t)); res.Add(GetRank(frth)); res.Add(GetRank(fA)); res.Add(GetRank(fB)); break;
	case 4: res.Add("Straight, "); res.Add(GetRank(s) + " high"); break;
	case 3: res.Add("Three of a Kind, "); res.Add(GetRanks(s)); res.Add(GetRank(t)); res.Add(GetRank(frth)); break;
	case 2: res.Add("Two Pair, "); res.Add(GetRanks(s) + " and "); res.Add(GetRanks(t)); res.Add(GetRank(frth)); break;
	case 1: res.Add("One Pair, "); res.Add(GetRanks(s)); res.Add(GetRank(t)); res.Add(GetRank(frth)); res.Add(GetRank(fifth)); break;
	case 0: res.Add("High Card, "); res.Add(GetRank(s)); res.Add(GetRank(t)); res.Add(GetRank(frth)); res.Add(GetRank(fA)); res.Add(GetRank(fB)); break;
	default: res.Add("ERROR"); break;
	}
	return res;
}

} // namespace Upp
