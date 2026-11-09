#ifndef _Umbrella_PathNode_h_
#define _Umbrella_PathNode_h_

#include <Core/Core.h>

using namespace Upp;

enum MoveType {
	MOVE_WALK,    // Horizontal walk on solid ground
	MOVE_JUMP,    // Jump up to higher tile
	MOVE_FALL     // Fall down to lower tile
};

struct PathNode : Moveable<PathNode> {
	int       col, row;    // Grid coordinates
	float     gScore;      // Cost from start
	float     fScore;      // gScore + heuristic
	int       parentIdx;   // Index into node pool (-1 = no parent)
	MoveType  moveType;    // How we arrived at this node

	PathNode() : col(0), row(0), gScore(0), fScore(0), parentIdx(-1), moveType(MOVE_WALK) {}
	PathNode(int c, int r) : col(c), row(r), gScore(0), fScore(0), parentIdx(-1), moveType(MOVE_WALK) {}
};

#endif
