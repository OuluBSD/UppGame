#ifndef _Umbrella_Pathfinder_h_
#define _Umbrella_Pathfinder_h_

#include "PathNode.h"
#include "MapGrid.h"
#include "LayerManager.h"

using namespace Upp;

// Forward declarations
class GameScreen;

// A* pathfinder for the Umbrella game.
// Operates on tile coordinates (not world pixels).
// Access via GameScriptBridge's find_path() Python function.
class Pathfinder {
public:
	// Player physics constants (from Player.h)
	static constexpr int MAX_JUMP_HEIGHT = 4;   // Tiles reachable with JUMP_VELOCITY=280
	static constexpr int MAX_JUMP_WIDTH  = 3;   // Max horizontal tiles per jump
	static constexpr int MAX_FALL_DEPTH  = 12;  // Max safe fall (no fall damage yet)
	static constexpr int MAX_OPEN_NODES  = 4096; // Safety cap

	Pathfinder();
	void SetGameScreen(const GameScreen* screen) { gameScreen = screen; }

	// Find path between tile coordinates. Returns empty vector if no path.
	Vector<PathNode> FindPath(int startCol, int startRow, int goalCol, int goalRow);

private:
	const GameScreen* gameScreen;
	Array<PathNode>   nodePool;    // All allocated nodes (stable pointers via indices)

	// Tile queries
	bool IsSolid(int col, int row) const;    // TILE_WALL or TILE_FULLBLOCK
	bool IsPassable(int col, int row) const; // Not solid, not out-of-bounds
	bool IsWalkable(int col, int row) const; // Passable + solid ground beneath
	bool CanJumpTo(int fc, int fr, int tc, int tr) const;
	bool CanFallTo(int fc, int fr, int tc, int tr) const;

	// A* helpers
	float Heuristic(int ac, int ar, int bc, int br) const;
	void  AddNeighbors(int nodeIdx, int goalCol, int goalRow,
	                   VectorMap<int64, int>& openSet,
	                   VectorMap<int64, int>& closedSet);
	Vector<PathNode> ReconstructPath(int goalIdx) const;

	static int64 TileKey(int col, int row) { return (int64)col | ((int64)row << 32); }
};

#endif
