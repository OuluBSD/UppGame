#ifndef _Umbrella_MapGrid_h_
#define _Umbrella_MapGrid_h_

#include "Tile.h"

using namespace Upp;

// 2D grid of tiles for map editing
class MapGrid {
private:
	int columns;       // Total grid width
	int rows;          // Total grid height
	int gridSize;      // Tile size in pixels (typically 14)
	int mapCols;       // Visible map width in tiles
	int mapRows;       // Visible map height in tiles

	Vector<TileType> tiles;  // Linear array of tiles (columns * rows)

public:
	MapGrid();
	MapGrid(int cols, int rows, int tileSize = 14);

	// Grid operations
	void Resize(int newCols, int newRows);
	void Clear();

	// Tile access (by column, row)
	TileType GetTile(int col, int row) const;
	void SetTile(int col, int row, TileType type);

	// Tile access (by linear index)
	TileType GetTileByIndex(int index) const;
	void SetTileByIndex(int index, TileType type);

	// Coordinate conversion
	int ToIndex(int col, int row) const;
	Point ToColRow(int index) const;

	// Bounds checking
	bool IsValid(int col, int row) const;

	// Getters
	int GetColumns() const { return columns; }
	int GetRows() const { return rows; }
	int GetGridSize() const { return gridSize; }
	int GetMapCols() const { return mapCols; }
	int GetMapRows() const { return mapRows; }
	Size GetTotalSize() const { return Size(columns, rows); }

	// Setters for visible map area
	void SetMapArea(int cols, int rows);
};

#endif
