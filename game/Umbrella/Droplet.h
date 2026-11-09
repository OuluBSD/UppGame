#ifndef _Umbrella_Droplet_h_
#define _Umbrella_Droplet_h_

#include "GameEntity.h"
#include "Player.h"

using namespace Upp;

enum DropletType {
	DROPLET_RAINBOW,  // All colors cycling
	DROPLET_ICE,      // Cyan/blue
	DROPLET_FIRE      // Orange/red
};

// Spawn point for droplets (from editor)
struct DropletSpawnPoint {
	int col;
	int row;
	DropletType mode;
	int direction;        // -1 left, 1 right (horizontal velocity)
	int intervalMs;       // Spawn interval in milliseconds (0 for one-shot)
	bool enabled;
	float timer;          // Internal timer for spawning

	DropletSpawnPoint() : col(0), row(0), mode(DROPLET_RAINBOW),
	                      direction(1), intervalMs(2000), enabled(true), timer(0.0f) {}
};

class Droplet : public GameEntity {
private:
	Pointf position;
	DropletType type;
	bool collected;       // True if collected and orbiting player
	bool thrown;          // True if thrown horizontally (no gravity)
	bool isHuge;          // True if merged from 5 droplets (3× size)
	float orbitAngle;     // Angle in orbit around player (radians)
	float rotation;       // Rotation angle in degrees
	float rotationSpeed;  // Degrees per second
	float size;           // Droplet radius

	// Physics constants — 80s arcade style (constant speeds, no acceleration)
	static constexpr float FALL_SPEED = 60.0f;         // Constant downward speed (px/s)
	static constexpr float HORIZONTAL_SPEED = 50.0f;   // Horizontal throw speed (px/s)
	static constexpr float DROPLET_SIZE = 6.0f;        // Default droplet radius

	// Collision helpers
	bool CheckGroundCollision(Player::CollisionHandler& collision);
	bool CheckWallCollision(Player::CollisionHandler& collision);

public:
	CLASSTYPE(Droplet)

	Droplet(VfsValue& v) : GameEntity(v), type(DROPLET_RAINBOW), collected(false),
	                       thrown(false), isHuge(false),
	                       orbitAngle(0.0f), rotation(0.0f), rotationSpeed(0.0f),
	                       size(DROPLET_SIZE) {}
	void Init(float x, float y, DropletType t);

	void Update(float delta, Player::CollisionHandler& collision);
	void UpdateOrbit(float delta, Pointf playerPos, int dropletIndex, int totalDroplets);
	void Render(Draw& w, Player::CoordinateConverter& coords);

	bool IsActive() const { return active; }
	bool IsCollected() const { return collected; }
	bool IsThrown() const { return thrown; }
	bool IsHuge() const { return isHuge; }
	void Collect(float angle) {
		collected = true;
		thrown = false;
		orbitAngle = angle;
	}
	void Throw(int facing) {
		collected = false;
		thrown = true;
		velocity.x = facing * HORIZONTAL_SPEED;
		velocity.y = 0;
	}
	void MakeHuge() {
		isHuge = true;
		size = DROPLET_SIZE * 3;
	}
	void Deactivate() { active = false; }
	Pointf GetPosition() const { return position; }
	float GetSize() const { return size; }
	DropletType GetType() const { return type; }

	// Collision bounds (center + radius -> rect)
	Rectf GetBounds() const override {
		return Rectf(position.x - size, position.y - size,
		             position.x + size, position.y + size);
	}
};

#endif
