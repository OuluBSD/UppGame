#include "Umbrella.h"
#include "GameScreen.h"
#include "GameSettings.h"
#include "MapSerializer.h"
#include "EnemyPatroller.h"
#include "EnemyJumper.h"
#include "EnemyShooter.h"
#include "Treat.h"
#include "Droplet.h"
#include "Pickup.h"
#include "EnemyFlyer.h"
#include "AudioSystem.h"
#include "GrimReaper.h"
#include "LevelManifest.h"

using namespace Upp;

GameScreen::GameScreen() : player(100, 100, 12, 12) {
	Title("Umbrella - Game");
	Sizeable().Zoomable();
	SetRect(0, 0, 1280, 720);
	NoWantFocus();  // Disable focus navigation so arrow keys reach Key()

	zoom = 2.0f;
	cameraOffset = Point(0, 0);
	cameraMode = CAMERA_FIXED;  // Default: fixed camera with level centered
	accumulator = 0.0;
	gameState = PLAYING;
	levelCompleteTimer = 0.0f;
	allEnemiesKilled = false;
	transitionOffset = 0.0f;
	nextLevelPath = "";
	hoverTimer = 0.0f;
	dropTimer = 0.0f;
	levelColumns = 0;
	levelRows = 0;
	gridSize = 14;

	// Droplet system
	dropletsCollected = 0;
	totalDroplets = 0;
	hasHugeDroplet = false;

	// Level stats
	levelElapsedTime = 0.0f;
	damageTakenThisLevel = 0;
	scoreSummaryTimer = 0.0f;
	levelScoreBonus = 0;
	levelGrade = "C";

	// Debug overlay
	showDebugOverlay = false;
	debugFpsCounter = 0;
	debugFpsDisplay = 0;
	debugFpsTimer = 0.0f;

	// AI frame counter
	gameFrame = 0;

	// Input state
	keyLeft = keyRight = keyJump = keyAttack = false;
	prevKeyJump = prevKeyAttack = false;

	lastTime = GetTickCount();

	// Start game loop
	SetTimeCallback(16, [=] { LayoutLoop(); });
}

GameScreen::GameScreen(const String& level) : GameScreen() {
	levelPath = level;
	LoadLevel(level);
}

bool GameScreen::LoadLevel(const String& path) {
	if(!MapSerializer::LoadFromFile(path, layerManager)) {
		Exclamation("Failed to load: " + path);
		return false;
	}

	// Get level dimensions from terrain layer
	Layer* terrainLayer = layerManager.FindLayerByType(LAYER_TERRAIN);
	if(terrainLayer) {
		const MapGrid& grid = terrainLayer->GetGrid();
		levelColumns = grid.GetColumns();
		levelRows = grid.GetRows();
		gridSize = 14;  // Default grid size
	}

	// Load tile sprite sheet once (same mod root for all levels)
	if(tilesheet.IsEmpty()) {
		String modRoot = GetFileFolder(GetFileFolder(path));
		String tilesPath = AppendFileName(modRoot, "tiles.png");
		tilesheet = StreamRaster::LoadFileAny(tilesPath);
	}

	// Build shared pathfinding structures for this level
	pathfinder.SetGameScreen(this);
	navGraph.Build(this);

	// Spawn player at appropriate location
	RespawnPlayer();

	// Spawn enemies (WireAI called inside SpawnEnemies)
	SpawnEnemies();

	// Reset time-pressure boss for this level
	reaper.Reset();

	// Load pickup spawn points from level data
	pickups.Clear();
	pickupRoot.sub.Clear();
	{
		Array<PickupSpawnPoint> pickupSpawns;
		MapSerializer::LoadPickupSpawns(path, pickupSpawns);
		float gs = (float)gridSize;
		for(int i = 0; i < pickupSpawns.GetCount(); i++) {
			Pickup& p = pickupRoot.Add<Pickup>();
			p.Init(gs * pickupSpawns[i].col, gs * pickupSpawns[i].row, pickupSpawns[i].type);
			pickups.Add(&p);
		}
	}

	// Load droplet spawn points
	dropletSpawns.Clear();
	MapSerializer::LoadDropletSpawns(path, dropletSpawns);
	totalDroplets = dropletSpawns.GetCount();

	// Load enemy spawn points
	enemySpawns.Clear();
	MapSerializer::LoadEnemySpawns(path, enemySpawns);

	// Reset per-level stats
	levelElapsedTime = 0.0f;
	damageTakenThisLevel = 0;
	dropletsCollected = 0;
	hasHugeDroplet = false;
	allEnemiesKilled = false;
	levelCompleteTimer = 0.0f;

	Title("Umbrella - " + GetFileName(path));
	return true;
}

String GameScreen::GetNextLevelPath(const String& currentPath) {
	return GetLevelManifest().GetNextLevel(currentPath);
}

Point GameScreen::FindSpawnPoint() {
	// Find first solid ground starting from top-left of map
	// Search column by column, top to bottom
	for(int col = 1; col < levelColumns - 1; col++) {
		// Start from near top of map and search downward (remember Y-up: high Y = top)
		for(int row = levelRows - 3; row >= 1; row--) {
			// Check if there's solid ground at this position
			if(IsFloorTile(col, row)) {
				// Found ground - spawn player one tile above it
				int spawnX = col * gridSize + gridSize / 2 - 6;  // Center of tile, adjusted for player width
				int spawnY = (row + 1) * gridSize;  // One tile above ground
				return Point(spawnX, spawnY);
			}
		}
	}

	// Fallback: spawn near top-center of map
	int spawnX = (levelColumns / 2) * gridSize;
	int spawnY = (levelRows - 5) * gridSize;
	return Point(spawnX, spawnY);
}

void GameScreen::RespawnPlayer() {
	Point spawn = FindSpawnPoint();
	player.SetPosition((float)spawn.x, (float)spawn.y);
}

void GameScreen::LayoutLoop() {
	// Update game logic if playing, in score summary, or in transition
	if(gameState == PLAYING || gameState == SCORE_SUMMARY ||
	   gameState == TRANSITION_HOVER || gameState == TRANSITION_SCROLL ||
	   gameState == TRANSITION_DROP) {
		int64 now = GetTickCount();
		double delta = (now - lastTime) / 1000.0;
		lastTime = now;

		// Cap delta to prevent spiral of death
		if(delta > 0.25) {
			delta = 0.25;
		}

		accumulator += delta;

		// Fixed timestep update (60 FPS physics)
		while(accumulator >= FIXED_TIMESTEP) {
			GameTick((float)FIXED_TIMESTEP);
			accumulator -= FIXED_TIMESTEP;
		}
	}

	// FPS counting
	debugFpsCounter++;
	debugFpsTimer += 0.016f;  // approximate frame time
	if(debugFpsTimer >= 1.0f) {
		debugFpsDisplay = debugFpsCounter;
		debugFpsCounter = 0;
		debugFpsTimer = 0.0f;
	}

	Refresh();  // Always render (shows pause/game over screens)
	SetTimeCallback(16, [=] { LayoutLoop(); });  // ~60 FPS render
}

void GameScreen::GameTick(float delta) {
	gameFrame++;

	// Score summary: just count down the auto-advance timer
	if(gameState == SCORE_SUMMARY) {
		scoreSummaryTimer -= delta;
		if(scoreSummaryTimer <= 0.0f) {
			if(nextLevelPath.IsEmpty()) {
				EmitEvent("level_complete");
				SetGameState(LEVEL_COMPLETE);
			} else {
				SetGameState(TRANSITION_HOVER);
				transitionOffset = 0.0f;
				hoverTimer = 0.0f;
				player.ForceGlideState();
			}
		}
		return;
	}

	// Accumulate level time while actively playing
	if(gameState == PLAYING)
		levelElapsedTime += delta;

	// Update input state
	UpdateInput();

	// Update player
	player.Update(delta, inputState, *this);

	// Handle throwing captured enemies (when attack button released)
	bool buttonReleased = !inputState.glideHeld && prevKeyAttack;
	bool hasCaptured = player.HasCapturedEnemies();

	if(buttonReleased && hasCaptured) {
		Enemy* enemy = player.ReleaseCapturedEnemy();
		if(enemy) {
			// Throw enemy horizontally in facing direction (no upward velocity - purely horizontal)
			Pointf playerCenter = player.GetCenter();
			float throwVX = player.GetFacing() * Player::THROW_VELOCITY_X;

			enemy->ThrowFrom(playerCenter.x, playerCenter.y, throwVX, 0.0f);
			player.AddScore(50);  // Bonus for throwing
			// TODO: Play throw sound
		}
	}

	// Check if player fell off bottom of map - counts as a death
	Pointf playerPos = player.GetPosition();
	if(playerPos.y < -gridSize * 2) {
		if(!player.IsInvincible()) damageTakenThisLevel++;
		player.TakeDamage(1);
		if(player.GetLives() <= 0) {
			SetGameState(GAME_OVER);
			return;
		}
		RespawnPlayer();
	}

	// Update camera to follow player
	Pointf playerCenter = player.GetCenter();
	UpdateCamera(Point((int)playerCenter.x, (int)playerCenter.y));

	// Update enemies (skip captured ones)
	for(int i = 0; i < enemies.GetCount(); i++) {
		if(enemies[i]->IsCaptured()) continue;  // Don't update captured enemies

		bool wasThrown = enemies[i]->IsThrown();
		bool wasAlive = enemies[i]->IsAlive();

		enemies[i]->Update(delta, player, *this);

		// Check if thrown enemy just died (hit wall) - spawn treats for it and all carried enemies
		if(wasThrown && wasAlive && !enemies[i]->IsAlive()) {
			Rectf enemyBounds = enemies[i]->GetBounds();
			Pointf enemyCenter;
			enemyCenter.x = (enemyBounds.left + enemyBounds.right) / 2.0f;
			enemyCenter.y = (enemyBounds.top + enemyBounds.bottom) / 2.0f;

			TreatType treatType = TREAT_PEAR;
			switch(enemies[i]->GetType()) {
				case ENEMY_PATROLLER: treatType = TREAT_PEAR; break;
				case ENEMY_JUMPER: treatType = TREAT_BANANA; break;
				case ENEMY_SHOOTER: treatType = TREAT_BLUEBERRY; break;
				case ENEMY_FLYER: treatType = TREAT_SODA; break;
			}

			RLOG("Spawning treat at (" << enemyCenter.x << "," << enemyCenter.y << ") type=" << (int)treatType);
			{ Treat& t = treatRoot.Add<Treat>(); t.Init(enemyCenter.x, enemyCenter.y, treatType); treats.Add(&t); }
			player.AddScore(200);  // Bonus for defeating via throw

			// Kill and spawn treats for all enemies that were carried by this thrown enemy
			for(int j = 0; j < enemies.GetCount(); j++) {
				if(i == j) continue;  // Skip self
				if(enemies[j]->IsCarriedByThrown()) {
					Rectf carriedBounds = enemies[j]->GetBounds();
					Pointf carriedCenter;
					carriedCenter.x = (carriedBounds.left + carriedBounds.right) / 2.0f;
					carriedCenter.y = (carriedBounds.top + carriedBounds.bottom) / 2.0f;

					TreatType carriedTreatType = TREAT_PEAR;
					switch(enemies[j]->GetType()) {
						case ENEMY_PATROLLER: carriedTreatType = TREAT_PEAR; break;
						case ENEMY_JUMPER: carriedTreatType = TREAT_BANANA; break;
						case ENEMY_SHOOTER: carriedTreatType = TREAT_BLUEBERRY; break;
						case ENEMY_FLYER: carriedTreatType = TREAT_SODA; break;
					}

					{ Treat& t = treatRoot.Add<Treat>(); t.Init(carriedCenter.x, carriedCenter.y, carriedTreatType); treats.Add(&t); }
					enemies[j]->Defeat();
					player.AddScore(150);  // Bonus for each carried enemy
				}
			}
		}
	}

	// Check dead enemies for ground collision and spawn treats
	for(int i = enemies.GetCount() - 1; i >= 0; i--) {
		if(enemies[i]->IsAlive()) continue;  // Only check dead enemies
		if(enemies[i]->IsCarriedByThrown()) continue;  // Skip carried dead enemies (handled elsewhere)

		// Check if dead enemy is on ground
		Rectf bounds = enemies[i]->GetBounds();
		int minCol = (int)(bounds.left / gridSize);
		int maxCol = (int)(bounds.right / gridSize);
		float feetY = min(bounds.top, bounds.bottom);
		int floorRow = (int)(feetY / gridSize);

		bool onGround = false;
		for(int col = minCol; col <= maxCol; col++) {
			if(IsFloorTile(col, floorRow)) {
				onGround = true;
				break;
			}
		}

		// If on ground, spawn treat and remove enemy
		if(onGround) {
			Pointf center;
			center.x = (bounds.left + bounds.right) / 2.0f;
			center.y = (bounds.top + bounds.bottom) / 2.0f;

			TreatType treatType = TREAT_PEAR;
			switch(enemies[i]->GetType()) {
				case ENEMY_PATROLLER: treatType = TREAT_PEAR; break;
				case ENEMY_JUMPER: treatType = TREAT_BANANA; break;
				case ENEMY_SHOOTER: treatType = TREAT_BLUEBERRY; break;
				case ENEMY_FLYER: treatType = TREAT_SODA; break;
			}

			{ Treat& t = treatRoot.Add<Treat>(); t.Init(center.x, center.y, treatType); treats.Add(&t); }
			player.AddScore(100);  // Reward for defeating enemy
			GetAudioSystem().Play("defeat");
			EmitEvent("enemy_killed", i);

			// Remove enemy from world
			enemyRoot.Remove(&enemies[i]->val);
			enemies.Remove(i);
		}
	}

	// Check if all enemies are killed (level completion)
	// Note: Captured enemies still count as alive - only dead enemies count toward completion
	if(gameState == PLAYING && !allEnemiesKilled) {
		int aliveCount = 0;
		for(int i = 0; i < enemies.GetCount(); i++) {
			if(enemies[i]->IsAlive()) {
				aliveCount++;
			}
		}

		if(aliveCount == 0) {
			// All enemies defeated (or level had none)! Start level completion sequence
			allEnemiesKilled = true;
			EmitEvent("all_enemies_killed");
			levelCompleteTimer = GameSettings::LEVEL_COMPLETE_TREAT_TIMEOUT;
			LOG("Level complete! All enemies defeated. Treat collection time: " << levelCompleteTimer << "s");
		}
	}

	// Handle level completion timer countdown
	if(allEnemiesKilled && gameState == PLAYING) {
		levelCompleteTimer -= delta;

		if(levelCompleteTimer <= 0.0f) {
			// Time's up! Determine next level, calculate stats, show summary
			nextLevelPath = GetNextLevelPath(levelPath);

			// Time bonus
			levelScoreBonus = 0;
			if(levelElapsedTime < 30.0f)      levelScoreBonus += 2000;
			else if(levelElapsedTime < 60.0f) levelScoreBonus += 1500;
			else if(levelElapsedTime < 90.0f) levelScoreBonus += 1000;
			else                              levelScoreBonus +=  500;

			// No-damage bonus
			if(damageTakenThisLevel == 0) levelScoreBonus += 1000;

			// All-droplets bonus
			if(totalDroplets > 0 && dropletsCollected >= totalDroplets) levelScoreBonus += 500;

			player.AddScore(levelScoreBonus);

			// Letter grade based on performance
			if(damageTakenThisLevel == 0 && levelElapsedTime < 30.0f)        levelGrade = "S";
			else if(damageTakenThisLevel <= 1 && levelElapsedTime < 60.0f)   levelGrade = "A";
			else if(damageTakenThisLevel <= 2 && levelElapsedTime < 90.0f)   levelGrade = "B";
			else                                                              levelGrade = "C";

			scoreSummaryTimer = 5.0f;
			SetGameState(SCORE_SUMMARY);
			LOG("Score summary: grade=" << levelGrade << " bonus=" << levelScoreBonus
			    << " time=" << levelElapsedTime << "s damage=" << damageTakenThisLevel);
		}
	}

	// Handle transition states
	if(gameState == TRANSITION_HOVER) {
		// Player hovers with umbrella
		player.ForceGlideState();  // Keep umbrella open

		// Apply slight upward hover (reduce gravity effect)
		Pointf playerVel = player.GetVelocity();
		if(playerVel.y < -50.0f) {  // If falling too fast
			player.SetVelocity(Pointf(0, -50.0f));  // Slow fall to gentle hover
		}

		hoverTimer += delta;
		if(hoverTimer >= GameSettings::TRANSITION_HOVER_TIME) {
			hoverTimer = 0.0f;

			// Load next level into nextLayerManager for dual rendering
			if(MapSerializer::LoadFromFile(nextLevelPath, nextLayerManager)) {
				Layer* terrainLayer = nextLayerManager.FindLayerByType(LAYER_TERRAIN);
				if(terrainLayer) {
					const MapGrid& grid = terrainLayer->GetGrid();
					nextLevelColumns = grid.GetColumns();
					nextLevelRows = grid.GetRows();
				}
				LOG("Preloaded next level for transition: " << nextLevelPath);
			}

			SetGameState(TRANSITION_SCROLL);
			LOG("Starting level scroll...");
		}
	}
	else if(gameState == TRANSITION_SCROLL) {
		// Scroll level horizontally to the left
		transitionOffset += GameSettings::LEVEL_TRANSITION_SCROLL_SPEED * delta;

		// Keep player hovering during scroll
		player.ForceGlideState();
		Pointf playerVel = player.GetVelocity();
		player.SetVelocity(Pointf(0, -50.0f));  // Gentle hover

		// When scrolled off screen, swap to new level
		int levelWidth = levelColumns * gridSize;
		if(transitionOffset >= levelWidth) {
			// Swap current level with next level (already loaded)
			LOG("Swapping to next level: " << nextLevelPath);
			layerManager = pick(nextLayerManager);  // Move ownership
			levelPath = nextLevelPath;
			levelColumns = nextLevelColumns;
			levelRows = nextLevelRows;

			// Reload spawn data and rebuild pathfinding for the new level
			dropletSpawns.Clear();
			MapSerializer::LoadDropletSpawns(levelPath, dropletSpawns);
			totalDroplets = dropletSpawns.GetCount();
			enemySpawns.Clear();
			MapSerializer::LoadEnemySpawns(levelPath, enemySpawns);

			// Reset per-level stats for the new level
			levelElapsedTime = 0.0f;
			damageTakenThisLevel = 0;
			dropletsCollected = 0;
			hasHugeDroplet = false;
			pathfinder.SetGameScreen(this);
			navGraph.Build(this);

			// Respawn player and enemies for new level
			RespawnPlayer();
			ClearEnemies();
			SpawnEnemies();
			Title("Umbrella - " + GetFileName(levelPath));

			// Reset completion state
			allEnemiesKilled = false;
			levelCompleteTimer = 0.0f;
			transitionOffset = 0.0f;  // Reset scroll offset

			// Start drop transition
			SetGameState(TRANSITION_DROP);
			dropTimer = 0.0f;  // Reset drop timer
			player.ForceIdleState();  // Close umbrella
			LOG("Player dropping into new level...");
		}
	}
	else if(gameState == TRANSITION_DROP) {
		// Player drops into new level
		player.ForceIdleState();  // Keep umbrella closed

		dropTimer += delta;
		if(dropTimer >= GameSettings::TRANSITION_DROP_TIME) {
			dropTimer = 0.0f;
			SetGameState(PLAYING);
			LOG("Transition complete! Playing new level.");
		}
	}

	// Check thrown enemy collisions with other enemies (capturing them)
	for(int i = 0; i < enemies.GetCount(); i++) {
		if(!enemies[i]->IsThrown()) continue;  // Only check thrown enemies
		if(!enemies[i]->IsAlive()) continue;   // Skip dead enemies

		Rectf throwerBounds = enemies[i]->GetBounds();
		Pointf throwerVelocity = enemies[i]->GetVelocity();
		float throwerSize = enemies[i]->GetSize();

		// Check collision with other enemies
		for(int j = 0; j < enemies.GetCount(); j++) {
			if(i == j) continue;  // Skip self

			// Skip checks for enemies that can't be captured
			if(enemies[j]->IsCarriedByThrown()) continue;  // Already carried
			if(enemies[j]->IsCaptured()) continue;  // Captured by player
			if(!enemies[j]->IsActive()) continue;  // Skip inactive enemies

			Rectf targetBounds = enemies[j]->GetBounds();
			float targetSize = enemies[j]->GetSize();
			bool targetAlive = enemies[j]->IsAlive();

			// Check bounds collision
			bool collisionX = throwerBounds.left < targetBounds.right && throwerBounds.right > targetBounds.left;
			bool collisionY = min(throwerBounds.top, throwerBounds.bottom) < max(targetBounds.top, targetBounds.bottom) &&
			                  max(throwerBounds.top, throwerBounds.bottom) > min(targetBounds.top, targetBounds.bottom);

			if(collisionX && collisionY) {
				// Collision detected!
				RLOG("THROWN ENEMY COLLISION: thrower[" << i << "] size=" << throwerSize << " hit target[" << j << "] size=" << targetSize << " alive=" << targetAlive);

				bool canCapture = false;

				if(!targetAlive) {
					// Always capture deactivated/dead enemies
					canCapture = true;
					RLOG("  Can capture: target not alive");
				}
				else {
					// Capture living enemies unless target is significantly larger
					// Example: thrower=12, target must be <= 21 (12 * tolerance)
					float maxTargetSize = throwerSize * GameSettings::THROWN_ENEMY_SIZE_TOLERANCE;
					if(targetSize <= maxTargetSize) {
						canCapture = true;
						RLOG("  Can capture: target size " << targetSize << " <= max " << maxTargetSize);
					}
					else {
						RLOG("  Cannot capture: target size " << targetSize << " > max " << maxTargetSize << " (too large)");
					}
				}

				if(canCapture) {
					RLOG("  >>> CAPTURING TARGET ENEMY <<<");
					enemies[j]->CaptureByThrown(throwerVelocity);
				}
			}
		}
	}

	// Update treats
	for(int i = 0; i < treats.GetCount(); i++) {
		treats[i]->Update(delta, *this);
	}

	// Remove inactive treats
	for(int i = treats.GetCount() - 1; i >= 0; i--) {
		if(!treats[i]->IsActive()) {
			treatRoot.Remove(&treats[i]->val);
			treats.Remove(i);
		}
	}

	// Check treat-player collision (get player bounds again)
	Rectf playerBoundsForTreats = player.GetBounds();
	for(int i = 0; i < treats.GetCount(); i++) {
		if(!treats[i]->IsActive()) continue;

		Rectf treatBounds = treats[i]->GetBounds();

		// Check if player collects treat
		if(playerBoundsForTreats.left < treatBounds.right &&
		   playerBoundsForTreats.right > treatBounds.left &&
		   min(playerBoundsForTreats.top, playerBoundsForTreats.bottom) < max(treatBounds.top, treatBounds.bottom) &&
		   max(playerBoundsForTreats.top, playerBoundsForTreats.bottom) > min(treatBounds.top, treatBounds.bottom)) {
			// Treat collected!
			player.AddScore(treats[i]->GetScoreValue());
			treats[i]->Collect();
			GetAudioSystem().Play("collect");
		}
	}

	// Droplet spawning from spawn points
	for(int i = 0; i < dropletSpawns.GetCount(); i++) {
		DropletSpawnPoint& spawn = dropletSpawns[i];

		if(!spawn.enabled) continue;

		// Update spawn timer
		spawn.timer += delta;
		float intervalSec = spawn.intervalMs / 1000.0f;

		if(spawn.timer >= intervalSec) {
			spawn.timer = 0.0f;

			// Calculate spawn position (center of tile)
			float spawnX = spawn.col * gridSize + gridSize / 2.0f;
			float spawnY = spawn.row * gridSize + gridSize / 2.0f;

			// Create droplet via VFS tree with initial horizontal velocity
			Droplet& d = dropletRoot.Add<Droplet>();
			d.Init(spawnX, spawnY, spawn.mode);

			// Set horizontal velocity if direction is specified
			if(spawn.direction != 0) {
				Pointf vel = d.GetVelocity();
				vel.x = spawn.direction * 50.0f;  // 50 pixels/sec horizontal speed
				d.SetVelocity(vel);
			}

			droplets.Add(&d);
			RLOG("Spawned droplet at (" << spawnX << "," << spawnY << ") type=" << (int)spawn.mode << " dir=" << spawn.direction);
		}
	}

	// Update droplets (physics for uncollected, orbit for collected)
	Pointf dropletPlayerPos = Pointf(player.GetBounds().left, player.GetBounds().top);
	int collectedCount = 0;
	for(int i = 0; i < droplets.GetCount(); i++) {
		if(droplets[i]->IsCollected()) {
			collectedCount++;
		}
	}

	// Force umbrella on top when player has collected droplets
	player.ForceUmbrellaOnTop(collectedCount > 0);

	int orbitIndex = 0;
	for(int i = 0; i < droplets.GetCount(); i++) {
		if(droplets[i]->IsCollected()) {
			droplets[i]->UpdateOrbit(delta, dropletPlayerPos, orbitIndex, collectedCount);
			orbitIndex++;
		} else {
			droplets[i]->Update(delta, *this);
		}
	}

	// Remove inactive droplets (but keep collected ones)
	for(int i = droplets.GetCount() - 1; i >= 0; i--) {
		if(!droplets[i]->IsActive() && !droplets[i]->IsCollected()) {
			dropletRoot.Remove(&droplets[i]->val);
			droplets.Remove(i);
		}
	}

	// Check droplet-player collision (only for uncollected droplets)
	Rectf playerBoundsForDroplets = player.GetBounds();
	for(int i = 0; i < droplets.GetCount(); i++) {
		if(!droplets[i]->IsActive()) continue;
		if(droplets[i]->IsCollected()) continue;  // Already collected

		Rectf dropletBounds = droplets[i]->GetBounds();

		// Check collision
		if(playerBoundsForDroplets.left < dropletBounds.right &&
		   playerBoundsForDroplets.right > dropletBounds.left &&
		   min(playerBoundsForDroplets.top, playerBoundsForDroplets.bottom) < max(dropletBounds.top, dropletBounds.bottom) &&
		   max(playerBoundsForDroplets.top, playerBoundsForDroplets.bottom) > min(dropletBounds.top, dropletBounds.bottom)) {
			// Droplet collected!
			float startAngle = (M_2PI / max(1, collectedCount + 1)) * collectedCount;
			droplets[i]->Collect(startAngle);
			dropletsCollected++;
			EmitEvent("droplet_collected", dropletsCollected);
			player.AddScore(15);  // 15 points per droplet
			GetAudioSystem().Play("droplet");
			RLOG("Droplet collected! Total: " << dropletsCollected);
		}
	}

	// Merge at 5: when 5 orbiting droplets collected, merge into 1 huge droplet
	if(dropletsCollected >= 5 && !hasHugeDroplet) {
		// Remove all orbiting (collected, non-huge) droplets
		for(int i = droplets.GetCount() - 1; i >= 0; i--) {
			if(droplets[i]->IsCollected() && !droplets[i]->IsHuge()) {
				dropletRoot.Remove(&droplets[i]->val);
				droplets.Remove(i);
			}
		}
		// Spawn 1 huge droplet in collected/orbit state
		Pointf playerCenter = player.GetCenter();
		Droplet& huge = dropletRoot.Add<Droplet>();
		huge.Init(playerCenter.x, playerCenter.y + 20, DROPLET_RAINBOW);
		huge.MakeHuge();
		huge.Collect(0.0f);
		droplets.Add(&huge);
		hasHugeDroplet = true;
		dropletsCollected = 0;
		RLOG("5 droplets merged into HUGE droplet!");
		GetAudioSystem().Play("droplet");
	}

	// Action button release: throw droplets or trigger water weapon
	bool dropletButtonReleased = !inputState.glideHeld && prevKeyAttack;
	if(dropletButtonReleased && !player.HasCapturedEnemies()) {
		if(hasHugeDroplet) {
			// Release huge droplet → trigger water weapon
			// Remove the huge droplet
			for(int i = droplets.GetCount() - 1; i >= 0; i--) {
				if(droplets[i]->IsHuge() && droplets[i]->IsCollected()) {
					// Get player grid position for water weapon start
					Pointf pc = player.GetCenter();
					int startCol = (int)(pc.x / gridSize);
					int startRow = (int)(pc.y / gridSize);

					dropletRoot.Remove(&droplets[i]->val);
					droplets.Remove(i);

					// Activate water weapon (will be wired in task 4)
					waterWeapon.Activate(startCol, startRow, player.GetFacing());
					RLOG("Water weapon activated at (" << startCol << "," << startRow << ")");
					break;
				}
			}
			hasHugeDroplet = false;
		}
		else {
			// Throw all orbiting droplets horizontally
			int throwFacing = player.GetFacing();
			for(int i = 0; i < droplets.GetCount(); i++) {
				if(droplets[i]->IsCollected() && !droplets[i]->IsHuge()) {
					droplets[i]->Throw(throwFacing);
					RLOG("Droplet thrown horizontally, facing=" << throwFacing);
				}
			}
			dropletsCollected = 0;
		}
	}

	// Update water weapon (grid-stepping snake)
	if(waterWeapon.IsActive()) {
		bool wasActive = waterWeapon.IsActive();
		waterWeapon.Update(delta, *this, enemies);

		// If water weapon just became inactive (fell off map), release enemies with death-arc
		if(wasActive && !waterWeapon.IsActive()) {
			Vector<Enemy*> released = waterWeapon.Release();
			for(int i = 0; i < released.GetCount(); i++) {
				Enemy* e = released[i];
				if(!e) continue;

				// Kill the enemy
				e->Defeat();

				// Teleport to top of level with arc velocity
				float arcX = (levelColumns / 2.0f) * gridSize + (Randomf() - 0.5f) * 160.0f;
				float arcY = (levelRows + 2) * (float)gridSize;
				e->SetPositionXY(arcX, arcY);
				float vx = (Randomf() - 0.5f) * 160.0f;
				float vy = 200.0f;
				e->SetVelocity(Pointf(vx, vy));

				RLOG("Water-killed enemy launched in arc from top of level");
			}
		}
	}

	// Update pickups and check player collection
	{
		Rectf pb = player.GetBounds();
		for(int i = 0; i < pickups.GetCount(); i++) {
			if(!pickups[i]->IsActive()) continue;
			pickups[i]->Update(delta);

			Rectf pkb = pickups[i]->GetBounds();
			if(pb.left < pkb.right && pb.right > pkb.left &&
			   min(pb.top,  pb.bottom)  < max(pkb.top,  pkb.bottom) &&
			   max(pb.top,  pb.bottom)  > min(pkb.top,  pkb.bottom)) {
				PickupType t = pickups[i]->GetType();
				pickups[i]->Collect();
				switch(t) {
					case PU_HEART:
						if(player.GetLives() < 5)
							player.ResetLives();  // restore to full (simple impl)
						break;
					case PU_GEM:
						player.AddScore(100);
						break;
					case PU_LIGHTNING:
						player.SetInvincible(5.0f);
						break;
					case PU_SPEED:
						player.SetSpeedBoost(10.0f);
						break;
				}
			}
		}
		// Remove collected pickups
		for(int i = pickups.GetCount() - 1; i >= 0; i--) {
			if(!pickups[i]->IsActive()) {
				pickupRoot.Remove(&pickups[i]->val);
				pickups.Remove(i);
			}
		}

		// GrimReaper: tick and check death
		{
			float reaperSpawnX = -3.0f * gridSize;
			float reaperSpawnY = player.GetBounds().top;
			reaper.Update(delta, player, reaperSpawnX, reaperSpawnY);
			if(reaper.TouchesPlayer(player)) {
				GetAudioSystem().Play("gameover");
				SetGameState(GAME_OVER);
				return;
			}
		}
	}

	// Check parasol-enemy collisions (capturing enemies)
	if(player.IsAttacking()) {
		Rectf parasolBox = player.GetParasolHitbox();
		for(int i = 0; i < enemies.GetCount(); i++) {
			if(!enemies[i]->IsAlive()) continue;
			if(enemies[i]->IsCaptured()) continue;  // Skip already captured
			if(!enemies[i]->IsActive()) continue;    // Skip inactive

			Rectf enemyBounds = enemies[i]->GetBounds();

			// Check if parasol hits enemy
			if(parasolBox.left < enemyBounds.right &&
			   parasolBox.right > enemyBounds.left &&
			   min(parasolBox.top, parasolBox.bottom) < max(enemyBounds.top, enemyBounds.bottom) &&
			   max(parasolBox.top, parasolBox.bottom) > min(enemyBounds.top, enemyBounds.bottom)) {

				// Try to capture enemy on umbrella
				if(player.CanCapture(enemies[i])) {
					player.CaptureEnemy(enemies[i]);
					player.AddScore(150);  // Score for capture
					// TODO: Play capture sound
				}
				else {
					// Can't capture (too heavy or full) - damage enemy instead
					enemies[i]->TakeDamage(1);
					player.AddScore(100);
					// TODO: Play hit sound
				}
			}
		}
	}

	// Check player-enemy collisions (taking damage)
	Rectf playerBounds = player.GetBounds();
	for(int i = 0; i < enemies.GetCount(); i++) {
		if(!enemies[i]->IsAlive()) continue;
		if(enemies[i]->IsCaptured()) continue;  // Skip captured enemies

		Rectf enemyBounds = enemies[i]->GetBounds();

		// Check if bounds intersect
		if(playerBounds.left < enemyBounds.right &&
		   playerBounds.right > enemyBounds.left &&
		   min(playerBounds.top, playerBounds.bottom) < max(enemyBounds.top, enemyBounds.bottom) &&
		   max(playerBounds.top, playerBounds.bottom) > min(enemyBounds.top, enemyBounds.bottom)) {
			// Collision detected - player takes damage
			if(!player.IsInvincible()) damageTakenThisLevel++;
			player.TakeDamage(1);
			EmitEvent("player_hit", 1);
			if(player.GetLives() <= 0) {
				SetGameState(GAME_OVER);
				return;
			}
		}

		// Check projectile collisions (for shooters)
		if(enemies[i]->GetType() == ENEMY_SHOOTER) {
			EnemyShooter* shooter = dynamic_cast<EnemyShooter*>(enemies[i]);
			if(shooter) {
				const Array<Projectile*>& projectiles = shooter->GetProjectiles();
				for(int j = 0; j < projectiles.GetCount(); j++) {
					if(!projectiles[j]->IsActive()) continue;

					Rectf projBounds = projectiles[j]->GetBounds();

					// Check if projectile hits player
					if(playerBounds.left < projBounds.right &&
					   playerBounds.right > projBounds.left &&
					   min(playerBounds.top, playerBounds.bottom) < max(projBounds.top, projBounds.bottom) &&
					   max(playerBounds.top, playerBounds.bottom) > min(projBounds.top, projBounds.bottom)) {
						// Projectile hit player
						if(!player.IsInvincible()) damageTakenThisLevel++;
						player.TakeDamage(1);
						EmitEvent("player_hit", 1);
						projectiles[j]->Deactivate();
						if(player.GetLives() <= 0) {
							SetGameState(GAME_OVER);
							return;
						}
					}
				}
			}
		}
	}

	// Update previous key states at END of frame after all logic has run
	prevKeyJump = keyJump;
	prevKeyAttack = keyAttack;
}

void GameScreen::UpdateCamera(Point targetPos) {
	Size screenSize = GetSize();
	int viewWidth = screenSize.cx / zoom;
	int viewHeight = screenSize.cy / zoom;
	int levelWidth = levelColumns * gridSize;
	int levelHeight = levelRows * gridSize;

	if(cameraMode == CAMERA_FIXED) {
		// Fixed camera: center level horizontally and vertically
		// If level is smaller than view, center it (negative offset pushes level right/down)
		// If level is larger than view, show from top-left (offset 0,0)
		if(levelWidth < viewWidth) {
			cameraOffset.x = -(viewWidth - levelWidth) / 2;  // Negative to center smaller level
		} else {
			cameraOffset.x = 0;  // Show from left edge
		}

		if(levelHeight < viewHeight) {
			cameraOffset.y = -(viewHeight - levelHeight) / 2;  // Negative to center smaller level
		} else {
			cameraOffset.y = 0;  // Show from top edge
		}
	}
	else {
		// Follow player (classic platformer)
		cameraOffset.x = targetPos.x - viewWidth / 2;
		cameraOffset.y = targetPos.y - viewHeight / 2;

		// Clamp to level bounds (skip during transition)
		if(gameState != TRANSITION_SCROLL) {
			cameraOffset.x = max(0, min(cameraOffset.x, levelWidth - viewWidth));
			cameraOffset.y = max(0, min(cameraOffset.y, levelHeight - viewHeight));
		}
	}

	// Apply transition offset during level scroll (scroll right to move level left)
	if(gameState == TRANSITION_SCROLL) {
		cameraOffset.x += (int)transitionOffset;
	}
}

Point GameScreen::WorldToScreen(Point worldPos) {
	// World coordinates: Y-up (like OpenGL, standard game coordinates)
	// Screen coordinates: Y-down (U++ Draw/CtrlCore convention)
	// Flip Y at the last moment when converting to screen space
	Size sz = GetSize();
	int viewHeight = sz.cy / zoom;

	Point screenPos;
	screenPos.x = (worldPos.x - cameraOffset.x) * zoom;
	// Flip Y: worldY=0 is bottom, screenY=0 is top
	screenPos.y = sz.cy - ((worldPos.y - cameraOffset.y) * zoom);
	return screenPos;
}

Point GameScreen::ScreenToWorld(Point screenPos) {
	// Inverse of WorldToScreen
	Size sz = GetSize();
	int viewHeight = sz.cy / zoom;

	Point worldPos;
	worldPos.x = screenPos.x / zoom + cameraOffset.x;
	// Unflip Y
	worldPos.y = (sz.cy - screenPos.y) / zoom + cameraOffset.y;
	return worldPos;
}

void GameScreen::Paint(Draw& w) {
	Size sz = GetSize();

	// Clear background (dark blue)
	w.DrawRect(sz, Color(12, 17, 30));

	// Render tiles
	RenderTiles(w);

	// Render enemies
	for(int i = 0; i < enemies.GetCount(); i++) {
		enemies[i]->Render(w, *this);
	}

	// Render treats
	for(int i = 0; i < treats.GetCount(); i++) {
		treats[i]->Render(w, *this);
	}

	// Render droplets
	for(int i = 0; i < droplets.GetCount(); i++) {
		droplets[i]->Render(w, *this);
	}

	// Render pickups
	for(int i = 0; i < pickups.GetCount(); i++) {
		pickups[i]->Render(w, *this);
	}

	// Render water weapon (above pickups, below player)
	waterWeapon.Render(w, *this, gridSize);

	// Render GrimReaper (above pickups, below player)
	reaper.Render(w, *this, sz.cx, sz.cy);

	// Render player (using WorldToScreen for proper Y-flip)
	player.Render(w, *this);

	// Render HUD (lives, score)
	RenderHUD(w);

	// Render debug overlay (F3 toggle)
	if(showDebugOverlay)
		RenderDebugOverlay(w);

	// Render overlays based on game state
	switch(gameState) {
		case PLAYING:
			// Treat-collection countdown banner while waiting for timer
			if(allEnemiesKilled)
				RenderLevelCompleteScreen(w);
			break;
		case SCORE_SUMMARY:
			RenderLevelCompleteScreen(w);
			break;
		case PAUSED:
			RenderPauseScreen(w);
			break;
		case GAME_OVER:
			RenderGameOverScreen(w);
			break;
		case LEVEL_COMPLETE:
			RenderLevelCompleteScreen(w);
			break;
		default:
			break;
	}
}

// Returns tilesheet source rect for a given tile type and 4-neighbor mask.
// Tilesheet layout: 9 columns x N rows, each tile 14x14 px.
//   col 0=TL outer, 1=top edge, 2=TR outer, 3=left edge, 4=center,
//       5=right edge, 6=BL outer, 7=bottom edge, 8=BR outer
// Row assignment: TILE_BACKGROUND=0, TILE_WALL=2, TILE_FULLBLOCK=3
static Rect AutotileRect(TileType tile, bool hasL, bool hasR, bool hasT, bool hasB) {
	int row;
	switch(tile) {
	case TILE_BACKGROUND: row = 0; break;
	case TILE_WALL:       row = 2; break;
	case TILE_FULLBLOCK:  row = 3; break;
	default:              return Rect(0, 0, 0, 0);  // No sprite for this type
	}
	int col;
	if     (!hasT && !hasL) col = 0;
	else if(!hasT && !hasR) col = 2;
	else if(!hasB && !hasL) col = 6;
	else if(!hasB && !hasR) col = 8;
	else if(!hasT)          col = 1;
	else if(!hasB)          col = 7;
	else if(!hasL)          col = 3;
	else if(!hasR)          col = 5;
	else                    col = 4;
	return Rect(col * 14, row * 14, col * 14 + 14, row * 14 + 14);
}

// Returns true if two tile types should autotile together (same visual group).
static bool SameAutotileGroup(TileType a, TileType b) {
	// Walls and fullblocks form one solid group
	bool aWall = (a == TILE_WALL || a == TILE_FULLBLOCK);
	bool bWall = (b == TILE_WALL || b == TILE_FULLBLOCK);
	if(aWall && bWall) return true;
	return a == b;
}

void GameScreen::RenderTiles(Draw& w) {
	Size sz = GetSize();

	// During transition, render both old and new levels
	if(gameState == TRANSITION_SCROLL) {
		// Render current level (scrolling left/off-screen)
		int tileSize = int(gridSize * zoom);
		if(tileSize < 1) tileSize = 1;

		int viewCols = sz.cx / tileSize + 2;
		int viewRows = sz.cy / tileSize + 2;

		int startCol = max(0, int(cameraOffset.x / gridSize));
		int startRow = max(0, int(cameraOffset.y / gridSize));
		int endCol = min(levelColumns, startCol + viewCols);
		int endRow = min(levelRows, startRow + viewRows);

		// Render current level
		for(int layerIndex = layerManager.GetLayerCount() - 1; layerIndex >= 0; layerIndex--) {
			const Layer& layer = layerManager.GetLayer(layerIndex);
			if(!layer.IsVisible()) continue;

			const MapGrid& grid = layer.GetGrid();
			int opacity = layer.GetOpacity();

			for(int row = startRow; row < endRow; row++) {
				for(int col = startCol; col < endCol; col++) {
					TileType tile = grid.GetTile(col, row);
					if(tile == TILE_EMPTY) continue;

					Point worldBottomLeft(col * gridSize, row * gridSize);
					Point worldTopRight((col + 1) * gridSize, (row + 1) * gridSize);
					Point screenBottomLeft = WorldToScreen(worldBottomLeft);
					Point screenTopRight = WorldToScreen(worldTopRight);

					int screenX = min(screenBottomLeft.x, screenTopRight.x);
					int screenY = min(screenBottomLeft.y, screenTopRight.y);
					int width = abs(screenTopRight.x - screenBottomLeft.x);
					int height = abs(screenTopRight.y - screenBottomLeft.y);

					// Sprite rendering: use tilesheet when available and opacity is full
					if(!tilesheet.IsEmpty() && opacity == 100) {
						bool hasL = grid.IsValid(col-1, row) && SameAutotileGroup(grid.GetTile(col-1, row), tile);
						bool hasR = grid.IsValid(col+1, row) && SameAutotileGroup(grid.GetTile(col+1, row), tile);
						bool hasT = grid.IsValid(col, row-1) && SameAutotileGroup(grid.GetTile(col, row-1), tile);
						bool hasB = grid.IsValid(col, row+1) && SameAutotileGroup(grid.GetTile(col, row+1), tile);
						Rect src = AutotileRect(tile, hasL, hasR, hasT, hasB);
						if(src.IsEmpty()) {
							w.DrawRect(screenX, screenY, width, height, TileTypeToColor(tile));
						} else {
							w.DrawImage(screenX, screenY, width, height, tilesheet, src);
						}
					} else {
						Color tileColor = TileTypeToColor(tile);
						if(opacity < 100) {
							Color bgColor = Color(12, 17, 30);
							int alpha = opacity * 255 / 100;
							tileColor = Color(
								(tileColor.GetR() * alpha + bgColor.GetR() * (255 - alpha)) / 255,
								(tileColor.GetG() * alpha + bgColor.GetG() * (255 - alpha)) / 255,
								(tileColor.GetB() * alpha + bgColor.GetB() * (255 - alpha)) / 255
							);
						}
						w.DrawRect(screenX, screenY, width, height, tileColor);
					}
				}
			}
		}

		// Render next level (scrolling in from right)
		// Offset by current level width
		int levelWidth = levelColumns * gridSize;
		for(int layerIndex = nextLayerManager.GetLayerCount() - 1; layerIndex >= 0; layerIndex--) {
			const Layer& layer = nextLayerManager.GetLayer(layerIndex);
			if(!layer.IsVisible()) continue;

			const MapGrid& grid = layer.GetGrid();
			int opacity = layer.GetOpacity();

			for(int row = 0; row < nextLevelRows; row++) {
				for(int col = 0; col < nextLevelColumns; col++) {
					TileType tile = grid.GetTile(col, row);
					if(tile == TILE_EMPTY) continue;

					// World position offset by current level width
					Point worldBottomLeft(levelWidth + col * gridSize, row * gridSize);
					Point worldTopRight(levelWidth + (col + 1) * gridSize, (row + 1) * gridSize);
					Point screenBottomLeft = WorldToScreen(worldBottomLeft);
					Point screenTopRight = WorldToScreen(worldTopRight);

					int screenX = min(screenBottomLeft.x, screenTopRight.x);
					int screenY = min(screenBottomLeft.y, screenTopRight.y);
					int width = abs(screenTopRight.x - screenBottomLeft.x);
					int height = abs(screenTopRight.y - screenBottomLeft.y);

					// Sprite rendering: use tilesheet when available and opacity is full
					if(!tilesheet.IsEmpty() && opacity == 100) {
						bool hasL = grid.IsValid(col-1, row) && SameAutotileGroup(grid.GetTile(col-1, row), tile);
						bool hasR = grid.IsValid(col+1, row) && SameAutotileGroup(grid.GetTile(col+1, row), tile);
						bool hasT = grid.IsValid(col, row-1) && SameAutotileGroup(grid.GetTile(col, row-1), tile);
						bool hasB = grid.IsValid(col, row+1) && SameAutotileGroup(grid.GetTile(col, row+1), tile);
						Rect src = AutotileRect(tile, hasL, hasR, hasT, hasB);
						if(src.IsEmpty()) {
							w.DrawRect(screenX, screenY, width, height, TileTypeToColor(tile));
						} else {
							w.DrawImage(screenX, screenY, width, height, tilesheet, src);
						}
					} else {
						Color tileColor = TileTypeToColor(tile);
						if(opacity < 100) {
							Color bgColor = Color(12, 17, 30);
							int alpha = opacity * 255 / 100;
							tileColor = Color(
								(tileColor.GetR() * alpha + bgColor.GetR() * (255 - alpha)) / 255,
								(tileColor.GetG() * alpha + bgColor.GetG() * (255 - alpha)) / 255,
								(tileColor.GetB() * alpha + bgColor.GetB() * (255 - alpha)) / 255
							);
						}
						w.DrawRect(screenX, screenY, width, height, tileColor);
					}
				}
			}
		}
	}
	else {
		// Normal rendering (single level)
		int tileSize = int(gridSize * zoom);
		if(tileSize < 1) tileSize = 1;

		int viewCols = sz.cx / tileSize + 2;
		int viewRows = sz.cy / tileSize + 2;

		int startCol = max(0, int(cameraOffset.x / gridSize));
		int startRow = max(0, int(cameraOffset.y / gridSize));
		int endCol = min(levelColumns, startCol + viewCols);
		int endRow = min(levelRows, startRow + viewRows);

		for(int layerIndex = layerManager.GetLayerCount() - 1; layerIndex >= 0; layerIndex--) {
			const Layer& layer = layerManager.GetLayer(layerIndex);
			if(!layer.IsVisible()) continue;

			const MapGrid& grid = layer.GetGrid();
			int opacity = layer.GetOpacity();

			for(int row = startRow; row < endRow; row++) {
				for(int col = startCol; col < endCol; col++) {
					TileType tile = grid.GetTile(col, row);
					if(tile == TILE_EMPTY) continue;

					Point worldBottomLeft(col * gridSize, row * gridSize);
					Point worldTopRight((col + 1) * gridSize, (row + 1) * gridSize);
					Point screenBottomLeft = WorldToScreen(worldBottomLeft);
					Point screenTopRight = WorldToScreen(worldTopRight);

					int screenX = min(screenBottomLeft.x, screenTopRight.x);
					int screenY = min(screenBottomLeft.y, screenTopRight.y);
					int width = abs(screenTopRight.x - screenBottomLeft.x);
					int height = abs(screenTopRight.y - screenBottomLeft.y);

					// Sprite rendering: use tilesheet when available and opacity is full
					if(!tilesheet.IsEmpty() && opacity == 100) {
						bool hasL = grid.IsValid(col-1, row) && SameAutotileGroup(grid.GetTile(col-1, row), tile);
						bool hasR = grid.IsValid(col+1, row) && SameAutotileGroup(grid.GetTile(col+1, row), tile);
						bool hasT = grid.IsValid(col, row-1) && SameAutotileGroup(grid.GetTile(col, row-1), tile);
						bool hasB = grid.IsValid(col, row+1) && SameAutotileGroup(grid.GetTile(col, row+1), tile);
						Rect src = AutotileRect(tile, hasL, hasR, hasT, hasB);
						if(src.IsEmpty()) {
							w.DrawRect(screenX, screenY, width, height, TileTypeToColor(tile));
						} else {
							w.DrawImage(screenX, screenY, width, height, tilesheet, src);
						}
					} else {
						Color tileColor = TileTypeToColor(tile);
						if(opacity < 100) {
							Color bgColor = Color(12, 17, 30);
							int alpha = opacity * 255 / 100;
							tileColor = Color(
								(tileColor.GetR() * alpha + bgColor.GetR() * (255 - alpha)) / 255,
								(tileColor.GetG() * alpha + bgColor.GetG() * (255 - alpha)) / 255,
								(tileColor.GetB() * alpha + bgColor.GetB() * (255 - alpha)) / 255
							);
						}
						w.DrawRect(screenX, screenY, width, height, tileColor);
					}
				}
			}
		}
	}
}

bool GameScreen::Key(dword key, int) {
	// DEBUG - log every key
	RLOG("Key: 0x" << FormatIntHex(key) << " (" << (key & K_KEYUP ? "UP" : "DOWN")
	     << ") base=0x" << FormatIntHex(key & 0xFFFF)
	     << " keyLeft=" << keyLeft << " keyRight=" << keyRight);

	// Handle key downs based on game state
	switch(key) {
		case K_ESCAPE:
			if(gameState == PLAYING) {
				SetGameState(PAUSED);
				return true;
			} else if(gameState == PAUSED) {
				SetGameState(PLAYING);
				return true;
			}
			Close();
			return true;

		case K_P:
			if(gameState == PLAYING) {
				SetGameState(PAUSED);
			} else if(gameState == PAUSED) {
				SetGameState(PLAYING);
			}
			return true;

		case K_R:
			// Restart on game over or level complete
			if(gameState == GAME_OVER || gameState == LEVEL_COMPLETE) {
				RestartLevel();
				return true;
			}
			break;

		case K_RETURN:
			// Skip score summary immediately on Enter
			if(gameState == SCORE_SUMMARY) {
				scoreSummaryTimer = 0.0f;
				return true;
			}
			break;

		case K_C:
			// Toggle camera mode (only when not K_KEYUP)
			if(!(key & K_KEYUP)) {
				cameraMode = (cameraMode == CAMERA_FIXED) ? CAMERA_FOLLOW : CAMERA_FIXED;
				LOG("Camera mode: " << (cameraMode == CAMERA_FIXED ? "FIXED" : "FOLLOW"));
			}
			return true;

		case K_F3:
			if(!(key & K_KEYUP)) {
				showDebugOverlay = !showDebugOverlay;
				LOG("Debug overlay: " << (showDebugOverlay ? "ON" : "OFF"));
			}
			return true;

		// Movement keys (only work when playing)
		case K_LEFT:
		case K_A:
			if(gameState == PLAYING) keyLeft = true;
			return true;
		case K_RIGHT:
		case K_D:
			if(gameState == PLAYING) keyRight = true;
			return true;

		// Jump keys (only work when playing)
		case K_W:
		case K_UP:
			if(gameState == PLAYING) keyJump = true;
			return true;

		// Attack/Glide/Umbrella - Space bar (only work when playing)
		case K_SPACE:
			if(gameState == PLAYING) keyAttack = true;
			return true;

		// Key ups - handled separately to avoid modifier issues
		default:
			break;
	}

	// Handle key releases by checking K_KEYUP bit and masking modifiers
	if(key & K_KEYUP) {
		// Strip only the modifier bits, keep K_DELTA and actual key code
		dword baseKey = key & ~(K_KEYUP | K_SHIFT | K_CTRL | K_ALT);
		switch(baseKey) {
			case K_LEFT:
			case K_A:
				keyLeft = false;
				return true;
			case K_RIGHT:
			case K_D:
				keyRight = false;
				return true;
			case K_W:
			case K_UP:
				keyJump = false;
				return true;
			case K_SPACE:
				keyAttack = false;
				return true;
		}
	}

	return false;
}

void GameScreen::UpdateInput() {
	// Build input state from tracked keys
	inputState.moveLeft = keyLeft;
	inputState.moveRight = keyRight;
	inputState.jumpHeld = keyJump;
	inputState.jumpPressed = keyJump && !prevKeyJump;
	inputState.attackPressed = keyAttack && !prevKeyAttack;
	inputState.glideHeld = keyAttack;

	// DEBUG: Log input state
	static int inputLogCount = 0;
	inputLogCount++;
	if(inputLogCount % 60 == 0 || keyAttack) {
		RLOG("Input: keyLeft=" << keyLeft << " keyRight=" << keyRight
		     << " keyJump=" << keyJump << " keyAttack=" << keyAttack
		     << " glideHeld=" << inputState.glideHeld);
	}

	// NOTE: Don't update prevKey* here - do it at END of GameTick after all logic runs
}

bool GameScreen::IsFullBlockTile(int col, int row) {
	Layer* terrainLayer = layerManager.FindLayerByType(LAYER_TERRAIN);
	if(!terrainLayer) return false;

	return terrainLayer->GetGrid().GetTile(col, row) == TILE_FULLBLOCK;
}

bool GameScreen::IsWallTile(int col, int row) {
	Layer* terrainLayer = layerManager.FindLayerByType(LAYER_TERRAIN);
	if(!terrainLayer) return false;

	return terrainLayer->GetGrid().GetTile(col, row) == TILE_WALL;
}

bool GameScreen::IsFloorTile(int col, int row) {
	return IsWallTile(col, row) || IsFullBlockTile(col, row);
}

void GameScreen::RenderHUD(Draw& w) {
	Size sz = GetSize();

	// Draw dark background bar at top
	Color bgColor = Color(0, 0, 0);
	w.DrawRect(0, 0, sz.cx, 40, bgColor);

	// Draw lives (hearts)
	String livesText = Format("Lives: %d", player.GetLives());
	Font fnt = Arial(20).Bold();
	w.DrawText(20, 10, livesText, fnt, White());

	// Draw heart symbols
	Color heartColor = Color(255, 50, 50);
	for(int i = 0; i < player.GetLives(); i++) {
		int x = 120 + i * 25;
		int y = 10;
		// Draw simple heart shape as two rects (placeholder)
		w.DrawRect(x, y + 5, 8, 8, heartColor);
		w.DrawRect(x + 8, y + 5, 8, 8, heartColor);
		w.DrawRect(x + 2, y + 13, 12, 8, heartColor);
	}

	// Draw score
	String scoreText = Format("Score: %d", player.GetScore());
	Size textSz = GetTextSize(scoreText, fnt);
	w.DrawText(sz.cx - textSz.cx - 20, 10, scoreText, fnt, White());

	// Draw droplet counter (center)
	String dropletText = Format("Droplets: %d", dropletsCollected);
	Size dropletSz = GetTextSize(dropletText, fnt);
	w.DrawText(sz.cx / 2 - dropletSz.cx / 2, 10, dropletText, fnt, Color(100, 200, 255));
}

void GameScreen::RenderDebugOverlay(Draw& w) {
	Size sz = GetSize();
	Font fnt = Courier(14);
	Color bg = Color(0, 0, 0);
	Color fg = Color(0, 255, 0);
	int x = 10, y = 50;
	int lineH = 16;

	// Semi-transparent background panel
	int panelW = 320, panelH = lineH * 16 + 10;
	w.DrawRect(x - 4, y - 4, panelW, panelH, bg);

	auto Line = [&](const String& text) {
		w.DrawText(x, y, text, fnt, fg);
		y += lineH;
	};

	// FPS
	Line(Format("FPS: %d", debugFpsDisplay));

	// Player
	Pointf pp = player.GetPosition();
	Pointf pv = player.GetVelocity();
	Line(Format("Player pos: %.1f, %.1f", pp.x, pp.y));
	Line(Format("Player vel: %.1f, %.1f", pv.x, pv.y));
	Line(Format("Player lives: %d  score: %d", player.GetLives(), player.GetScore()));
	Line(Format("OnGround: %s  Facing: %d", player.IsOnGround() ? "yes" : "no", player.GetFacing()));
	Line(Format("Invincible: %s  Attacking: %s", player.IsInvincible() ? "yes" : "no", player.IsAttacking() ? "yes" : "no"));

	// Enemies
	int aliveEnemies = 0;
	for(int i = 0; i < enemies.GetCount(); i++)
		if(enemies[i]->IsAlive()) aliveEnemies++;
	Line(Format("Enemies: %d alive / %d total", aliveEnemies, enemies.GetCount()));

	// Droplets
	int orbitingCount = 0;
	for(int i = 0; i < droplets.GetCount(); i++)
		if(droplets[i]->IsCollected()) orbitingCount++;
	Line(Format("Droplets: %d collected, %d orbiting, huge=%s",
	     dropletsCollected, orbitingCount, hasHugeDroplet ? "yes" : "no"));
	Line(Format("Droplet spawns: %d  active: %d", dropletSpawns.GetCount(), droplets.GetCount()));

	// Water weapon
	if(waterWeapon.IsActive())
		Line(Format("WaterWeapon: (%d,%d) active", waterWeapon.GetCol(), waterWeapon.GetRow()));
	else
		Line("WaterWeapon: inactive");

	// GrimReaper
	if(reaper.IsSpawned())
		Line("GrimReaper: SPAWNED");
	else
		Line(Format("GrimReaper: spawns in %.1fs", reaper.TimeUntilSpawn()));

	// Camera
	Line(Format("Camera: %d,%d  zoom=%.1f  mode=%s",
	     cameraOffset.x, cameraOffset.y, zoom,
	     cameraMode == CAMERA_FIXED ? "FIXED" : "FOLLOW"));

	// Game state
	const char* stateNames[] = {"PLAYING", "PAUSED", "GAME_OVER", "LEVEL_COMPLETE",
	                             "HOVER", "SCROLL", "DROP", "SUMMARY"};
	Line(Format("State: %s  frame: %d  time: %.1fs",
	     stateNames[gameState], gameFrame, levelElapsedTime));
}

void GameScreen::RenderPauseScreen(Draw& w) {
	Size sz = GetSize();

	// Semi-transparent overlay
	Color overlayColor = Color(0, 0, 0);
	w.DrawRect(0, 0, sz.cx, sz.cy, overlayColor);

	// Title
	Font titleFont = Arial(48).Bold();
	String pauseText = "PAUSED";
	Size titleSz = GetTextSize(pauseText, titleFont);
	w.DrawText((sz.cx - titleSz.cx) / 2, sz.cy / 2 - 60, pauseText, titleFont, White());

	// Instructions
	Font instructFont = Arial(24);
	String resumeText = "Press ESC or P to Resume";
	Size resumeSz = GetTextSize(resumeText, instructFont);
	w.DrawText((sz.cx - resumeSz.cx) / 2, sz.cy / 2 + 20, resumeText, instructFont, White());
}

void GameScreen::RenderGameOverScreen(Draw& w) {
	Size sz = GetSize();

	// Dark overlay
	Color overlayColor = Color(20, 0, 0);
	w.DrawRect(0, 0, sz.cx, sz.cy, overlayColor);

	// Title
	Font titleFont = Arial(56).Bold();
	String gameOverText = "GAME OVER";
	Size titleSz = GetTextSize(gameOverText, titleFont);
	w.DrawText((sz.cx - titleSz.cx) / 2, sz.cy / 2 - 80, gameOverText, titleFont, Color(255, 50, 50));

	// Score
	Font scoreFont = Arial(32);
	String finalScore = Format("Final Score: %d", player.GetScore());
	Size scoreSz = GetTextSize(finalScore, scoreFont);
	w.DrawText((sz.cx - scoreSz.cx) / 2, sz.cy / 2, finalScore, scoreFont, White());

	// Instructions
	Font instructFont = Arial(24);
	String restartText = "Press R to Restart";
	Size restartSz = GetTextSize(restartText, instructFont);
	w.DrawText((sz.cx - restartSz.cx) / 2, sz.cy / 2 + 60, restartText, instructFont, Color(200, 200, 200));

	String quitText = "Press ESC to Quit";
	Size quitSz = GetTextSize(quitText, instructFont);
	w.DrawText((sz.cx - quitSz.cx) / 2, sz.cy / 2 + 100, quitText, instructFont, Color(200, 200, 200));
}

void GameScreen::RenderLevelCompleteScreen(Draw& w) {
	if(!GameSettings::SHOW_COMPLETION_MESSAGES) return;

	Size sz = GetSize();

	// Treat-collection countdown banner (shown during PLAYING while timer ticks)
	if(gameState == PLAYING) {
		Font timerFont = Arial(28).Bold();
		String timerText = Format("All Enemies Defeated!  Collect treats: %.1f", (double)levelCompleteTimer);
		Size timerSz = GetTextSize(timerText, timerFont);
		w.DrawRect((sz.cx - timerSz.cx) / 2 - 12, 18, timerSz.cx + 24, timerSz.cy + 12, Color(0, 0, 0));
		w.DrawText((sz.cx - timerSz.cx) / 2, 24, timerText, timerFont, Color(100, 255, 100));
		return;
	}

	// Full stats overlay for SCORE_SUMMARY and LEVEL_COMPLETE
	// --- dark semi-transparent panel ---
	int panelW = 520, panelH = 360;
	int panelX = (sz.cx - panelW) / 2;
	int panelY = (sz.cy - panelH) / 2;
	w.DrawRect(panelX, panelY, panelW, panelH, Color(10, 10, 30));
	// Panel border
	w.DrawRect(panelX,              panelY,              panelW, 3, Color(80, 180, 255));
	w.DrawRect(panelX,              panelY + panelH - 3, panelW, 3, Color(80, 180, 255));
	w.DrawRect(panelX,              panelY,              3, panelH, Color(80, 180, 255));
	w.DrawRect(panelX + panelW - 3, panelY,              3, panelH, Color(80, 180, 255));

	int cx = panelX + panelW / 2;
	int y  = panelY + 20;

	// Title
	Font titleFont = Arial(44).Bold();
	bool isFinalVictory = (gameState == LEVEL_COMPLETE);
	String titleText = isFinalVictory ? "VICTORY!" : "LEVEL COMPLETE!";
	Color titleColor = isFinalVictory ? Color(255, 215, 0) : Color(80, 255, 80);
	Size titleSz = GetTextSize(titleText, titleFont);
	w.DrawText(cx - titleSz.cx / 2, y, titleText, titleFont, titleColor);
	y += titleSz.cy + 16;

	// Separator
	w.DrawRect(panelX + 20, y, panelW - 40, 2, Color(80, 180, 255));
	y += 10;

	// Stat rows
	Font statFont  = Arial(22);
	Font statBFont = Arial(22).Bold();
	int labelX = panelX + 40;
	int valueX = panelX + panelW - 40;
	int rowH   = 36;

	auto DrawStatRow = [&](const String& label, const String& value, Color valueColor) {
		w.DrawText(labelX, y, label, statFont, Color(200, 200, 200));
		Size vSz = GetTextSize(value, statBFont);
		w.DrawText(valueX - vSz.cx, y, value, statBFont, valueColor);
		y += rowH;
	};

	// Time
	int mins = (int)levelElapsedTime / 60;
	int secs = (int)levelElapsedTime % 60;
	String timeStr = Format("%d:%02d", mins, secs);
	DrawStatRow("Time:", timeStr, Color(220, 220, 100));

	// Droplets
	String dropStr = Format("%d / %d", dropletsCollected, totalDroplets);
	Color dropColor = (totalDroplets > 0 && dropletsCollected >= totalDroplets)
	                  ? Color(100, 200, 255) : Color(180, 180, 180);
	DrawStatRow("Droplets:", dropStr, dropColor);

	// Damage taken
	Color dmgColor = (damageTakenThisLevel == 0) ? Color(100, 255, 100) : Color(255, 120, 80);
	DrawStatRow("Hits taken:", AsString(damageTakenThisLevel), dmgColor);

	// Bonus points
	DrawStatRow("Bonus:", "+" + AsString(levelScoreBonus), Color(255, 200, 50));

	// Total score
	w.DrawRect(panelX + 20, y, panelW - 40, 2, Color(80, 180, 255));
	y += 8;
	DrawStatRow("Score:", AsString(player.GetScore()), Color(255, 255, 255));

	// Grade (large, right side of panel)
	Font gradeFont = Arial(72).Bold();
	Color gradeColor = (levelGrade == "S") ? Color(255, 215, 0)
	                 : (levelGrade == "A") ? Color(100, 255, 100)
	                 : (levelGrade == "B") ? Color(100, 200, 255)
	                                       : Color(200, 200, 200);
	Size gradeSz = GetTextSize(levelGrade, gradeFont);
	int gradeX = panelX + panelW - 40 - gradeSz.cx;
	int gradeY = panelY + panelH - 100;
	w.DrawText(gradeX, gradeY, levelGrade, gradeFont, gradeColor);

	// Continue prompt
	Font promptFont = Arial(18);
	String promptText = isFinalVictory ? "Press R to play again"
	                                   : Format("Press Enter to continue  (%.0f)", (double)scoreSummaryTimer);
	Size promptSz = GetTextSize(promptText, promptFont);
	w.DrawText(cx - promptSz.cx / 2, panelY + panelH - 30, promptText, promptFont, Color(160, 160, 160));
}

void GameScreen::SetGameState(GameState newState) {
	const char* stateNames[] = {"PLAYING", "PAUSED", "GAME_OVER", "LEVEL_COMPLETE", "TRANSITION_HOVER", "TRANSITION_SCROLL", "TRANSITION_DROP", "SCORE_SUMMARY"};
	LOG("SetGameState: " << stateNames[gameState] << " -> " << stateNames[newState]);
	gameState = newState;
	if(newState == GAME_OVER)     GetAudioSystem().Play("gameover");
	else if(newState == LEVEL_COMPLETE) GetAudioSystem().Play("victory");

	// Reset input state when changing states
	keyLeft = keyRight = keyJump = keyAttack = false;
	prevKeyJump = prevKeyAttack = false;
}

void GameScreen::HandleGameOver() {
	EmitEvent("game_over");
	SetGameState(GAME_OVER);
}

void GameScreen::RestartLevel() {
	// Reload the level
	if(!levelPath.IsEmpty()) {
		player.ResetLives();
		reaper.Reset();
		LoadLevel(levelPath);
		SetGameState(PLAYING);
		lastTime = GetTickCount();
		accumulator = 0.0;
	}
}

void GameScreen::ClearEnemies() {
	// Clear observer pointer arrays before freeing VFS trees
	enemies.Clear();
	enemyRoot.sub.Clear();

	treats.Clear();
	treatRoot.sub.Clear();

	pickups.Clear();
	pickupRoot.sub.Clear();
}

void GameScreen::SpawnEnemies() {
	ClearEnemies();

	// Use enemy spawn points from level data if available
	if(enemySpawns.GetCount() > 0) {
		LOG("Spawning " << enemySpawns.GetCount() << " enemies from spawn points");

		for(int i = 0; i < enemySpawns.GetCount(); i++) {
			const EnemySpawnPoint& spawn = enemySpawns[i];

			// Calculate spawn position (use gridSize from level)
			float spawnX = spawn.col * gridSize;
			float spawnY = spawn.row * gridSize;

			// Create enemy based on type via VFS tree
			Enemy* enemy = nullptr;
			String eid = Format("e%d", i);
			switch(spawn.type) {
				case ENEMY_PATROLLER: {
					EnemyPatroller& ep = enemyRoot.Add<EnemyPatroller>(eid);
					ep.Init(spawnX, spawnY);
					enemy = &ep;
					break;
				}
				case ENEMY_JUMPER: {
					EnemyJumper& ej = enemyRoot.Add<EnemyJumper>(eid);
					ej.Init(spawnX, spawnY);
					enemy = &ej;
					break;
				}
				case ENEMY_SHOOTER: {
					EnemyShooter& es = enemyRoot.Add<EnemyShooter>(eid);
					es.Init(spawnX, spawnY);
					enemy = &es;
					break;
				}
				case ENEMY_FLYER: {
					EnemyFlyer& ef = enemyRoot.Add<EnemyFlyer>(eid);
					ef.Init(spawnX, spawnY);
					enemy = &ef;
					break;
				}
			}

			if(enemy) {
				enemy->WireAI(&pathfinder, &navGraph, this, spawn.col, spawn.row);
				enemies.Add(enemy);
				LOG("Spawned " << (spawn.type == ENEMY_PATROLLER ? "Patroller" :
				                    spawn.type == ENEMY_JUMPER ? "Jumper" :
				                    spawn.type == ENEMY_FLYER ? "Flyer" : "Shooter")
				    << " at (" << spawn.col << ", " << spawn.row << ")");
			}
		}
	}
	// Fallback: Use hardcoded spawning if no spawn points defined
	else if(levelColumns > 10 && levelRows > 5) {
		LOG("No enemy spawn points found, using fallback spawning");

		int fallbackIdx = 0;
		auto SpawnAt = [&](int col, int enemyRow, Enemy& e) {
			e.WireAI(&pathfinder, &navGraph, this, col, enemyRow);
			enemies.Add(&e);
		};

		// Spawn patroller at column 8, looking for ground
		for(int row = levelRows - 3; row >= 1; row--) {
			if(IsFloorTile(8, row)) {
				int spawnX = 8 * gridSize;
				int spawnY = (row + 1) * gridSize;
				EnemyPatroller& ep = enemyRoot.Add<EnemyPatroller>(Format("fb%d", fallbackIdx++));
				ep.Init((float)spawnX, (float)spawnY);
				SpawnAt(8, row + 1, ep);
				break;
			}
		}

		// Spawn jumper at column 15
		for(int row = levelRows - 3; row >= 1; row--) {
			if(IsFloorTile(15, row)) {
				int spawnX = 15 * gridSize;
				int spawnY = (row + 1) * gridSize;
				EnemyJumper& ej = enemyRoot.Add<EnemyJumper>(Format("fb%d", fallbackIdx++));
				ej.Init((float)spawnX, (float)spawnY);
				SpawnAt(15, row + 1, ej);
				break;
			}
		}

		// Spawn patroller at column 22
		for(int row = levelRows - 3; row >= 1; row--) {
			if(IsFloorTile(22, row)) {
				int spawnX = 22 * gridSize;
				int spawnY = (row + 1) * gridSize;
				EnemyPatroller& ep = enemyRoot.Add<EnemyPatroller>(Format("fb%d", fallbackIdx++));
				ep.Init((float)spawnX, (float)spawnY);
				SpawnAt(22, row + 1, ep);
				break;
			}
		}

		// Spawn shooter at column 12
		for(int row = levelRows - 3; row >= 1; row--) {
			if(IsFloorTile(12, row)) {
				int spawnX = 12 * gridSize;
				int spawnY = (row + 1) * gridSize;
				EnemyShooter& es = enemyRoot.Add<EnemyShooter>(Format("fb%d", fallbackIdx++));
				es.Init((float)spawnX, (float)spawnY);
				SpawnAt(12, row + 1, es);
				break;
			}
		}
		// Spawn flyer at center, mid-height (no floor needed)
		{
			int flyCol = levelColumns / 2;
			float flyX = flyCol * gridSize;
			float flyY = (levelRows / 2) * (float)gridSize;
			EnemyFlyer& ef = enemyRoot.Add<EnemyFlyer>(Format("fb%d", fallbackIdx++));
			ef.Init(flyX, flyY);
			SpawnAt(flyCol, levelRows / 2, ef);
		}
	}
}
