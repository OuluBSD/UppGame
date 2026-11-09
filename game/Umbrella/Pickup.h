#ifndef _Umbrella_Pickup_h_
#define _Umbrella_Pickup_h_

#include <Core/Core.h>
#include "GameEntity.h"
#include "Player.h"

using namespace Upp;

enum PickupType { PU_HEART, PU_GEM, PU_LIGHTNING, PU_SPEED };

struct PickupSpawnPoint : Moveable<PickupSpawnPoint> {
	int col = 0;
	int row = 0;
	PickupType type = PU_HEART;
};

class Pickup : public GameEntity {
	PickupType type;
	float  bobTimer;   // drives sinusoidal vertical offset

	static constexpr float SIZE      = 12.0f;
	static constexpr float BOB_SPEED = 2.5f;   // radians/sec
	static constexpr float BOB_AMP   = 3.0f;   // pixels

public:
	CLASSTYPE(Pickup)

	Pickup(VfsValue& v) : GameEntity(v), type(PU_HEART), bobTimer(0.0f) {}
	void Init(float x, float y, PickupType t);

	void Update(float delta);
	void Render(Draw& w, Player::CoordinateConverter& coords);

	bool        IsActive()   const { return active; }
	void        Collect()          { active = false; }
	Rectf       GetBounds()  const override { return bounds; }
	PickupType  GetType()    const { return type; }
};

#endif
