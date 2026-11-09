#ifndef _Umbrella_GrimReaper_h_
#define _Umbrella_GrimReaper_h_

#include "GameEntity.h"
#include "Player.h"

using namespace Upp;

// Indestructible time-pressure entity.
// Spawns after SPAWN_DELAY seconds; slowly homes in on the player;
// touching it is instant death. Cannot be captured, damaged, or thrown.
class GrimReaper : public GameEntity {
	VfsValue selfNode;  // Owned VfsValue for standalone (non-VFS-tree) creation

	static constexpr float WIDTH       = 14.0f;
	static constexpr float HEIGHT      = 20.0f;
	static constexpr float STALK_SPEED = 38.0f;  // px/s
	static constexpr float BOB_SPEED   = 2.0f;   // rad/s
	static constexpr float BOB_AMP     = 5.0f;   // px
	static constexpr float SPAWN_DELAY = 30.0f;  // seconds after level load

	float  bobTimer   = 0.0f;
	bool   spawned    = false;   // visible and chasing
	float  spawnTimer = 0.0f;   // counts up to SPAWN_DELAY

	// Pulse effect for pre-spawn warning
	float  warningPulse = 0.0f;

public:
	CLASSTYPE(GrimReaper)

	// VFS constructor (for tree-based creation)
	GrimReaper(VfsValue& v) : GameEntity(v) { Reset(); }
	// Standalone constructor (delegates via selfNode)
	GrimReaper() : GrimReaper(selfNode) {}

	// Call once per GameTick. spawnX/Y = off-screen entry position.
	void Update(float delta, const Player& player, float spawnX, float spawnY);

	void Render(Draw& w, Player::CoordinateConverter& coords, int screenW, int screenH);

	// Returns true when player should die (spawned + overlapping + player not invincible)
	bool TouchesPlayer(const Player& player) const;

	// True once GrimReaper has entered the field
	bool IsSpawned()  const { return spawned; }

	// Seconds until spawn (negative = already spawned)
	float TimeUntilSpawn() const { return SPAWN_DELAY - spawnTimer; }

	Rectf GetBounds() const override { return bounds; }

	void Reset();
};

#endif
