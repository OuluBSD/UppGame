#include "Tile.h"

// Color scheme from MapPlaytestScreen.java
// Converts RGB floats (0.0-1.0) to RGB bytes (0-255)
Color TileTypeToColor(TileType type) {
	switch(type) {
		case TILE_EMPTY:
			// Canvas: RGB(0.05, 0.07, 0.12) - very dark blue
			return Color(12, 17, 30);
		case TILE_WALL:
			// Wall: RGB(0.78, 0.31, 0.38) - reddish pink
			return Color(198, 79, 97);
		case TILE_BACKGROUND:
			// Background: RGB(0.16, 0.24, 0.34) - dark blue
			return Color(40, 61, 87);
		case TILE_FULLBLOCK:
			// FullBlock: RGB(0.98, 0.85, 0.22) - yellow
			return Color(250, 217, 56);
		case TILE_GOAL:
			// Goal: Bright green/gold - very visible
			return Color(100, 255, 100);
		default:
			return Gray();
	}
}

String TileTypeToString(TileType type) {
	switch(type) {
		case TILE_EMPTY: return "Empty";
		case TILE_WALL: return "Wall";
		case TILE_BACKGROUND: return "Background";
		case TILE_FULLBLOCK: return "FullBlock";
		case TILE_GOAL: return "Goal";
		default: return "Unknown";
	}
}
