#ifndef _game_CardGame_DistributedReconstruction_h_
#define _game_CardGame_DistributedReconstruction_h_

enum DistributedActionKind {
	DISTRIBUTED_ACTION_PASSIVE,
	DISTRIBUTED_ACTION_MATCH,
	DISTRIBUTED_ACTION_INCREASE,
	DISTRIBUTED_ACTION_REMOVE,
};

struct DistributedParticipantState : Moveable<DistributedParticipantState> {
	bool active = false;
	double committed = -1;
};

struct DistributedStateSnapshot : Moveable<DistributedStateSnapshot> {
	String phase;
	Vector<DistributedParticipantState> participants;
	double total = -1;
};

struct DistributedActionObservation : Moveable<DistributedActionObservation> {
	int participant = -1;
	DistributedActionKind kind = DISTRIBUTED_ACTION_PASSIVE;
	int64 timestamp = -1;
	bool amount_known = false;
	double amount = -1;
	String source;
};

struct DistributedReconstructedAction : Moveable<DistributedReconstructedAction> {
	DistributedActionObservation observation;
	bool inferred = false;
	String reason;
};

struct DistributedReconstructionResult : Moveable<DistributedReconstructionResult> {
	Vector<DistributedReconstructedAction> actions;
	Vector<String> diagnostics;
	bool complete = false;
	bool ambiguous = false;
	bool invalid = false;
};

struct DistributedBufferedEvent : Moveable<DistributedBufferedEvent> {
	String stream;
	String identity;
	int64 sequence = -1;
	int64 timestamp = -1;
	DistributedActionObservation observation;
	bool authoritative = true;
	bool late = false;
};

struct DistributedEventBatch : Moveable<DistributedEventBatch> {
	int64 order_key = -1;
	bool sequence_order = false;
	Vector<DistributedBufferedEvent> events;
};

struct DistributedEventBufferResult : Moveable<DistributedEventBufferResult> {
	Vector<DistributedEventBatch> batches;
	Vector<DistributedBufferedEvent> conflicts;
	Vector<String> diagnostics;
	bool overflow = false;
};

enum DistributedLegalityStatus {
	DISTRIBUTED_LEGALITY_LEGAL,
	DISTRIBUTED_LEGALITY_ILLEGAL,
	DISTRIBUTED_LEGALITY_UNDETERMINED,
};

struct DistributedLegalityIssue : Moveable<DistributedLegalityIssue> {
	String code;
	String message;
};

struct DistributedLegalityReport : Moveable<DistributedLegalityReport> {
	String stream;
	int64 round = -1;
	int64 timestamp = -1;
	DistributedLegalityStatus status = DISTRIBUTED_LEGALITY_UNDETERMINED;
	Vector<DistributedLegalityIssue> issues;
	bool override_applied = false;
	String override_reason;
};

struct DistributedLegalityOverride : Moveable<DistributedLegalityOverride> {
	String stream;
	int64 round = -1;
	int64 timestamp = -1;
	String reason;
	DistributedLegalityStatus prior_status = DISTRIBUTED_LEGALITY_UNDETERMINED;
};

struct DistributedRecognizedFrameLine : Moveable<DistributedRecognizedFrameLine> {
	String stream;
	int64 timestamp_seconds = 0;
	// Task 0362: millisecond-precision printed timestamp, -1 sentinel means
	// "not supplied" -- WriteStream falls back to the legacy whole-second
	// Timestamp(timestamp_seconds) formatting in that case, so existing
	// producers/fixtures that never set this (CardGameTest.cpp,
	// VisualStateModelTest/main.cpp) keep their exact prior output.
	int64 timestamp_ms = -1;
	String text;
	int hand = -1;
	bool hand_start = false;
	bool checked = false;
	bool comment = false;
	DistributedLegalityReport legality;
};

class DistributedRecognizedFrameWriter {
	static String StatusName(DistributedLegalityStatus status);
	static String Timestamp(int64 seconds);
	// Task 0362: the trusted ground-truth hand.txt convention -- "00:00:"
	// followed by the FULL unbounded elapsed-seconds count (no carry into
	// minutes) and a zero-padded 3-digit millisecond fraction, e.g.
	// "00:00:106.141" for 106.141s elapsed. Confirmed against
	// VideoSidecarGroundTruthParser::ParseTimestampMs, which already parses
	// this form (and the legacy rolled-over HH:MM:SS form) into the same
	// total millisecond integer either way.
	static String TimestampMs(int64 ms);
	static void WriteStream(String& out, const Vector<DistributedRecognizedFrameLine>& lines,
	                        const String& stream);

public:
	String Generate(const Vector<DistributedRecognizedFrameLine>& lines) const;
};

struct DistributedServiceResult : Moveable<DistributedServiceResult> {
	String stream;
	DistributedEventBufferResult buffered;
	DistributedReconstructionResult reconstruction;
	DistributedLegalityReport legality;
	bool authoritative_applied = false;
};

class DistributedLiveAssertion {
	Vector<DistributedLegalityOverride> overrides;

	static void AddIssue(DistributedLegalityReport& report,
	                    const char* code, const String& message);

public:
	DistributedLegalityReport Validate(
		const String& stream, int64 round, int64 timestamp,
		const DistributedReconstructionResult& reconstruction,
		const DistributedEventBufferResult& buffered) const;
	bool Accepts(const DistributedLegalityReport& report) const;
	bool ApplyOverride(DistributedLegalityReport& report,
	                  const String& reason, int64 timestamp);
	const Vector<DistributedLegalityOverride>& GetOverrides() const { return overrides; }
};

class DistributedEventBuffer {
	int max_events = 256;
	Vector<DistributedBufferedEvent> pending;
	Vector<DistributedBufferedEvent> conflicts;
	Vector<DistributedBufferedEvent> seen;
	Vector<String> diagnostics;
	Index<String> identities;
	bool overflow = false;
	int64 last_order_key = -1;
	bool last_sequence_order = false;
	bool has_last_order = false;

	static bool Equivalent(const DistributedBufferedEvent& a,
	                       const DistributedBufferedEvent& b);
	static String IdentityKey(const DistributedBufferedEvent& event);
	static int Compare(const DistributedBufferedEvent& a,
	                   const DistributedBufferedEvent& b);

public:
	DistributedEventBuffer() = default;
	DistributedEventBuffer(int max_events);

	bool Push(const DistributedBufferedEvent& event);
	DistributedEventBufferResult Drain();
	void Clear();
	int GetPendingCount() const { return pending.GetCount(); }
};

class DistributedReconstructionService {
	struct StreamState : Moveable<StreamState> {
		String id;
		DistributedStateSnapshot before;
		DistributedStateSnapshot authoritative;
		DistributedEventBuffer buffer;
		bool has_authoritative = false;
		DistributedStateSnapshot pending;
		DistributedLegalityReport pending_legality;
		bool has_pending = false;
	};

	Vector<StreamState> streams;
	DistributedLiveAssertion* assertion = nullptr;
	StreamState* Find(const String& id);
	const StreamState* Find(const String& id) const;

public:
	void SetLiveAssertion(DistributedLiveAssertion* assertion) { this->assertion = assertion; }
	void Begin(const String& stream, const DistributedStateSnapshot& before);
	bool Observe(const String& stream, const DistributedBufferedEvent& event);
	DistributedServiceResult Complete(const String& stream,
	                                  const DistributedStateSnapshot& after,
	                                  int64 round = -1, int64 timestamp = -1);
	bool ApplyOverride(const String& stream, const String& reason, int64 timestamp);
	bool GetAuthoritative(const String& stream, DistributedStateSnapshot& out) const;
};

class DistributedEventReconstructor {
public:
	DistributedReconstructionResult Reconstruct(
		const DistributedStateSnapshot& before,
		const DistributedStateSnapshot& after,
		const Vector<DistributedActionObservation>& observed) const;
};

#endif
