#include "Umbrella.h"
#include "AIController.h"
#include "GameScreen.h"

using namespace Upp;

// ============================================================================
// AIController
// ============================================================================

AIController::AIController()
	: pathfinder(nullptr), nodeIdx(0), pathValid(false),
	  segFrame(0), stuckFrames(0), lastGoal(-1, -1),
	  goalCol(-1), goalRow(-1)
{}

bool AIController::AtTile(float worldX, float worldY, bool onGround,
                          int col, int row) const {
	if(!onGround) return false;
	int ec = TileCol(worldX);
	int er = TileRow(worldY);
	return abs(ec - col) <= COL_TOL && er == row;
}

BehaviorCtx AIController::MakeCtx(int selfCol, int selfRow,
                                   float selfX, float selfY,
                                   bool onGround, bool pathComplete, bool stuck,
                                   const GameScreen* screen, int frame) const {
	BehaviorCtx ctx;
	ctx.selfCol       = selfCol;
	ctx.selfRow       = selfRow;
	ctx.selfX         = selfX;
	ctx.selfY         = selfY;
	ctx.onGround      = onGround;
	ctx.pathComplete  = pathComplete;
	ctx.stuck         = stuck;
	ctx.screen        = screen;
	ctx.navGraph      = navGraph;
	ctx.frame         = frame;

	// Player info
	if(screen) {
		Pointf pc = screen->player.GetCenter();
		ctx.playerX   = pc.x;
		ctx.playerY   = pc.y;
		ctx.playerCol = TileCol(pc.x);
		ctx.playerRow = TileRow(pc.y);
		// Simple LOS: same row or same column (not through walls, approximate)
		ctx.playerVisible = (abs(selfRow - ctx.playerRow) <= 2);
	} else {
		ctx.playerX = ctx.playerY = 0;
		ctx.playerCol = ctx.playerRow = 0;
		ctx.playerVisible = false;
	}
	return ctx;
}

ActionSet AIController::Update(const Rectf& enemyBounds, bool onGround, int frame) {
	ActionSet out;

	if(!pathfinder || !behavior) return out;

	Pointf center = enemyBounds.CenterPoint();
	int selfCol = TileCol(center.x);
	int selfRow = TileRow(center.y);

	// Determine path state
	bool pathComplete = pathValid && (nodeIdx >= path.GetCount());
	bool stuck        = stuckFrames >= STUCK_LIMIT;

	BehaviorCtx ctx = MakeCtx(selfCol, selfRow, center.x, center.y,
	                          onGround, pathComplete, stuck, screen, frame);

	// Check if behavior wants a new plan
	if(!pathValid || behavior->ShouldRePlan(ctx)) {
		Point goal = behavior->PickGoal(ctx);
		if(goal.x >= 0 && goal.y >= 0) {
			// Avoid re-planning to same goal repeatedly when stuck
			if(goal != lastGoal || stuck) {
				path     = pathfinder->FindPath(selfCol, selfRow, goal.x, goal.y);
				nodeIdx  = 0;
				segFrame = 0;
				stuckFrames = 0;
				goalCol  = goal.x;
				goalRow  = goal.y;
				lastGoal = goal;
				pathValid = true;
			}
		} else {
			// Stand still
			pathValid = false;
			return out;
		}
	}

	if(!pathValid || path.IsEmpty() || nodeIdx >= path.GetCount()) {
		return out;
	}

	// ---- Follow current node ----
	const PathNode& target = path[nodeIdx];

	if(AtTile(center.x, center.y, onGround, target.col, target.row)) {
		// Arrived at this node
		nodeIdx++;
		segFrame    = 0;
		stuckFrames = 0;
		if(nodeIdx >= path.GetCount()) {
			return out;  // Path complete
		}
	}

	// Safety: if we've been on this segment too long, count stuck frames
	segFrame++;
	int timeout = WALK_TIMEOUT;
	if(nodeIdx < path.GetCount()) {
		MoveType mt = path[nodeIdx].moveType;
		if(mt == MOVE_JUMP) timeout = JUMP_TIMEOUT;
		else if(mt == MOVE_FALL) timeout = FALL_TIMEOUT;
	}

	if(segFrame > timeout) {
		stuckFrames++;
	} else {
		stuckFrames = 0;
	}

	// ---- Output movement keys ----
	if(nodeIdx < path.GetCount()) {
		const PathNode& node = path[nodeIdx];
		int dir = 0;
		if(node.col > selfCol) dir = 1;
		else if(node.col < selfCol) dir = -1;

		if(dir < 0) out.Set(ACT_LEFT);
		if(dir > 0) out.Set(ACT_RIGHT);

		// Jump: press on first frame of jump segment
		if(node.moveType == MOVE_JUMP && segFrame == 1) {
			out.Set(ACT_JUMP);
		}
		// Hold jump for height (dh * 8 frames)
		else if(node.moveType == MOVE_JUMP) {
			int dh = node.row - (nodeIdx > 0 ? path[nodeIdx - 1].row : selfRow);
			int holdFrames = max(1, dh * 8);
			if(segFrame <= holdFrames) {
				out.Set(ACT_JUMP);
			}
		}
	}

	// ---- Shoot check ----
	if(behavior->ShouldShoot(ctx)) {
		out.Set(ACT_SHOOT);
		out.Set(ACT_AIM_X, behavior->AimX(ctx));
	}

	return out;
}
