#include "SweetSandy.h"

NAMESPACE_UPP

Value ActionContext::Read(const String& key)
{
	if(reads.Find(key) < 0)
		reads.Add(key);
	int i = graph.store.Find(key);
	return i >= 0 ? graph.store[i] : Value();
}

void ActionContext::Write(const String& key, const Value& v)
{
	if(writes.Find(key) < 0)
		writes.Add(key);
	graph.store.GetAdd(key) = v;
}

int ActionGraph::FindIndex(ActionId id) const
{
	for(int i = 0; i < actions.GetCount(); i++)
		if(actions[i].id == id)
			return i;
	return -1;
}

bool ActionGraph::ExecuteOne(ActionRecord& rec, Index<String>* refreshed)
{
	// On a replay (not the first execution), rec.writes still holds the
	// PREVIOUS execution's write-set, and the store still has those old
	// values sitting there. Without clearing them first, an action that
	// reads-then-writes the same key across replays (e.g. a terrain
	// brush re-painting the same grid node) would read its own stale
	// prior output as if it were an earlier action's legitimate write,
	// compounding instead of recomputing fresh from what's actually
	// before it in list order.
	//
	// Can't clear unconditionally though: if a *later* action in this
	// same pass already wrote one of these keys fresh (two overlapping
	// strokes both touching the same grid node - the later one reads the
	// earlier one's brand-new value, that's the whole point), clearing it
	// here would destroy real, correct, already-computed work. `refreshed`
	// tracks exactly that: keys some earlier-in-this-pass action has
	// already freshly rewritten - only clear what's NOT in there.
	for(const String& k : rec.writes) {
		if(refreshed && refreshed->Find(k) >= 0)
			continue;
		int i = store.Find(k);
		if(i >= 0)
			store.Remove(i);
	}

	ActionContext ctx(*this, rec.id);
	rec.execute(ctx, rec.param);
	bool fits = !rec.fit_check || rec.fit_check(ctx, rec.param);

	rec.reads = pick(ctx.reads);
	rec.writes = pick(ctx.writes);
	rec.status = fits ? ActionRecord::OK : ActionRecord::ERROR;

	// Dependencies: for each key this action read, whichever action last
	// wrote it (as of this point in the list - last_writer only ever
	// reflects actions processed so far, never something later) is a
	// real dependency. No manual declaration anywhere.
	rec.depends_on.Clear();
	for(const String& k : rec.reads) {
		int wi = last_writer.Find(k);
		if(wi < 0)
			continue;
		ActionId writer = last_writer[wi];
		if(writer != rec.id && rec.depends_on.Find(writer) < 0)
			rec.depends_on.Add(writer);
	}

	for(const String& k : rec.writes) {
		last_writer.GetAdd(k) = rec.id;
		if(refreshed)
			refreshed->FindAdd(k);
	}

	return fits;
}

Vector<ActionId> ActionGraph::ReplayFrom(ActionId id)
{
	Index<ActionId> propagate; // ids whose re-execution succeeded this pass - dependents inherit dirtiness only through these
	Index<String> refreshed;   // keys already freshly rewritten by an earlier action in this same pass - see ExecuteOne
	Vector<ActionId> failed;

	int start = FindIndex(id);
	ASSERT_(start >= 0, "ActionGraph::ReplayFrom: unknown action id");

	for(int i = start; i < actions.GetCount(); i++) {
		ActionRecord& a = actions[i];
		bool dirty = a.id == id;
		if(!dirty)
			for(ActionId dep : a.depends_on)
				if(propagate.Find(dep) >= 0) { dirty = true; break; }
		if(!dirty)
			continue; // untouched: old reads/writes/depends_on and its store contribution stand as-is

		bool fits = ExecuteOne(a, &refreshed);
		if(fits)
			propagate.Add(a.id);
		else
			failed.Add(a.id); // its own dependents won't find a.id in `propagate` - cascade stops here
	}

	return failed;
}

ActionId ActionGraph::Append(const String& type, const Value& param,
                              ActionExecuteFn execute, ActionFitCheckFn fit_check)
{
	ActionRecord& rec = actions.Add();
	rec.id = next_id++;
	rec.type = type;
	rec.param = param;
	rec.execute = execute;
	rec.fit_check = fit_check;
	ExecuteOne(rec);
	return rec.id;
}

Vector<ActionId> ActionGraph::SetParam(ActionId id, const Value& new_param)
{
	int i = FindIndex(id);
	ASSERT_(i >= 0, "ActionGraph::SetParam: unknown action id");
	actions[i].param = new_param;
	return ReplayFrom(id);
}

const ActionRecord* ActionGraph::Find(ActionId id) const
{
	int i = FindIndex(id);
	return i >= 0 ? &actions[i] : nullptr;
}

Value ActionGraph::GetStoreValue(const String& key) const
{
	int i = store.Find(key);
	return i >= 0 ? store[i] : Value();
}

Vector<String> ActionGraph::GetStoreKeys() const
{
	Vector<String> out;
	for(int i = 0; i < store.GetCount(); i++)
		out.Add(store.GetKey(i));
	return out;
}

Vector<ActionId> ActionGraph::GetDependents(ActionId id) const
{
	Vector<ActionId> out;
	for(const ActionRecord& a : actions)
		if(a.depends_on.Find(id) >= 0)
			out.Add(a.id);
	return out;
}

END_UPP_NAMESPACE
