#include "Umbrella.h"
#include "FillTool.h"

using namespace Upp;

FillTool::FillTool()
	: fillTile(TILE_WALL) {
}

void FillTool::Fill(int col, int row, LayerManager& layerMgr) {
	Layer* activeLayer = layerMgr.GetActiveLayer();
	if(!activeLayer) return;

	if(activeLayer->IsLocked()) return;

	MapGrid& grid = activeLayer->GetGrid();

	if(!grid.IsValid(col, row)) return;

	TileType targetTile = grid.GetTile(col, row);

	// Don't fill if target is already the fill tile
	if(targetTile == fillTile) return;

	// Perform flood fill
	FloodFill(col, row, targetTile, grid);
}

void FillTool::FloodFill(int col, int row, TileType targetTile, MapGrid& grid) {
	// Use queue-based approach (non-recursive to avoid stack overflow)
	Vector<Point> queue;
	queue.Add(Point(col, row));

	int fillCount = 0;
	const int MAX_FILL = 10000;  // Prevent excessive fills

	while(queue.GetCount() > 0 && fillCount < MAX_FILL) {
		Point pt = queue.Pop();

		int c = pt.x;
		int r = pt.y;

		// Check bounds
		if(!grid.IsValid(c, r)) continue;

		// Check if this tile matches target
		if(grid.GetTile(c, r) != targetTile) continue;

		// Fill this tile
		grid.SetTile(c, r, fillTile);
		fillCount++;

		// Add neighbors to queue (4-directional)
		queue.Add(Point(c + 1, r));     // Right
		queue.Add(Point(c - 1, r));     // Left
		queue.Add(Point(c, r + 1));     // Down
		queue.Add(Point(c, r - 1));     // Up
	}

	if(fillCount >= MAX_FILL) {
		Exclamation("Fill stopped: too many tiles (limit: 10,000)");
	}
}
