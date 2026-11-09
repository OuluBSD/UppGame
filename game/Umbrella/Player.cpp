#include "Umbrella.h"
#include "Player.h"
#include "Enemy.h"
#include "Tile.h"
#include "AudioSystem.h"

using namespace Upp;
using CollisionHandler = Player::CollisionHandler;

Player::Player(VfsValue& v) : GameEntity(v), onGround(false), coyoteTimer(0.0f),
                              jumpBufferTimer(0.0f), jumpHeld(false), jumpHoldQueued(false),
                              lives(3), score(0), invincibleTimer(0.0f),
                              knockbackTimer(0.0f), speedBoostTimer(0.0f),
                              parasolState(PARASOL_IDLE), attackHeld(false),
                              wasAttackHeld(false), attackTimer(0.0f),
                              attackCooldown(0.0f), forceUmbrellaOnTop(false),
                              carryWeight(0.0f), maxCarryWeight(3.0f)
{
	parasolHitbox = Rectf(0, 0, 0, 0);
}

void Player::Init(float x, float y, float width, float height) {
	bounds = Rectf(x, y, x + width, y + height);
	velocity = Pointf(0, 0);
	facing = 1;
}

void Player::Update(float delta, const InputState& input, CollisionHandler& collision) {
	// Update timers
	if(invincibleTimer > 0.0f) {
		invincibleTimer = invincibleTimer - delta;
		if(invincibleTimer < 0.0f) invincibleTimer = 0.0f;
	}
	if(speedBoostTimer > 0.0f) {
		speedBoostTimer = speedBoostTimer - delta;
		if(speedBoostTimer < 0.0f) speedBoostTimer = 0.0f;
	}

	bool recovering = knockbackTimer > 0.0f;
	if(recovering) {
		knockbackTimer = knockbackTimer - delta;
		if(knockbackTimer < 0.0f) knockbackTimer = 0.0f;
	}

	// Jump buffering
	if(!recovering && input.jumpPressed) {
		jumpBufferTimer = JUMP_BUFFER_TIME;
		if(!onGround) {
			jumpHoldQueued = true;
		}
	} else {
		jumpBufferTimer = jumpBufferTimer - delta;
		if(jumpBufferTimer < 0.0f) jumpBufferTimer = 0.0f;
	}

	if(!input.jumpHeld) {
		jumpHoldQueued = false;
	} else if(jumpHoldQueued && !onGround) {
		jumpBufferTimer = JUMP_BUFFER_TIME;
	}

	// Coyote time
	if(onGround) {
		coyoteTimer = COYOTE_TIME;
	} else {
		coyoteTimer = coyoteTimer - delta;
		if(coyoteTimer < 0.0f) coyoteTimer = 0.0f;
	}

	// Horizontal movement (instant velocity, no lerping)
	if(!recovering) {
		bool touchingLeftWall = IsTouchingWallOnLeft(collision);
		bool touchingRightWall = IsTouchingWallOnRight(collision);

		float speedMul = (speedBoostTimer > 0.0f) ? 1.5f : 1.0f;
		if(input.moveLeft && !input.moveRight) {
			if(!touchingLeftWall) {
				velocity.x = -MOVE_SPEED * speedMul;
			}
		} else if(input.moveRight && !input.moveLeft) {
			if(!touchingRightWall) {
				velocity.x = MOVE_SPEED * speedMul;
			}
		} else {
			velocity.x = 0.0f;
		}
	} else {
		velocity.x = 0.0f;
	}

	// Update facing direction
	if(velocity.x > 5.0f) {
		facing = 1;
	} else if(velocity.x < -5.0f) {
		facing = -1;
	}

	// Jump logic
	bool wantsJump = jumpBufferTimer > 0.0f && (onGround || coyoteTimer > 0.0f);

	// Update parasol state machine
	wasAttackHeld = attackHeld;
	attackHeld = input.glideHeld;
	if(attackHeld && !wasAttackHeld)
		GetAudioSystem().Play("whoosh");

	// DEBUG: Print state every 60 frames
	static int frameCount = 0;
	frameCount++;
	if(frameCount % 60 == 0) {
		RLOG("Player State: onGround=" << (onGround ? "true" : "false")
		     << " velocity.y=" << velocity.y
		     << " pos.y=" << bounds.top
		     << " recovering=" << (recovering ? "true" : "false"));
	}

	// Parasol state transitions (simpler logic matching Java)
	if(!recovering) {
		if(attackHeld && !onGround) {
			// Holding button while airborne -> GLIDING
			parasolState = PARASOL_GLIDING;
		}
		else if(attackHeld && onGround) {
			// Holding button while on ground -> ATTACKING
			parasolState = PARASOL_ATTACKING;
		}
		else if(onGround) {
			// On ground without button -> IDLE
			parasolState = PARASOL_IDLE;
		}
		else {
			// Airborne without button -> IDLE
			parasolState = PARASOL_IDLE;
		}
	}
	else {
		// Recovering from hit -> force IDLE
		parasolState = PARASOL_IDLE;
	}

	// Apply gravity (reduced when gliding)
	float effectiveGravity = GRAVITY;
	if(parasolState == PARASOL_GLIDING) {
		effectiveGravity *= PARASOL_GLIDE_GRAVITY_SCALE;
	}

	float prevVelY = velocity.y;
	velocity.y += effectiveGravity * delta;
	if(velocity.y < MAX_FALL_SPEED) velocity.y = MAX_FALL_SPEED;

	if(frameCount % 60 == 0) {
		RLOG("  Gravity: prevVel=" << prevVelY << " + (" << effectiveGravity << " * " << delta
		     << ") = " << velocity.y << " (clamped to " << MAX_FALL_SPEED << ")");
	}

	if(wantsJump) {
		velocity.y = JUMP_VELOCITY;
		onGround = false;
		coyoteTimer = 0.0f;
		jumpBufferTimer = 0.0f;
		jumpHoldQueued = false;
		GetAudioSystem().Play("jump");
		RLOG("  JUMP! velocity.y set to " << JUMP_VELOCITY);
	}

	// Variable jump height (release jump early)
	bool jumpHeldNow = input.jumpHeld;
	if(!jumpHeldNow && velocity.y > MIN_JUMP_VELOCITY && jumpHeld) {
		velocity.y = MIN_JUMP_VELOCITY;
	}
	jumpHeld = jumpHeldNow;

	// Apply movement with collision detection
	ResolveCollisionX(velocity.x * delta, collision);
	bool wasOnGround = onGround;
	ResolveCollisionY(velocity.y * delta, collision);
	if(onGround && !wasOnGround)
		GetAudioSystem().Play("land");

	// Stop horizontal movement if touching a wall after collision resolution
	// This handles cases where player drifts into wall while falling
	if(IsTouchingWallOnLeft(collision) || IsTouchingWallOnRight(collision)) {
		velocity.x = 0.0f;
	}

	// Update parasol hitbox position
	// Show horizontal parasol when:
	// 1. PARASOL_ATTACKING (on ground), OR
	// 2. PARASOL_GLIDING while going up (not falling yet)
	bool showHorizontalParasol = (parasolState == PARASOL_ATTACKING) ||
	                              (parasolState == PARASOL_GLIDING && velocity.y >= 0.0f);

	if(showHorizontalParasol) {
		// Parasol extends from player in facing direction
		// Made larger (1.2x width, 1.0x height) and extends upward to capture from top
		float parasolWidth = bounds.Width() * 1.2f;
		float parasolHeight = bounds.Height() * 1.0f;
		float centerX = bounds.left + bounds.Width() / 2.0f;
		float centerY = bounds.top + bounds.Height() / 2.0f;

		// Offset upward slightly so it can capture enemies from above
		float upwardOffset = bounds.Height() * 0.3f;

		if(facing > 0) {
			// Facing right - parasol extends to the right and upward
			parasolHitbox = Rectf(
				bounds.right,
				centerY - parasolHeight / 2.0f - upwardOffset,
				bounds.right + parasolWidth,
				centerY + parasolHeight / 2.0f - upwardOffset
			);
		} else {
			// Facing left - parasol extends to the left and upward
			parasolHitbox = Rectf(
				bounds.left - parasolWidth,
				centerY - parasolHeight / 2.0f - upwardOffset,
				bounds.left,
				centerY + parasolHeight / 2.0f - upwardOffset
			);
		}
	} else {
		// No attack - collapse hitbox
		parasolHitbox = Rectf(0, 0, 0, 0);
	}

	// Align captured enemies to player position
	AlignCapturedEnemies();
}

void Player::ResolveCollisionX(float deltaX, CollisionHandler& collision) {
	if(deltaX == 0.0f) return;

	float step = deltaX > 0 ? COLLISION_STEP : -COLLISION_STEP;
	float remaining = deltaX;

	while(abs(remaining) > COLLISION_STEP) {
		bounds.left += step;
		bounds.right += step;

		// Check collision
		int gridSize = (int)collision.GetGridSize();
		int minCol = (int)(bounds.left / gridSize);
		int maxCol = (int)(bounds.right / gridSize);

		// Get Y range - remember bounds naming is backwards for Y-up!
		float minY = min(bounds.top, bounds.bottom);
		float maxY = max(bounds.top, bounds.bottom);
		int minRow = (int)(minY / gridSize);
		int maxRow = (int)(maxY / gridSize);

		bool collided = false;
		for(int row = minRow; row <= maxRow; row++) {
			for(int col = minCol; col <= maxCol; col++) {
				if(collision.IsWallTile(col, row) || collision.IsFullBlockTile(col, row)) {
					collided = true;
					break;
				}
			}
			if(collided) break;
		}

		if(collided) {
			bounds.left -= step;
			bounds.right -= step;
			velocity.x = 0.0f;
			return;
		}

		remaining -= step;
	}

	// Store position before applying remaining
	float left_before = bounds.left;
	float right_before = bounds.right;

	// Apply remaining movement
	bounds.left += remaining;
	bounds.right += remaining;

	// Final wall check on remaining movement
	int gridSize = (int)collision.GetGridSize();
	float minY = min(bounds.top, bounds.bottom);
	float maxY = max(bounds.top, bounds.bottom);
	int minRow = (int)(minY / gridSize);
	int maxRow = (int)(maxY / gridSize);

	int minCol = (int)(bounds.left / gridSize);
	int maxCol = (int)(bounds.right / gridSize);

	for(int row = minRow; row <= maxRow; row++) {
		for(int col = minCol; col <= maxCol; col++) {
			if(collision.IsWallTile(col, row) || collision.IsFullBlockTile(col, row)) {
				// Hit wall - undo remaining movement
				bounds.left = left_before;
				bounds.right = right_before;
				velocity.x = 0.0f;
				return;
			}
		}
	}
}


void Player::ResolveCollisionY(float deltaY, CollisionHandler& collision) {
	if(deltaY == 0.0f) return;

	// DEBUG
	static int collisionLogCount = 0;
	collisionLogCount++;
	bool shouldLog = collisionLogCount % 60 == 0;

	float step = deltaY > 0 ? COLLISION_STEP : -COLLISION_STEP;
	float remaining = deltaY;

	float startFeetY = min(bounds.top, bounds.bottom);
	if(shouldLog) {
		RLOG("=== Y Collision ===");
		RLOG("  deltaY=" << deltaY << " velocity.y=" << velocity.y << " onGround=" << onGround);
		RLOG("  startFeetY=" << startFeetY << " bounds.top=" << bounds.top << " bounds.bottom=" << bounds.bottom);
	}

	onGround = false;
	int gridSize = (int)collision.GetGridSize();

	while(abs(remaining) > COLLISION_STEP) {
		// Check collision BEFORE applying movement to avoid jitter
		// NOTE: In Y-up coordinates:
		//  - bounds.top is the LOWER Y value (player's feet/bottom)
		//  - bounds.bottom is the HIGHER Y value (player's head/top)
		//  - deltaY < 0 means falling (gravity), deltaY > 0 means jumping
		int minCol = (int)(bounds.left / gridSize);
		int maxCol = (int)(bounds.right / gridSize);

		bool willCollide = false;

		// When falling, check for floor tiles below feet
		if(deltaY < 0) {
			// Calculate where we WOULD be after this step
			float testFeetY = min(bounds.top, bounds.bottom) + step;
			int feetRow = (int)(testFeetY / gridSize);
			float currentFeetY = min(bounds.top, bounds.bottom);

			for(int col = minCol; col <= maxCol; col++) {
				// Check if this is a valid floor tile (has air above, not wall-to-wall edge)
				bool isValidFloor = collision.IsFloorTile(col, feetRow);
				if(isValidFloor) {
					int rowAbove = feetRow + 1;
					// Must have air above (not full block and not wall)
					if(collision.IsFullBlockTile(col, rowAbove) || collision.IsWallTile(col, rowAbove)) {
						isValidFloor = false; // Wall-to-wall edge
					}
				}

				if(isValidFloor) {
					// Check if this step would cross the floor boundary
					float tileTopY = (feetRow + 1) * gridSize;
					if(currentFeetY >= tileTopY && testFeetY < tileTopY) {
						if(shouldLog) {
							RLOG("  WILL COLLIDE: col=" << col << " feetRow=" << feetRow
							     << " tileTopY=" << tileTopY << " currentFeetY=" << currentFeetY);
						}
						// Snap exactly to tile top
						float snapDelta = tileTopY - currentFeetY;
						bounds.top += snapDelta;
						bounds.bottom += snapDelta;
						velocity.y = 0.0f;
						onGround = true;
						return;
					}
				}
			}
		} else {
			// Jumping up - FullBlocks block upward movement
			float testMinY = min(bounds.top, bounds.bottom) + step;
			float testMaxY = max(bounds.top, bounds.bottom) + step;
			int minRow = (int)(testMinY / gridSize);
			int maxRow = (int)(testMaxY / gridSize);

			for(int row = minRow; row <= maxRow; row++) {
				for(int col = minCol; col <= maxCol; col++) {
					if(collision.IsFullBlockTile(col, row)) {
						willCollide = true;
						break;
					}
				}
				if(willCollide) break;
			}

			if(willCollide) {
				velocity.y = 0.0f;
				return;
			}
		}

		// No collision, apply the step
		bounds.top += step;
		bounds.bottom += step;
		remaining -= step;
	}

	if(shouldLog) {
		RLOG("  After loop: onGround=" << onGround << " velocity.y=" << velocity.y);
	}

	// Check floor collision BEFORE applying remaining movement
	if(deltaY < 0 && remaining < 0) {
		int minCol = (int)(bounds.left / gridSize);
		int maxCol = (int)(bounds.right / gridSize);
		float testFeetY = min(bounds.top, bounds.bottom) + remaining;
		int feetRow = (int)(testFeetY / gridSize);
		float currentFeetY = min(bounds.top, bounds.bottom);

		for(int col = minCol; col <= maxCol; col++) {
			// Check if this is a valid floor tile (has air above, not wall-to-wall edge)
			bool isValidFloor = collision.IsFloorTile(col, feetRow);
			if(isValidFloor) {
				int rowAbove = feetRow + 1;
				// Must have air above (not full block and not wall)
				if(collision.IsFullBlockTile(col, rowAbove) || collision.IsWallTile(col, rowAbove)) {
					isValidFloor = false; // Wall-to-wall edge
				}
			}

			if(isValidFloor) {
				// Check if remaining movement would cross the floor boundary
				float tileTopY = (feetRow + 1) * gridSize;
				if(currentFeetY >= tileTopY && testFeetY < tileTopY) {
					// Snap exactly to tile top
					float snapDelta = tileTopY - currentFeetY;
					bounds.top += snapDelta;
					bounds.bottom += snapDelta;
					velocity.y = 0.0f;
					onGround = true;
					if(shouldLog) {
						RLOG("  FINAL FLOOR SNAP: col=" << col << " feetRow=" << feetRow
						     << " tileTopY=" << tileTopY << " snapDelta=" << snapDelta);
					}
					return;
				}
			}
		}
	}

	// No collision, apply remaining movement
	bounds.top += remaining;
	bounds.bottom += remaining;

	if(shouldLog) {
		RLOG("  Final position: onGround=" << onGround << " feetY=" << min(bounds.top, bounds.bottom));
	}
}

bool Player::IsTouchingWallOnLeft(CollisionHandler& collision) {
	int gridSize = (int)collision.GetGridSize();
	int checkCol = (int)((bounds.left - 1.0f) / gridSize);

	// Get Y range - remember bounds naming is backwards for Y-up!
	float minY = min(bounds.top, bounds.bottom);
	float maxY = max(bounds.top, bounds.bottom);
	int minRow = (int)(minY / gridSize);
	int maxRow = (int)(maxY / gridSize);

	for(int row = minRow; row <= maxRow; row++) {
		if(collision.IsWallTile(checkCol, row) || collision.IsFullBlockTile(checkCol, row)) {
			return true;
		}
	}
	return false;
}

bool Player::IsTouchingWallOnRight(CollisionHandler& collision) {
	int gridSize = (int)collision.GetGridSize();
	int checkCol = (int)((bounds.right + 1.0f) / gridSize);

	// Get Y range - remember bounds naming is backwards for Y-up!
	float minY = min(bounds.top, bounds.bottom);
	float maxY = max(bounds.top, bounds.bottom);
	int minRow = (int)(minY / gridSize);
	int maxRow = (int)(maxY / gridSize);

	for(int row = minRow; row <= maxRow; row++) {
		if(collision.IsWallTile(checkCol, row) || collision.IsFullBlockTile(checkCol, row)) {
			return true;
		}
	}
	return false;
}

void Player::Render(Draw& w, CoordinateConverter& coords) {
	// Get world-space corners
	Point topLeft((int)bounds.left, (int)bounds.top);
	Point bottomRight((int)bounds.right, (int)bounds.bottom);

	// Convert to screen space (handles Y-flip)
	Point screenTopLeft = coords.WorldToScreen(topLeft);
	Point screenBottomRight = coords.WorldToScreen(bottomRight);

	// Since Y is flipped, screenBottomRight.y will be < screenTopLeft.y
	// Normalize to get proper screen rect
	int screenX = min(screenTopLeft.x, screenBottomRight.x);
	int screenY = min(screenTopLeft.y, screenBottomRight.y);
	int width = abs(screenBottomRight.x - screenTopLeft.x);
	int height = abs(screenBottomRight.y - screenTopLeft.y);

	// Draw player as blue rectangle (placeholder)
	Color playerColor = Color(50, 150, 255);
	if(invincibleTimer > 0.0f) {
		// Flash while invincible
		int flash = ((int)(invincibleTimer * 10) % 2);
		if(flash) playerColor = Color(255, 255, 255);
	}

	w.DrawRect(screenX, screenY, width, height, playerColor);

	// Draw facing direction indicator
	Color dirColor = Color(255, 255, 0);
	if(facing > 0) {
		w.DrawRect(screenX + width - 2, screenY + height/2 - 2, 4, 4, dirColor);
	} else {
		w.DrawRect(screenX - 2, screenY + height/2 - 2, 4, 4, dirColor);
	}

	// Draw umbrella when:
	// 1. Falling down (velocity.y < 0) AND gliding, OR
	// 2. Holding captured enemies (umbrella needed to carry them), OR
	// 3. Force umbrella on top (for droplets)
	bool isFallingDown = velocity.y < 0.0f;
	bool showUmbrellaAbove = (parasolState == PARASOL_GLIDING && isFallingDown) || (carriedEnemies.GetCount() > 0) || forceUmbrellaOnTop;

	if(showUmbrellaAbove) {
		// Draw umbrella above player (when falling or carrying enemies)
		float centerX = bounds.left + bounds.Width() / 2.0f;
		float topY = max(bounds.top, bounds.bottom);  // Top of player in Y-up coords

		// Umbrella dimensions
		float umbrellaWidth = 20.0f;
		float umbrellaHeight = 8.0f;
		float umbrellaY = topY + 4.0f;  // Slightly above player

		// Umbrella canopy (arc/dome shape simulated with rectangle)
		Point umbrellaTopLeft((int)(centerX - umbrellaWidth/2), (int)umbrellaY);
		Point umbrellaBottomRight((int)(centerX + umbrellaWidth/2), (int)(umbrellaY + umbrellaHeight));

		Point screenUmbrellaTL = coords.WorldToScreen(umbrellaTopLeft);
		Point screenUmbrellaBR = coords.WorldToScreen(umbrellaBottomRight);

		int uX = min(screenUmbrellaTL.x, screenUmbrellaBR.x);
		int uY = min(screenUmbrellaTL.y, screenUmbrellaBR.y);
		int uWidth = abs(screenUmbrellaBR.x - screenUmbrellaTL.x);
		int uHeight = abs(screenUmbrellaBR.y - screenUmbrellaTL.y);

		// Draw umbrella canopy (cyan/aqua color)
		Color umbrellaColor = Color(100, 255, 255);
		w.DrawRect(uX, uY, uWidth, uHeight, umbrellaColor);

		// Draw umbrella handle (small vertical line from player to umbrella)
		Point handleTop((int)centerX, (int)umbrellaY);
		Point handleBottom((int)centerX, (int)topY);

		Point screenHandleTop = coords.WorldToScreen(handleTop);
		Point screenHandleBottom = coords.WorldToScreen(handleBottom);

		Color handleColor = Color(150, 150, 150);
		w.DrawLine(screenHandleTop, screenHandleBottom, 2, handleColor);
	}
	else if(attackHeld && parasolHitbox.Width() > 0 && parasolHitbox.Height() > 0) {
		// Draw parasol hitbox when attacking or jumping with action held (horizontal)
		// This includes: PARASOL_ATTACKING (on ground) and PARASOL_GLIDING while going up
		Point parasolTopLeft((int)parasolHitbox.left, (int)parasolHitbox.top);
		Point parasolBottomRight((int)parasolHitbox.right, (int)parasolHitbox.bottom);

		Point screenParasolTL = coords.WorldToScreen(parasolTopLeft);
		Point screenParasolBR = coords.WorldToScreen(parasolBottomRight);

		int pX = min(screenParasolTL.x, screenParasolBR.x);
		int pY = min(screenParasolTL.y, screenParasolBR.y);
		int pWidth = abs(screenParasolBR.x - screenParasolTL.x);
		int pHeight = abs(screenParasolBR.y - screenParasolTL.y);

		// Draw parasol as semi-transparent cyan rectangle
		Color parasolColor = Color(100, 255, 255);
		w.DrawRect(pX, pY, pWidth, pHeight, parasolColor);
	}
}

void Player::Render(Draw& w, Point cameraOffset, float zoom) {
	// Deprecated version - does NOT handle Y-flip correctly
	// Calculate screen position
	Point screenPos;
	screenPos.x = (int)((bounds.left - cameraOffset.x) * zoom);
	screenPos.y = (int)((bounds.top - cameraOffset.y) * zoom);

	int width = (int)(bounds.Width() * zoom);
	int height = (int)(bounds.Height() * zoom);

	// Draw player as blue rectangle (placeholder)
	Color playerColor = Color(50, 150, 255);
	if(invincibleTimer > 0.0f) {
		// Flash while invincible
		int flash = ((int)(invincibleTimer * 10) % 2);
		if(flash) playerColor = Color(255, 255, 255);
	}

	w.DrawRect(screenPos.x, screenPos.y, width, height, playerColor);

	// Draw facing direction indicator
	Color dirColor = Color(255, 255, 0);
	if(facing > 0) {
		w.DrawRect(screenPos.x + width - 2, screenPos.y + height/2 - 2, 4, 4, dirColor);
	} else {
		w.DrawRect(screenPos.x - 2, screenPos.y + height/2 - 2, 4, 4, dirColor);
	}
}

void Player::TakeDamage(int amount) {
	if(invincibleTimer > 0.0f) return;

	lives -= amount;
	invincibleTimer = 2.0f;  // 2 seconds of invincibility
	knockbackTimer = KNOCKBACK_DURATION;
	GetAudioSystem().Play("hit");
}

void Player::SetPosition(float x, float y) {
	float width = bounds.Width();
	float height = bounds.Height();
	bounds = Rectf(x, y, x + width, y + height);
}

bool Player::CanCapture(Enemy* enemy) const {
	if(!enemy || !enemy->IsAlive() || !enemy->IsActive()) {
		return false;
	}
	// Check if we have weight capacity
	return carryWeight + enemy->GetCarryWeight() <= maxCarryWeight;
}

void Player::CaptureEnemy(Enemy* enemy) {
	if(!CanCapture(enemy)) {
		return;
	}

	carriedEnemies.Add(enemy);
	carryWeight += enemy->GetCarryWeight();
	enemy->Capture();
	AlignCapturedEnemies();
}

Enemy* Player::ReleaseCapturedEnemy() {
	if(carriedEnemies.GetCount() == 0) {
		return nullptr;
	}

	Enemy* enemy = carriedEnemies[carriedEnemies.GetCount() - 1];
	carriedEnemies.Remove(carriedEnemies.GetCount() - 1);
	carryWeight = max(0.0f, carryWeight - enemy->GetCarryWeight());
	return enemy;
}

void Player::AlignCapturedEnemies() {
	if(carriedEnemies.GetCount() == 0) {
		return;
	}

	// Position captured enemies on the umbrella
	// Stack them above the player
	float centerX = bounds.left + bounds.Width() / 2.0f;
	float baseY = bounds.top + bounds.Height() * 0.2f;

	for(int i = 0; i < carriedEnemies.GetCount(); i++) {
		Enemy* enemy = carriedEnemies[i];
		Rectf enemyBounds = enemy->GetBounds();
		float enemyWidth = enemyBounds.Width();
		float enemyHeight = enemyBounds.Height();

		// Position enemy centered above player
		float enemyX = centerX - enemyWidth / 2.0f;
		float enemyY = baseY + (i * enemyHeight);

		enemy->SetBounds(Rectf(enemyX, enemyY, enemyX + enemyWidth, enemyY + enemyHeight));
	}
}

void Player::ForceUmbrellaOnTop(bool force) {
	forceUmbrellaOnTop = force;
	if(force) {
		// Force glide state to ensure umbrella is visible
		parasolState = PARASOL_GLIDING;
	}
}
