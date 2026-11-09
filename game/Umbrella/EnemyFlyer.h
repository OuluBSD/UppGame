#ifndef _Umbrella_EnemyFlyer_h_
#define _Umbrella_EnemyFlyer_h_

#include "Enemy.h"

using namespace Upp;

// Gravity-free stalker that homes in on the player.
// Immune to tile collision while alive; falls under gravity when defeated.
class EnemyFlyer : public Enemy {
	static constexpr float STALK_SPEED = 55.0f;
	static constexpr float BOB_SPEED   = 3.0f;   // rad/s
	static constexpr float BOB_AMP     = 4.0f;   // pixels

	float bobTimer = 0.0f;

public:
	EnemyFlyer(VfsValue& v) : Enemy(v, ENEMY_FLYER) { carryWeight = 0.8f; }
	void Init(float x, float y, int spawnFacing = -1);

	virtual void Update(float delta, const Player& player, Player::CollisionHandler& collision) override;
	virtual void Render(Draw& w, Player::CoordinateConverter& coords) override;
};

#endif
