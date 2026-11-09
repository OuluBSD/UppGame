#include "Umbrella.h"
#include "Droplet.h"

using namespace Upp;

void Droplet::Init(float x, float y, DropletType t) {
	position = Pointf(x, y);
	velocity = Pointf(0, 0);  // Start stationary — will fall at constant speed
	type = t;
	active = true;
	collected = false;
	thrown = false;
	isHuge = false;
	orbitAngle = 0.0f;
	rotation = 0.0f;
	rotationSpeed = 180.0f + Randomf() * 180.0f;  // 180-360 degrees per second
	size = DROPLET_SIZE;
}

bool Droplet::CheckGroundCollision(Player::CollisionHandler& collision) {
	int gridSize = (int)collision.GetGridSize();

	// Check tiles at droplet's bottom edge
	int minCol = (int)((position.x - size) / gridSize);
	int maxCol = (int)((position.x + size) / gridSize);

	// In Y-up, bottom is at lower Y value
	float bottomY = position.y - size;
	int floorRow = (int)(bottomY / gridSize);

	for(int col = minCol; col <= maxCol; col++) {
		if(collision.IsFloorTile(col, floorRow)) {
			return true;
		}
	}
	return false;
}

bool Droplet::CheckWallCollision(Player::CollisionHandler& collision) {
	int gridSize = (int)collision.GetGridSize();

	// Check tiles at droplet's edges
	int leftCol = (int)((position.x - size) / gridSize);
	int rightCol = (int)((position.x + size) / gridSize);

	// Get Y range
	float minY = position.y - size;
	float maxY = position.y + size;
	int minRow = (int)(minY / gridSize);
	int maxRow = (int)(maxY / gridSize);

	for(int row = minRow; row <= maxRow; row++) {
		if(collision.IsWallTile(leftCol, row) || collision.IsFullBlockTile(leftCol, row) ||
		   collision.IsWallTile(rightCol, row) || collision.IsFullBlockTile(rightCol, row)) {
			return true;
		}
	}
	return false;
}

void Droplet::Update(float delta, Player::CollisionHandler& collision) {
	if(!active || collected) return;  // Skip physics if collected (orbiting player)

	int gridSize = (int)collision.GetGridSize();

	if(thrown) {
		// Thrown mode: horizontal only, no gravity
		position.x += velocity.x * delta;

		// Wall collision → deactivate
		if(CheckWallCollision(collision)) {
			active = false;
			return;
		}
	}
	else {
		// Normal mode: constant-speed falling (80s arcade style)
		velocity.y = -FALL_SPEED;
		position.y += velocity.y * delta;

		// Ground collision → stop on ground
		if(CheckGroundCollision(collision)) {
			velocity.y = 0;
			// Snap to ground
			float bottomY = position.y - size;
			int floorRow = (int)(bottomY / gridSize);
			position.y = (floorRow + 1) * gridSize + size;
		}
	}

	// Update rotation (spin while moving)
	rotation += rotationSpeed * delta;
	while(rotation >= 360.0f) rotation -= 360.0f;

	// Deactivate if off map
	if(position.y < -gridSize * 5 || position.y > gridSize * 100 ||
	   position.x < -gridSize * 5 || position.x > gridSize * 200) {
		active = false;
	}
}

void Droplet::Render(Draw& w, Player::CoordinateConverter& coords) {
	if(!active) return;

	// Get screen position
	Point screenPos = coords.WorldToScreen(Point((int)position.x, (int)position.y));

	// Get droplet color based on type
	Color dropletColor;
	switch(type) {
		case DROPLET_RAINBOW:
			// Cycle through rainbow colors based on rotation
			{
				int hue = ((int)rotation) % 360;
				// Simple HSV to RGB conversion for rainbow effect
				if(hue < 60) dropletColor = Color(255, hue * 255 / 60, 0);
				else if(hue < 120) dropletColor = Color((120 - hue) * 255 / 60, 255, 0);
				else if(hue < 180) dropletColor = Color(0, 255, (hue - 120) * 255 / 60);
				else if(hue < 240) dropletColor = Color(0, (240 - hue) * 255 / 60, 255);
				else if(hue < 300) dropletColor = Color((hue - 240) * 255 / 60, 0, 255);
				else dropletColor = Color(255, 0, (360 - hue) * 255 / 60);
			}
			break;
		case DROPLET_ICE:
			dropletColor = Color(100, 200, 255);  // Cyan
			break;
		case DROPLET_FIRE:
			dropletColor = Color(255, 150, 50);  // Orange
			break;
	}

	// Draw droplet as circle (approximate with filled rect for now)
	int renderSize = (int)(size * 2);

	// Huge droplet: pulsing glow border
	if(isHuge) {
		float pulse = 0.5f + 0.5f * sinf(rotation * 0.05f);
		int glowSize = renderSize + 4 + (int)(pulse * 4);
		Color glowColor = Color(
			min(255, dropletColor.GetR() + 60),
			min(255, dropletColor.GetG() + 60),
			min(255, dropletColor.GetB() + 60));
		w.DrawRect(screenPos.x - glowSize/2, screenPos.y - glowSize/2,
		           glowSize, glowSize, glowColor);
	}

	w.DrawRect(screenPos.x - renderSize/2, screenPos.y - renderSize/2,
	           renderSize, renderSize, dropletColor);

	// Draw highlight (top-left quarter for shine effect)
	Color highlightColor = Color(255, 255, 255);
	int highlightSize = renderSize / 3;
	w.DrawRect(screenPos.x - renderSize/2 + 1, screenPos.y - renderSize/2 + 1,
	           highlightSize, highlightSize, highlightColor);
}

void Droplet::UpdateOrbit(float delta, Pointf playerPos, int dropletIndex, int totalDroplets) {
	if(!collected || !active) return;

	// Orbit parameters
	const float ORBIT_RADIUS = 25.0f;
	const float ORBIT_SPEED = 2.0f;  // Radians per second

	// Update orbit angle
	orbitAngle += ORBIT_SPEED * delta;
	while(orbitAngle >= M_2PI) orbitAngle -= M_2PI;

	// Calculate position in orbit above player
	// Offset each droplet by its index to spread them around the circle
	float angleOffset = (M_2PI / max(1, totalDroplets)) * dropletIndex;
	float finalAngle = orbitAngle + angleOffset;

	position.x = playerPos.x + cos(finalAngle) * ORBIT_RADIUS;
	position.y = playerPos.y + 20.0f + sin(finalAngle) * ORBIT_RADIUS;  // 20 pixels above player

	// Continue rotation
	rotation += rotationSpeed * delta;
	while(rotation >= 360.0f) rotation -= 360.0f;
}
