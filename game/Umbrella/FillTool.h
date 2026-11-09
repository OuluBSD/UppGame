#ifndef _Umbrella_FillTool_h_
#define _Umbrella_FillTool_h_

#include "LayerManager.h"
#include "Tile.h"

using namespace Upp;

class FillTool {
private:
	TileType fillTile;

public:
	FillTool();

	void SetFillTile(TileType tile) { fillTile = tile; }
	TileType GetFillTile() const { return fillTile; }

	// Perform flood fill starting at (col, row)
	void Fill(int col, int row, LayerManager& layerMgr);

private:
	void FloodFill(int col, int row, TileType targetTile, MapGrid& grid);
};

#endif
