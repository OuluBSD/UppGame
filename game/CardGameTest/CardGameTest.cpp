#include <Core/Core.h>
#include <CardGame/CardGame.h>

using namespace Upp;

void TestSuitFollowing() {
	CardGameState state;
	state.Deal();
	
	state.players[0].Clear();
	state.players[0].Add(Card("clubs", "2"));
	state.players[0].Add(Card("hearts", "3"));
	
	state.players[1].Clear();
	state.players[1].Add(Card("clubs", "5"));
	state.players[1].Add(Card("diamonds", "2"));

	state.turn = 0;
	state.phase = "PLAYING";
	state.hearts_broken = false;
	state.trick.Clear();
	state.leading_suit = "";

	PlayResult r1 = state.PlayCard(0, Card("clubs", "2"));
	ASSERT(r1.success);

	// Player 1 has clubs, so trying to play diamonds must fail
	PlayResult r2 = state.PlayCard(1, Card("diamonds", "2"));
	ASSERT(!r2.success);

	// Playing clubs must succeed
	PlayResult r3 = state.PlayCard(1, Card("clubs", "5"));
	ASSERT(r3.success);

	Cout() << "✓ TestSuitFollowing passed\n";
}

void TestHeartsBreaking() {
	CardGameState state;
	state.Deal();
	
	state.players[0].Clear();
	state.players[0].Add(Card("clubs", "3"));
	state.players[0].Add(Card("hearts", "3"));

	state.turn = 0;
	state.phase = "PLAYING";
	state.hearts_broken = false;
	state.trick.Clear();
	state.leading_suit = "";

	// Cannot lead hearts when not broken
	PlayResult r1 = state.PlayCard(0, Card("hearts", "3"));
	ASSERT(!r1.success);

	// Can lead other suit
	PlayResult r2 = state.PlayCard(0, Card("clubs", "3"));
	ASSERT(r2.success);

	Cout() << "✓ TestHeartsBreaking passed\n";
}

void TestTrickResolution() {
	CardGameState state;
	state.Deal();

	state.players[0].Clear(); state.players[0].Add(Card("clubs", "2"));
	state.players[1].Clear(); state.players[1].Add(Card("clubs", "5"));
	state.players[2].Clear(); state.players[2].Add(Card("clubs", "jack"));
	state.players[3].Clear(); state.players[3].Add(Card("clubs", "3"));

	state.turn = 0;
	state.phase = "PLAYING";
	state.trick.Clear();
	state.leading_suit = "";

	state.PlayCard(0, Card("clubs", "2"));
	state.PlayCard(1, Card("clubs", "5"));
	state.PlayCard(2, Card("clubs", "jack"));
	state.PlayCard(3, Card("clubs", "3"));

	ASSERT(state.trick_pending);
	int pts = 0;
	int winner = state.GetTrickResult(pts);
	ASSERT(winner == 2); // Jack of clubs should win
	ASSERT(pts == 0);

	state.ResolveTrick();
	ASSERT(state.turn == 2);

	Cout() << "✓ TestTrickResolution passed\n";
}

void TestPointsCalculation() {
	CardGameState state;
	state.Deal();

	state.players[0].Clear(); state.players[0].Add(Card("clubs", "2"));
	state.players[1].Clear(); state.players[1].Add(Card("hearts", "5"));
	state.players[2].Clear(); state.players[2].Add(Card("spades", "queen"));
	state.players[3].Clear(); state.players[3].Add(Card("clubs", "jack"));

	state.turn = 0;
	state.phase = "PLAYING";
	state.trick.Clear();
	state.leading_suit = "";

	state.PlayCard(0, Card("clubs", "2"));
	state.PlayCard(1, Card("hearts", "5")); // Discard heart (1 pt)
	state.PlayCard(2, Card("spades", "queen")); // Discard Q-Spades (13 pts)
	state.PlayCard(3, Card("clubs", "jack")); // Wins trick

	int pts = 0;
	int winner = state.GetTrickResult(pts);
	ASSERT(winner == 3); // Jack of clubs wins (highest lead suit)
	ASSERT(pts == 14); // 1 (heart) + 13 (Q-spades)

	Cout() << "✓ TestPointsCalculation passed\n";
}

void TestAI() {
	CardGameState state;
	state.Deal();

	// AI plays when leading
	Vector<Card> hand;
	hand.Add(Card("hearts", "5"));
	hand.Add(Card("diamonds", "10"));
	hand.Add(Card("spades", "queen"));

	Vector<TrickItem> trick;
	Card chosen = HeartsAI::ChooseCard(0, hand, trick, "", false);
	ASSERT(chosen.suit == "diamonds"); // Cannot lead hearts, and should not lead Q-Spades if possible

	Cout() << "✓ TestAI passed\n";
}

void TestDistributedPassiveReconstruction() {
	DistributedStateSnapshot before, after;
	before.phase = after.phase = "phase-1";
	before.total = after.total = 10;
	for(int i = 0; i < 3; i++) {
		DistributedParticipantState& a = before.participants.Add();
		DistributedParticipantState& b = after.participants.Add();
		a.active = b.active = true;
	}
	Vector<DistributedActionObservation> observed;
	DistributedActionObservation& action = observed.Add();
	action.participant = 0;
	action.kind = DISTRIBUTED_ACTION_PASSIVE;
	DistributedReconstructionResult result =
		DistributedEventReconstructor().Reconstruct(before, after, observed);
	ASSERT(result.complete);
	ASSERT(!result.ambiguous);
	ASSERT(result.actions.GetCount() == 3);
	ASSERT(result.actions[1].inferred);
	ASSERT(result.actions[2].inferred);
	Cout() << "TestDistributedPassiveReconstruction passed\n";
}

void TestDistributedIncreaseGap() {
	DistributedStateSnapshot before, after;
	before.phase = after.phase = "phase-1";
	before.total = 10;
	after.total = 20;
	for(int i = 0; i < 2; i++) {
		DistributedParticipantState& a = before.participants.Add();
		DistributedParticipantState& b = after.participants.Add();
		a.active = b.active = true;
	}
	Vector<DistributedActionObservation> observed;
	DistributedActionObservation& action = observed.Add();
	action.participant = 0;
	action.kind = DISTRIBUTED_ACTION_INCREASE;
	DistributedReconstructionResult result =
		DistributedEventReconstructor().Reconstruct(before, after, observed);
	ASSERT(!result.complete);
	ASSERT(result.ambiguous);
	Cout() << "TestDistributedIncreaseGap passed\n";
}

void TestDistributedUnobservedRemoval() {
	DistributedStateSnapshot before, after;
	before.phase = after.phase = "phase-1";
	before.total = after.total = 10;
	for(int i = 0; i < 2; i++) {
		DistributedParticipantState& a = before.participants.Add();
		DistributedParticipantState& b = after.participants.Add();
		a.active = true;
		b.active = i == 0;
	}
	DistributedReconstructionResult result =
		DistributedEventReconstructor().Reconstruct(before, after, {});
	ASSERT(!result.complete);
	ASSERT(result.ambiguous);
	Cout() << "TestDistributedUnobservedRemoval passed\n";
}

void TestDistributedSameTimeBatch() {
	DistributedStateSnapshot before, after;
	before.phase = after.phase = "phase-1";
	before.total = after.total = 10;
	for(int i = 0; i < 2; i++) {
		DistributedParticipantState& a = before.participants.Add();
		DistributedParticipantState& b = after.participants.Add();
		a.active = b.active = true;
	}
	Vector<DistributedActionObservation> observed;
	for(int i = 0; i < 2; i++) {
		DistributedActionObservation& action = observed.Add();
		action.participant = i;
		action.kind = DISTRIBUTED_ACTION_PASSIVE;
		action.timestamp = 100;
	}
	DistributedReconstructionResult result =
		DistributedEventReconstructor().Reconstruct(before, after, observed);
	ASSERT(result.complete);
	ASSERT(result.actions.GetCount() == 2);
	ASSERT(result.actions[0].observation.timestamp == result.actions[1].observation.timestamp);
	Cout() << "TestDistributedSameTimeBatch passed\n";
}

void TestDistributedDuplicateObservation() {
	DistributedStateSnapshot before, after;
	before.phase = after.phase = "phase-1";
	before.total = after.total = 10;
	DistributedParticipantState& a = before.participants.Add();
	DistributedParticipantState& b = after.participants.Add();
	a.active = b.active = true;
	Vector<DistributedActionObservation> observed;
	for(int i = 0; i < 2; i++) {
		DistributedActionObservation& action = observed.Add();
		action.participant = 0;
		action.kind = DISTRIBUTED_ACTION_PASSIVE;
		action.timestamp = 100 + i;
	}
	DistributedReconstructionResult result =
		DistributedEventReconstructor().Reconstruct(before, after, observed);
	ASSERT(result.complete);
	ASSERT(result.diagnostics.GetCount() == 1);
	Cout() << "TestDistributedDuplicateObservation passed\n";
}

void TestDistributedPhaseGap() {
	DistributedStateSnapshot before, after;
	before.phase = "phase-1";
	after.phase = "phase-2";
	before.total = after.total = 10;
	DistributedParticipantState& a = before.participants.Add();
	DistributedParticipantState& b = after.participants.Add();
	a.active = b.active = true;
	DistributedReconstructionResult result =
		DistributedEventReconstructor().Reconstruct(before, after, {});
	ASSERT(!result.complete);
	ASSERT(result.ambiguous);
	ASSERT(result.diagnostics.GetCount() == 1);
	Cout() << "TestDistributedPhaseGap passed\n";
}

void TestDistributedObservedRemoval() {
	DistributedStateSnapshot before, after;
	before.phase = after.phase = "phase-1";
	before.total = after.total = 10;
	DistributedParticipantState& a = before.participants.Add();
	DistributedParticipantState& b = after.participants.Add();
	a.active = true;
	b.active = true;
	Vector<DistributedActionObservation> observed;
	DistributedActionObservation& action = observed.Add();
	action.participant = 0;
	action.kind = DISTRIBUTED_ACTION_REMOVE;
	b.active = false;
	DistributedReconstructionResult result =
		DistributedEventReconstructor().Reconstruct(before, after, observed);
	ASSERT(result.complete);
	ASSERT(!result.ambiguous);
	Cout() << "TestDistributedObservedRemoval passed\n";
}

DistributedBufferedEvent MakeBufferedEvent(const char* identity, int64 sequence,
	                                         int participant)
{
	DistributedBufferedEvent event;
	event.identity = identity;
	event.sequence = sequence;
	event.timestamp = sequence * 10;
	event.observation.participant = participant;
	event.observation.kind = DISTRIBUTED_ACTION_PASSIVE;
	return event;
}

void TestDistributedUniquePhaseCompletion() {
	DistributedStateSnapshot before, after;
	before.phase = "phase-1";
	after.phase = "phase-2";
	before.total = after.total = 10;
	for(int i = 0; i < 2; i++) {
		DistributedParticipantState& a = before.participants.Add();
		DistributedParticipantState& b = after.participants.Add();
		a.active = b.active = true;
	}
	Vector<DistributedActionObservation> observed;
	DistributedActionObservation& action = observed.Add();
	action.participant = 0;
	action.kind = DISTRIBUTED_ACTION_PASSIVE;
	DistributedReconstructionResult result =
		DistributedEventReconstructor().Reconstruct(before, after, observed);
	ASSERT(result.complete);
	ASSERT(result.actions.GetCount() == 2);
	ASSERT(result.actions[1].inferred);
	Cout() << "TestDistributedUniquePhaseCompletion passed\n";
}

void TestDistributedEventBufferOrdering() {
	DistributedEventBuffer buffer;
	ASSERT(buffer.Push(MakeBufferedEvent("second", 2, 1)));
	ASSERT(buffer.Push(MakeBufferedEvent("first", 1, 0)));
	DistributedEventBufferResult result = buffer.Drain();
	ASSERT(result.batches.GetCount() == 2);
	ASSERT(result.batches[0].order_key == 1);
	ASSERT(result.batches[1].order_key == 2);
	Cout() << "TestDistributedEventBufferOrdering passed\n";
}

void TestDistributedEventBufferSameTimeBatch() {
	DistributedEventBuffer buffer;
	DistributedBufferedEvent second = MakeBufferedEvent("second", -1, 1);
	DistributedBufferedEvent first = MakeBufferedEvent("first", -1, 0);
	second.timestamp = first.timestamp = 100;
	ASSERT(buffer.Push(second));
	ASSERT(buffer.Push(first));
	DistributedEventBufferResult result = buffer.Drain();
	ASSERT(result.batches.GetCount() == 1);
	ASSERT(result.batches[0].events.GetCount() == 2);
	ASSERT(result.batches[0].events[0].identity == "first");
	Cout() << "TestDistributedEventBufferSameTimeBatch passed\n";
}

void TestDistributedEventBufferConflictsAndLateEvents() {
	DistributedEventBuffer buffer;
	DistributedBufferedEvent event = MakeBufferedEvent("stable", 5, 0);
	ASSERT(buffer.Push(event));
	buffer.Drain();
	ASSERT(!buffer.Push(event));
	event.observation.participant = 1;
	ASSERT(!buffer.Push(event));
	ASSERT(buffer.Push(MakeBufferedEvent("late", 4, 0)));
	DistributedEventBufferResult result = buffer.Drain();
	ASSERT(result.conflicts.GetCount() == 1);
	ASSERT(result.batches.GetCount() == 1);
	ASSERT(!result.batches[0].events[0].authoritative);
	ASSERT(result.batches[0].events[0].late);
	Cout() << "TestDistributedEventBufferConflictsAndLateEvents passed\n";
}

void TestDistributedEventBufferIndependentStreams() {
	DistributedEventBuffer buffer;
	DistributedBufferedEvent left = MakeBufferedEvent("same", 1, 0);
	DistributedBufferedEvent right = left;
	left.stream = "stream-a";
	right.stream = "stream-b";
	ASSERT(buffer.Push(left));
	ASSERT(buffer.Push(right));
	DistributedEventBufferResult result = buffer.Drain();
	ASSERT(result.conflicts.IsEmpty());
	ASSERT(result.batches.GetCount() == 1);
	ASSERT(result.batches[0].events.GetCount() == 2);
	Cout() << "TestDistributedEventBufferIndependentStreams passed\n";
}

void TestDistributedEventBufferOverflow() {
	DistributedEventBuffer buffer(1);
	ASSERT(buffer.Push(MakeBufferedEvent("one", 1, 0)));
	ASSERT(!buffer.Push(MakeBufferedEvent("two", 2, 1)));
	DistributedEventBufferResult result = buffer.Drain();
	ASSERT(result.batches.GetCount() == 1);
	ASSERT(result.overflow);
	ASSERT(result.diagnostics.GetCount() == 1);
	Cout() << "TestDistributedEventBufferOverflow passed\n";
}

void TestDistributedReconstructionService() {
	DistributedStateSnapshot before, after;
	before.phase = after.phase = "phase-1";
	before.total = after.total = 10;
	for(int i = 0; i < 2; i++) {
		DistributedParticipantState& a = before.participants.Add();
		DistributedParticipantState& b = after.participants.Add();
		a.active = b.active = true;
	}
	DistributedReconstructionService service;
	service.Begin("left", before);
	DistributedBufferedEvent event = MakeBufferedEvent("observed", 2, 0);
	ASSERT(service.Observe("left", event));
	DistributedServiceResult result = service.Complete("left", after);
	ASSERT(result.authoritative_applied);
	ASSERT(result.reconstruction.complete);
	DistributedStateSnapshot authoritative;
	ASSERT(service.GetAuthoritative("left", authoritative));
	ASSERT(authoritative.phase == after.phase);

	service.Begin("right", before);
	after.total = 20;
	result = service.Complete("right", after);
	ASSERT(!result.authoritative_applied);
	ASSERT(service.GetAuthoritative("right", authoritative));
	ASSERT(authoritative.total == before.total);
	Cout() << "TestDistributedReconstructionService passed\n";
}

static void FillScenarioSnapshot(DistributedStateSnapshot& state,
	                              const char* phase, double total, int count,
	                              int active_count);

void TestDistributedLiveAssertion() {
	DistributedStateSnapshot before, after;
	FillScenarioSnapshot(before, "start", 10, 2, 2);
	FillScenarioSnapshot(after, "start", 10, 2, 2);
	DistributedLiveAssertion assertion;
	DistributedReconstructionService service;
	service.SetLiveAssertion(&assertion);
	service.Begin("left", before);
	service.Observe("left", MakeBufferedEvent("first", 1, 0));
	DistributedServiceResult legal = service.Complete("left", after);
	ASSERT(legal.legality.status == DISTRIBUTED_LEGALITY_LEGAL);
	ASSERT(legal.authoritative_applied);

	FillScenarioSnapshot(after, "next", 10, 2, 2);
	service.Begin("left", before);
	DistributedServiceResult uncertain = service.Complete("left", after);
	ASSERT(uncertain.legality.status == DISTRIBUTED_LEGALITY_UNDETERMINED);
	ASSERT(!uncertain.authoritative_applied);
	DistributedStateSnapshot authoritative;
	ASSERT(service.GetAuthoritative("left", authoritative));
	ASSERT(authoritative.phase == before.phase);
	ASSERT(!service.ApplyOverride("left", "", 123));

	ASSERT(service.ApplyOverride("left", "operator confirmed missing passive actions", 124));
	ASSERT(assertion.GetOverrides().GetCount() == 1);
	ASSERT(service.GetAuthoritative("left", authoritative));
	ASSERT(authoritative.phase == after.phase);
	ASSERT(!service.ApplyOverride("left", "duplicate", 125));

	FillScenarioSnapshot(after, "next", 20, 3, 3);
	service.Begin("invalid", before);
	DistributedServiceResult invalid = service.Complete("invalid", after);
	ASSERT(invalid.legality.status == DISTRIBUTED_LEGALITY_ILLEGAL);
	ASSERT(!invalid.authoritative_applied);
	ASSERT(!service.ApplyOverride("invalid", "", 126));
	ASSERT(service.ApplyOverride("invalid", "test fixture accepted", 127));
	ASSERT(assertion.GetOverrides().GetCount() == 2);
	Cout() << "TestDistributedLiveAssertion passed: legal=1 undetermined=1 illegal=1 overrides=2\n";
}

void TestDistributedRecognizedFrameWriter() {
	Vector<DistributedRecognizedFrameLine> lines;
	DistributedRecognizedFrameLine right;
	right.stream = "R";
	right.timestamp_seconds = 5;
	right.text = "new hand, dealer=seat4";
	right.hand = 1;
	right.hand_start = true;
	right.legality.status = DISTRIBUTED_LEGALITY_UNDETERMINED;
	right.legality.issues.Add().code = "missing-event";
	right.legality.issues[0].message = "input gap";
	lines.Add(pick(right));
	DistributedRecognizedFrameLine left;
	left.stream = "L";
	left.timestamp_seconds = 2;
	left.text = "seat1 match";
	lines.Add(pick(left));
	DistributedRecognizedFrameLine inferred;
	inferred.stream = "R";
	inferred.text = "inferred participant2 passive reason=\"forced completion\"";
	inferred.comment = true;
	lines.Add(pick(inferred));
	String output = DistributedRecognizedFrameWriter().Generate(lines);
	ASSERT(output.Find("# R HAND 1 legal=undetermined checked=n") >= 0);
	ASSERT(output.Find("# R HAND 1 issue=missing-event: input gap") >= 0);
	ASSERT(output.Find("R 00:00:05: new hand, dealer=seat4") >= 0);
	ASSERT(output.Find("L 00:00:02: seat1 match") >= 0);
	ASSERT(output.Find("# R inferred participant2 passive reason=\"forced completion\"") >= 0);
	ASSERT(output.Find("R 00:00:05: new hand, dealer=seat4") <
	       output.Find("L 00:00:02: seat1 match"));
	Cout() << "TestDistributedRecognizedFrameWriter passed: separated streams and assertion comments OK\n";
}

void TestDistributedLegalityMatrix() {
	DistributedStateSnapshot before, after;
	FillScenarioSnapshot(before, "phase-1", 10, 2, 2);
	FillScenarioSnapshot(after, "phase-1", 10, 2, 2);
	DistributedActionObservation valid;
	valid.participant = 0;
	valid.kind = DISTRIBUTED_ACTION_MATCH;
	valid.amount_known = true;
	valid.amount = 1;
	DistributedReconstructionResult result =
		DistributedEventReconstructor().Reconstruct(before, after, {valid});
	ASSERT(result.complete);
	ASSERT(!result.invalid);

	DistributedActionObservation negative = valid;
	negative.amount = -1;
	result = DistributedEventReconstructor().Reconstruct(before, after, {negative});
	ASSERT(result.invalid);
	ASSERT(!result.complete);

	DistributedActionObservation removal = valid;
	removal.kind = DISTRIBUTED_ACTION_REMOVE;
	result = DistributedEventReconstructor().Reconstruct(before, after, {removal});
	ASSERT(result.invalid);
	ASSERT(!result.complete);

	FillScenarioSnapshot(after, "phase-1", 10, 2, 2);
	after.participants[0].committed = before.participants[0].committed;
	DistributedActionObservation increase = valid;
	increase.kind = DISTRIBUTED_ACTION_INCREASE;
	increase.amount = 2;
	result = DistributedEventReconstructor().Reconstruct(before, after, {increase});
	ASSERT(result.invalid);
	ASSERT(!result.complete);
	Cout() << "Distributed legality matrix passed: valid=1 negative=1 removal=1 increase=1\n";
}

void TestDistributedAssertionReplayModes() {
	DistributedStateSnapshot before, after;
	FillScenarioSnapshot(before, "start", 10, 2, 2);
	FillScenarioSnapshot(after, "start", 10, 2, 2);
	DistributedLiveAssertion assertion;
	DistributedReconstructionService service;
	service.SetLiveAssertion(&assertion);

	service.Begin("R", before);
	service.Begin("L", before);
	DistributedBufferedEvent first = MakeBufferedEvent("first", 1, 0);
	DistributedBufferedEvent second = MakeBufferedEvent("second", 2, 1);
	first.stream = "R";
	second.stream = "R";
	ASSERT(service.Observe("R", second));
	ASSERT(service.Observe("R", first));
	DistributedBufferedEvent left = first;
	left.stream = "L";
	left.identity = "left-first";
	ASSERT(service.Observe("L", left));
	DistributedServiceResult right_result = service.Complete("R", after, 7, 70);
	DistributedServiceResult left_result = service.Complete("L", after, 7, 70);
	ASSERT(right_result.legality.status == DISTRIBUTED_LEGALITY_LEGAL);
	ASSERT(left_result.legality.status == DISTRIBUTED_LEGALITY_LEGAL);
	ASSERT(right_result.authoritative_applied && left_result.authoritative_applied);

	service.Begin("delayed", before);
	DistributedStateSnapshot delayed_after;
	FillScenarioSnapshot(delayed_after, "next", 10, 2, 2);
	DistributedServiceResult delayed = service.Complete("delayed", delayed_after, 8, 80);
	ASSERT(delayed.legality.status == DISTRIBUTED_LEGALITY_UNDETERMINED);
	ASSERT(!delayed.authoritative_applied);

	service.Begin("conflict", before);
	DistributedBufferedEvent conflicting = MakeBufferedEvent("same", 1, 0);
	ASSERT(service.Observe("conflict", conflicting));
	conflicting.observation.participant = 1;
	ASSERT(!service.Observe("conflict", conflicting));
	DistributedServiceResult conflict = service.Complete("conflict", after, 9, 90);
	ASSERT(conflict.legality.status == DISTRIBUTED_LEGALITY_UNDETERMINED);
	ASSERT(!conflict.authoritative_applied);
	Cout() << "TestDistributedAssertionReplayModes passed: ordered=1 delayed=1 reordered=1 concurrent=1 conflict-held=1\n";
}

static bool ReadArgument(const Vector<String>& args, const String& name,
	                       String& value)
{
	for(int i = 0; i + 1 < args.GetCount(); i++)
		if(args[i] == name) {
			value = args[i + 1];
			return true;
		}
	return false;
}

static int RunDistributedDiagnostics(bool permuted, bool jsonl)
{
	DistributedStateSnapshot before, after;
	before.phase = after.phase = "phase-1";
	before.total = after.total = 10;
	for(int i = 0; i < 2; i++) {
		DistributedParticipantState& a = before.participants.Add();
		DistributedParticipantState& b = after.participants.Add();
		a.active = b.active = true;
	}
	DistributedReconstructionService service;
	service.Begin("diagnostic", before);
	DistributedBufferedEvent first = MakeBufferedEvent("first", 1, 0);
	DistributedBufferedEvent second = MakeBufferedEvent("second", 2, 1);
	if(permuted) {
		service.Observe("diagnostic", second);
		service.Observe("diagnostic", first);
	}
	else {
		service.Observe("diagnostic", first);
		service.Observe("diagnostic", second);
	}
	DistributedServiceResult result = service.Complete("diagnostic", after);
	int observed = 0;
	for(const DistributedEventBatch& batch : result.buffered.batches)
		observed += batch.events.GetCount();
	if(jsonl) {
		for(const DistributedEventBatch& batch : result.buffered.batches)
			for(const DistributedBufferedEvent& event : batch.events)
				Cout() << Format("{\"stream\":\"diagnostic\",\"identity\":\"%s\",\"kind\":\"observed\"}\n", event.identity);
		Cout() << Format("{\"stream\":\"diagnostic\",\"complete\":%d,\"authoritative\":%d}\n",
		                 result.reconstruction.complete, result.authoritative_applied);
	}
	else
		Cout() << Format("distributed stream=diagnostic batches=%d observed=%d inferred=0 ambiguous=%d authoritative=%d permuted=%d\n",
		                 result.buffered.batches.GetCount(), observed,
		                 result.reconstruction.ambiguous, result.authoritative_applied,
		                 permuted);
	return result.authoritative_applied ? 0 : 1;
}

static const char* AssertionStatusName(DistributedLegalityStatus status)
{
	switch(status) {
	case DISTRIBUTED_LEGALITY_LEGAL: return "legal";
	case DISTRIBUTED_LEGALITY_ILLEGAL: return "illegal";
	default: return "undetermined";
	}
}

static int RunDistributedAssertionDiagnostics(bool jsonl)
{
	DistributedStateSnapshot before, after;
	FillScenarioSnapshot(before, "start", 10, 2, 2);
	FillScenarioSnapshot(after, "start", 10, 2, 2);
	DistributedLiveAssertion assertion;
	DistributedReconstructionService service;
	service.SetLiveAssertion(&assertion);
	service.Begin("R", before);
	service.Observe("R", MakeBufferedEvent("observed", 1, 0));
	DistributedServiceResult legal = service.Complete("R", after, 1, 5);
	FillScenarioSnapshot(after, "next", 10, 2, 2);
	service.Begin("R", before);
	DistributedServiceResult uncertain = service.Complete("R", after, 2, 9);
	bool overridden = service.ApplyOverride("R", "operator confirmed replay gap", 10);
	if(jsonl) {
		Cout() << Format("{\"stream\":\"R\",\"round\":1,\"legal\":\"%s\",\"authoritative\":%d}\n",
			AssertionStatusName(legal.legality.status), legal.authoritative_applied);
		Cout() << Format("{\"stream\":\"R\",\"round\":2,\"legal\":\"%s\",\"authoritative\":%d,\"override\":%d}\n",
			AssertionStatusName(uncertain.legality.status), uncertain.authoritative_applied, overridden);
	}
	else {
		Cout() << Format("live_assertion stream=R round=1 legal=%s authoritative=%d\n",
			AssertionStatusName(legal.legality.status), legal.authoritative_applied);
		Cout() << Format("live_assertion stream=R round=2 legal=%s authoritative=%d override=%d\n",
			AssertionStatusName(uncertain.legality.status), uncertain.authoritative_applied, overridden);
	}
	return legal.legality.status == DISTRIBUTED_LEGALITY_LEGAL &&
	       uncertain.legality.status == DISTRIBUTED_LEGALITY_UNDETERMINED && overridden ? 0 : 1;
}

static void FillScenarioSnapshot(DistributedStateSnapshot& state,
	                              const char* phase, double total, int count,
	                              int active_count)
{
	state.phase = phase;
	state.total = total;
	state.participants.Clear();
	for(int i = 0; i < count; i++) {
		DistributedParticipantState& participant = state.participants.Add();
		participant.active = i < active_count;
		participant.committed = 1 + i;
	}
}

static DistributedActionObservation MakeScenarioObservation(
	int participant, DistributedActionKind kind, int64 timestamp = 100)
{
	DistributedActionObservation observation;
	observation.participant = participant;
	observation.kind = kind;
	observation.timestamp = timestamp;
	return observation;
}

static void AssertScenario(const char* name,
	                       const DistributedReconstructionResult& result,
	                       bool complete, bool ambiguous)
{
	ASSERT(result.complete == complete);
	ASSERT(result.ambiguous == ambiguous);
	int inferred = 0;
	for(const DistributedReconstructedAction& action : result.actions)
		inferred += action.inferred;
	Cout() << Format("Matrix %s passed: complete=%d ambiguous=%d inferred=%d\n",
	                 name, result.complete, result.ambiguous,
	                 inferred);
}

void TestDistributedScenarioMatrix() {
	DistributedStateSnapshot before, after;
	Vector<DistributedActionObservation> observed;

	FillScenarioSnapshot(before, "start", 10, 3, 3);
	FillScenarioSnapshot(after, "start", 10, 3, 3);
	for(int i = 0; i < 3; i++)
		observed.Add(MakeScenarioObservation(i, DISTRIBUTED_ACTION_PASSIVE, 100 + i));
	AssertScenario("complete_ordered", DistributedEventReconstructor().Reconstruct(before, after, observed), true, false);

	observed.Clear();
	observed.Add(MakeScenarioObservation(0, DISTRIBUTED_ACTION_PASSIVE));
	AssertScenario("one_passive_gap", DistributedEventReconstructor().Reconstruct(before, after, observed), true, false);

	observed.Clear();
	observed.Add(MakeScenarioObservation(0, DISTRIBUTED_ACTION_MATCH));
	AssertScenario("multiple_passive_gaps", DistributedEventReconstructor().Reconstruct(before, after, observed), true, false);

	FillScenarioSnapshot(after, "next", 10, 3, 3);
	observed.Clear();
	observed.Add(MakeScenarioObservation(0, DISTRIBUTED_ACTION_PASSIVE));
	observed.Add(MakeScenarioObservation(1, DISTRIBUTED_ACTION_PASSIVE));
	AssertScenario("unique_phase_completion", DistributedEventReconstructor().Reconstruct(before, after, observed), true, false);

	observed.Clear();
	AssertScenario("phase_gap_without_evidence", DistributedEventReconstructor().Reconstruct(before, after, observed), false, true);

	FillScenarioSnapshot(before, "start", 10, 4, 4);
	FillScenarioSnapshot(after, "next", 10, 4, 4);
	observed.Add(MakeScenarioObservation(0, DISTRIBUTED_ACTION_PASSIVE));
	AssertScenario("phase_multiple_completions", DistributedEventReconstructor().Reconstruct(before, after, observed), false, true);

	FillScenarioSnapshot(before, "start", 10, 3, 3);
	FillScenarioSnapshot(after, "start", 20, 3, 3);
	observed.Clear();
	observed.Add(MakeScenarioObservation(0, DISTRIBUTED_ACTION_PASSIVE));
	AssertScenario("unknown_amount_gap", DistributedEventReconstructor().Reconstruct(before, after, observed), false, true);

	observed.Clear();
	observed.Add(MakeScenarioObservation(0, DISTRIBUTED_ACTION_INCREASE));
	AssertScenario("unresolved_increase", DistributedEventReconstructor().Reconstruct(before, after, observed), false, true);

	FillScenarioSnapshot(before, "start", 10, 2, 2);
	FillScenarioSnapshot(after, "start", 20, 2, 2);
	observed.Clear();
	observed.Add(MakeScenarioObservation(0, DISTRIBUTED_ACTION_INCREASE));
	observed.Add(MakeScenarioObservation(1, DISTRIBUTED_ACTION_PASSIVE));
	AssertScenario("observed_increase_complete", DistributedEventReconstructor().Reconstruct(before, after, observed), true, false);

	FillScenarioSnapshot(before, "start", 10, 3, 3);
	FillScenarioSnapshot(after, "start", 10, 3, 2);
	observed.Clear();
	AssertScenario("unobserved_removal", DistributedEventReconstructor().Reconstruct(before, after, observed), false, true);

	observed.Clear();
	observed.Add(MakeScenarioObservation(2, DISTRIBUTED_ACTION_REMOVE));
	AssertScenario("observed_removal_with_passive_gap", DistributedEventReconstructor().Reconstruct(before, after, observed), true, false);

	FillScenarioSnapshot(before, "terminal", 10, 2, 2);
	FillScenarioSnapshot(after, "terminal", 10, 2, 0);
	observed.Clear();
	observed.Add(MakeScenarioObservation(0, DISTRIBUTED_ACTION_REMOVE));
	observed.Add(MakeScenarioObservation(1, DISTRIBUTED_ACTION_REMOVE));
	AssertScenario("terminal_all_removals_observed", DistributedEventReconstructor().Reconstruct(before, after, observed), true, false);

	FillScenarioSnapshot(before, "terminal", 10, 2, 0);
	FillScenarioSnapshot(after, "terminal", 10, 2, 0);
	observed.Clear();
	AssertScenario("terminal_no_active_participants", DistributedEventReconstructor().Reconstruct(before, after, observed), true, false);

	FillScenarioSnapshot(before, "start", 10, 3, 3);
	FillScenarioSnapshot(after, "next", 10, 3, 2);
	observed.Clear();
	observed.Add(MakeScenarioObservation(0, DISTRIBUTED_ACTION_REMOVE));
	AssertScenario("phase_and_unobserved_removal", DistributedEventReconstructor().Reconstruct(before, after, observed), false, true);

	FillScenarioSnapshot(before, "start", 10, 2, 1);
	FillScenarioSnapshot(after, "start", 10, 2, 2);
	observed.Clear();
	AssertScenario("unobserved_activation", DistributedEventReconstructor().Reconstruct(before, after, observed), false, true);

	FillScenarioSnapshot(before, "start", 10, 2, 2);
	FillScenarioSnapshot(after, "start", 20, 2, 2);
	observed.Clear();
	observed.Add(MakeScenarioObservation(0, DISTRIBUTED_ACTION_INCREASE, 100));
	AssertScenario("same_time_increase_with_gap", DistributedEventReconstructor().Reconstruct(before, after, observed), false, true);

	FillScenarioSnapshot(before, "start", 10, 1, 1);
	FillScenarioSnapshot(after, "start", 10, 1, 1);
	observed.Clear();
	observed.Add(MakeScenarioObservation(5, DISTRIBUTED_ACTION_PASSIVE));
	DistributedReconstructionResult invalid =
		DistributedEventReconstructor().Reconstruct(before, after, observed);
	ASSERT(invalid.invalid);
	ASSERT(!invalid.complete);
	Cout() << "Matrix invalid_participant passed\n";

	FillScenarioSnapshot(before, "start", 10, 1, 1);
	FillScenarioSnapshot(after, "start", 10, 2, 2);
	invalid = DistributedEventReconstructor().Reconstruct(before, after, {});
	ASSERT(invalid.invalid);
	ASSERT(!invalid.complete);
	Cout() << "Matrix participant_count_mismatch passed\n";
	Cout() << "Distributed scenario matrix passed: 18 scenarios\n";
}

CONSOLE_APP_MAIN {
	const Vector<String>& args = CommandLine();
	bool diagnostic = false, assertion_diagnostic = false, permuted = false, jsonl = false;
	for(const String& arg : args) {
		diagnostic |= arg == "--distributed-diagnostics";
		assertion_diagnostic |= arg == "--distributed-assertion-diagnostics";
		permuted |= arg == "--distributed-permuted";
		jsonl |= arg == "--distributed-jsonl";
	}
	String fixture;
	if(ReadArgument(args, "--distributed-fixture", fixture) &&
		fixture != "basic") {
		Cerr() << "ERROR: unknown distributed fixture: " << fixture << "\n";
		SetExitCode(2);
		return;
	}
	String timeout_text;
	int timeout_ms = 0;
	if(ReadArgument(args, "--distributed-timeout-ms", timeout_text)) {
		if(timeout_text.IsEmpty() || !IsDigit(timeout_text[0])) {
			Cerr() << "ERROR: invalid --distributed-timeout-ms\n";
			SetExitCode(2);
			return;
		}
		timeout_ms = ScanInt(timeout_text);
		if(timeout_ms < 0 || timeout_ms > 60000) {
			Cerr() << "ERROR: --distributed-timeout-ms must be 0..60000\n";
			SetExitCode(2);
			return;
		}
	}
	if(diagnostic) {
		int64 start = msecs();
		int rc = RunDistributedDiagnostics(permuted, jsonl);
		if(timeout_ms > 0 && msecs() - start > timeout_ms) {
			Cerr() << "ERROR: distributed diagnostics timeout\n";
			SetExitCode(3);
			return;
		}
		SetExitCode(rc);
		return;
	}
	if(assertion_diagnostic) {
		SetExitCode(RunDistributedAssertionDiagnostics(jsonl));
		return;
	}
	Cout() << "Running C++ CardGame tests...\n";
	TestSuitFollowing();
	TestHeartsBreaking();
	TestTrickResolution();
	TestPointsCalculation();
	TestAI();
	TestDistributedPassiveReconstruction();
	TestDistributedIncreaseGap();
	TestDistributedUnobservedRemoval();
	TestDistributedSameTimeBatch();
	TestDistributedDuplicateObservation();
	TestDistributedPhaseGap();
	TestDistributedObservedRemoval();
	TestDistributedUniquePhaseCompletion();
	TestDistributedEventBufferOrdering();
	TestDistributedEventBufferSameTimeBatch();
	TestDistributedEventBufferConflictsAndLateEvents();
	TestDistributedEventBufferIndependentStreams();
	TestDistributedEventBufferOverflow();
	TestDistributedReconstructionService();
	TestDistributedLiveAssertion();
	TestDistributedRecognizedFrameWriter();
	TestDistributedAssertionReplayModes();
	TestDistributedScenarioMatrix();
	TestDistributedLegalityMatrix();
	Cout() << "All CardGame tests passed! 🐍💎✨🚀❤️🃏\n";
}
