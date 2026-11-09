#ifndef _Umbrella_AIController_h_
#define _Umbrella_AIController_h_

#include "ActionSet.h"
#include "PathNode.h"
#include "Pathfinder.h"
#include "NavGraph.h"

using namespace Upp;

// Forward declarations
class GameScreen;
class Player;

// ============================================================================
// BehaviorCtx - snapshot of world state passed to EnemyBehavior each tick
// ============================================================================
struct BehaviorCtx {
	// Enemy position (tile coords)
	int   selfCol, selfRow;
	float selfX,   selfY;    // World coords (center of enemy)
	bool  onGround;

	// Path state
	bool pathComplete;
	bool stuck;           // True if stuck counter exceeded threshold

	// Player info
	int   playerCol, playerRow;
	float playerX,   playerY;
	bool  playerVisible;  // Line-of-sight (not behind wall) -- approximate

	// Level info
	const GameScreen* screen;
	const NavGraph*   navGraph;  // Optional: for reachability checks in behaviors

	int   frame;          // Global frame counter for timing variety
};

// ============================================================================
// EnemyBehavior - pluggable strategy; override to implement AI personality
// ============================================================================
class EnemyBehavior {
public:
	virtual ~EnemyBehavior() {}

	// Fast check: should we pick a new goal right now?
	// Called every tick. Default: only when path is complete or stuck.
	virtual bool ShouldRePlan(const BehaviorCtx& ctx) {
		return ctx.pathComplete || ctx.stuck;
	}

	// Pick next goal tile. Called when ShouldRePlan returns true.
	// Return (-1,-1) to stand still.
	virtual Point PickGoal(const BehaviorCtx& ctx) = 0;

	// Should we shoot this tick?  (Optionally called after Update)
	virtual bool ShouldShoot(const BehaviorCtx& ctx) { (void)ctx; return false; }

	// Aim direction if shooting (normalized X). Default: face toward player.
	virtual float AimX(const BehaviorCtx& ctx) {
		return (ctx.playerX >= ctx.selfX) ? 1.0f : -1.0f;
	}
};

// ============================================================================
// AIController - real-time per-tick path follower
//
// Each call to Update():
//   1. Build BehaviorCtx from current world state
//   2. If behavior says ShouldRePlan -> FindPath -> reset nodeIdx
//   3. Advance along current path one node
//   4. Return ActionSet with movement keys to press
// ============================================================================
class AIController {
public:
	// Timing constants
	static constexpr int WALK_TIMEOUT  = 40;   // frames per walk step
	static constexpr int JUMP_TIMEOUT  = 60;
	static constexpr int FALL_TIMEOUT  = 60;
	static constexpr int COL_TOL       = 1;    // tile column tolerance
	static constexpr int STUCK_LIMIT   = 30;   // frames without progress = stuck
	static constexpr int GRID          = 14;   // pixels per tile

	AIController();

	// Attach pathfinder and nav graph (owned externally)
	void SetPathfinder(Pathfinder* pf) { pathfinder = pf; }
	void SetNavGraph(const NavGraph* ng) { navGraph = ng; }

	// Set behavior (AIController takes ownership)
	void SetBehavior(EnemyBehavior* b) { behavior = b; }

	// Attach game screen (set at init time, doesn't change per tick)
	void SetGameScreen(const GameScreen* s) { screen = s; }

	// Main entry point: call once per game tick.
	// enemyBounds.CenterPoint() used for position.
	ActionSet Update(const Rectf& enemyBounds, bool onGround, int frame);

	// Force a replan next tick
	void Invalidate() { pathValid = false; }

	// Debug
	int  GetNodeIdx()   const { return nodeIdx; }
	bool IsPathValid()  const { return pathValid; }

private:
	Pathfinder*         pathfinder = nullptr;
	const NavGraph*     navGraph   = nullptr;
	const GameScreen*   screen     = nullptr;
	One<EnemyBehavior>  behavior;

	Vector<PathNode>    path;
	int                 nodeIdx   = 0;
	bool                pathValid = false;

	// State tracking within current segment
	int                 segFrame  = 0;    // frames spent on current segment
	int                 stuckFrames = 0;
	Point               lastGoal  = { -1, -1 };

	// Goal tile for current path
	int                 goalCol = -1, goalRow = -1;

	// Helpers
	int  TileCol(float worldX) const { return (int)(worldX / GRID); }
	int  TileRow(float worldY) const { return (int)(worldY / GRID); }
	bool AtTile(float worldX, float worldY, bool onGround, int col, int row) const;

	BehaviorCtx MakeCtx(int selfCol, int selfRow, float selfX, float selfY,
	                    bool onGround, bool pathComplete, bool stuck,
	                    const GameScreen* screen, int frame) const;
};

#endif
