#ifndef _Umbrella_EnemyBehaviors_h_
#define _Umbrella_EnemyBehaviors_h_

#include "AIController.h"

using namespace Upp;

// ============================================================================
// WandererBehavior
//
// "Lives its life" - explores the map by picking random reachable tiles.
// Settles into a loop when it reaches the edge of its comfortable range.
// Very low probability (1 in AGGRO_CHANCE) to chase the player.
//
// Archetypes: Dra-kun, Char, Cyclon, Keiten
// ============================================================================
class WandererBehavior : public EnemyBehavior {
public:
	static constexpr int AGGRO_CHANCE     = 300;  // 1/300 chance to chase player per replan
	static constexpr int WANDER_RADIUS    = 8;    // Max tiles away from home col to wander
	static constexpr int JITTER_FRAMES    = 5;    // Random direction change interval (in replans)

	WandererBehavior(int homeCol, int homeRow)
		: homeCol(homeCol), homeRow(homeRow), replanCount(0) {}

	bool ShouldRePlan(const BehaviorCtx& ctx) override {
		return ctx.pathComplete || ctx.stuck;
	}

	Point PickGoal(const BehaviorCtx& ctx) override;

private:
	int homeCol, homeRow;
	int replanCount;
};

// ============================================================================
// PatrolBehavior
//
// Simple back-and-forth between two fixed points on the same platform.
// Archetypes: Casta-kun (Fordon), Token/Coin
// ============================================================================
class PatrolBehavior : public EnemyBehavior {
public:
	PatrolBehavior(int col1, int row1, int col2, int row2)
		: colA(col1), rowA(row1), colB(col2), rowB(row2), goingToB(true) {}

	Point PickGoal(const BehaviorCtx& ctx) override;

private:
	int  colA, rowA, colB, rowB;
	bool goingToB;
};

// ============================================================================
// StalkerBehavior
//
// Always tries to move to the player's current tile.
// Replans every REPLAN_INTERVAL ticks (not just on completion) for tracking.
// Archetypes: Cyclon (aggressive), Knighton
// ============================================================================
class StalkerBehavior : public EnemyBehavior {
public:
	static constexpr int REPLAN_INTERVAL = 90;  // ~1.5 sec at 60fps

	StalkerBehavior() : replanTimer(0) {}

	bool ShouldRePlan(const BehaviorCtx& ctx) override;
	Point PickGoal(const BehaviorCtx& ctx) override;

private:
	int replanTimer;
	int lastPlayerCol = -1, lastPlayerRow = -1;
};

// ============================================================================
// ShooterBehavior
//
// Wanders like WandererBehavior but shoots the player when in the same row
// and within sight range (horizontal distance <= SHOOT_RANGE).
// Bullets travel slowly (not physics-based).
// Archetypes: Trooper, Gun-chan
// ============================================================================
class ShooterBehavior : public EnemyBehavior {
public:
	static constexpr int SHOOT_RANGE      = 12;   // tiles
	static constexpr int SHOOT_COOLDOWN   = 90;   // frames between shots
	static constexpr int AGGRO_CHANCE     = WandererBehavior::AGGRO_CHANCE;
	static constexpr int WANDER_RADIUS    = WandererBehavior::WANDER_RADIUS;

	ShooterBehavior(int homeCol, int homeRow)
		: homeCol(homeCol), homeRow(homeRow), shootTimer(0), replanCount(0) {}

	bool ShouldRePlan(const BehaviorCtx& ctx) override {
		return ctx.pathComplete || ctx.stuck;
	}
	Point PickGoal(const BehaviorCtx& ctx) override;
	bool  ShouldShoot(const BehaviorCtx& ctx) override;
	float AimX(const BehaviorCtx& ctx) override {
		return (ctx.playerX >= ctx.selfX) ? 1.0f : -1.0f;
	}

private:
	int homeCol, homeRow;
	int shootTimer;
	int replanCount;
};

// ============================================================================
// StationaryBehavior
//
// Never moves. Optionally shoots. Archetypes: Vio-kun
// ============================================================================
class StationaryBehavior : public EnemyBehavior {
public:
	explicit StationaryBehavior(bool shoots = false) : canShoot(shoots), shootTimer(0) {}

	bool ShouldRePlan(const BehaviorCtx&) override { return false; }
	Point PickGoal(const BehaviorCtx& ctx) override {
		return { ctx.selfCol, ctx.selfRow };  // stay put
	}
	bool ShouldShoot(const BehaviorCtx& ctx) override;
	float AimX(const BehaviorCtx& ctx) override {
		return (ctx.playerX >= ctx.selfX) ? 1.0f : -1.0f;
	}

private:
	bool canShoot;
	int  shootTimer;
};

#endif
