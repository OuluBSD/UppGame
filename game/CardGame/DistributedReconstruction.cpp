#include "CardGame.h"

NAMESPACE_UPP

static bool SameAmount(double a, double b)
{
	return a >= 0 && b >= 0 && fabs(a - b) < 0.001;
}

static void AddDiagnostic(DistributedReconstructionResult& result, const String& text)
{
	result.diagnostics.Add(text);
}

static void CopySnapshot(DistributedStateSnapshot& dst,
	                      const DistributedStateSnapshot& src)
{
	dst.phase = src.phase;
	dst.total = src.total;
	dst.participants.Clear();
	for(const DistributedParticipantState& participant : src.participants) {
		DistributedParticipantState& copy = dst.participants.Add();
		copy.active = participant.active;
		copy.committed = participant.committed;
	}
}

static void CopyLegalityReport(DistributedLegalityReport& dst,
	                            const DistributedLegalityReport& src)
{
	dst.stream = src.stream;
	dst.round = src.round;
	dst.timestamp = src.timestamp;
	dst.status = src.status;
	dst.override_applied = src.override_applied;
	dst.override_reason = src.override_reason;
	dst.issues.Clear();
	for(const DistributedLegalityIssue& issue : src.issues) {
		DistributedLegalityIssue& copy = dst.issues.Add();
		copy.code = issue.code;
		copy.message = issue.message;
	}
}

static const char* LegalityStatusName(DistributedLegalityStatus status)
{
	switch(status) {
	case DISTRIBUTED_LEGALITY_LEGAL: return "legal";
	case DISTRIBUTED_LEGALITY_ILLEGAL: return "illegal";
	default: return "undetermined";
	}
}

void DistributedLiveAssertion::AddIssue(DistributedLegalityReport& report,
	                                       const char* code, const String& message)
{
	DistributedLegalityIssue& issue = report.issues.Add();
	issue.code = code;
	issue.message = message;
}

DistributedLegalityReport DistributedLiveAssertion::Validate(
	const String& stream, int64 round, int64 timestamp,
	const DistributedReconstructionResult& reconstruction,
	const DistributedEventBufferResult& buffered) const
{
	DistributedLegalityReport report;
	report.stream = stream;
	report.round = round;
	report.timestamp = timestamp;
	if(reconstruction.invalid) {
		report.status = DISTRIBUTED_LEGALITY_ILLEGAL;
		AddIssue(report, "invalid-reconstruction", "reconstruction violates a state invariant");
	}
	else if(reconstruction.ambiguous || buffered.overflow || !buffered.conflicts.IsEmpty()) {
		report.status = DISTRIBUTED_LEGALITY_UNDETERMINED;
		if(reconstruction.ambiguous)
			AddIssue(report, "ambiguous-reconstruction", "one or more actions cannot be uniquely reconstructed");
		if(buffered.overflow)
			AddIssue(report, "buffer-overflow", "event buffer overflowed before assertion");
		if(!buffered.conflicts.IsEmpty())
			AddIssue(report, "conflicting-events", "the same event identity had conflicting payloads");
	}
	else {
		bool late = false;
		for(const DistributedEventBatch& batch : buffered.batches)
			for(const DistributedBufferedEvent& event : batch.events)
				late |= event.late;
		if(late) {
			report.status = DISTRIBUTED_LEGALITY_UNDETERMINED;
			AddIssue(report, "late-event", "late input cannot be committed authoritatively");
		}
		else if(reconstruction.complete)
			report.status = DISTRIBUTED_LEGALITY_LEGAL;
		else {
			report.status = DISTRIBUTED_LEGALITY_UNDETERMINED;
			AddIssue(report, "incomplete-reconstruction", "reconstruction did not reach a complete result");
		}
	}
	String summary = Format("live assertion stream=%s round=%lld status=%s issues=%d",
		stream, round, LegalityStatusName(report.status), report.issues.GetCount());
	LOG(summary);
	return report;
}

bool DistributedLiveAssertion::Accepts(const DistributedLegalityReport& report) const
{
	return report.status == DISTRIBUTED_LEGALITY_LEGAL || report.override_applied;
}

bool DistributedLiveAssertion::ApplyOverride(DistributedLegalityReport& report,
	                                           const String& reason, int64 timestamp)
{
	if(reason.IsEmpty() || report.status == DISTRIBUTED_LEGALITY_LEGAL)
		return false;
	DistributedLegalityOverride& audit = overrides.Add();
	audit.stream = report.stream;
	audit.round = report.round;
	audit.timestamp = timestamp;
	audit.reason = reason;
	audit.prior_status = report.status;
	report.override_applied = true;
	report.override_reason = reason;
	LOG(Format("live assertion override stream=%s round=%lld prior=%s reason=%s",
		report.stream, report.round, LegalityStatusName(audit.prior_status), reason));
	return true;
}

String DistributedRecognizedFrameWriter::StatusName(DistributedLegalityStatus status)
{
	return LegalityStatusName(status);
}

String DistributedRecognizedFrameWriter::Timestamp(int64 seconds)
{
	if(seconds < 0) seconds = 0;
	int64 hours = seconds / 3600;
	int minutes = (int)((seconds / 60) % 60);
	int secs = (int)(seconds % 60);
	return Format("%02d:%02d:%02d", (int)hours, minutes, secs);
}

String DistributedRecognizedFrameWriter::TimestampMs(int64 ms)
{
	if(ms < 0) ms = 0;
	int64 total_sec = ms / 1000;
	int   frac      = (int)(ms % 1000);
	// Deliberately NOT rolled into minutes/hours -- hours/minutes stay
	// pinned at "00" and the seconds field is the full unbounded elapsed
	// count, matching the repo owner's own hand-authored hand.txt
	// convention exactly (see the header comment for the confirming trace).
	return Format("00:00:%02lld.%03d", total_sec, frac);
}

void DistributedRecognizedFrameWriter::WriteStream(
	String& out, const Vector<DistributedRecognizedFrameLine>& lines, const String& stream)
{
	bool wrote = false;
	for(const DistributedRecognizedFrameLine& line : lines) {
		if(line.stream != stream)
			continue;
		if(line.hand_start || line.hand >= 0) {
			out << Format("# %s HAND %d legal=%s checked=%c", line.stream,
				line.hand, StatusName(line.legality.status), line.checked ? 'y' : 'n');
			if(line.legality.override_applied)
				out << Format(" override=y reason=%s", line.legality.override_reason);
			out << "\n";
			for(const DistributedLegalityIssue& issue : line.legality.issues)
				out << Format("# %s HAND %d issue=%s: %s\n", line.stream,
					line.hand, issue.code, issue.message);
		}
		if(line.comment)
			out << Format("# %s %s\n", line.stream, line.text);
		else if(line.timestamp_ms >= 0)
			out << Format("%s %s: %s\n", line.stream, TimestampMs(line.timestamp_ms), line.text);
		else
			out << Format("%s %s: %s\n", line.stream, Timestamp(line.timestamp_seconds), line.text);
		wrote = true;
	}
	if(wrote)
		out << "\n";
}

String DistributedRecognizedFrameWriter::Generate(
	const Vector<DistributedRecognizedFrameLine>& lines) const
{
	String out;
	out << "# recognized-frames generated from recognized events\n";
	out << "# legal=legal|illegal|undetermined; checked=n|y\n";
	WriteStream(out, lines, "R");
	WriteStream(out, lines, "L");
	return out;
}

DistributedEventBuffer::DistributedEventBuffer(int max_events)
{
	ASSERT(max_events > 0);
	this->max_events = max_events > 0 ? max_events : 1;
}

bool DistributedEventBuffer::Equivalent(const DistributedBufferedEvent& a,
	                                      const DistributedBufferedEvent& b)
{
	const DistributedActionObservation& x = a.observation;
	const DistributedActionObservation& y = b.observation;
	return a.stream == b.stream && a.identity == b.identity &&
		a.sequence == b.sequence && a.timestamp == b.timestamp &&
		x.participant == y.participant && x.kind == y.kind &&
		x.amount_known == y.amount_known &&
		(!x.amount_known || fabs(x.amount - y.amount) < 0.001) &&
		x.source == y.source;
}

String DistributedEventBuffer::IdentityKey(const DistributedBufferedEvent& event)
{
	return event.stream + "::" + event.identity;
}

int DistributedEventBuffer::Compare(const DistributedBufferedEvent& a,
	                                  const DistributedBufferedEvent& b)
{
	bool a_sequence = a.sequence >= 0;
	bool b_sequence = b.sequence >= 0;
	if(a_sequence != b_sequence)
		return a_sequence ? -1 : 1;
	int64 ak = a_sequence ? a.sequence : a.timestamp;
	int64 bk = b_sequence ? b.sequence : b.timestamp;
	if(ak != bk)
		return ak < bk ? -1 : 1;
	if(a.identity != b.identity)
		return a.identity < b.identity ? -1 : 1;
	return 0;
}

bool DistributedEventBuffer::Push(const DistributedBufferedEvent& input)
{
	DistributedBufferedEvent event = input;
	if(!event.identity.IsEmpty()) {
		String key = IdentityKey(event);
		int id = identities.Find(key);
		if(id >= 0) {
			for(const DistributedBufferedEvent& old : seen)
				if(IdentityKey(old) == key) {
					if(!Equivalent(old, event)) {
						conflicts.Add(event);
						diagnostics.Add(Format("conflicting event identity: %s", event.identity));
					}
					return false;
				}
		}
		else
			identities.Add(key);
	}
	if(has_last_order) {
		bool sequence_order = event.sequence >= 0;
		int64 key = sequence_order ? event.sequence : event.timestamp;
		if(sequence_order == last_sequence_order && key < last_order_key) {
			event.late = true;
			event.authoritative = false;
			diagnostics.Add("late event retained as non-authoritative");
		}
	}
	if(pending.GetCount() >= max_events) {
		overflow = true;
		diagnostics.Add("event buffer capacity exceeded");
		return false;
	}
	pending.Add(event);
	seen.Add(event);
	return true;
}

DistributedEventBufferResult DistributedEventBuffer::Drain()
{
	DistributedEventBufferResult result;
	result.overflow = overflow;
	result.conflicts = pick(conflicts);
	result.diagnostics = pick(diagnostics);
	Sort(pending, [](const DistributedBufferedEvent& a, const DistributedBufferedEvent& b) {
		return DistributedEventBuffer::Compare(a, b) < 0;
	});
	for(int i = 0; i < pending.GetCount();) {
		DistributedBufferedEvent first = pending[i];
		DistributedEventBatch& batch = result.batches.Add();
		batch.order_key = first.sequence >= 0 ? first.sequence : first.timestamp;
		batch.sequence_order = first.sequence >= 0;
		while(i < pending.GetCount()) {
			const DistributedBufferedEvent& event = pending[i];
			bool same_order = (event.sequence >= 0) == batch.sequence_order;
			int64 key = event.sequence >= 0 ? event.sequence : event.timestamp;
			if(!same_order || key != batch.order_key)
				break;
			batch.events.Add(event);
			i++;
		}
		last_order_key = batch.order_key;
		last_sequence_order = batch.sequence_order;
		has_last_order = true;
	}
	pending.Clear();
	return result;
}

void DistributedEventBuffer::Clear()
{
	pending.Clear();
	conflicts.Clear();
	seen.Clear();
	diagnostics.Clear();
	identities.Clear();
	overflow = false;
	last_order_key = -1;
	has_last_order = false;
}

DistributedReconstructionService::StreamState*
DistributedReconstructionService::Find(const String& id)
{
	for(StreamState& stream : streams)
		if(stream.id == id)
			return &stream;
	return nullptr;
}

const DistributedReconstructionService::StreamState*
DistributedReconstructionService::Find(const String& id) const
{
	for(const StreamState& stream : streams)
		if(stream.id == id)
			return &stream;
	return nullptr;
}

void DistributedReconstructionService::Begin(const String& stream,
	                                           const DistributedStateSnapshot& before)
{
	StreamState* state = Find(stream);
	if(!state) {
		state = &streams.Add();
		state->id = stream;
	}
	CopySnapshot(state->before, before);
	CopySnapshot(state->authoritative, before);
	state->has_authoritative = true;
	state->has_pending = false;
	state->buffer.Clear();
}

bool DistributedReconstructionService::Observe(const String& stream,
	                                             const DistributedBufferedEvent& event)
{
	StreamState* state = Find(stream);
	if(!state)
		return false;
	DistributedBufferedEvent copy = event;
	copy.stream = stream;
	return state->buffer.Push(copy);
}

DistributedServiceResult DistributedReconstructionService::Complete(
	const String& stream, const DistributedStateSnapshot& after,
	int64 round, int64 timestamp)
{
	DistributedServiceResult service;
	service.stream = stream;
	StreamState* state = Find(stream);
	if(!state) {
		service.reconstruction.invalid = true;
		service.reconstruction.diagnostics.Add("stream was not started");
		return service;
	}
	service.buffered = state->buffer.Drain();
	if(!service.buffered.conflicts.IsEmpty())
		service.reconstruction.ambiguous = true;
	Vector<DistributedActionObservation> observations;
	for(const DistributedEventBatch& batch : service.buffered.batches)
		for(const DistributedBufferedEvent& event : batch.events)
			observations.Add(event.observation);
	service.reconstruction = DistributedEventReconstructor().Reconstruct(
		state->before, after, observations);
	for(const String& diagnostic : service.buffered.diagnostics)
		service.reconstruction.diagnostics.Add(diagnostic);
	bool has_late = false;
	for(const DistributedEventBatch& batch : service.buffered.batches)
		for(const DistributedBufferedEvent& event : batch.events)
			has_late |= event.late;
	if(assertion) {
		DistributedLegalityReport report = assertion->Validate(stream,
			round, timestamp,
			service.reconstruction, service.buffered);
		service.legality = pick(report);
		service.authoritative_applied = assertion->Accepts(service.legality);
	}
	else
		service.authoritative_applied = service.reconstruction.complete &&
		!service.reconstruction.ambiguous && !service.reconstruction.invalid &&
		!service.buffered.overflow && service.buffered.conflicts.IsEmpty() && !has_late;
	if(service.authoritative_applied) {
		CopySnapshot(state->authoritative, after);
		CopySnapshot(state->before, after);
	}
	else if(assertion) {
		CopySnapshot(state->pending, after);
		CopyLegalityReport(state->pending_legality, service.legality);
		state->has_pending = true;
	}
	return service;
}

bool DistributedReconstructionService::ApplyOverride(const String& stream,
	                                                   const String& reason,
	                                                   int64 timestamp)
{
	if(!assertion)
		return false;
	StreamState* state = Find(stream);
	if(!state || !state->has_pending)
		return false;
	if(!assertion->ApplyOverride(state->pending_legality, reason, timestamp))
		return false;
	CopySnapshot(state->authoritative, state->pending);
	CopySnapshot(state->before, state->pending);
	state->has_pending = false;
	return true;
}

bool DistributedReconstructionService::GetAuthoritative(
	const String& stream, DistributedStateSnapshot& out) const
{
	const StreamState* state = Find(stream);
	if(!state || !state->has_authoritative)
		return false;
	CopySnapshot(out, state->authoritative);
	return true;
}

DistributedReconstructionResult DistributedEventReconstructor::Reconstruct(
	const DistributedStateSnapshot& before,
	const DistributedStateSnapshot& after,
	const Vector<DistributedActionObservation>& observed) const
{
	DistributedReconstructionResult result;
	if(before.participants.GetCount() != after.participants.GetCount()) {
		result.invalid = true;
		AddDiagnostic(result, "participant count changed");
		return result;
	}

	const int count = before.participants.GetCount();
	Vector<bool> covered;
	covered.SetCount(count, false);
	bool increase_seen = false;
	bool activation_unobserved = false;
	for(const DistributedActionObservation& action : observed) {
		if(action.participant < 0 || action.participant >= count) {
			result.invalid = true;
			AddDiagnostic(result, "observation references an invalid participant");
			continue;
		}
		if(action.amount_known && (!IsFin(action.amount) || action.amount < 0)) {
			result.invalid = true;
			AddDiagnostic(result, Format("participant %d has an invalid action amount", action.participant));
		}
		const DistributedParticipantState& before_participant = before.participants[action.participant];
		const DistributedParticipantState& after_participant = after.participants[action.participant];
		if(action.kind == DISTRIBUTED_ACTION_REMOVE) {
			if(!before_participant.active || after_participant.active) {
				result.invalid = true;
				AddDiagnostic(result, Format("participant %d removal does not match active-state transition",
				                            action.participant));
			}
		}
		else {
			if(!before_participant.active || !after_participant.active) {
				result.invalid = true;
				AddDiagnostic(result, Format("participant %d action is outside its active interval",
				                            action.participant));
			}
			if(before_participant.committed >= 0 && after_participant.committed >= 0
			   && after_participant.committed + 0.001 < before_participant.committed) {
				result.invalid = true;
				AddDiagnostic(result, Format("participant %d committed value decreased",
				                            action.participant));
			}
			if(action.kind == DISTRIBUTED_ACTION_INCREASE && action.amount_known
			   && before_participant.committed >= 0 && after_participant.committed >= 0
			   && after_participant.committed <= before_participant.committed + 0.001) {
				result.invalid = true;
				AddDiagnostic(result, Format("participant %d increase has no committed increase",
				                            action.participant));
			}
		}
		if(covered[action.participant]) {
			AddDiagnostic(result, Format("participant %d has multiple observations", action.participant));
		}
		covered[action.participant] = true;
		if(action.kind == DISTRIBUTED_ACTION_INCREASE)
			increase_seen = true;
		DistributedReconstructedAction& copy = result.actions.Add();
		copy.observation = action;
		copy.inferred = false;
		copy.reason = "observed";
	}

	Vector<int> missing_active;
	Vector<int> removed;
	for(int i = 0; i < count; i++) {
		if(!before.participants[i].active && after.participants[i].active && !covered[i]) {
			activation_unobserved = true;
			result.ambiguous = true;
			AddDiagnostic(result, "participant activation is unobserved");
		}
		if(before.participants[i].active && !after.participants[i].active && !covered[i])
			removed.Add(i);
		if(before.participants[i].active && after.participants[i].active && !covered[i])
			missing_active.Add(i);
	}

	if(!removed.IsEmpty()) {
		result.ambiguous = true;
		AddDiagnostic(result, "participant removal is unobserved");
	}

	bool total_unchanged = SameAmount(before.total, after.total);
	bool phase_unchanged = before.phase == after.phase;
	if(!missing_active.IsEmpty()) {
		if(!phase_unchanged && !observed.IsEmpty() && missing_active.GetCount() == 1 &&
			!increase_seen && total_unchanged && removed.IsEmpty()) {
			DistributedReconstructedAction& inferred = result.actions.Add();
			inferred.observation.participant = missing_active[0];
			inferred.observation.kind = DISTRIBUTED_ACTION_PASSIVE;
			inferred.observation.timestamp = -1;
			inferred.inferred = true;
			inferred.reason = "single passive completion is forced by the observed transition";
		}
		else if(!removed.IsEmpty() || activation_unobserved) {
			AddDiagnostic(result, "missing actions coexist with an unobserved removal");
		}
		else if(!phase_unchanged) {
			result.ambiguous = true;
			AddDiagnostic(result, "phase changed while participant actions were missing");
		}
		else if(increase_seen || !total_unchanged) {
			result.ambiguous = true;
			AddDiagnostic(result, "missing actions coexist with an unresolved increase or total change");
		}
		else {
			for(int participant : missing_active) {
				DistributedReconstructedAction& inferred = result.actions.Add();
				inferred.observation.participant = participant;
				inferred.observation.kind = DISTRIBUTED_ACTION_PASSIVE;
				inferred.observation.timestamp = -1;
				inferred.inferred = true;
				inferred.reason = "only passive completion preserves phase and total";
			}
		}
	}

	result.complete = !result.invalid && !result.ambiguous;
	return result;
}

END_UPP_NAMESPACE
