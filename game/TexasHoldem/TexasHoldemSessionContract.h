#ifndef _CardEngine_TexasHoldemSessionContract_h_
#define _CardEngine_TexasHoldemSessionContract_h_

#include <Core/Core.h>

NAMESPACE_UPP

struct TexasHoldemPlayerSnapshot : Moveable<TexasHoldemPlayerSnapshot> {
	int seat = -1;
	int uid = 0;
	String name;
	bool hero = false;
	bool active = false;
	int stack = 0;
	int bet = 0;
	int action = 0;
	int button = 0;
	Vector<int> hole_cards;

	void Jsonize(JsonIO& jio);
};

struct TexasHoldemGroundTruthRecord : Moveable<TexasHoldemGroundTruthRecord> {
	int schema = 1;
	String session_id;
	int frame_id = 0;
	int render_step = 0;
	int64 timestamp_ms = 0;
	String provider;
	int table_width = 0;
	int table_height = 0;
	int seed = -1;
	int game_id = 0;
	int hand_id = 0;
	int street = -1;
	int turn_uid = -1;
	int pot = 0;
	Vector<int> board_cards;
	Vector<TexasHoldemPlayerSnapshot> players;

	void Jsonize(JsonIO& jio);
};

struct TexasHoldemSessionMetadata : Moveable<TexasHoldemSessionMetadata> {
	int schema = 1;
	String kind = "texas_holdem_source_contract_sample";
	String session_id;
	String provider;
	int table_width = 0;
	int table_height = 0;
	int seed = -1;
	int frame_count = 1;
	String frame_format = "png";
	String frame_pattern = "frames/00000000.png";
	String ground_truth = "groundtruth.jsonl";
	// task 0137 (M07): additive recording-progress marker. "recording" while
	// --record-session is still writing frames/ground-truth; "complete" once the
	// session is fully finalized. Absent from old (pre-0137) fixtures, which every
	// reader must treat as already "complete" (see VsmLiveM01M02SessionSource).
	String status = "complete";

	void Jsonize(JsonIO& jio);
};

String TexasHoldemFrameName(int frame_id);
int ValidateTexasHoldemSourceSession(const String& root);
int ReplayTexasHoldemSession(const String& root, const String& expected_provider,
                             Size expected_size, int expected_frame_ms);

END_UPP_NAMESPACE

#endif
