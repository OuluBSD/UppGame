#ifndef _Umbrella_Projectile_h_
#define _Umbrella_Projectile_h_

#include <Core/Core.h>
#include "GameEntity.h"
#include "Player.h"

using namespace Upp;

class Projectile : public GameEntity {
	int direction;  // -1 left, 1 right

	static constexpr float PROJECTILE_SIZE = 6.0f;
	static constexpr float PROJECTILE_SPEED = 90.0f;

public:
	CLASSTYPE(Projectile)

	Projectile(VfsValue& v) : GameEntity(v), direction(1) {}
	void Init(float x, float y, int dir);

	void Update(float delta, Player::CollisionHandler& collision);
	void Render(Draw& w, Player::CoordinateConverter& coords);

	bool IsActive()       const { return active; }
	Rectf GetBounds()     const override { return bounds; }
	void Deactivate()           { active = false; }

private:
	bool CheckWallCollision(Player::CollisionHandler& collision);
};

#endif
