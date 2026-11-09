#include "CardGame.h"

NAMESPACE_UPP

TrickItem::TrickItem() : player_index(-1) {}
TrickItem::TrickItem(int p_idx, const Card& c) : player_index(p_idx), card(c) {}

PlayResult::PlayResult(bool s, const String& m) : success(s), message(m) {}

static void ShuffleDeck(Vector<Card>& deck) {
	for (int i = deck.GetCount() - 1; i > 0; i--) {
		int j = Random(i + 1);
		Swap(deck[i], deck[j]);
	}
}

static void SortHand(Vector<Card>& hand) {
	Sort(hand, [](const Card& a, const Card& b) {
		static const char* SUITS[] = { "clubs", "diamonds", "spades", "hearts" };
		int sa = 0, sb = 0;
		for (int i = 0; i < 4; i++) {
			if (a.suit == SUITS[i]) sa = i;
			if (b.suit == SUITS[i]) sb = i;
		}
		if (sa != sb) return sa < sb;
		return RankIndex(a.rank) < RankIndex(b.rank);
	});
}

CardGameState::CardGameState() {
	for (int i = 0; i < 4; i++) {
		scores[i] = 0;
		round_scores[i] = 0;
		last_round_scores[i] = 0;
	}
	round_number = 0;
	last_trick_winner = -1;
	last_trick_points = 0;
	last_round_moon_shooter = -1;
	game_over = false;
	trick_pending = false;
	pending_trick_winner = -1;
	pending_trick_points = 0;
	hearts_broken = false;
	turn = 0;
	phase = "PASSING";
}

void CardGameState::Log(const String& msg) {
	if (LogCallback) {
		LogCallback(msg);
	} else {
		Cout() << msg << "\n";
	}
}

void CardGameState::Deal() {
	round_number++;
	
	// Create standard deck
	Vector<Card> deck;
	static const char* SUITS[] = { "clubs", "diamonds", "hearts", "spades" };
	static const char* RANKS[] = { "2", "3", "4", "5", "6", "7", "8", "9", "10", "jack", "queen", "king", "ace" };
	for (int s = 0; s < 4; s++) {
		for (int r = 0; r < 13; r++) {
			deck.Add(Card(SUITS[s], RANKS[r]));
		}
	}
	
	ShuffleDeck(deck);
	
	for (int i = 0; i < 4; i++) {
		players[i].Clear();
		round_scores[i] = 0;
		passed_cards[i].Clear();
		last_round_scores[i] = 0;
	}
	
	// Deal hands
	for (int i = 0; i < 52; i++) {
		players[i % 4].Add(deck[i]);
	}

	for (int i = 0; i < 4; i++) {
		SortHand(players[i]);
	}
	
	hearts_broken = false;
	last_trick_winner = -1;
	last_trick_points = 0;
	last_round_moon_shooter = -1;
	game_over = false;
	trick_pending = false;
	pending_trick_winner = -1;
	pending_trick_points = 0;
	trick.Clear();

	Log(Format("Dealing round %d", round_number));

	// If round_number is a multiple of 4, no passing phase
	if (round_number % 4 == 0) {
		phase = "PLAYING";
		StartPlayPhase();
	} else {
		phase = "PASSING";
	}
}

bool CardGameState::SelectPass(int player_index, const Vector<Card>& cards) {
	if (phase != "PASSING") return false;
	if (cards.GetCount() != 3) return false;
	
	passed_cards[player_index] <<= cards;
	
	bool everyone_selected = true;
	for (int i = 0; i < 4; i++) {
		if (passed_cards[i].GetCount() != 3) {
			everyone_selected = false;
			break;
		}
	}
	
	if (everyone_selected) {
		ExecutePass();
	}
	
	return true;
}

void CardGameState::ExecutePass() {
	int pass_direction = round_number % 4; // 1 = Left, 2 = Right, 3 = Across
	Vector<Card> received[4];

	for (int i = 0; i < 4; i++) {
		int target = i;
		if (pass_direction == 1) {
			target = (i + 1) % 4;
		} else if (pass_direction == 2) {
			target = (i + 3) % 4;
		} else if (pass_direction == 3) {
			target = (i + 2) % 4;
		}
		received[target] <<= passed_cards[i];
	}

	for (int i = 0; i < 4; i++) {
		// Remove passed cards
		for (const Card& c : passed_cards[i]) {
			int idx = FindIndex(players[i], c);
			if (idx != -1) {
				players[i].Remove(idx);
			}
		}
		// Add received cards
		for (const Card& c : received[i]) {
			players[i].Add(c);
		}
		SortHand(players[i]);
	}

	for (int i = 0; i < 4; i++) {
		passed_cards[i].Clear();
	}

	phase = "PLAYING";
	StartPlayPhase();
}

void CardGameState::StartPlayPhase() {
	// Find player with 2 of clubs
	for (int i = 0; i < 4; i++) {
		for (const Card& c : players[i]) {
			if (c.suit == "clubs" && c.rank == "2") {
				turn = i;
				leading_suit = "";
				Log(Format("Player %d has the 2 of Clubs and leads.", i));
				return;
			}
		}
	}
	// Fallback if 2 of clubs is missing somehow
	turn = 0;
	leading_suit = "";
}

PlayResult CardGameState::ValidatePlay(int player_index, const Card& card) const {
	if (phase != "PLAYING") {
		return PlayResult(false, "Not in PLAYING phase.");
	}
	if (player_index != turn) {
		return PlayResult(false, "Not your turn.");
	}
	
	int card_idx = FindIndex(players[player_index], card);
	if (card_idx == -1) {
		return PlayResult(false, "Card not in hand.");
	}

	bool is_first_trick = (players[player_index].GetCount() == 13);

	if (trick.IsEmpty()) {
		// Leading card
		if (is_first_trick) {
			// First trick of the round, must lead 2 of clubs
			if (card.suit != "clubs" || card.rank != "2") {
				return PlayResult(false, "First trick must start with the 2 of Clubs.");
			}
		} else {
			// Hearts breaking rule
			if (card.suit == "hearts" && !hearts_broken) {
				bool has_other = false;
				for (const Card& c : players[player_index]) {
					if (c.suit != "hearts") {
						has_other = true;
						break;
					}
				}
				if (has_other) {
					return PlayResult(false, "Hearts have not been broken yet.");
				}
			}
		}
	} else {
		// Following card
		if (card.suit != leading_suit) {
			bool has_suit = false;
			for (const Card& c : players[player_index]) {
				if (c.suit == leading_suit) {
					has_suit = true;
					break;
				}
			}
			if (has_suit) {
				return PlayResult(false, "Must follow suit.");
			}
			
			// Point discard constraint on the first trick
			if (is_first_trick) {
				if (card.suit == "hearts" || (card.suit == "spades" && card.rank == "queen")) {
					bool only_points = true;
					for (const Card& c : players[player_index]) {
						if (c.suit != "hearts" && !(c.suit == "spades" && c.rank == "queen")) {
							only_points = false;
							break;
						}
					}
					if (!only_points) {
						return PlayResult(false, "Cannot play points on the first trick.");
					}
				}
			}
		}
	}

	return PlayResult(true, "");
}

PlayResult CardGameState::PlayCard(int player_index, const Card& card) {
	PlayResult res = ValidatePlay(player_index, card);
	if (!res.success) {
		return res;
	}

	int idx = FindIndex(players[player_index], card);
	players[player_index].Remove(idx);

	if (trick.IsEmpty()) {
		leading_suit = card.suit;
	}

	trick.Add(TrickItem(player_index, card));

	if (card.suit == "hearts" && !hearts_broken) {
		hearts_broken = true;
		Log("Hearts have been broken.");
	}

	Log(Format("Player %d plays %s", player_index, card.ToString()));

	if (trick.GetCount() == 4) {
		trick_pending = true;
		pending_trick_winner = GetTrickResult(pending_trick_points);
	} else {
		turn = (turn + 1) % 4;
	}

	return PlayResult(true, "Play recorded.");
}

int CardGameState::GetTrickResult(int& points_out) const {
	if (trick.GetCount() != 4) return -1;
	
	int winner = trick[0].player_index;
	Card highest = trick[0].card;
	
	for (int i = 1; i < 4; i++) {
		if (trick[i].card.suit == leading_suit) {
			if (RankIndex(trick[i].card.rank) > RankIndex(highest.rank)) {
				highest = trick[i].card;
				winner = trick[i].player_index;
			}
		}
	}

	int pts = 0;
	for (int i = 0; i < 4; i++) {
		pts += trick[i].card.GetPoints();
	}
	
	points_out = pts;
	return winner;
}

void CardGameState::ResolveTrick() {
	if (!trick_pending) return;

	round_scores[pending_trick_winner] += pending_trick_points;
	last_trick_winner = pending_trick_winner;
	last_trick_points = pending_trick_points;
	
	Log(Format("Player %d wins the trick with %d points.", pending_trick_winner, pending_trick_points));

	turn = pending_trick_winner;
	leading_suit = "";
	trick.Clear();
	trick_pending = false;

	// Check round end
	bool round_over = true;
	for (int i = 0; i < 4; i++) {
		if (!players[i].IsEmpty()) {
			round_over = false;
			break;
		}
	}

	if (round_over) {
		ResolveRound();
	}
}

void CardGameState::ResolveRound() {
	int moon_shooter = -1;
	for (int i = 0; i < 4; i++) {
		if (round_scores[i] == 26) {
			moon_shooter = i;
			break;
		}
	}

	if (moon_shooter != -1) {
		last_round_moon_shooter = moon_shooter;
		Log(Format("Player %d SHOT THE MOON! 🚀🌕", moon_shooter));
		for (int i = 0; i < 4; i++) {
			if (i == moon_shooter) {
				round_scores[i] = 0;
			} else {
				round_scores[i] = 26;
			}
		}
	} else {
		last_round_moon_shooter = -1;
	}

	for (int i = 0; i < 4; i++) {
		scores[i] += round_scores[i];
		last_round_scores[i] = round_scores[i];
	}

	Log(Format("Round finished. Scores: P0=%d, P1=%d, P2=%d, P3=%d", scores[0], scores[1], scores[2], scores[3]));

	bool over = false;
	for (int i = 0; i < 4; i++) {
		if (scores[i] >= 100) {
			over = true;
			break;
		}
	}

	if (over) {
		game_over = true;
		phase = "GAME_OVER";
		Log("Game Over!");
	} else {
		phase = "ROUND_END";
	}
}

void CardGameState::BeginNextRound() {
	Deal();
}

END_UPP_NAMESPACE
