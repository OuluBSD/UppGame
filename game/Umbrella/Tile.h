#ifndef _Umbrella_Tile_h_
#define _Umbrella_Tile_h_

#include <CtrlLib/CtrlLib.h>

using namespace Upp;

// Tile types for map editing
enum TileType {
	TILE_EMPTY = 0,      // No tile (passable)
	TILE_WALL,           // Solid collision wall
	TILE_BACKGROUND,     // Visual only, no collision
	TILE_FULLBLOCK,      // Solid block
	TILE_GOAL,           // Level exit/goal (triggers level completion)
};

// Convert TileType to color for rendering (colored rectangles)
// Color scheme from MapPlaytestScreen.java
Color TileTypeToColor(TileType type);

// Convert TileType to string (for debugging)
String TileTypeToString(TileType type);

#endif
