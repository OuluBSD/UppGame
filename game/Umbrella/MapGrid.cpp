#include "MapGrid.h"

MapGrid::MapGrid()
	: columns(100), rows(100), gridSize(14), mapCols(32), mapRows(24) {
	tiles.SetCount(columns * rows, TILE_EMPTY);
}

MapGrid::MapGrid(int cols, int rows, int tileSize)
	: columns(cols), rows(rows), gridSize(tileSize), mapCols(32), mapRows(24) {
	tiles.SetCount(columns * rows, TILE_EMPTY);
}

void MapGrid::Resize(int newCols, int newRows) {
	Vector<TileType> newTiles;
	newTiles.SetCount(newCols * newRows, TILE_EMPTY);

	// Copy old tiles (preserve content)
	int minCols = min(columns, newCols);
	int minRows = min(rows, newRows);

	for(int row = 0; row < minRows; row++) {
		for(int col = 0; col < minCols; col++) {
			int oldIndex = row * columns + col;
			int newIndex = row * newCols + col;
			newTiles[newIndex] = tiles[oldIndex];
		}
	}

	columns = newCols;
	rows = newRows;
	tiles = pick(newTiles);
}

void MapGrid::Clear() {
	for(int i = 0; i < tiles.GetCount(); i++) {
		tiles[i] = TILE_EMPTY;
	}
}

TileType MapGrid::GetTile(int col, int row) const {
	if(!IsValid(col, row)) return TILE_EMPTY;
	return tiles[ToIndex(col, row)];
}

void MapGrid::SetTile(int col, int row, TileType type) {
	if(!IsValid(col, row)) return;
	tiles[ToIndex(col, row)] = type;
}

TileType MapGrid::GetTileByIndex(int index) const {
	if(index < 0 || index >= tiles.GetCount()) return TILE_EMPTY;
	return tiles[index];
}

void MapGrid::SetTileByIndex(int index, TileType type) {
	if(index < 0 || index >= tiles.GetCount()) return;
	tiles[index] = type;
}

int MapGrid::ToIndex(int col, int row) const {
	return row * columns + col;
}

Point MapGrid::ToColRow(int index) const {
	return Point(index % columns, index / columns);
}

bool MapGrid::IsValid(int col, int row) const {
	return col >= 0 && col < columns && row >= 0 && row < rows;
}

void MapGrid::SetMapArea(int cols, int rows) {
	mapCols = cols;
	mapRows = rows;
}
