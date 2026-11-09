#ifndef _Umbrella_BrushTool_h_
#define _Umbrella_BrushTool_h_

#include "LayerManager.h"
#include "Tile.h"

using namespace Upp;

enum BrushSize {
	BRUSH_1X1 = 1,
	BRUSH_2X2 = 2,
	BRUSH_3X3 = 3,
	BRUSH_5X5 = 5
};

enum BrushMode {
	BRUSH_MODE_PAINT,
	BRUSH_MODE_ERASE
};

class BrushTool {
private:
	BrushSize brushSize;
	TileType paintTile;
	BrushMode mode;
	bool isPainting;
	Point lastPaintPos;

public:
	BrushTool();

	// Settings
	void SetBrushSize(BrushSize size) { brushSize = size; }
	BrushSize GetBrushSize() const { return brushSize; }

	void SetPaintTile(TileType tile) { paintTile = tile; }
	TileType GetPaintTile() const { return paintTile; }

	void SetMode(BrushMode m) { mode = m; }
	BrushMode GetMode() const { return mode; }

	// Effective tile to paint (TILE_EMPTY if erasing)
	TileType GetEffectiveTile() const {
		return (mode == BRUSH_MODE_ERASE) ? TILE_EMPTY : paintTile;
	}

	// Painting
	void StartPainting(int col, int row, LayerManager& layerMgr);
	void ContinuePainting(int col, int row, LayerManager& layerMgr);
	void StopPainting();

	// Preview
	void GetBrushTiles(int centerCol, int centerRow, Vector<Point>& outTiles) const;

	bool IsPainting() const { return isPainting; }

private:
	void PaintAt(int col, int row, LayerManager& layerMgr);
};

#endif
