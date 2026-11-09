#include "Umbrella.h"
#include "BrushTool.h"

using namespace Upp;

BrushTool::BrushTool()
	: brushSize(BRUSH_1X1), paintTile(TILE_WALL), mode(BRUSH_MODE_PAINT),
	  isPainting(false), lastPaintPos(-1, -1) {
}

void BrushTool::StartPainting(int col, int row, LayerManager& layerMgr) {
	isPainting = true;
	lastPaintPos = Point(col, row);
	PaintAt(col, row, layerMgr);
}

void BrushTool::ContinuePainting(int col, int row, LayerManager& layerMgr) {
	if(!isPainting) return;

	// Check if position changed
	if(col == lastPaintPos.x && row == lastPaintPos.y) return;

	// Paint at new position
	PaintAt(col, row, layerMgr);
	lastPaintPos = Point(col, row);
}

void BrushTool::StopPainting() {
	isPainting = false;
	lastPaintPos = Point(-1, -1);
}

void BrushTool::PaintAt(int col, int row, LayerManager& layerMgr) {
	Layer* activeLayer = layerMgr.GetActiveLayer();
	if(!activeLayer) return;

	if(activeLayer->IsLocked()) return;

	MapGrid& grid = activeLayer->GetGrid();

	// Get brush tiles
	Vector<Point> brushTiles;
	GetBrushTiles(col, row, brushTiles);

	// Get effective tile (respects erase mode)
	TileType tileToPaint = GetEffectiveTile();

	// Paint each tile in brush area
	for(const Point& pt : brushTiles) {
		if(grid.IsValid(pt.x, pt.y)) {
			grid.SetTile(pt.x, pt.y, tileToPaint);
		}
	}
}

void BrushTool::GetBrushTiles(int centerCol, int centerRow, Vector<Point>& outTiles) const {
	outTiles.Clear();

	int size = (int)brushSize;
	int halfSize = size / 2;

	for(int dy = -halfSize; dy <= halfSize; dy++) {
		for(int dx = -halfSize; dx <= halfSize; dx++) {
			outTiles.Add(Point(centerCol + dx, centerRow + dy));
		}
	}
}
