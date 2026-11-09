#ifndef _Umbrella_NavGraph_h_
#define _Umbrella_NavGraph_h_

#include "Pathfinder.h"

using namespace Upp;

// Forward declaration
class GameScreen;

// Pre-computed navigation graph for one level.
//
// Nodes     : every walkable tile (IsSolid(c,r+1) && IsPassable(c,r) && IsPassable(c,r-1))
// Edges     : walk left/right, jump arcs (reusing Pathfinder constants), fall arcs
// Components: flood-fill connected-component IDs so IsReachable() is O(1)
//
// Call Build() once after a level loads.  Query without rebuilding.

class NavGraph {
public:
	static constexpr int INVALID_COMPONENT = -1;

	NavGraph();

	// Build or rebuild from the current level.
	void Build(const GameScreen* screen);

	bool IsBuilt() const { return built; }

	// O(1) reachability check via component IDs.
	bool IsReachable(int c1, int r1, int c2, int r2) const;

	// Component ID for a tile (-1 if not walkable / not built).
	int  GetComponentId(int col, int row) const;

	// Stats
	int  GetWalkableCount()   const { return walkableCount; }
	int  GetComponentCount()  const { return componentCount; }
	int  GetEdgeCount()       const { return edgeCount; }
	int  GetCols()            const { return cols; }
	int  GetRows()            const { return rows; }

private:
	bool built;
	int  cols, rows;
	int  walkableCount;
	int  componentCount;
	int  edgeCount;

	// componentId[row * cols + col] = component ID, or INVALID_COMPONENT
	Vector<int> componentId;

	// Tile queries (mirrors Pathfinder, but operates on stored layer pointer)
	const GameScreen* gameScreen;
	bool IsSolid(int col, int row)    const;
	bool IsPassable(int col, int row) const;
	bool IsWalkable(int col, int row) const;

	// Key helpers
	int  CellIdx(int col, int row) const { return row * cols + col; }
	bool InBounds(int col, int row) const {
		return col >= 0 && row >= 0 && col < cols && row < rows;
	}

	// BFS flood-fill assigning component IDs via navigation edges
	void FloodFill(int startCol, int startRow, int compId);
	bool HasEdge(int fc, int fr, int tc, int tr) const;
};

#endif
