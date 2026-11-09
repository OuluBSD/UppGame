#include "Umbrella.h"
#include "WaterWeapon.h"
#include "Enemy.h"

using namespace Upp;

void WaterWeapon::Activate(int startCol, int startRow, int facing) {
	col = startCol;
	row = startRow;
	directionX = facing;
	falling = true;
	stepTimer = 0.0f;
	active = true;
	trail.Clear();
	attached.Clear();
	trail.Add(Point(col, row));
	LOG("WaterWeapon activated at (" << col << "," << row << ") dir=" << directionX);
}

bool WaterWeapon::IsSolid(int c, int r, Player::CollisionHandler& collision) {
	return collision.IsFloorTile(c, r);
}

void WaterWeapon::Step(Player::CollisionHandler& collision) {
	// Add current position to trail
	trail.Add(Point(col, row));
	if(trail.GetCount() > TRAIL_LENGTH)
		trail.Remove(0);

	if(falling) {
		// Try to move down one tile
		int nextRow = row - 1;  // Y-up: down means row--
		if(nextRow < 0) {
			// Fell off bottom of map — destroy
			active = false;
			return;
		}
		if(!IsSolid(col, nextRow, collision)) {
			// Empty below — fall
			row = nextRow;
		}
		else {
			// Solid below — switch to horizontal mode
			falling = false;
		}
	}
	else {
		// Horizontal mode: try to move in current direction
		int nextCol = col + directionX;

		if(IsSolid(nextCol, row, collision)) {
			// Blocked by wall — reverse direction (mirror)
			directionX = -directionX;
			nextCol = col + directionX;

			// If still blocked after reversing, try falling
			if(IsSolid(nextCol, row, collision)) {
				falling = true;
				return;
			}
		}

		// Move to next column
		col = nextCol;

		// Check if tile below is now empty — switch back to falling
		int belowRow = row - 1;
		if(belowRow < 0) {
			active = false;
			return;
		}
		if(!IsSolid(col, belowRow, collision)) {
			falling = true;
		}
	}

	// Move attached enemies to current grid position
	float gridSize = collision.GetGridSize();
	float worldX = col * gridSize + gridSize / 2.0f;
	float worldY = row * gridSize + gridSize / 2.0f;
	for(int i = 0; i < attached.GetCount(); i++) {
		if(attached[i] && attached[i]->IsActive()) {
			attached[i]->SetPositionXY(worldX, worldY);
		}
	}
}

void WaterWeapon::CheckEnemyAttachment(Array<Enemy*>& enemies, int gridSize) {
	for(int i = 0; i < enemies.GetCount(); i++) {
		if(!enemies[i]->IsAlive()) continue;
		if(enemies[i]->IsCaptured()) continue;

		// Check if enemy overlaps current water grid square
		Rectf eb = enemies[i]->GetBounds();
		float waterLeft = col * gridSize;
		float waterRight = (col + 1) * gridSize;
		float waterBottom = row * gridSize;
		float waterTop = (row + 1) * gridSize;

		float eBottom = min(eb.top, eb.bottom);
		float eTop = max(eb.top, eb.bottom);

		if(eb.left < waterRight && eb.right > waterLeft &&
		   eBottom < waterTop && eTop > waterBottom) {
			// Enemy overlaps water square — attach it
			bool alreadyAttached = false;
			for(int j = 0; j < attached.GetCount(); j++) {
				if(attached[j] == enemies[i]) { alreadyAttached = true; break; }
			}
			if(!alreadyAttached) {
				attached.Add(enemies[i]);
				RLOG("Enemy attached to water weapon at (" << col << "," << row << ")");
			}
		}
	}
}

void WaterWeapon::Update(float delta, Player::CollisionHandler& collision, Array<Enemy*>& enemies) {
	if(!active) return;

	int gridSize = (int)collision.GetGridSize();

	stepTimer += delta;
	while(stepTimer >= STEP_INTERVAL) {
		stepTimer -= STEP_INTERVAL;
		Step(collision);

		if(!active) return;  // Destroyed during step

		// Check for enemy attachment after each step
		CheckEnemyAttachment(enemies, gridSize);
	}
}

Vector<Enemy*> WaterWeapon::Release() {
	Vector<Enemy*> released;
	for(int i = 0; i < attached.GetCount(); i++) {
		if(attached[i])
			released.Add(attached[i]);
	}
	attached.Clear();
	active = false;
	trail.Clear();
	return released;
}

void WaterWeapon::Render(Draw& w, Player::CoordinateConverter& coords, int gridSize) {
	if(!active) return;

	// Draw trail (fading from bright to dim)
	for(int i = 0; i < trail.GetCount(); i++) {
		float alpha = (float)(i + 1) / trail.GetCount();
		int brightness = (int)(alpha * 200);
		Color trailColor = Color(brightness / 3, brightness / 2, brightness);

		Point worldPos(trail[i].x * gridSize, trail[i].y * gridSize);
		Point screenPos = coords.WorldToScreen(worldPos);
		Point screenEnd = coords.WorldToScreen(Point(worldPos.x + gridSize, worldPos.y + gridSize));

		int sx = min(screenPos.x, screenEnd.x);
		int sy = min(screenPos.y, screenEnd.y);
		int sw = abs(screenEnd.x - screenPos.x);
		int sh = abs(screenEnd.y - screenPos.y);
		w.DrawRect(sx, sy, sw, sh, trailColor);
	}

	// Draw current position (bright cyan)
	Color waterColor = Color(80, 220, 255);
	Point worldPos(col * gridSize, row * gridSize);
	Point screenPos = coords.WorldToScreen(worldPos);
	Point screenEnd = coords.WorldToScreen(Point(worldPos.x + gridSize, worldPos.y + gridSize));

	int sx = min(screenPos.x, screenEnd.x);
	int sy = min(screenPos.y, screenEnd.y);
	int sw = abs(screenEnd.x - screenPos.x);
	int sh = abs(screenEnd.y - screenPos.y);
	w.DrawRect(sx, sy, sw, sh, waterColor);

	// Draw bright highlight in center
	int hlSize = max(2, sw / 3);
	w.DrawRect(sx + sw/2 - hlSize/2, sy + sh/2 - hlSize/2, hlSize, hlSize, Color(200, 255, 255));
}
