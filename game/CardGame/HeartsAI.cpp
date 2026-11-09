#include "CardGame.h"

NAMESPACE_UPP

Vector<Card> HeartsAI::ChoosePassCards(const Vector<Card>& hand) {
	struct ScoredCard : public Moveable<ScoredCard> {
		int score;
		Card card;
	};
	Vector<ScoredCard> scored_hand;
	for (const Card& c : hand) {
		int score = c.GetPoints() * 10;
		if (c.suit == "spades") {
			if (c.rank == "queen" || c.rank == "king" || c.rank == "ace") {
				score += 15;
			}
		}
		if (c.rank == "ace" || c.rank == "king" || c.rank == "queen") {
			score += 5;
		}
		ScoredCard sc;
		sc.score = score;
		sc.card = c;
		scored_hand.Add(sc);
	}

	for (int i = 0; i < scored_hand.GetCount(); i++) {
		int best = i;
		for (int j = i + 1; j < scored_hand.GetCount(); j++) {
			if (scored_hand[j].score > scored_hand[best].score) {
				best = j;
			}
		}
		if (best != i) {
			Swap(scored_hand[i], scored_hand[best]);
		}
	}

	Vector<Card> result;
	int count = scored_hand.GetCount() < 3 ? scored_hand.GetCount() : 3;
	for (int i = 0; i < count; i++) {
		result.Add(scored_hand[i].card);
	}
	return result;
}

Card HeartsAI::ChooseCard(int player_index, const Vector<Card>& hand, const Vector<TrickItem>& trick, const String& leading_suit, bool hearts_broken) {
	if (hand.IsEmpty()) {
		return Card();
	}

	bool is_first_trick = (hand.GetCount() == 13);

	// Rule 1: First turn, must play 2 of Clubs if we have it
	if (is_first_trick) {
		for (const Card& c : hand) {
			if (c.suit == "clubs" && c.rank == "2") {
				return c;
			}
		}
	}

	if (leading_suit.IsEmpty()) {
		// We are leading the trick
		Vector<Card> valid_leads;
		for (const Card& c : hand) {
			if (c.suit == "hearts" && !hearts_broken) {
				bool only_hearts = true;
				for (const Card& card : hand) {
					if (card.suit != "hearts") {
						only_hearts = false;
						break;
					}
				}
				if (only_hearts) {
					valid_leads.Add(c);
				}
			} else {
				valid_leads.Add(c);
			}
		}
		
		if (valid_leads.IsEmpty()) {
			valid_leads <<= hand; // Fallback
		}

		// Choose the lowest card among valid leads
		Card best_card = valid_leads[0];
		int best_rank = RankIndex(best_card.rank);
		for (int i = 1; i < valid_leads.GetCount(); i++) {
			int r = RankIndex(valid_leads[i].rank);
			if (r < best_rank) {
				best_rank = r;
				best_card = valid_leads[i];
			}
		}
		return best_card;
	}

	// We are following a lead
	Vector<Card> follow_cards;
	for (const Card& c : hand) {
		if (c.suit == leading_suit) {
			follow_cards.Add(c);
		}
	}

	if (!follow_cards.IsEmpty()) {
		// We can follow suit. Play the lowest card of that suit
		Card best_card = follow_cards[0];
		int best_rank = RankIndex(best_card.rank);
		for (int i = 1; i < follow_cards.GetCount(); i++) {
			int r = RankIndex(follow_cards[i].rank);
			if (r < best_rank) {
				best_rank = r;
				best_card = follow_cards[i];
			}
		}
		return best_card;
	}

	// We cannot follow suit. Discard!
	// Filter out points if it is the first trick
	Vector<Card> valid_discards;
	for (const Card& c : hand) {
		if (is_first_trick) {
			if (c.suit == "hearts") continue;
			if (c.suit == "spades" && c.rank == "queen") continue;
		}
		valid_discards.Add(c);
	}

	if (valid_discards.IsEmpty()) {
		valid_discards <<= hand; // Fallback
	}

	// 1. Try to discard Queen of Spades
	for (const Card& c : valid_discards) {
		if (c.suit == "spades" && c.rank == "queen") {
			return c;
		}
	}

	// 2. Try to discard highest Heart
	Card highest_heart;
	int highest_heart_rank = -1;
	for (const Card& c : valid_discards) {
		if (c.suit == "hearts") {
			int r = RankIndex(c.rank);
			if (r > highest_heart_rank) {
				highest_heart_rank = r;
				highest_heart = c;
			}
		}
	}
	if (highest_heart_rank != -1) {
		return highest_heart;
	}

	// 3. Try to discard highest card overall
	Card best_card = valid_discards[0];
	int best_rank = RankIndex(best_card.rank);
	for (int i = 1; i < valid_discards.GetCount(); i++) {
		int r = RankIndex(valid_discards[i].rank);
		if (r > best_rank) {
			best_rank = r;
			best_card = valid_discards[i];
		}
	}
	return best_card;
}

END_UPP_NAMESPACE