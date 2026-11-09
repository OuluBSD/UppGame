#include "Umbrella.h"
#include "Projectile.h"

using namespace Upp;

void Projectile::Init(float x, float y, int dir) {
	direction = dir;
	bounds = Rectf(x, y, x + PROJECTILE_SIZE, y + PROJECTILE_SIZE);
	velocity.x = direction * PROJECTILE_SPEED;
	velocity.y = 0.0f;
	active = true;
}

bool Projectile::CheckWallCollision(Player::CollisionHandler& collision) {
	int gridSize = (int)collision.GetGridSize();
	int minCol = (int)(bounds.left / gridSize);
	int maxCol = (int)(bounds.right / gridSize);

	// Get Y range - remember bounds naming is backwards for Y-up!
	float minY = min(bounds.top, bounds.bottom);
	float maxY = max(bounds.top, bounds.bottom);
	int minRow = (int)(minY / gridSize);
	int maxRow = (int)(maxY / gridSize);

	for(int row = minRow; row <= maxRow; row++) {
		for(int col = minCol; col <= maxCol; col++) {
			if(collision.IsWallTile(col, row) || collision.IsFullBlockTile(col, row)) {
				return true;
			}
		}
	}
	return false;
}

void Projectile::Update(float delta, Player::CollisionHandler& collision) {
	if(!active) return;

	// Move projectile
	bounds.left += velocity.x * delta;
	bounds.right += velocity.x * delta;
	bounds.top += velocity.y * delta;
	bounds.bottom += velocity.y * delta;

	// Check collision with walls
	if(CheckWallCollision(collision)) {
		active = false;
		return;
	}

	// Deactivate if off screen (beyond map bounds)
	float gridSize = collision.GetGridSize();
	if(bounds.right < 0 || bounds.left > gridSize * 100) {  // Assume max 100 tiles wide
		active = false;
	}
}

void Projectile::Render(Draw& w, Player::CoordinateConverter& coords) {
	if(!active) return;

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

	// Draw projectile as orange/yellow rectangle
	Color projectileColor = Color(255, 200, 0);
	w.DrawRect(screenX, screenY, width, height, projectileColor);
}
