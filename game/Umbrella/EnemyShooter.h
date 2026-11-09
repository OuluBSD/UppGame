#ifndef _Umbrella_EnemyShooter_h_
#define _Umbrella_EnemyShooter_h_

#include "Enemy.h"
#include "Projectile.h"
#include "AIController.h"
#include "EnemyBehaviors.h"

using namespace Upp;

class EnemyShooter : public Enemy {
private:
	static constexpr float WALK_SPEED      = 60.0f;
	static constexpr float SHOOT_COOLDOWN  = 2.0f;
	static constexpr float DETECTION_RANGE = 15.0f * 14.0f;

	float              shootTimer;
	VfsValue           projectileRoot;  // Owns Projectile VfsValue nodes
	Array<Projectile*> projectiles;

	AIController aiController;
	bool         aiEnabled    = false;
	int          frameCounter = 0;

public:
	EnemyShooter(VfsValue& v) : Enemy(v, ENEMY_SHOOTER) { shootTimer = 0.0f; }
	void Init(float x, float y, int spawnFacing = 1);
	~EnemyShooter();

	virtual void Update(float delta, const Player& player, Player::CollisionHandler& collision) override;
	virtual void Render(Draw& w, Player::CoordinateConverter& coords) override;

	virtual void WireAI(Pathfinder* pf, const NavGraph* ng,
	                    const GameScreen* gs, int spawnCol, int spawnRow) override;

	// Projectile access
	const Array<Projectile*>& GetProjectiles() const { return projectiles; }

private:
	bool CanSeePlayer(const Player& player);
	void ShootAtPlayer(const Player& player);
	void ShootInDirection(int dir);   // dir: -1 left, 1 right
	void UpdateProjectiles(float delta, Player::CollisionHandler& collision);
};

#endif
