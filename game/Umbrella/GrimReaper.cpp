#include "Umbrella.h"
#include "GrimReaper.h"

using namespace Upp;

void GrimReaper::Reset() {
	spawned    = false;
	spawnTimer = 0.0f;
	bobTimer   = 0.0f;
	warningPulse = 0.0f;
	bounds = Rectf(-999, -999, -999 + WIDTH, -999 + HEIGHT);
}

void GrimReaper::Update(float delta, const Player& player, float spawnX, float spawnY) {
	warningPulse += 4.0f * delta;

	if(!spawned) {
		spawnTimer += delta;
		if(spawnTimer >= 30.0f) {  // match SPAWN_DELAY
			spawned = true;
			bounds = Rectf(spawnX, spawnY, spawnX + WIDTH, spawnY + HEIGHT);
			RLOG("GrimReaper spawned at (" << spawnX << ", " << spawnY << ")");
		}
		return;
	}

	bobTimer += BOB_SPEED * delta;

	Rectf pb = player.GetBounds();
	float playerCX = (pb.left  + pb.right)  / 2.0f;
	float playerCY = (pb.top   + pb.bottom) / 2.0f;
	float cx       = (bounds.left  + bounds.right)  / 2.0f;
	float cy       = (bounds.top   + bounds.bottom) / 2.0f;

	float dx   = playerCX - cx;
	float dy   = playerCY - cy;
	float dist = (float)sqrt((double)(dx * dx + dy * dy));

	float vx = 0.0f, vy = 0.0f;
	if(dist > 1.0f) {
		vx = (dx / dist) * STALK_SPEED;
		vy = (dy / dist) * STALK_SPEED;
	}

	float bobDelta = BOB_AMP * BOB_SPEED * (float)cos(bobTimer) * delta;

	bounds.left   += vx * delta;
	bounds.right  += vx * delta;
	bounds.top    += vy * delta + bobDelta;
	bounds.bottom += vy * delta + bobDelta;
}

void GrimReaper::Render(Draw& w, Player::CoordinateConverter& coords, int screenW, int screenH) {
	// Pre-spawn warning: flashing skull icon in corner
	if(!spawned) {
		float remaining = 30.0f - spawnTimer;
		if(remaining <= 8.0f) {
			// Flash faster as time runs out
			float flashRate = 4.0f + (8.0f - remaining) * 2.0f;
			bool visible = (int)(warningPulse * flashRate) % 2 == 0;
			if(visible) {
				// Red warning box in top-right
				int bw = 60, bh = 16;
				int bx = screenW - bw - 4;
				int by = 4;
				w.DrawRect(bx, by, bw, bh, Color(180, 0, 0));
				w.DrawRect(bx + 1, by + 1, bw - 2, bh - 2, Color(80, 0, 0));
				// Draw "!!!" text approximation as small rects
				for(int i = 0; i < 3; i++) {
					int tx = bx + 8 + i * 14;
					w.DrawRect(tx, by + 2, 3, 9,  Color(255, 80, 80));
					w.DrawRect(tx, by + 13, 3, 2, Color(255, 80, 80));
				}
			}
		}
		return;
	}

	// Spawn animation: brief pulse on first few frames
	Point screenTL = coords.WorldToScreen(Point((int)bounds.left,  (int)bounds.top));
	Point screenBR = coords.WorldToScreen(Point((int)bounds.right, (int)bounds.bottom));

	int sx = min(screenTL.x, screenBR.x);
	int sy = min(screenTL.y, screenBR.y);
	int sw = abs(screenBR.x - screenTL.x);
	int sh = abs(screenBR.y - screenTL.y);
	if(sw < 1) sw = 1;
	if(sh < 1) sh = 1;

	// Dark cloak body
	w.DrawRect(sx, sy, sw, sh, Color(20, 0, 30));

	// White skull face (upper third)
	int faceW = sw * 2 / 3;
	int faceH = sh / 3;
	int faceX = sx + (sw - faceW) / 2;
	int faceY = sy + 1;
	w.DrawRect(faceX, faceY, faceW, faceH, Color(220, 220, 220));

	// Black eye sockets
	int eyeSize = max(1, faceW / 4);
	w.DrawRect(faceX + faceW / 4 - eyeSize / 2, faceY + faceH / 4, eyeSize, eyeSize, Color(10, 0, 15));
	w.DrawRect(faceX + 3 * faceW / 4 - eyeSize / 2, faceY + faceH / 4, eyeSize, eyeSize, Color(10, 0, 15));

	// Red glowing eyes
	w.DrawRect(faceX + faceW / 4 - 1, faceY + faceH / 4 + 1, max(1, eyeSize - 2), max(1, eyeSize - 2), Color(200, 0, 0));
	w.DrawRect(faceX + 3 * faceW / 4 - 1, faceY + faceH / 4 + 1, max(1, eyeSize - 2), max(1, eyeSize - 2), Color(200, 0, 0));
}

bool GrimReaper::TouchesPlayer(const Player& player) const {
	if(!spawned) return false;
	if(player.IsInvincible()) return false;

	Rectf pb = player.GetBounds();
	return bounds.left  < pb.right  &&
	       bounds.right > pb.left   &&
	       min(bounds.top, bounds.bottom) < max(pb.top, pb.bottom) &&
	       max(bounds.top, bounds.bottom) > min(pb.top, pb.bottom);
}
