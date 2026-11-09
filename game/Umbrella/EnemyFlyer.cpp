#include "Umbrella.h"
#include "EnemyFlyer.h"

using namespace Upp;

void EnemyFlyer::Init(float x, float y, int spawnFacing)
{
	bounds = Rectf(x, y, x + 10, y + 10);
	originalSize = 10;
	facing = spawnFacing;
}

void EnemyFlyer::Update(float delta, const Player& player, Player::CollisionHandler& collision) {
	UpdateRotation(delta);

	// Dead: fall under gravity with tile collision
	if(!alive) {
		velocity.y += GRAVITY * delta;
		if(velocity.y < MAX_FALL_SPEED) velocity.y = MAX_FALL_SPEED;
		ResolveCollisionX(velocity.x * delta, collision);
		ResolveCollisionY(velocity.y * delta, collision);
		return;
	}

	if(carriedByThrown) {
		ResolveCollisionX(velocity.x * delta, collision);
		return;
	}

	if(!active) return;

	if(thrown) {
		if(CheckThrownWallCollision(velocity.x * delta, collision)) {
			DefeatPreservingVelocity();
			return;
		}
		bounds.left  += velocity.x * delta;
		bounds.right += velocity.x * delta;
		return;
	}

	// Alive: home in on player, ignoring gravity and tile collision
	bobTimer += BOB_SPEED * delta;

	Rectf pb = player.GetBounds();
	float playerCX = (pb.left  + pb.right)  / 2.0f;
	float playerCY = (pb.top   + pb.bottom) / 2.0f;
	float cx       = (bounds.left  + bounds.right)  / 2.0f;
	float cy       = (bounds.top   + bounds.bottom) / 2.0f;

	float dx   = playerCX - cx;
	float dy   = playerCY - cy;
	float dist = (float)sqrt((double)(dx * dx + dy * dy));

	if(dist > 1.0f) {
		velocity.x = (dx / dist) * STALK_SPEED;
		velocity.y = (dy / dist) * STALK_SPEED;
	} else {
		velocity.x = 0.0f;
		velocity.y = 0.0f;
	}

	// Bob: derivative of sin(bobTimer)*BOB_AMP gives smooth Y drift per frame
	float bobDelta = BOB_AMP * BOB_SPEED * (float)cos(bobTimer) * delta;

	bounds.left   += velocity.x * delta;
	bounds.right  += velocity.x * delta;
	bounds.top    += velocity.y * delta + bobDelta;
	bounds.bottom += velocity.y * delta + bobDelta;

	// Update facing toward player
	facing = (dx >= 0.0f) ? 1 : -1;
}

void EnemyFlyer::Render(Draw& w, Player::CoordinateConverter& coords) {
	Point screenTL = coords.WorldToScreen(Point((int)bounds.left,  (int)bounds.top));
	Point screenBR = coords.WorldToScreen(Point((int)bounds.right, (int)bounds.bottom));

	int sx = min(screenTL.x, screenBR.x);
	int sy = min(screenTL.y, screenBR.y);
	int sw = abs(screenBR.x - screenTL.x);
	int sh = abs(screenBR.y - screenTL.y);
	if(sw < 1) sw = 1;
	if(sh < 1) sh = 1;

	// Purple body with state tint
	Color baseColor = Color(180, 0, 220);
	Color tint      = GetTintColor();
	int r = (baseColor.GetR() * tint.GetR()) / 255;
	int g = (baseColor.GetG() * tint.GetG()) / 255;
	int b = (baseColor.GetB() * tint.GetB()) / 255;

	w.DrawRect(sx, sy, sw, sh, Color(r, g, b));

	// Small white center dot
	w.DrawRect(sx + sw / 2 - 1, sy + sh / 2 - 1, 2, 2, White());
}
