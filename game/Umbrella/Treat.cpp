#include "Umbrella.h"
#include "Treat.h"

using namespace Upp;

void Treat::Init(float x, float y, TreatType treatType) {
	type = treatType;
	bounds = Rectf(x, y, x + TREAT_SIZE, y + TREAT_SIZE);
	active = true;
	onGround = false;
	lifetime = 0.0f;

	switch(type) {
		case TREAT_PEAR:       scoreValue = 50;  break;
		case TREAT_BANANA:     scoreValue = 75;  break;
		case TREAT_BLUEBERRY:  scoreValue = 100; break;
		case TREAT_SODA:       scoreValue = 200; break;
		case TREAT_CAKE:       scoreValue = 500; break;
		default:               scoreValue = 50;  break;
	}

	// Initial velocity (thrown upward with random horizontal spread)
	velocity.y = INITIAL_VY;
	velocity.x = (Randomf() - 0.5f) * INITIAL_VX_RANGE;
}

void Treat::Update(float delta, Player::CollisionHandler& collision) {
	if(!active) return;

	lifetime += delta;

	// Despawn after lifetime expires
	if(lifetime > MAX_LIFETIME) {
		active = false;
		return;
	}

	// Apply gravity only if not resting on ground
	if(!onGround) {
		velocity.y += GRAVITY * delta;
	}
	else {
		// Stop horizontal movement when resting on ground
		velocity.x *= 0.95f;  // Strong friction
		if(abs(velocity.x) < 1.0f) {
			velocity.x = 0.0f;  // Stop completely when very slow
		}
	}

	// Apply movement
	bounds.left += velocity.x * delta;
	bounds.right += velocity.x * delta;
	bounds.top += velocity.y * delta;
	bounds.bottom += velocity.y * delta;

	// Check floor collision AFTER movement
	int gridSize = (int)collision.GetGridSize();
	int minCol = (int)(bounds.left / gridSize);
	int maxCol = (int)(bounds.right / gridSize);
	float feetY = min(bounds.top, bounds.bottom);
	int floorRow = (int)(feetY / gridSize);

	static int debugCounter = 0;
	bool shouldLog = (debugCounter++ % 30 == 0);  // Log every 30 frames

	// Only check floor if falling down
	if(velocity.y < 0) {
		for(int col = minCol; col <= maxCol; col++) {
			if(collision.IsFloorTile(col, floorRow)) {
				// Found floor - check if we passed through it
				float tileTopY = (floorRow + 1) * gridSize;

				if(shouldLog) {
					RLOG("TREAT floor check: feetY=" << feetY << " tileTopY=" << tileTopY << " diff=" << (feetY - tileTopY));
				}

				// If feet are at or below floor top, snap to it
				if(feetY <= tileTopY) {
					float correction = tileTopY - feetY;
					bounds.top += correction;
					bounds.bottom += correction;
					velocity.y = 0.0f;
					velocity.x *= 0.5f;  // Strong initial friction on landing
					onGround = true;  // Mark as resting on ground

					if(shouldLog) {
						RLOG("  Snapped to floor, correction=" << correction << " NOW ON GROUND");
					}
					break;
				}
			}
		}
	}

	// Despawn if falls off map
	if(feetY < -gridSize * 2) {
		active = false;
	}
}

Color Treat::GetTreatColor() const {
	switch(type) {
		case TREAT_PEAR:
			return Color(100, 255, 100);  // Green
		case TREAT_BANANA:
			return Color(255, 255, 100);  // Yellow
		case TREAT_BLUEBERRY:
			return Color(100, 100, 255);  // Blue
		case TREAT_SODA:
			return Color(255, 150, 255);  // Pink
		case TREAT_CAKE:
			return Color(255, 200, 150);  // Orange/tan
		default:
			return Color(255, 255, 255);  // White
	}
}

void Treat::Render(Draw& w, Player::CoordinateConverter& coords) {
	if(!active) return;

	// Get world-space corners
	Point topLeft((int)bounds.left, (int)bounds.top);
	Point bottomRight((int)bounds.right, (int)bounds.bottom);

	// Convert to screen space
	Point screenTopLeft = coords.WorldToScreen(topLeft);
	Point screenBottomRight = coords.WorldToScreen(bottomRight);

	// Normalize to get proper screen rect
	int screenX = min(screenTopLeft.x, screenBottomRight.x);
	int screenY = min(screenTopLeft.y, screenBottomRight.y);
	int width = abs(screenBottomRight.x - screenTopLeft.x);
	int height = abs(screenBottomRight.y - screenTopLeft.y);

	// Draw treat as colored circle (using square for now)
	Color treatColor = GetTreatColor();
	w.DrawRect(screenX, screenY, width, height, treatColor);

	// Draw outline for visibility
	w.DrawRect(screenX, screenY, width, 1, Black());  // Top
	w.DrawRect(screenX, screenY + height - 1, width, 1, Black());  // Bottom
	w.DrawRect(screenX, screenY, 1, height, Black());  // Left
	w.DrawRect(screenX + width - 1, screenY, 1, height, Black());  // Right
}
