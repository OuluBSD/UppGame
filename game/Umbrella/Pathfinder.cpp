#include "Umbrella.h"
#include "Pathfinder.h"
#include "GameScreen.h"

using namespace Upp;

Pathfinder::Pathfinder() : gameScreen(nullptr) {}

// ============================================================================
// Tile queries
// ============================================================================

bool Pathfinder::IsSolid(int col, int row) const {
	if(!gameScreen) return false;
	const Layer* terrain = gameScreen->layerManager.FindLayerByType(LAYER_TERRAIN);
	if(!terrain) return false;
	const MapGrid& g = terrain->GetGrid();
	if(col < 0 || row < 0 || col >= g.GetColumns() || row >= g.GetRows()) return false;
	TileType t = g.GetTile(col, row);
	return t == TILE_WALL || t == TILE_FULLBLOCK;
}

bool Pathfinder::IsPassable(int col, int row) const {
	if(!gameScreen) return false;
	const Layer* terrain = gameScreen->layerManager.FindLayerByType(LAYER_TERRAIN);
	if(!terrain) return false;
	const MapGrid& g = terrain->GetGrid();
	if(col < 0 || row < 0 || col >= g.GetColumns() || row >= g.GetRows()) return false;
	TileType t = g.GetTile(col, row);
	return t != TILE_WALL && t != TILE_FULLBLOCK;
}

// A tile is walkable if it's passable, has head clearance above, and solid ground below.
// NOTE: Y-UP coordinate system - row+1 is above (ceiling), row-1 is below (floor).
bool Pathfinder::IsWalkable(int col, int row) const {
	if(!IsPassable(col, row)) return false;
	// Head clearance: tile above (row+1) must be passable
	if(!IsPassable(col, row + 1)) return false;
	// Must have solid ground directly below (row-1)
	return IsSolid(col, row - 1);
}

bool Pathfinder::CanJumpTo(int fc, int fr, int tc, int tr) const {
	// Y-UP: jumping up means tr > fr (larger row = higher in world)
	int dh = tr - fr;  // Height gain (positive = jumping up in Y-UP)
	int dw = tc - fc;  // Horizontal offset (signed)
	if(dh <= 0 || dh > MAX_JUMP_HEIGHT) return false;
	if(abs(dw) > MAX_JUMP_WIDTH) return false;
	if(!IsWalkable(tc, tr)) return false;
	// Head clearance at destination: tile above (tr+1) must be passable
	return IsPassable(tc, tr + 1);
}

bool Pathfinder::CanFallTo(int fc, int fr, int tc, int tr) const {
	// Y-UP: falling down means tr < fr (smaller row = lower in world)
	int dd = fr - tr;  // Drop depth (positive = falling down in Y-UP)
	int dw = tc - fc;
	if(dd <= 0 || dd > MAX_FALL_DEPTH) return false;
	if(abs(dw) > 1) return false;  // Can only fall 1 tile sideways
	if(!IsWalkable(tc, tr)) return false;
	// Make sure the fall path is clear (no solid between fr-1 down to tr+1)
	for(int r = fr - 1; r > tr; r--) {
		if(!IsPassable(fc, r)) return false;
	}
	return true;
}

// ============================================================================
// A* helpers
// ============================================================================

float Pathfinder::Heuristic(int ac, int ar, int bc, int br) const {
	// Manhattan distance
	return (float)(abs(ac - bc) + abs(ar - br));
}

void Pathfinder::AddNeighbors(int nodeIdx, int goalCol, int goalRow,
                               VectorMap<int64, int>& openSet,
                               VectorMap<int64, int>& closedSet) {
	const PathNode& node = nodePool[nodeIdx];
	int c = node.col, r = node.row;

	auto TryAdd = [&](int nc, int nr, MoveType mt, float extraCost) {
		if(closedSet.Find(TileKey(nc, nr)) >= 0) return;
		float tentG = node.gScore + Heuristic(c, r, nc, nr) * extraCost;
		int64 key = TileKey(nc, nr);
		int idx = openSet.Find(key);
		if(idx >= 0) {
			PathNode& existing = nodePool[openSet[idx]];
			if(tentG >= existing.gScore) return;
			existing.gScore  = tentG;
			existing.fScore  = tentG + Heuristic(nc, nr, goalCol, goalRow);
			existing.parentIdx = nodeIdx;
			existing.moveType  = mt;
		} else {
			PathNode& nn = nodePool.Add();
			nn.col       = nc;
			nn.row       = nr;
			nn.gScore    = tentG;
			nn.fScore    = tentG + Heuristic(nc, nr, goalCol, goalRow);
			nn.parentIdx = nodeIdx;
			nn.moveType  = mt;
			openSet.Add(key, nodePool.GetCount() - 1);
		}
	};

	// Walk left/right
	if(IsWalkable(c - 1, r)) TryAdd(c - 1, r, MOVE_WALK, 1.0f);
	if(IsWalkable(c + 1, r)) TryAdd(c + 1, r, MOVE_WALK, 1.0f);

	// Jump up (Y-UP: up = increasing row, nr = r + dh)
	for(int dh = 1; dh <= MAX_JUMP_HEIGHT; dh++) {
		for(int dw = -MAX_JUMP_WIDTH; dw <= MAX_JUMP_WIDTH; dw++) {
			int nc = c + dw, nr = r + dh;
			if(CanJumpTo(c, r, nc, nr)) TryAdd(nc, nr, MOVE_JUMP, 1.5f);
		}
	}

	// Fall down (Y-UP: down = decreasing row, nr = r - dd)
	for(int dd = 1; dd <= MAX_FALL_DEPTH; dd++) {
		for(int dw = -1; dw <= 1; dw++) {
			int nc = c + dw, nr = r - dd;
			if(CanFallTo(c, r, nc, nr)) TryAdd(nc, nr, MOVE_FALL, 1.2f);
		}
	}
}

Vector<PathNode> Pathfinder::ReconstructPath(int goalIdx) const {
	Vector<PathNode> path;
	int idx = goalIdx;
	while(idx >= 0) {
		path.Add(nodePool[idx]);
		idx = nodePool[idx].parentIdx;
	}
	// Reverse: path was built from goal back to start
	Vector<PathNode> result;
	for(int i = path.GetCount() - 1; i >= 0; i--)
		result.Add(path[i]);
	return result;
}

// ============================================================================
// Main A* search
// ============================================================================

Vector<PathNode> Pathfinder::FindPath(int startCol, int startRow, int goalCol, int goalRow) {
	nodePool.Clear();

	if(!gameScreen) {
		LOG("Pathfinder: GameScreen not set");
		return Vector<PathNode>();
	}
	if(!IsWalkable(startCol, startRow)) {
		LOG("Pathfinder: Start " << startCol << "," << startRow << " is not walkable");
		return Vector<PathNode>();
	}
	if(!IsWalkable(goalCol, goalRow)) {
		LOG("Pathfinder: Goal " << goalCol << "," << goalRow << " is not walkable");
		return Vector<PathNode>();
	}

	// openSet: tile key -> node pool index
	// closedSet: tile key -> node pool index
	VectorMap<int64, int> openSet;
	VectorMap<int64, int> closedSet;

	// Create start node
	PathNode& start = nodePool.Add();
	start.col       = startCol;
	start.row       = startRow;
	start.gScore    = 0;
	start.fScore    = Heuristic(startCol, startRow, goalCol, goalRow);
	start.parentIdx = -1;
	start.moveType  = MOVE_WALK;
	openSet.Add(TileKey(startCol, startRow), 0);

	int iterations = 0;
	while(openSet.GetCount() > 0 && iterations < MAX_OPEN_NODES) {
		iterations++;

		// Find node with lowest fScore in openSet
		int bestIdx = -1;
		float bestF = 1e9f;
		for(int i = 0; i < openSet.GetCount(); i++) {
			const PathNode& n = nodePool[openSet[i]];
			if(n.fScore < bestF) {
				bestF   = n.fScore;
				bestIdx = i;
			}
		}
		if(bestIdx < 0) break;

		int poolIdx    = openSet[bestIdx];
		int64 bestKey  = openSet.GetKey(bestIdx);
		openSet.Remove(bestIdx);

		const PathNode& current = nodePool[poolIdx];

		// Goal reached?
		if(current.col == goalCol && current.row == goalRow) {
			LOG("Pathfinder: Found path in " << iterations << " iterations, "
			    << nodePool.GetCount() << " nodes allocated");
			return ReconstructPath(poolIdx);
		}

		closedSet.Add(bestKey, poolIdx);
		AddNeighbors(poolIdx, goalCol, goalRow, openSet, closedSet);
	}

	LOG("Pathfinder: No path found (" << iterations << " iterations)");
	return Vector<PathNode>();
}
