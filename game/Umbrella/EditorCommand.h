#ifndef _Umbrella_EditorCommand_h_
#define _Umbrella_EditorCommand_h_

#include "LayerManager.h"

using namespace Upp;

// Record of a single tile change (before/after)
struct TileChange {
	int layerIndex;
	int col;
	int row;
	TileType oldTile;
	TileType newTile;
};

// Abstract editor command (Command pattern)
class EditorCommand {
public:
	virtual ~EditorCommand() {}
	virtual void Undo(LayerManager& lm) = 0;
	virtual void Redo(LayerManager& lm) = 0;
	virtual String GetDescription() const = 0;
};

// Undo/redo a set of tile changes (paint, erase, fill)
class TileChangeCommand : public EditorCommand {
	Vector<TileChange> changes;
	String description;

public:
	TileChangeCommand(Vector<TileChange>&& ch, const String& desc)
		: changes(pick(ch)), description(desc) {}

	bool IsEmpty() const { return changes.IsEmpty(); }

	void Undo(LayerManager& lm) override {
		for(int i = 0; i < changes.GetCount(); i++) {
			const TileChange& c = changes[i];
			if(c.layerIndex >= 0 && c.layerIndex < lm.GetLayerCount())
				lm.GetLayer(c.layerIndex).GetGrid().SetTile(c.col, c.row, c.oldTile);
		}
	}

	void Redo(LayerManager& lm) override {
		for(int i = 0; i < changes.GetCount(); i++) {
			const TileChange& c = changes[i];
			if(c.layerIndex >= 0 && c.layerIndex < lm.GetLayerCount())
				lm.GetLayer(c.layerIndex).GetGrid().SetTile(c.col, c.row, c.newTile);
		}
	}

	String GetDescription() const override { return description; }
};

#endif
