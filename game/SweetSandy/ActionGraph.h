#ifndef _SweetSandy_ActionGraph_h_
#define _SweetSandy_ActionGraph_h_

// Map Editor phase, Task 2: the action-DAG core. Standalone and
// Terrain/WorldBuilder-agnostic - see Manager repo
// 012-map-editor/002-task2-action-graph-core.md for the full design.
//
// The map is an ordered list of actions. Each action reads/writes a
// generic key-value store (ActionContext::Read/Write) - never touches
// shared state any other way - so ActionGraph can record, purely from
// those reads/writes, which earlier action(s) each action actually
// depends on, with no manual dependency declaration. Editing an
// earlier action's parameter (SetParam) re-executes it and cascades
// to its transitive dependents, in list order (the list is always a
// valid topological order by construction - an action can only depend
// on something already before it, since Append always runs
// immediately against the current store); a dependent's own
// fit_check() can reject the new result, which stops the cascade from
// propagating past it without touching anything further downstream.

typedef int ActionId; // 0 = none/invalid

class ActionGraph;

struct ActionContext {
	Index<String> reads, writes; // accumulated as Read/Write are called

	ActionGraph& graph;
	ActionId     acting_id;

	ActionContext(ActionGraph& graph, ActionId acting_id) : graph(graph), acting_id(acting_id) {}

	Value Read(const String& key);
	void  Write(const String& key, const Value& v);
};

typedef Function<void(ActionContext&, const Value& param)> ActionExecuteFn;
typedef Function<bool(ActionContext&, const Value& param)> ActionFitCheckFn; // true = fits; empty -> always fits

struct ActionRecord : Moveable<ActionRecord> {
	ActionId id = 0;
	String   type; // free-form label, for logging/errors only

	Value    param;
	ActionExecuteFn  execute;
	ActionFitCheckFn fit_check;

	Index<String>   reads, writes;
	Index<ActionId> depends_on;

	enum Status { OK, ERROR };
	Status status = OK;
};

class ActionGraph {
	Vector<ActionRecord>      actions;
	VectorMap<String, ActionId> last_writer; // key -> action that most recently wrote it, in list order
	VectorMap<String, Value>    store;
	ActionId next_id = 1;

	int FindIndex(ActionId id) const;
	// Runs rec.execute then rec.fit_check against one shared ActionContext,
	// updates rec's own reads/writes/depends_on and the store/last_writer
	// entries for rec's own writes. Returns the fit_check result.
	//
	// `refreshed`, when given (a live ReplayFrom pass, not Append's own
	// first-time call), is the set of keys some *earlier* action in this
	// same pass has already freshly rewritten. Before rec runs, each of
	// rec's own *old* write-keys not already in `refreshed` is cleared
	// from the store - it's stale (either rec's own leftover output from
	// before, or an action after rec that hasn't been reached yet this
	// pass), so a fresh ctx.Read must not see it. A key already in
	// `refreshed`, though, was *just* written by an earlier action this
	// same pass (e.g. two overlapping terrain-paint strokes writing the
	// same grid node in a read-modify-write chain) and must be preserved,
	// not blown away - that fresh value is rec's own real, correct input.
	bool ExecuteOne(ActionRecord& rec, Index<String>* refreshed = nullptr);
	// Forward scan from id's own position to the end: re-executes id and
	// every action transitively dependent on it (through still-fitting
	// actions only - see this task's own planning doc on why a failed
	// fit_check stops propagation through that specific action without
	// needing a separate "skip subtree" step). Returns ids that failed
	// their fit_check this pass.
	Vector<ActionId> ReplayFrom(ActionId id);

public:
	friend struct ActionContext;

	// Appends and immediately executes a new action against the current
	// store - there is no "declared but not yet run" state; the ordered
	// list is both the authoring history and, via the store, the current
	// materialized result.
	ActionId Append(const String& type, const Value& param,
	                ActionExecuteFn execute, ActionFitCheckFn fit_check = ActionFitCheckFn());

	// Updates an existing action's parameter and replays it plus its
	// transitive dependents (see ReplayFrom). Returns ids that failed
	// their fit_check this pass (empty if everything still fits).
	Vector<ActionId> SetParam(ActionId id, const Value& new_param);

	const ActionRecord* Find(ActionId id) const;
	Value GetStoreValue(const String& key) const;
	// Every key currently present in the store - e.g. used by
	// TerrainPaintAction's own CompileTerrainPaintToTerrain to find
	// every painted grid node without needing to track "what changed"
	// separately from "what's the current state".
	Vector<String> GetStoreKeys() const;
	// Direct (one hop) dependents of id - for inspection/tests, not used
	// by the cascade itself (which scans forward instead).
	Vector<ActionId> GetDependents(ActionId id) const;

	int GetCount() const { return actions.GetCount(); }
	const ActionRecord& Get(int i) const { return actions[i]; }
};

#endif
