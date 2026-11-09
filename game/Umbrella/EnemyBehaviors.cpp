#include "Umbrella.h"
#include "EnemyBehaviors.h"
#include "GameScreen.h"
#include "NavGraph.h"

using namespace Upp;

// ============================================================================
// Helpers
// ============================================================================

// Pick a random walkable tile within the same component as (col, row),
// within a bounded horizontal search range. Falls back to self if none found.
static Point PickRandomReachableTile(const NavGraph* nav,
                                     int selfCol, int selfRow,
                                     int minCol, int maxCol,
                                     int frame) {
	if(!nav || !nav->IsBuilt()) {
		// No NavGraph: just pick random column on same row
		int spread = maxCol - minCol;
		if(spread <= 0) return { selfCol, selfRow };
		int col = minCol + ((frame * 1103515245 + 12345) & 0x7fff) % (spread + 1);
		return { col, selfRow };
	}

	int myComp = nav->GetComponentId(selfCol, selfRow);
	if(myComp == NavGraph::INVALID_COMPONENT) return { selfCol, selfRow };

	// Collect candidates in range
	Vector<Point> candidates;
	for(int c = minCol; c <= maxCol; c++) {
		for(int r = selfRow - 2; r <= selfRow + 2; r++) {
			if(c == selfCol && r == selfRow) continue;
			if(nav->GetComponentId(c, r) == myComp) {
				candidates.Add({ c, r });
			}
		}
	}
	if(candidates.IsEmpty()) return { selfCol, selfRow };

	// Pseudo-random pick using frame as seed variation
	int idx = ((frame * 1103515245 + 12345) & 0x7fffffff) % candidates.GetCount();
	return candidates[idx];
}

// ============================================================================
// WandererBehavior
// ============================================================================

Point WandererBehavior::PickGoal(const BehaviorCtx& ctx) {
	replanCount++;

	// Low probability: chase player
	if(ctx.playerVisible) {
		int roll = ((ctx.frame * 1103515245 + replanCount * 6364136223) & 0x7fffffff) % AGGRO_CHANCE;
		if(roll == 0) {
			return { ctx.playerCol, ctx.playerRow };
		}
	}

	// Wander within radius around home
	int minC = max(0, homeCol - WANDER_RADIUS);
	int maxC = homeCol + WANDER_RADIUS;

	// Clamp to level bounds
	if(ctx.screen) {
		const Layer* terrain = ctx.screen->layerManager.FindLayerByType(LAYER_TERRAIN);
		if(terrain) {
			maxC = min(maxC, terrain->GetGrid().GetColumns() - 1);
		}
	}

	return PickRandomReachableTile(ctx.navGraph, ctx.selfCol, ctx.selfRow,
	                               minC, maxC, ctx.frame + replanCount * 31337);
}

// ============================================================================
// PatrolBehavior
// ============================================================================

Point PatrolBehavior::PickGoal(const BehaviorCtx& ctx) {
	(void)ctx;
	if(goingToB) {
		goingToB = false;
		return { colB, rowB };
	} else {
		goingToB = true;
		return { colA, rowA };
	}
}

// ============================================================================
// StalkerBehavior
// ============================================================================

bool StalkerBehavior::ShouldRePlan(const BehaviorCtx& ctx) {
	replanTimer++;
	if(ctx.pathComplete || ctx.stuck) return true;
	// Also replan if player moved to a different tile
	if(ctx.playerCol != lastPlayerCol || ctx.playerRow != lastPlayerRow)
		if(replanTimer >= REPLAN_INTERVAL) return true;
	return false;
}

Point StalkerBehavior::PickGoal(const BehaviorCtx& ctx) {
	lastPlayerCol = ctx.playerCol;
	lastPlayerRow = ctx.playerRow;
	replanTimer   = 0;
	return { ctx.playerCol, ctx.playerRow };
}

// ============================================================================
// ShooterBehavior
// ============================================================================

Point ShooterBehavior::PickGoal(const BehaviorCtx& ctx) {
	replanCount++;

	// Low probability: chase player
	if(ctx.playerVisible) {
		int roll = ((ctx.frame * 1103515245 + replanCount * 6364136223) & 0x7fffffff) % AGGRO_CHANCE;
		if(roll == 0) {
			return { ctx.playerCol, ctx.playerRow };
		}
	}

	int minC = max(0, homeCol - WANDER_RADIUS);
	int maxC = homeCol + WANDER_RADIUS;

	if(ctx.screen) {
		const Layer* terrain = ctx.screen->layerManager.FindLayerByType(LAYER_TERRAIN);
		if(terrain) {
			maxC = min(maxC, terrain->GetGrid().GetColumns() - 1);
		}
	}

	return PickRandomReachableTile(ctx.navGraph, ctx.selfCol, ctx.selfRow,
	                               minC, maxC, ctx.frame + replanCount * 31337);
}

bool ShooterBehavior::ShouldShoot(const BehaviorCtx& ctx) {
	shootTimer++;

	if(shootTimer < SHOOT_COOLDOWN) return false;

	// Same row, within range, player visible
	if(abs(ctx.selfRow - ctx.playerRow) > 1) return false;
	if(abs(ctx.selfCol - ctx.playerCol) > SHOOT_RANGE) return false;
	if(!ctx.playerVisible) return false;

	shootTimer = 0;
	return true;
}

// ============================================================================
// StationaryBehavior
// ============================================================================

bool StationaryBehavior::ShouldShoot(const BehaviorCtx& ctx) {
	if(!canShoot) return false;

	shootTimer++;
	if(shootTimer < ShooterBehavior::SHOOT_COOLDOWN) return false;

	if(abs(ctx.selfRow - ctx.playerRow) > 1) return false;
	if(abs(ctx.selfCol - ctx.playerCol) > ShooterBehavior::SHOOT_RANGE) return false;

	shootTimer = 0;
	return true;
}
