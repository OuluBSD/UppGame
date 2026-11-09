#ifndef _Umbrella_UndoStack_h_
#define _Umbrella_UndoStack_h_

#include "EditorCommand.h"

using namespace Upp;

// Undo/redo stack with a configurable max depth
class UndoStack {
	static const int MAX_SIZE = 100;

	Vector<EditorCommand*> undos;
	Vector<EditorCommand*> redos;

	void ClearVec(Vector<EditorCommand*>& v) {
		for(int i = 0; i < v.GetCount(); i++)
			delete v[i];
		v.Clear();
	}

public:
	~UndoStack() {
		ClearVec(undos);
		ClearVec(redos);
	}

	// Push a new command. Clears redo stack and takes ownership of cmd.
	void Push(EditorCommand* cmd) {
		ClearVec(redos);
		undos.Add(cmd);
		while(undos.GetCount() > MAX_SIZE) {
			delete undos[0];
			undos.Remove(0);
		}
	}

	void Undo(LayerManager& lm) {
		if(!CanUndo()) return;
		EditorCommand* cmd = undos.Pop();
		cmd->Undo(lm);
		redos.Add(cmd);
	}

	void Redo(LayerManager& lm) {
		if(!CanRedo()) return;
		EditorCommand* cmd = redos.Pop();
		cmd->Redo(lm);
		undos.Add(cmd);
	}

	void Clear() {
		ClearVec(undos);
		ClearVec(redos);
	}

	bool CanUndo() const { return undos.GetCount() > 0; }
	bool CanRedo() const { return redos.GetCount() > 0; }

	String GetUndoDescription() const {
		return CanUndo() ? undos.Top()->GetDescription() : String();
	}
	String GetRedoDescription() const {
		return CanRedo() ? redos.Top()->GetDescription() : String();
	}
};

#endif
