#include "Umbrella.h"
#include "Enemy.h"
#include "GameSettings.h"

using namespace Upp;

Enemy::Enemy(VfsValue& v, EnemyType t) : GameEntity(v) {
	health = 1;
	captured = false;
	thrown = false;
	carriedByThrown = false;
	stateTimer = 0.0f;
	facing = -1;  // Start facing left
	type = t;
	carryWeight = 1.0f;
	originalSize = 0.0f;
	rotation = 0.0f;
}

Enemy::Enemy(VfsValue& v, float x, float y, float width, float height, EnemyType t)
    : GameEntity(v)
{
	bounds = Rectf(x, y, x + width, y + height);
	health = 1;
	captured = false;
	thrown = false;
	carriedByThrown = false;
	stateTimer = 0.0f;
	facing = -1;  // Start facing left
	type = t;
	carryWeight = 1.0f;
	originalSize = width;
	rotation = 0.0f;
}

void Enemy::TakeDamage(int amount) {
	health -= amount;
	if(health <= 0) {
		Kill();
	}
}

void Enemy::Kill() {
	alive = false;
	active = false;
}

void Enemy::Defeat() {
	// Killed by player parasol attack
	alive = false;
	active = false;  // Disable AI
	captured = false;
	thrown = false;
	carriedByThrown = false;

	// Apply random arc velocity for death animation
	velocity.x = GameSettings::DEAD_ENEMY_MIN_VX +
	             Randomf() * (GameSettings::DEAD_ENEMY_MAX_VX - GameSettings::DEAD_ENEMY_MIN_VX);
	velocity.y = GameSettings::DEAD_ENEMY_MIN_VY +
	             Randomf() * (GameSettings::DEAD_ENEMY_MAX_VY - GameSettings::DEAD_ENEMY_MIN_VY);

	// Dead enemies will continue moving with physics until ground impact
	// TODO: Play defeat sound
	// TODO: Spawn particle effect
}

void Enemy::DefeatPreservingVelocity() {
	// Killed while thrown (hit wall) - keep horizontal momentum but add upward bounce
	alive = false;
	active = false;
	captured = false;
	bool wasThrown = thrown;  // Remember if thrown
	thrown = false;
	carriedByThrown = false;

	// If was thrown, keep horizontal velocity direction but slightly reduce magnitude
	// Add upward bounce for visual effect
	if(wasThrown) {
		velocity.x *= 0.6f;  // Reduce to 60% of throw speed
		velocity.y = GameSettings::DEAD_ENEMY_MIN_VY +
		             Randomf() * (GameSettings::DEAD_ENEMY_MAX_VY - GameSettings::DEAD_ENEMY_MIN_VY);
	}
	else {
		// Fallback to normal defeat
		velocity.x = GameSettings::DEAD_ENEMY_MIN_VX +
		             Randomf() * (GameSettings::DEAD_ENEMY_MAX_VX - GameSettings::DEAD_ENEMY_MIN_VX);
		velocity.y = GameSettings::DEAD_ENEMY_MIN_VY +
		             Randomf() * (GameSettings::DEAD_ENEMY_MAX_VY - GameSettings::DEAD_ENEMY_MIN_VY);
	}
}

void Enemy::Capture() {
	// Enemy captured by player umbrella
	captured = true;
	active = false;
	velocity = Pointf(0, 0);  // Stop movement
}

void Enemy::ThrowFrom(float x, float y, float vx, float vy) {
	// Thrown by player - reactivate with new position and velocity
	bounds.left = x;
	bounds.right = x + bounds.Width();
	bounds.top = y;
	bounds.bottom = y + bounds.Height();
	velocity.x = vx;
	velocity.y = 0.0f;  // Thrown enemies move purely horizontal (gravity will pull down)
	captured = false;
	thrown = true;
	carriedByThrown = false;
	active = true;
}

void Enemy::CaptureByThrown(const Pointf& throwerVelocity) {
	// Captured by a thrown enemy - match velocity and disable AI
	velocity = throwerVelocity;
	carriedByThrown = true;
	active = false;  // Disable normal AI/updates
	captured = false;  // Not captured by player
	thrown = false;  // Not the thrower
}

bool Enemy::IsTouchingWallOnLeft(Player::CollisionHandler& collision) {
	int gridSize = (int)collision.GetGridSize();
	int checkCol = (int)((bounds.left - 1.0f) / gridSize);

	// Get Y range - remember bounds naming is backwards for Y-up!
	float minY = min(bounds.top, bounds.bottom);
	float maxY = max(bounds.top, bounds.bottom);
	int minRow = (int)(minY / gridSize);
	int maxRow = (int)(maxY / gridSize);

	for(int row = minRow; row <= maxRow; row++) {
		if(collision.IsWallTile(checkCol, row) || collision.IsFullBlockTile(checkCol, row)) {
			return true;
		}
	}
	return false;
}

bool Enemy::IsTouchingWallOnRight(Player::CollisionHandler& collision) {
	int gridSize = (int)collision.GetGridSize();
	int checkCol = (int)((bounds.right + 1.0f) / gridSize);

	// Get Y range - remember bounds naming is backwards for Y-up!
	float minY = min(bounds.top, bounds.bottom);
	float maxY = max(bounds.top, bounds.bottom);
	int minRow = (int)(minY / gridSize);
	int maxRow = (int)(maxY / gridSize);

	for(int row = minRow; row <= maxRow; row++) {
		if(collision.IsWallTile(checkCol, row) || collision.IsFullBlockTile(checkCol, row)) {
			return true;
		}
	}
	return false;
}

bool Enemy::IsOnGround(Player::CollisionHandler& collision) {
	int gridSize = (int)collision.GetGridSize();
	int minCol = (int)(bounds.left / gridSize);
	int maxCol = (int)(bounds.right / gridSize);

	// In Y-up, feet are at the LOWER Y value
	float feetY = min(bounds.top, bounds.bottom);
	int floorRow = (int)((feetY - 1.0f) / gridSize);

	for(int col = minCol; col <= maxCol; col++) {
		if(collision.IsFloorTile(col, floorRow)) {
			return true;
		}
	}
	return false;
}

bool Enemy::IsFloorAhead(Player::CollisionHandler& collision) {
	// Check if there's floor ahead in the direction enemy is facing
	int gridSize = (int)collision.GetGridSize();

	// Check one tile ahead
	int checkCol = facing < 0 ? (int)((bounds.left - gridSize) / gridSize)
	                           : (int)((bounds.right + gridSize) / gridSize);

	// Check at feet level
	float feetY = min(bounds.top, bounds.bottom);
	int floorRow = (int)((feetY - 1.0f) / gridSize);

	return collision.IsFloorTile(checkCol, floorRow);
}

void Enemy::ResolveCollisionX(float deltaX, Player::CollisionHandler& collision) {
	if(deltaX == 0.0f) return;

	float step = deltaX > 0 ? COLLISION_STEP : -COLLISION_STEP;
	float remaining = deltaX;

	while(abs(remaining) > COLLISION_STEP) {
		bounds.left += step;
		bounds.right += step;

		// Check collision
		int gridSize = (int)collision.GetGridSize();
		int minCol = (int)(bounds.left / gridSize);
		int maxCol = (int)(bounds.right / gridSize);

		// Get Y range - remember bounds naming is backwards for Y-up!
		float minY = min(bounds.top, bounds.bottom);
		float maxY = max(bounds.top, bounds.bottom);
		int minRow = (int)(minY / gridSize);
		int maxRow = (int)(maxY / gridSize);

		bool collided = false;
		for(int row = minRow; row <= maxRow; row++) {
			for(int col = minCol; col <= maxCol; col++) {
				if(collision.IsWallTile(col, row) || collision.IsFullBlockTile(col, row)) {
					collided = true;
					break;
				}
			}
			if(collided) break;
		}

		if(collided) {
			bounds.left -= step;
			bounds.right -= step;
			velocity.x = 0.0f;  // Stop horizontal movement for all enemies
			return;
		}

		remaining -= step;
	}

	// Apply remaining movement
	bounds.left += remaining;
	bounds.right += remaining;
}

void Enemy::ResolveCollisionY(float deltaY, Player::CollisionHandler& collision) {
	if(deltaY == 0.0f) return;

	float step = deltaY > 0 ? COLLISION_STEP : -COLLISION_STEP;
	float remaining = deltaY;

	while(abs(remaining) > COLLISION_STEP) {
		bounds.top += step;
		bounds.bottom += step;

		// Check collision
		int gridSize = (int)collision.GetGridSize();
		int minCol = (int)(bounds.left / gridSize);
		int maxCol = (int)(bounds.right / gridSize);

		// Get Y range - remember bounds naming is backwards for Y-up!
		float minY = min(bounds.top, bounds.bottom);
		float maxY = max(bounds.top, bounds.bottom);
		int minRow = (int)(minY / gridSize);
		int maxRow = (int)(maxY / gridSize);

		bool collided = false;
		for(int row = minRow; row <= maxRow; row++) {
			for(int col = minCol; col <= maxCol; col++) {
				if(collision.IsWallTile(col, row) || collision.IsFullBlockTile(col, row)) {
					collided = true;
					break;
				}
			}
			if(collided) break;
		}

		if(collided) {
			bounds.top -= step;
			bounds.bottom -= step;
			velocity.y = 0.0f;
			return;
		}

		remaining -= step;
	}

	// Apply remaining movement
	bounds.top += remaining;
	bounds.bottom += remaining;
}

bool Enemy::CheckThrownWallCollision(float deltaX, Player::CollisionHandler& collision) {
	// Only check if thrown enemy is moving horizontally
	if(!thrown || deltaX == 0.0f) return false;

	// Check if enemy will hit wall in movement direction
	int gridSize = (int)collision.GetGridSize();

	// Get Y range
	float minY = min(bounds.top, bounds.bottom);
	float maxY = max(bounds.top, bounds.bottom);
	int minRow = (int)(minY / gridSize);
	int maxRow = (int)(maxY / gridSize);

	// Check column ahead of movement
	int checkCol = deltaX > 0 ? (int)((bounds.right + abs(deltaX)) / gridSize)
	                           : (int)((bounds.left + deltaX) / gridSize);

	// Check if wall exists in path
	for(int row = minRow; row <= maxRow; row++) {
		if(collision.IsWallTile(checkCol, row) || collision.IsFullBlockTile(checkCol, row)) {
			// Wall collision detected - mark enemy for destruction
			return true;
		}
	}

	return false;
}

void Enemy::UpdateRotation(float delta) {
	// Spin when captured or thrown (deactivated state)
	if(captured || thrown || carriedByThrown) {
		rotation += GameSettings::ENEMY_ROTATION_SPEED * delta;
		// Keep rotation in 0-360 range
		while(rotation >= 360.0f) rotation -= 360.0f;
	}
	else {
		rotation = 0.0f;  // Reset rotation when active
	}
}

Color Enemy::GetTintColor() const {
	if(!alive) {
		// Dead enemy - red tint
		return Color(GameSettings::DEAD_TINT_R,
		             GameSettings::DEAD_TINT_G,
		             GameSettings::DEAD_TINT_B);
	}
	else if(captured || thrown || carriedByThrown) {
		// Deactivated enemy - green tint
		return Color(GameSettings::DEACTIVATED_TINT_R,
		             GameSettings::DEACTIVATED_TINT_G,
		             GameSettings::DEACTIVATED_TINT_B);
	}
	else {
		// Normal enemy - no tint (white = no color change)
		return Color(GameSettings::NORMAL_TINT_R,
		             GameSettings::NORMAL_TINT_G,
		             GameSettings::NORMAL_TINT_B);
	}
}
