#ifndef _Umbrella_WaterWeapon_h_
#define _Umbrella_WaterWeapon_h_

#include "GameEntity.h"
#include "Player.h"

using namespace Upp;

class Enemy;

class WaterWeapon : public GameEntity {
private:
	VfsValue selfNode;  // Owned VfsValue for standalone (non-VFS-tree) creation

	int col, row;              // Current grid position
	int directionX;            // -1 or +1 (horizontal movement direction)
	bool falling;              // true = moving down, false = moving horizontal
	float stepTimer;           // Accumulator for grid-step timing
	int levelRows;             // Cached level height for bounds check

	// Trail (last N grid positions for visual effect)
	static constexpr int TRAIL_LENGTH = 8;
	Vector<Point> trail;

	// Attached enemies (riding the water to their doom)
	Vector<Enemy*> attached;

public:
	static constexpr float STEP_INTERVAL = 0.2f;  // ~5 steps/sec

	CLASSTYPE(WaterWeapon)

	// VFS constructor (for tree-based creation)
	WaterWeapon(VfsValue& v) : GameEntity(v), col(0), row(0), directionX(1),
	                            falling(true), stepTimer(0.0f), levelRows(0) {
		active = false;
	}
	// Standalone constructor
	WaterWeapon() : WaterWeapon(selfNode) {}

	void Activate(int startCol, int startRow, int facing);
	void Update(float delta, Player::CollisionHandler& collision, Array<Enemy*>& enemies);
	void Render(Draw& w, Player::CoordinateConverter& coords, int gridSize);

	// Release all attached enemies (called when water is destroyed)
	// Returns the attached enemies so caller can apply death-arc physics
	Vector<Enemy*> Release();

	bool IsActive() const { return active; }
	int GetCol() const { return col; }
	int GetRow() const { return row; }

private:
	void Step(Player::CollisionHandler& collision);
	void CheckEnemyAttachment(Array<Enemy*>& enemies, int gridSize);
	bool IsSolid(int c, int r, Player::CollisionHandler& collision);
};

#endif
