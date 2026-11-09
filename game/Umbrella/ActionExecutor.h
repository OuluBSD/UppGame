#ifndef _Umbrella_ActionExecutor_h_
#define _Umbrella_ActionExecutor_h_

#include "PathNode.h"

using namespace Upp;

// Forward declaration
class GameScreen;

// Executes a pre-computed path (Vector<PathNode>) by injecting keyboard
// inputs into GameScreen and stepping the simulation frame-by-frame.
//
// Designed to be called from GameScriptBridge's execute_path() and
// navigate_to() Python API functions.

class ActionExecutor {
public:
	// Pixels per tile (must match Player::GRID_SIZE = 14)
	static constexpr int GRID         = 14;

	// Per-node frame budgets
	static constexpr int WALK_TIMEOUT = 120;  // 2 s at 60 fps
	static constexpr int JUMP_TIMEOUT = 120;  // 2 s (jump arc ~34 frames + margin)
	static constexpr int FALL_TIMEOUT = 360;  // 6 s (max 12 tiles down)

	// Arrival tolerance in tiles
	static constexpr int COL_TOL      = 1;    // ±1 column accepted as "at tile"

	struct Result {
		bool   success;
		int    framesUsed;
		int    nodesCompleted;  // nodes successfully reached (0 = stuck on first)
		String reason;          // "success" | "timeout" | "stuck_at_N"
	};

	// Execute a path (from find_path output).
	// maxFrames is a global safety cap across all nodes.
	// Keys are cleared before return regardless of outcome.
	Result ExecutePath(GameScreen* screen,
	                   const Vector<PathNode>& path,
	                   int maxFrames = 3000);

private:
	// Tile coordinate of player center
	int  TileCol(const GameScreen* s) const;
	int  TileRow(const GameScreen* s) const;

	// True when player center is within COL_TOL of target col,
	// exactly at target row, and on the ground.
	bool AtTile(const GameScreen* s, int col, int row) const;

	// Advance one physics frame
	void Tick(GameScreen* s) const;

	// Set horizontal direction: -1=left, 0=neutral, 1=right
	void SetDir(GameScreen* s, int dir) const;

	// Release all keys
	void ClearAll(GameScreen* s) const;
};

#endif
