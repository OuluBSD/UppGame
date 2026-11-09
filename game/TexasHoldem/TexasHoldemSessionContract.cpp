#include "TexasHoldemSessionContract.h"

NAMESPACE_UPP

void TexasHoldemPlayerSnapshot::Jsonize(JsonIO& jio)
{
	jio("seat", seat)
	   ("uid", uid)
	   ("name", name)
	   ("hero", hero)
	   ("active", active)
	   ("stack", stack)
	   ("bet", bet)
	   ("action", action)
	   ("button", button)
	   ("hole_cards", hole_cards);
}

void TexasHoldemGroundTruthRecord::Jsonize(JsonIO& jio)
{
	jio("schema", schema)
	   ("session_id", session_id)
	   ("frame_id", frame_id)
	   ("render_step", render_step)
	   ("timestamp_ms", timestamp_ms)
	   ("provider", provider)
	   ("table_width", table_width)
	   ("table_height", table_height)
	   ("seed", seed)
	   ("game_id", game_id)
	   ("hand_id", hand_id)
	   ("street", street)
	   ("turn_uid", turn_uid)
	   ("pot", pot)
	   ("board_cards", board_cards)
	   ("players", players);
}

void TexasHoldemSessionMetadata::Jsonize(JsonIO& jio)
{
	jio("schema", schema)
	   ("kind", kind)
	   ("session_id", session_id)
	   ("provider", provider)
	   ("table_width", table_width)
	   ("table_height", table_height)
	   ("seed", seed)
	   ("frame_count", frame_count)
	   ("frame_format", frame_format)
	   ("frame_pattern", frame_pattern)
	   ("ground_truth", ground_truth)
	   ("status", status);
}

String TexasHoldemFrameName(int frame_id)
{
	return Format("%08d.png", frame_id);
}

static bool HasKey(const ValueMap& map, const char *key)
{
	return map.Find(key) >= 0;
}

static int SessionInt(ValueMap map, const char *key, int fallback = Null)
{
	Value value = map.Get(key, fallback);
	return IsNull(value) ? fallback : (int)value;
}

static int64 SessionInt64(ValueMap map, const char *key, int64 fallback = Null)
{
	Value value = map.Get(key, fallback);
	return IsNull(value) ? fallback : (int64)value;
}

static String SessionString(ValueMap map, const char *key)
{
	return map.Get(key, String()).ToString();
}

int ValidateTexasHoldemSourceSession(const String& root)
{
	if (!DirectoryExists(root)) {
		Cerr() << "ERROR: sample directory not found: " << root << "\n";
		return 1;
	}
	String metadata_path = AppendFileName(root, "metadata.json");
	String gt_path = AppendFileName(root, "groundtruth.jsonl");
	if (!FileExists(metadata_path)) {
		Cerr() << "ERROR: missing metadata.json\n";
		return 1;
	}
	if (!FileExists(gt_path)) {
		Cerr() << "ERROR: missing groundtruth.jsonl\n";
		return 1;
	}

	Value metadata_value;
	try {
		metadata_value = ParseJSON(LoadFile(metadata_path));
	}
	catch (...) {
		Cerr() << "ERROR: metadata.json parse failed\n";
		return 1;
	}
	if (!metadata_value.Is<ValueMap>()) {
		Cerr() << "ERROR: metadata.json is not an object\n";
		return 1;
	}
	ValueMap metadata = metadata_value;
	String session_id = SessionString(metadata, "session_id");
	String provider = SessionString(metadata, "provider");
	int table_width = SessionInt(metadata, "table_width");
	int table_height = SessionInt(metadata, "table_height");
	int frame_count = SessionInt(metadata, "frame_count");
	if (session_id.IsEmpty() || provider.IsEmpty() || table_width <= 0 || table_height <= 0 || frame_count <= 0) {
		Cerr() << "ERROR: metadata.json has invalid required fields\n";
		return 1;
	}

	Vector<String> rows = Split(LoadFile(gt_path), '\n', false);
	int checked = 0;
	int previous_frame = -1;
	for (String row : rows) {
		row = TrimBoth(row);
		if (row.IsEmpty())
			continue;
		Value row_value;
		try {
			row_value = ParseJSON(row);
		}
		catch (...) {
			Cerr() << "ERROR: groundtruth.jsonl parse failed at row " << checked << "\n";
			return 1;
		}
		if (!row_value.Is<ValueMap>()) {
			Cerr() << "ERROR: groundtruth row is not an object at row " << checked << "\n";
			return 1;
		}
		ValueMap gt = row_value;
		const char *required[] = {
			"session_id", "frame_id", "render_step", "timestamp_ms", "provider",
			"table_width", "table_height", "seed", "game_id", "hand_id", "players"
		};
		for (const char *key : required) {
			if (!HasKey(gt, key)) {
				Cerr() << "ERROR: groundtruth missing key '" << key << "' at row " << checked << "\n";
				return 1;
			}
		}
		int frame_id = SessionInt(gt, "frame_id");
		if (frame_id != checked || frame_id <= previous_frame) {
			Cerr() << "ERROR: non-monotonic frame_id at row " << checked << "\n";
			return 1;
		}
		if (SessionString(gt, "session_id") != session_id || SessionString(gt, "provider") != provider ||
		    SessionInt(gt, "table_width") != table_width || SessionInt(gt, "table_height") != table_height) {
			Cerr() << "ERROR: groundtruth identity mismatch at frame " << frame_id << "\n";
			return 1;
		}
		String frame_path = AppendFileName(AppendFileName(root, "frames"), TexasHoldemFrameName(frame_id));
		if (!FileExists(frame_path)) {
			Cerr() << "ERROR: missing frame file: " << frame_path << "\n";
			return 1;
		}
		previous_frame = frame_id;
		checked++;
	}
	if (checked != frame_count) {
		Cerr() << "ERROR: frame_count mismatch metadata=" << frame_count << " groundtruth=" << checked << "\n";
		return 1;
	}
	Cout() << "M01 validation PASS\n";
	Cout() << "session_id=" << session_id << " provider=" << provider
	       << " size=(" << table_width << ", " << table_height << ") frames=" << checked << "\n";
	Cout().Flush();
	return 0;
}

int ReplayTexasHoldemSession(const String& root, const String& expected_provider,
                             Size expected_size, int expected_frame_ms)
{
	int rc = ValidateTexasHoldemSourceSession(root);
	if (rc != 0)
		return rc;

	Value metadata_value = ParseJSON(LoadFile(AppendFileName(root, "metadata.json")));
	ValueMap metadata = metadata_value;
	String session_id = SessionString(metadata, "session_id");
	String provider = SessionString(metadata, "provider");
	int table_width = SessionInt(metadata, "table_width");
	int table_height = SessionInt(metadata, "table_height");
	int frame_count = SessionInt(metadata, "frame_count");
	if (!expected_provider.IsEmpty() && provider != expected_provider) {
		Cerr() << "ERROR: provider mismatch expected=" << expected_provider << " actual=" << provider << "\n";
		return 1;
	}
	if (expected_size.cx > 0 && expected_size.cy > 0 &&
	    (table_width != expected_size.cx || table_height != expected_size.cy)) {
		Cerr() << "ERROR: table-size mismatch expected=(" << expected_size.cx << ", " << expected_size.cy
		       << ") actual=(" << table_width << ", " << table_height << ")\n";
		return 1;
	}

	Cout() << "M02 replay START\n";
	Cout() << "session_id=" << session_id << " provider=" << provider
	       << " size=(" << table_width << ", " << table_height << ") frames=" << frame_count << "\n";

	Vector<String> rows = Split(LoadFile(AppendFileName(root, "groundtruth.jsonl")), '\n', false);
	int replayed = 0;
	int64 previous_timestamp_ms = Null;
	for (String row : rows) {
		row = TrimBoth(row);
		if (row.IsEmpty())
			continue;
		Value row_value = ParseJSON(row);
		ValueMap gt = row_value;
		int frame_id = SessionInt(gt, "frame_id");
		int render_step = SessionInt(gt, "render_step");
		int64 timestamp_ms = SessionInt64(gt, "timestamp_ms");
		if (!IsNull(previous_timestamp_ms)) {
			if (timestamp_ms < previous_timestamp_ms) {
				Cerr() << "ERROR: timestamp moved backwards at frame " << frame_id
				       << " previous=" << previous_timestamp_ms << " actual=" << timestamp_ms << "\n";
				return 1;
			}
			if (expected_frame_ms > 0 && timestamp_ms - previous_timestamp_ms != expected_frame_ms) {
				Cerr() << "ERROR: timing drift at frame " << frame_id
				       << " expected_delta_ms=" << expected_frame_ms
				       << " actual_delta_ms=" << (timestamp_ms - previous_timestamp_ms) << "\n";
				return 1;
			}
		}
		int game_id = SessionInt(gt, "game_id");
		int hand_id = SessionInt(gt, "hand_id");
		int street = SessionInt(gt, "street", -1);
		int pot = SessionInt(gt, "pot", 0);
		int players = 0;
		Value players_value = gt.Get("players", ValueArray());
		if (players_value.Is<ValueArray>())
			players = ValueArray(players_value).GetCount();
		Cout() << "frame=" << frame_id
		       << " render_step=" << render_step
		       << " timestamp_ms=" << timestamp_ms
		       << " game_id=" << game_id
		       << " hand_id=" << hand_id
		       << " street=" << street
		       << " pot=" << pot
		       << " players=" << players
		       << " image=" << AppendFileName("frames", TexasHoldemFrameName(frame_id)) << "\n";
		previous_timestamp_ms = timestamp_ms;
		replayed++;
	}

	Cout() << "M02 replay PASS frames=" << replayed << "\n";
	Cout().Flush();
	return 0;
}

END_UPP_NAMESPACE
