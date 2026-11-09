#ifndef _Umbrella_GameScreen_h_
#define _Umbrella_GameScreen_h_

#include <CtrlLib/CtrlLib.h>
#include "LayerManager.h"
#include "Player.h"
#include "Enemy.h"
#include "Droplet.h"
#include "Pathfinder.h"
#include "NavGraph.h"
#include "GrimReaper.h"
#include "WaterWeapon.h"

using namespace Upp;

// Forward declarations
class Treat;
class Pickup;

// ============================================================================
// Game event queue - decoupled producer/consumer for scripting and Shell integration
// ============================================================================
struct GameEvent : Moveable<GameEvent> {
	String type;   // "enemy_killed", "all_enemies_killed", "droplet_collected",
	               // "player_hit", "game_over", "level_complete", "treat_collected"
	int    data;   // Optional: enemy index for enemy_killed, damage for player_hit, etc.
	GameEvent() : data(0) {}
	GameEvent(const String& t, int d = 0) : type(t), data(d) {}
};

enum GameState {
	PLAYING,
	PAUSED,
	GAME_OVER,
	LEVEL_COMPLETE,
	TRANSITION_HOVER,      // Player hovering with umbrella
	TRANSITION_SCROLL,     // Levels scrolling horizontally
	TRANSITION_DROP,       // Player dropping into new level
	SCORE_SUMMARY          // Stats overlay before transitioning to next level
};

enum CameraMode {
	CAMERA_FIXED,          // Fixed camera: level centered, vertically fitted
	CAMERA_FOLLOW          // Follow player (classic platformer style)
};

class GameScreen : public TopWindow, public Player::CollisionHandler, public Player::CoordinateConverter {
public:  // Public for testing
	String levelPath;
	LayerManager layerManager;
	LayerManager nextLayerManager;  // For rendering next level during transition

	// Viewport/Camera
	Rect viewBounds;
	Point cameraOffset;
	float zoom;
	CameraMode cameraMode;

	// Game loop timing
	int64 lastTime;
	static constexpr double FIXED_TIMESTEP = 1.0 / 60.0;
	double accumulator;

	// Game state
	GameState gameState;
	float levelCompleteTimer;  // Countdown for treat collection before transition
	bool allEnemiesKilled;  // Track if all enemies have been defeated

	// Transition state
	float transitionOffset;  // Horizontal scroll offset during transition
	String nextLevelPath;  // Path to next level to load
	float hoverTimer;  // Timer for hover phase
	float dropTimer;  // Timer for drop phase
	int nextLevelColumns;  // Next level dimensions
	int nextLevelRows;

	// Level dimensions
	int levelColumns;
	int levelRows;
	int gridSize;

	// Player
	Player player;
	InputState inputState;

	// VFS roots: own the entity memory for each category (raw pointers below are observers)
	VfsValue enemyRoot;
	VfsValue treatRoot;
	VfsValue pickupRoot;
	VfsValue dropletRoot;

	// Enemies
	Array<Enemy*> enemies;

	// Treats (rewards from defeated enemies)
	Array<Treat*> treats;

	// Droplets (collectible water drops)
	Array<class Droplet*> droplets;
	Array<DropletSpawnPoint> dropletSpawns;  // Spawn points from level data
	Array<EnemySpawnPoint> enemySpawns;      // Enemy spawn points from level data
	int dropletsCollected;
	int totalDroplets;  // Total droplets available in current level
	bool hasHugeDroplet;  // True when 5 droplets merged into huge

	// Level stats (tracked per level for score summary)
	float levelElapsedTime;     // Time spent playing the current level
	int   damageTakenThisLevel; // Number of damage hits taken this level

	// Score summary state
	float  scoreSummaryTimer;   // Auto-advance countdown (seconds)
	int    levelScoreBonus;     // Bonus points calculated at completion
	String levelGrade;          // Letter grade: S / A / B / C

	// Pickups
	Array<Pickup*> pickups;

	// GrimReaper (indestructible time-pressure entity)
	GrimReaper reaper;

	// Water weapon (snake-like grid mover, activated by huge droplet)
	WaterWeapon waterWeapon;

	// Pathfinding (shared by all enemies, built on level load)
	Pathfinder pathfinder;
	NavGraph   navGraph;
	int        gameFrame;

	// Tile sprite sheet (loaded once from mod root alongside levels/)
	Image tilesheet;

	// Debug overlay
	bool showDebugOverlay;
	int  debugFpsCounter;
	int  debugFpsDisplay;
	float debugFpsTimer;

	// Input tracking
	bool keyLeft, keyRight, keyJump, keyAttack;
	bool prevKeyJump, prevKeyAttack;

	// Script event queue - populated during GameTick, drained by consumer (ScriptBridge/Shell)
	Vector<GameEvent> eventQueue;
	void EmitEvent(const String& type, int data = 0) { eventQueue.Add(GameEvent(type, data)); }

public:
	GameScreen();
	GameScreen(const String& level);

	// Main loop
	void GameTick(float delta);
	void LayoutLoop();

	// Rendering
	virtual void Paint(Draw& w) override;
	void RenderTiles(Draw& w);
	void RenderHUD(Draw& w);
	void RenderDebugOverlay(Draw& w);
	void RenderPauseScreen(Draw& w);
	void RenderGameOverScreen(Draw& w);
	void RenderLevelCompleteScreen(Draw& w);

	// Camera
	void UpdateCamera(Point targetPos);
	virtual Point WorldToScreen(Point worldPos) override;  // CoordinateConverter interface
	Point ScreenToWorld(Point screenPos);

	// Level loading
	bool LoadLevel(const String& path);
	String GetNextLevelPath(const String& currentPath);

	// Player spawning
	Point FindSpawnPoint();
	void RespawnPlayer();

	// Enemy management
	void SpawnEnemies();
	void ClearEnemies();

	// Input
	virtual bool Key(dword key, int) override;
	void UpdateInput();

	// Game state
	void SetGameState(GameState newState);
	GameState GetGameState() const { return gameState; }
	void HandleGameOver();
	void RestartLevel();

	// CollisionHandler interface
	virtual bool IsFullBlockTile(int col, int row) override;
	virtual bool IsWallTile(int col, int row) override;
	virtual bool IsFloorTile(int col, int row) override;
	virtual float GetGridSize() override { return (float)gridSize; }
};

#endif
