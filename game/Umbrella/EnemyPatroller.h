#ifndef _Umbrella_EnemyPatroller_h_
#define _Umbrella_EnemyPatroller_h_

#include "Enemy.h"
#include "AIController.h"
#include "EnemyBehaviors.h"

using namespace Upp;

class EnemyPatroller : public Enemy {
private:
	static constexpr float WALK_SPEED = 70.0f;

	AIController aiController;
	bool         aiEnabled = false;
	int          frameCounter = 0;

public:
	EnemyPatroller(VfsValue& v) : Enemy(v, ENEMY_PATROLLER) {}
	void Init(float x, float y, int spawnFacing = -1);

	virtual void Update(float delta, const Player& player, Player::CollisionHandler& collision) override;
	virtual void Render(Draw& w, Player::CoordinateConverter& coords) override;

	virtual void WireAI(Pathfinder* pf, const NavGraph* ng,
	                    const GameScreen* gs, int spawnCol, int spawnRow) override;
};

#endif
