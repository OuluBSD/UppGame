#include "Umbrella.h"
#include "EnemyJumper.h"
#include "Pathfinder.h"
#include "NavGraph.h"
#include "GameScreen.h"

using namespace Upp;

void EnemyJumper::WireAI(Pathfinder* pf, const NavGraph* ng,
                          const GameScreen* gs, int spawnCol, int spawnRow) {
	aiController.SetPathfinder(pf);
	aiController.SetNavGraph(ng);
	aiController.SetGameScreen(gs);
	aiController.SetBehavior(new StalkerBehavior());
	(void)spawnCol; (void)spawnRow;
	aiEnabled = true;
}

void EnemyJumper::Init(float x, float y, int spawnFacing)
{
	bounds = Rectf(x, y, x + 12, y + 12);
	originalSize = 12;
	facing = spawnFacing;
	velocity.x = spawnFacing * -WALK_SPEED;
}

void EnemyJumper::ResetJumpTimer() {
	// Random jump interval between MIN and MAX
	nextJumpTime = MIN_JUMP_INTERVAL + (Randomf() * (MAX_JUMP_INTERVAL - MIN_JUMP_INTERVAL));
	jumpTimer = 0.0f;
}

bool EnemyJumper::IsOnGround(Player::CollisionHandler& collision) {
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

void EnemyJumper::Update(float delta, const Player& player, Player::CollisionHandler& collision) {
	// Update rotation for visual effects (works for dead enemies too)
	UpdateRotation(delta);

	// Dead enemies: apply physics until ground collision
	if(!alive) {
		velocity.y += GRAVITY * delta;
		if(velocity.y < MAX_FALL_SPEED) velocity.y = MAX_FALL_SPEED;

		ResolveCollisionX(velocity.x * delta, collision);
		ResolveCollisionY(velocity.y * delta, collision);

		// Check if hit ground - will be handled in GameScreen for treat spawning
		return;
	}

	// If carried by thrown enemy, just apply movement (no AI, no gravity)
	if(carriedByThrown) {
		ResolveCollisionX(velocity.x * delta, collision);
		return;
	}

	if(!active) return;

	// If thrown, skip both gravity and AI - purely horizontal movement
	if(!thrown) {
		// Apply gravity
		velocity.y += GRAVITY * delta;
		if(velocity.y < MAX_FALL_SPEED) velocity.y = MAX_FALL_SPEED;

		if(aiEnabled) {
			// AI-driven via StalkerBehavior: chases player using pathfinding
			bool onGround = IsOnGround(collision);
			ActionSet acts = aiController.Update(bounds, onGround, ++frameCounter);

			if(acts.Has(ACT_LEFT)) {
				velocity.x = -WALK_SPEED;
				facing = -1;
			} else if(acts.Has(ACT_RIGHT)) {
				velocity.x = WALK_SPEED;
				facing = 1;
			} else {
				velocity.x = 0;
			}

			if(acts.Has(ACT_JUMP) && onGround) {
				velocity.y = JUMP_VELOCITY;
			}
		} else {
			// Fallback: timer-based jumper patrol
			velocity.x = facing * WALK_SPEED;

			bool hitWall = (facing < 0 && IsTouchingWallOnLeft(collision)) ||
			               (facing > 0 && IsTouchingWallOnRight(collision));
			bool noFloorAhead = !IsFloorAhead(collision) && IsOnGround(collision);

			if(hitWall || noFloorAhead) {
				facing = -facing;
				velocity.x = facing * WALK_SPEED;
				if(IsOnGround(collision)) {
					velocity.y = JUMP_VELOCITY;
					ResetJumpTimer();
				}
			}

			jumpTimer += delta;
			if(jumpTimer >= nextJumpTime && IsOnGround(collision)) {
				velocity.y = JUMP_VELOCITY;
				ResetJumpTimer();
			}
		}
	}
	else {
		// Check for wall collision when thrown
		if(CheckThrownWallCollision(velocity.x * delta, collision)) {
			// Mark for destruction - GameScreen will spawn treat
			// Use DefeatPreservingVelocity to keep horizontal momentum
			DefeatPreservingVelocity();
			return;
		}
	}

	// Apply movement with collision
	ResolveCollisionX(velocity.x * delta, collision);
	ResolveCollisionY(velocity.y * delta, collision);

	// Kill if falls off map
	float feetY = min(bounds.top, bounds.bottom);
	if(feetY < -collision.GetGridSize() * 2) {
		Kill();
	}
}

void EnemyJumper::Render(Draw& w, Player::CoordinateConverter& coords) {
	// Render dead enemies too (they fly through air with red tint)

	// Get world-space corners
	Point topLeft((int)bounds.left, (int)bounds.top);
	Point bottomRight((int)bounds.right, (int)bounds.bottom);

	// Convert to screen space (handles Y-flip)
	Point screenTopLeft = coords.WorldToScreen(topLeft);
	Point screenBottomRight = coords.WorldToScreen(bottomRight);

	// Normalize to get proper screen rect
	int screenX = min(screenTopLeft.x, screenBottomRight.x);
	int screenY = min(screenTopLeft.y, screenBottomRight.y);
	int width = abs(screenBottomRight.x - screenTopLeft.x);
	int height = abs(screenBottomRight.y - screenTopLeft.y);

	// Draw enemy with state-based tint
	Color baseColor = Color(50, 255, 50);  // Green base color
	Color tint = GetTintColor();

	// Apply tint (simple color modulation)
	int r = (baseColor.GetR() * tint.GetR()) / 255;
	int g = (baseColor.GetG() * tint.GetG()) / 255;
	int b = (baseColor.GetB() * tint.GetB()) / 255;
	Color enemyColor = Color(r, g, b);

	w.DrawRect(screenX, screenY, width, height, enemyColor);

	// Draw facing direction indicator
	Color dirColor = Color(255, 255, 0);
	if(facing > 0) {
		w.DrawRect(screenX + width - 2, screenY + height/2 - 2, 4, 4, dirColor);
	} else {
		w.DrawRect(screenX - 2, screenY + height/2 - 2, 4, 4, dirColor);
	}
}
