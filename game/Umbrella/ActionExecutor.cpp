#include "Umbrella.h"
#include "ActionExecutor.h"
#include "GameScreen.h"

using namespace Upp;

// ============================================================================
// Helpers
// ============================================================================

int ActionExecutor::TileCol(const GameScreen* s) const {
	return (int)(s->player.GetCenter().x / GRID);
}

int ActionExecutor::TileRow(const GameScreen* s) const {
	return (int)(s->player.GetCenter().y / GRID);
}

bool ActionExecutor::AtTile(const GameScreen* s, int col, int row) const {
	if(!s->player.IsOnGround()) return false;
	int pc = TileCol(s);
	int pr = TileRow(s);
	return abs(pc - col) <= COL_TOL && pr == row;
}

void ActionExecutor::Tick(GameScreen* s) const {
	s->GameTick((float)GameScreen::FIXED_TIMESTEP);
}

void ActionExecutor::SetDir(GameScreen* s, int dir) const {
	s->keyLeft  = (dir < 0);
	s->keyRight = (dir > 0);
}

void ActionExecutor::ClearAll(GameScreen* s) const {
	s->keyLeft = s->keyRight = s->keyJump = s->keyAttack = false;
}

// ============================================================================
// ExecutePath
// ============================================================================

ActionExecutor::Result ActionExecutor::ExecutePath(GameScreen* screen,
                                                    const Vector<PathNode>& path,
                                                    int maxFrames) {
	ClearAll(screen);

	if(path.IsEmpty()) {
		return { true, 0, 0, "empty_path" };
	}

	// Single-node path: already at goal
	if(path.GetCount() == 1) {
		return { true, 0, 1, "success" };
	}

	int totalFrames = 0;
	int nodesDone   = 0;

	for(int i = 0; i + 1 < path.GetCount() && totalFrames < maxFrames; i++) {
		const PathNode& from = path[i];
		const PathNode& to   = path[i + 1];
		MoveType        mt   = to.moveType;

		int dh  = to.row - from.row;   // positive = going up in Y-UP (jump)
		int dir = (to.col > from.col) ? 1 : (to.col < from.col) ? -1 : 0;

		int timeout = WALK_TIMEOUT;
		if(mt == MOVE_JUMP) timeout = JUMP_TIMEOUT;
		else if(mt == MOVE_FALL) timeout = FALL_TIMEOUT;

		// ---- Walk ----
		if(mt == MOVE_WALK) {
			SetDir(screen, dir);
			for(int f = 0; f < timeout && totalFrames < maxFrames; f++, totalFrames++) {
				Tick(screen);
				if(AtTile(screen, to.col, to.row)) break;
			}
		}
		// ---- Jump ----
		else if(mt == MOVE_JUMP) {
			// Press jump on frame 0, hold for (dh * 8) frames to reach target height
			// then release (cuts to MIN_JUMP_VELOCITY for small jumps).
			int holdFrames = max(1, dh * 8);
			screen->keyJump = true;
			SetDir(screen, dir);
			for(int f = 0; f < timeout && totalFrames < maxFrames; f++, totalFrames++) {
				if(f >= holdFrames)
					screen->keyJump = false;
				Tick(screen);
				if(AtTile(screen, to.col, to.row)) break;
			}
			screen->keyJump = false;
		}
		// ---- Fall ----
		else {  // MOVE_FALL
			SetDir(screen, dir);
			for(int f = 0; f < timeout && totalFrames < maxFrames; f++, totalFrames++) {
				Tick(screen);
				if(AtTile(screen, to.col, to.row)) break;
			}
		}

		ClearAll(screen);  // Release keys between nodes to let physics settle

		if(!AtTile(screen, to.col, to.row)) {
			LOG("ActionExecutor: stuck at node " << (i + 1)
			    << " target=(" << to.col << "," << to.row << ")"
			    << " player=(" << TileCol(screen) << "," << TileRow(screen) << ")"
			    << " onGround=" << screen->player.IsOnGround());
			return { false, totalFrames, nodesDone, "stuck_at_" + AsString(i + 1) };
		}

		nodesDone = i + 1;
		LOG("ActionExecutor: reached node " << nodesDone
		    << " (" << to.col << "," << to.row << ")");
	}

	ClearAll(screen);
	LOG("ActionExecutor: path complete, " << nodesDone << " nodes, " << totalFrames << " frames");
	return { true, totalFrames, nodesDone, "success" };
}
