#include "Umbrella.h"
#include "EnemyShooter.h"
#include "Pathfinder.h"
#include "NavGraph.h"
#include "GameScreen.h"

using namespace Upp;

void EnemyShooter::WireAI(Pathfinder* pf, const NavGraph* ng,
                           const GameScreen* gs, int spawnCol, int spawnRow) {
	aiController.SetPathfinder(pf);
	aiController.SetNavGraph(ng);
	aiController.SetGameScreen(gs);
	aiController.SetBehavior(new ShooterBehavior(spawnCol, spawnRow));
	aiEnabled = true;
}

void EnemyShooter::Init(float x, float y, int spawnFacing)
{
	bounds = Rectf(x, y, x + 14, y + 14);
	originalSize = 14;
	facing = spawnFacing;
	velocity.x = 0.0f;
	velocity.y = 0.0f;
}

EnemyShooter::~EnemyShooter() {
	projectiles.Clear();       // Clear pointer array first
	projectileRoot.sub.Clear();    // VFS tree frees memory
}

bool EnemyShooter::CanSeePlayer(const Player& player) {
	Pointf playerPos = player.GetPosition();
	Pointf enemyCenter;
	enemyCenter.x = (bounds.left + bounds.right) / 2.0f;
	enemyCenter.y = (bounds.top + bounds.bottom) / 2.0f;

	// Calculate distance
	float dx = playerPos.x - enemyCenter.x;
	float dy = playerPos.y - enemyCenter.y;
	float distSq = dx * dx + dy * dy;

	// Check if within range
	if(distSq > DETECTION_RANGE * DETECTION_RANGE) {
		return false;
	}

	// Update facing direction based on player position
	facing = (dx > 0) ? 1 : -1;

	return true;
}

void EnemyShooter::ShootAtPlayer(const Player& player) {
	float spawnX = (bounds.left + bounds.right) / 2.0f - 3.0f;
	float spawnY = (bounds.top + bounds.bottom) / 2.0f - 3.0f;
	Projectile& p = projectileRoot.Add<Projectile>();
	p.Init(spawnX, spawnY, facing);
	projectiles.Add(&p);
}

void EnemyShooter::ShootInDirection(int dir) {
	facing = dir;
	float spawnX = (bounds.left + bounds.right) / 2.0f - 3.0f;
	float spawnY = (bounds.top + bounds.bottom) / 2.0f - 3.0f;
	Projectile& p = projectileRoot.Add<Projectile>();
	p.Init(spawnX, spawnY, dir);
	projectiles.Add(&p);
}

void EnemyShooter::UpdateProjectiles(float delta, Player::CollisionHandler& collision) {
	// Update all projectiles
	for(int i = 0; i < projectiles.GetCount(); i++) {
		projectiles[i]->Update(delta, collision);
	}

	// Remove inactive projectiles
	for(int i = projectiles.GetCount() - 1; i >= 0; i--) {
		if(!projectiles[i]->IsActive()) {
			projectileRoot.Remove(&projectiles[i]->val);
			projectiles.Remove(i);
		}
	}
}

void EnemyShooter::Update(float delta, const Player& player, Player::CollisionHandler& collision) {
	// Update rotation for visual effects (works for dead enemies too)
	UpdateRotation(delta);

	// Dead enemies: apply physics until ground collision
	if(!alive) {
		velocity.y += GRAVITY * delta;
		if(velocity.y < MAX_FALL_SPEED) velocity.y = MAX_FALL_SPEED;

		ResolveCollisionX(velocity.x * delta, collision);
		ResolveCollisionY(velocity.y * delta, collision);

		// Check if hit ground - will be handled in GameScreen for treat spawning
		// Don't update projectiles when dead
		return;
	}

	// If carried by thrown enemy, just apply movement (no AI, no gravity)
	if(carriedByThrown) {
		ResolveCollisionX(velocity.x * delta, collision);
		// Update projectiles even when carried
		UpdateProjectiles(delta, collision);
		return;
	}

	if(!active) {
		// Update projectiles even when captured/inactive
		UpdateProjectiles(delta, collision);
		return;
	}

	// If thrown, purely horizontal movement (no gravity)
	if(thrown) {
		// Check for wall collision when thrown
		if(CheckThrownWallCollision(velocity.x * delta, collision)) {
			// Mark for destruction - GameScreen will spawn treat
			// Use DefeatPreservingVelocity to keep horizontal momentum
			DefeatPreservingVelocity();
			return;
		}

		// No gravity - purely horizontal flight
		ResolveCollisionX(velocity.x * delta, collision);
		// No Y collision needed since no vertical movement
	}
	else {
		if(aiEnabled) {
			// AI-driven: ShooterBehavior wanders + shoots on player sight
			// Apply gravity so it stays on ground
			velocity.y += GRAVITY * delta;
			if(velocity.y < MAX_FALL_SPEED) velocity.y = MAX_FALL_SPEED;

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
				velocity.y = 280.0f;
			}

			if(acts.Has(ACT_SHOOT)) {
				int dir = (acts.Get(ACT_AIM_X) >= 0) ? 1 : -1;
				ShootInDirection(dir);
			}
		} else {
			// Fallback: stationary, shoot when player in range
			shootTimer += delta;
			if(CanSeePlayer(player) && shootTimer >= SHOOT_COOLDOWN) {
				ShootAtPlayer(player);
				shootTimer = 0.0f;
			}
		}
	}

	// Update projectiles
	UpdateProjectiles(delta, collision);
}

void EnemyShooter::Render(Draw& w, Player::CoordinateConverter& coords) {
	// Render projectiles first (behind enemy) - but not if dead
	if(alive) {
		for(int i = 0; i < projectiles.GetCount(); i++) {
			projectiles[i]->Render(w, coords);
		}
	}

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
	Color baseColor = Color(255, 255, 50);  // Yellow base color
	Color tint = GetTintColor();

	// Apply tint (simple color modulation)
	int r = (baseColor.GetR() * tint.GetR()) / 255;
	int g = (baseColor.GetG() * tint.GetG()) / 255;
	int b = (baseColor.GetB() * tint.GetB()) / 255;
	Color enemyColor = Color(r, g, b);

	w.DrawRect(screenX, screenY, width, height, enemyColor);

	// Draw facing direction indicator
	Color dirColor = Color(255, 100, 100);
	if(facing > 0) {
		w.DrawRect(screenX + width - 2, screenY + height/2 - 2, 4, 4, dirColor);
	} else {
		w.DrawRect(screenX - 2, screenY + height/2 - 2, 4, 4, dirColor);
	}
}
