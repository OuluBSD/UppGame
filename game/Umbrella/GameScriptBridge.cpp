#include "Umbrella.h"
#include "GameScriptBridge.h"
#include "Player.h"
#include "LayerManager.h"
#include "Layer.h"
#include "Tile.h"
#include "Pathfinder.h"
#include "NavGraph.h"
#include "ActionExecutor.h"

using namespace Upp;

// ============================================================================
// Python API Functions - Player State Query
// ============================================================================

// Python API: get_player_position() -> [x, y]
static PyValue py_get_player_position(const Vector<PyValue>& args, void* user_data) {
	GameScriptBridge* bridge = (GameScriptBridge*)user_data;
	if(!bridge || !bridge->GetGameScreen()) {
		return PyValue::None();
	}

	const Player& player = bridge->GetGameScreen()->player;
	Pointf center = player.GetCenter();

	Vector<PyValue> result;
	result.Add(PyValue((double)center.x));
	result.Add(PyValue((double)center.y));
	return PyValue::FromVector(result);
}

// Python API: get_player_velocity() -> [vx, vy]
static PyValue py_get_player_velocity(const Vector<PyValue>& args, void* user_data) {
	GameScriptBridge* bridge = (GameScriptBridge*)user_data;
	if(!bridge || !bridge->GetGameScreen()) {
		return PyValue::None();
	}

	const Player& player = bridge->GetGameScreen()->player;
	Pointf vel = player.GetVelocity();

	Vector<PyValue> result;
	result.Add(PyValue((double)vel.x));
	result.Add(PyValue((double)vel.y));
	return PyValue::FromVector(result);
}

// Python API: get_player_lives() -> int
static PyValue py_get_player_lives(const Vector<PyValue>& args, void* user_data) {
	GameScriptBridge* bridge = (GameScriptBridge*)user_data;
	if(!bridge || !bridge->GetGameScreen()) {
		return PyValue::None();
	}

	int lives = bridge->GetGameScreen()->player.GetLives();
	return PyValue((int64)lives);
}

// Python API: get_player_score() -> int
static PyValue py_get_player_score(const Vector<PyValue>& args, void* user_data) {
	GameScriptBridge* bridge = (GameScriptBridge*)user_data;
	if(!bridge || !bridge->GetGameScreen()) {
		return PyValue::None();
	}

	int score = bridge->GetGameScreen()->player.GetScore();
	return PyValue((int64)score);
}

// Python API: get_player_state() -> str ("IDLE", "ATTACKING", "GLIDING")
static PyValue py_get_player_state(const Vector<PyValue>& args, void* user_data) {
	GameScriptBridge* bridge = (GameScriptBridge*)user_data;
	if(!bridge || !bridge->GetGameScreen()) {
		return PyValue::None();
	}

	const Player& player = bridge->GetGameScreen()->player;
	String stateStr;
	if(player.IsAttacking()) {
		stateStr = "ATTACKING";
	} else if(player.IsGliding()) {
		stateStr = "GLIDING";
	} else {
		stateStr = "IDLE";
	}

	return PyValue(stateStr.ToStd());
}

// Python API: is_player_on_ground() -> bool
static PyValue py_is_player_on_ground(const Vector<PyValue>& args, void* user_data) {
	GameScriptBridge* bridge = (GameScriptBridge*)user_data;
	if(!bridge || !bridge->GetGameScreen()) {
		return PyValue::None();
	}

	bool onGround = bridge->GetGameScreen()->player.IsOnGround();
	return PyValue(onGround);
}

// ============================================================================
// Python API Functions - Player State Manipulation
// ============================================================================

// Python API: set_player_position(x, y)
static PyValue py_set_player_position(const Vector<PyValue>& args, void* user_data) {
	GameScriptBridge* bridge = (GameScriptBridge*)user_data;
	if(!bridge || !bridge->GetGameScreen()) {
		return PyValue::None();
	}

	if(args.GetCount() < 2) {
		LOG("ERROR: set_player_position requires 2 arguments (x, y)");
		return PyValue::None();
	}

	float x = (float)args[0].AsDouble();
	float y = (float)args[1].AsDouble();

	bridge->GetGameScreen()->player.SetPosition(x, y);
	return PyValue::None();
}

// Python API: set_player_velocity(vx, vy)
static PyValue py_set_player_velocity(const Vector<PyValue>& args, void* user_data) {
	GameScriptBridge* bridge = (GameScriptBridge*)user_data;
	if(!bridge || !bridge->GetGameScreen()) {
		return PyValue::None();
	}

	if(args.GetCount() < 2) {
		LOG("ERROR: set_player_velocity requires 2 arguments (vx, vy)");
		return PyValue::None();
	}

	float vx = (float)args[0].AsDouble();
	float vy = (float)args[1].AsDouble();

	bridge->GetGameScreen()->player.SetVelocity(Pointf(vx, vy));
	return PyValue::None();
}

// Python API: add_player_score(points)
static PyValue py_add_player_score(const Vector<PyValue>& args, void* user_data) {
	GameScriptBridge* bridge = (GameScriptBridge*)user_data;
	if(!bridge || !bridge->GetGameScreen()) {
		return PyValue::None();
	}

	if(args.GetCount() < 1) {
		LOG("ERROR: add_player_score requires 1 argument (points)");
		return PyValue::None();
	}

	int points = (int)args[0].AsInt();
	bridge->GetGameScreen()->player.AddScore(points);
	return PyValue::None();
}

// ============================================================================
// Python API Functions - Enemy Management
// ============================================================================

// Python API: get_enemy_count() -> int
static PyValue py_get_enemy_count(const Vector<PyValue>& args, void* user_data) {
	GameScriptBridge* bridge = (GameScriptBridge*)user_data;
	if(!bridge || !bridge->GetGameScreen()) {
		return PyValue::None();
	}

	int count = bridge->GetGameScreen()->enemies.GetCount();
	return PyValue((int64)count);
}

// Python API: get_enemy(index) -> dict {position, type, alive, active, captured, facing}
static PyValue py_get_enemy(const Vector<PyValue>& args, void* user_data) {
	GameScriptBridge* bridge = (GameScriptBridge*)user_data;
	if(!bridge || !bridge->GetGameScreen()) {
		return PyValue::None();
	}

	if(args.GetCount() < 1) {
		LOG("ERROR: get_enemy requires 1 argument (index)");
		return PyValue::None();
	}

	int index = (int)args[0].AsInt();
	const Array<Enemy*>& enemies = bridge->GetGameScreen()->enemies;

	if(index < 0 || index >= enemies.GetCount()) {
		LOG("ERROR: get_enemy index out of range: " << index);
		return PyValue::None();
	}

	const Enemy* enemy = enemies[index];
	if(!enemy) {
		return PyValue::None();
	}

	// Create dict with enemy data
	PyValue result = PyValue::Dict();
	Rectf bounds = enemy->GetBounds();
	float centerX = bounds.left + bounds.Width() / 2.0f;
	float centerY = bounds.top + bounds.Height() / 2.0f;

	// Position as list [x, y]
	Vector<PyValue> pos;
	pos.Add(PyValue((double)centerX));
	pos.Add(PyValue((double)centerY));
	result.SetItem(PyValue("position"), PyValue::FromVector(pos));

	// Type as string
	String typeStr;
	switch(enemy->GetType()) {
		case ENEMY_PATROLLER: typeStr = "PATROLLER"; break;
		case ENEMY_JUMPER: typeStr = "JUMPER"; break;
		case ENEMY_SHOOTER: typeStr = "SHOOTER"; break;
		case ENEMY_FLYER: typeStr = "FLYER"; break;
	}
	result.SetItem(PyValue("type"), PyValue(typeStr.ToStd()));

	// States
	result.SetItem(PyValue("alive"), PyValue(enemy->IsAlive()));
	result.SetItem(PyValue("active"), PyValue(enemy->IsActive()));
	result.SetItem(PyValue("captured"), PyValue(enemy->IsCaptured()));
	result.SetItem(PyValue("thrown"), PyValue(enemy->IsThrown()));
	result.SetItem(PyValue("facing"), PyValue((int64)enemy->GetFacing()));

	return result;
}

// Python API: get_all_enemies() -> list of dicts
static PyValue py_get_all_enemies(const Vector<PyValue>& args, void* user_data) {
	GameScriptBridge* bridge = (GameScriptBridge*)user_data;
	if(!bridge || !bridge->GetGameScreen()) {
		return PyValue::None();
	}

	const Array<Enemy*>& enemies = bridge->GetGameScreen()->enemies;
	Vector<PyValue> result;

	for(int i = 0; i < enemies.GetCount(); i++) {
		Vector<PyValue> indexArg;
		indexArg.Add(PyValue((int64)i));
		PyValue enemyData = py_get_enemy(indexArg, user_data);
		if(!enemyData.IsNone()) {
			result.Add(enemyData);
		}
	}

	return PyValue::FromVector(result);
}

// Python API: kill_enemy(index)
static PyValue py_kill_enemy(const Vector<PyValue>& args, void* user_data) {
	GameScriptBridge* bridge = (GameScriptBridge*)user_data;
	if(!bridge || !bridge->GetGameScreen()) {
		return PyValue::None();
	}

	if(args.GetCount() < 1) {
		LOG("ERROR: kill_enemy requires 1 argument (index)");
		return PyValue::None();
	}

	int index = (int)args[0].AsInt();
	Array<Enemy*>& enemies = bridge->GetGameScreen()->enemies;

	if(index < 0 || index >= enemies.GetCount()) {
		LOG("ERROR: kill_enemy index out of range: " << index);
		return PyValue::None();
	}

	Enemy* enemy = enemies[index];
	if(enemy) {
		enemy->Kill();
	}

	return PyValue::None();
}

// Python API: clear_enemies()
static PyValue py_clear_enemies(const Vector<PyValue>& args, void* user_data) {
	GameScriptBridge* bridge = (GameScriptBridge*)user_data;
	if(!bridge || !bridge->GetGameScreen()) {
		return PyValue::None();
	}

	bridge->GetGameScreen()->ClearEnemies();
	return PyValue::None();
}

// ============================================================================
// Python API Functions - Collision/Physics Queries (Task 3)
// ============================================================================

// Helper: get terrain layer from GameScreen
static const MapGrid* GetTerrainGrid(GameScriptBridge* bridge) {
	if(!bridge || !bridge->GetGameScreen()) return nullptr;
	const Layer* terrainLayer = bridge->GetGameScreen()->layerManager.FindLayerByType(LAYER_TERRAIN);
	if(!terrainLayer) return nullptr;
	return &terrainLayer->GetGrid();
}

// Python API: get_tile_type(col, row) -> str ("EMPTY", "WALL", "BACKGROUND", "FULLBLOCK", "GOAL")
static PyValue py_get_tile_type(const Vector<PyValue>& args, void* user_data) {
	GameScriptBridge* bridge = (GameScriptBridge*)user_data;
	const MapGrid* grid = GetTerrainGrid(bridge);
	if(!grid) return PyValue::None();

	if(args.GetCount() < 2) {
		LOG("ERROR: get_tile_type requires 2 arguments (col, row)");
		return PyValue::None();
	}

	int col = (int)args[0].AsInt();
	int row = (int)args[1].AsInt();

	if(!grid->IsValid(col, row)) {
		return PyValue(String("EMPTY").ToStd());
	}

	TileType type = grid->GetTile(col, row);
	return PyValue(TileTypeToString(type).ToStd());
}

// Python API: is_tile_solid(col, row) -> bool
static PyValue py_is_tile_solid(const Vector<PyValue>& args, void* user_data) {
	GameScriptBridge* bridge = (GameScriptBridge*)user_data;
	if(!bridge || !bridge->GetGameScreen()) return PyValue(false);

	if(args.GetCount() < 2) {
		LOG("ERROR: is_tile_solid requires 2 arguments (col, row)");
		return PyValue(false);
	}

	int col = (int)args[0].AsInt();
	int row = (int)args[1].AsInt();

	bool solid = bridge->GetGameScreen()->IsFloorTile(col, row);
	return PyValue(solid);
}

// Python API: is_tile_wall(col, row) -> bool
static PyValue py_is_tile_wall(const Vector<PyValue>& args, void* user_data) {
	GameScriptBridge* bridge = (GameScriptBridge*)user_data;
	if(!bridge || !bridge->GetGameScreen()) return PyValue(false);

	if(args.GetCount() < 2) {
		LOG("ERROR: is_tile_wall requires 2 arguments (col, row)");
		return PyValue(false);
	}

	int col = (int)args[0].AsInt();
	int row = (int)args[1].AsInt();

	bool wall = bridge->GetGameScreen()->IsWallTile(col, row);
	return PyValue(wall);
}

// Python API: world_to_tile(x, y) -> [col, row]
static PyValue py_world_to_tile(const Vector<PyValue>& args, void* user_data) {
	GameScriptBridge* bridge = (GameScriptBridge*)user_data;
	if(!bridge || !bridge->GetGameScreen()) return PyValue::None();

	if(args.GetCount() < 2) {
		LOG("ERROR: world_to_tile requires 2 arguments (x, y)");
		return PyValue::None();
	}

	float wx = (float)args[0].AsDouble();
	float wy = (float)args[1].AsDouble();
	int gridSz = bridge->GetGameScreen()->gridSize;

	if(gridSz <= 0) return PyValue::None();

	int col = (int)(wx / gridSz);
	int row = (int)(wy / gridSz);

	Vector<PyValue> result;
	result.Add(PyValue((int64)col));
	result.Add(PyValue((int64)row));
	return PyValue::FromVector(result);
}

// Python API: tile_to_world(col, row) -> [x, y]
static PyValue py_tile_to_world(const Vector<PyValue>& args, void* user_data) {
	GameScriptBridge* bridge = (GameScriptBridge*)user_data;
	if(!bridge || !bridge->GetGameScreen()) return PyValue::None();

	if(args.GetCount() < 2) {
		LOG("ERROR: tile_to_world requires 2 arguments (col, row)");
		return PyValue::None();
	}

	int col = (int)args[0].AsInt();
	int row = (int)args[1].AsInt();
	int gridSz = bridge->GetGameScreen()->gridSize;

	float wx = (float)(col * gridSz);
	float wy = (float)(row * gridSz);

	Vector<PyValue> result;
	result.Add(PyValue((double)wx));
	result.Add(PyValue((double)wy));
	return PyValue::FromVector(result);
}

// ============================================================================
// Python API Functions - Level/Map State (Task 4)
// ============================================================================

// Python API: get_level_dimensions() -> {columns, rows, grid_size}
static PyValue py_get_level_dimensions(const Vector<PyValue>& args, void* user_data) {
	GameScriptBridge* bridge = (GameScriptBridge*)user_data;
	if(!bridge || !bridge->GetGameScreen()) return PyValue::None();

	const GameScreen* screen = bridge->GetGameScreen();
	PyValue result = PyValue::Dict();
	result.SetItem(PyValue("columns"), PyValue((int64)screen->levelColumns));
	result.SetItem(PyValue("rows"), PyValue((int64)screen->levelRows));
	result.SetItem(PyValue("grid_size"), PyValue((int64)screen->gridSize));
	return result;
}

// Python API: get_level_path() -> str
static PyValue py_get_level_path(const Vector<PyValue>& args, void* user_data) {
	GameScriptBridge* bridge = (GameScriptBridge*)user_data;
	if(!bridge || !bridge->GetGameScreen()) return PyValue::None();

	return PyValue(bridge->GetGameScreen()->levelPath.ToStd());
}

// Python API: get_game_state() -> str ("PLAYING", "PAUSED", "GAME_OVER", etc.)
static PyValue py_get_game_state(const Vector<PyValue>& args, void* user_data) {
	GameScriptBridge* bridge = (GameScriptBridge*)user_data;
	if(!bridge || !bridge->GetGameScreen()) return PyValue::None();

	String stateStr;
	switch(bridge->GetGameScreen()->GetGameState()) {
		case PLAYING:            stateStr = "PLAYING"; break;
		case PAUSED:             stateStr = "PAUSED"; break;
		case GAME_OVER:          stateStr = "GAME_OVER"; break;
		case LEVEL_COMPLETE:     stateStr = "LEVEL_COMPLETE"; break;
		case TRANSITION_HOVER:   stateStr = "TRANSITION_HOVER"; break;
		case TRANSITION_SCROLL:  stateStr = "TRANSITION_SCROLL"; break;
		case TRANSITION_DROP:    stateStr = "TRANSITION_DROP"; break;
		case SCORE_SUMMARY:      stateStr = "SCORE_SUMMARY"; break;
	}
	return PyValue(stateStr.ToStd());
}

// Python API: get_droplet_count() -> int
static PyValue py_get_droplet_count(const Vector<PyValue>& args, void* user_data) {
	GameScriptBridge* bridge = (GameScriptBridge*)user_data;
	if(!bridge || !bridge->GetGameScreen()) return PyValue::None();

	int count = bridge->GetGameScreen()->dropletsCollected;
	return PyValue((int64)count);
}

// Python API: get_droplet_remaining() -> int (total spawns minus collected)
static PyValue py_get_droplet_remaining(const Vector<PyValue>& args, void* user_data) {
	GameScriptBridge* bridge = (GameScriptBridge*)user_data;
	if(!bridge || !bridge->GetGameScreen()) return PyValue::None();

	const GameScreen* screen = bridge->GetGameScreen();
	int remaining = screen->droplets.GetCount();
	return PyValue((int64)remaining);
}

// ============================================================================
// Python API Functions - Input Injection (Phase 3, Task 1)
// ============================================================================

// Python API: set_key(name, pressed) - inject a key state
// Key names: "left", "right", "jump", "attack"
static PyValue py_set_key(const Vector<PyValue>& args, void* user_data) {
	GameScriptBridge* bridge = (GameScriptBridge*)user_data;
	if(!bridge || !bridge->GetGameScreen()) return PyValue::None();

	if(args.GetCount() < 2) {
		LOG("ERROR: set_key requires 2 arguments (key_name, pressed)");
		return PyValue::None();
	}

	String keyName = args[0].GetStr().ToString();
	bool pressed = args[1].IsTrue();
	GameScreen* screen = bridge->GetGameScreen();

	if(keyName == "left") {
		screen->keyLeft = pressed;
	} else if(keyName == "right") {
		screen->keyRight = pressed;
	} else if(keyName == "jump") {
		screen->keyJump = pressed;
	} else if(keyName == "attack") {
		screen->keyAttack = pressed;
	} else {
		LOG("ERROR: Unknown key name: " << keyName << " (valid: left, right, jump, attack)");
	}
	return PyValue::None();
}

// Python API: clear_keys() - release all keys
static PyValue py_clear_keys(const Vector<PyValue>& args, void* user_data) {
	GameScriptBridge* bridge = (GameScriptBridge*)user_data;
	if(!bridge || !bridge->GetGameScreen()) return PyValue::None();

	GameScreen* screen = bridge->GetGameScreen();
	screen->keyLeft = false;
	screen->keyRight = false;
	screen->keyJump = false;
	screen->keyAttack = false;
	return PyValue::None();
}

// Python API: get_keys() -> dict {left, right, jump, attack}
static PyValue py_get_keys(const Vector<PyValue>& args, void* user_data) {
	GameScriptBridge* bridge = (GameScriptBridge*)user_data;
	if(!bridge || !bridge->GetGameScreen()) return PyValue::None();

	const GameScreen* screen = bridge->GetGameScreen();
	PyValue result = PyValue::Dict();
	result.SetItem(PyValue("left"),   PyValue(screen->keyLeft));
	result.SetItem(PyValue("right"),  PyValue(screen->keyRight));
	result.SetItem(PyValue("jump"),   PyValue(screen->keyJump));
	result.SetItem(PyValue("attack"), PyValue(screen->keyAttack));
	return result;
}

// Python API: tick_game(delta) - advance one simulation step
// delta defaults to 1/60 (fixed timestep) if not provided
static PyValue py_tick_game(const Vector<PyValue>& args, void* user_data) {
	GameScriptBridge* bridge = (GameScriptBridge*)user_data;
	if(!bridge || !bridge->GetGameScreen()) return PyValue::None();

	float delta = (float)GameScreen::FIXED_TIMESTEP;
	if(args.GetCount() >= 1) {
		delta = (float)args[0].AsDouble();
	}

	bridge->GetGameScreen()->GameTick(delta);
	return PyValue::None();
}

// Python API: tick_game_frames(count) - advance N frames at fixed timestep
static PyValue py_tick_game_frames(const Vector<PyValue>& args, void* user_data) {
	GameScriptBridge* bridge = (GameScriptBridge*)user_data;
	if(!bridge || !bridge->GetGameScreen()) return PyValue::None();

	int count = 1;
	if(args.GetCount() >= 1) {
		count = (int)args[0].AsInt();
	}

	GameScreen* screen = bridge->GetGameScreen();
	float delta = (float)GameScreen::FIXED_TIMESTEP;
	for(int i = 0; i < count; i++) {
		screen->GameTick(delta);
	}
	return PyValue::None();
}

// ============================================================================
// Phase 5: Game Event Queue API
// Events are produced by GameScreen during GameTick and consumed here.
// The queue is decoupled (plain struct) so Shell integration can reuse it.
// ============================================================================

// get_events() -> list of {type, data} dicts (non-destructive peek)
static PyValue py_get_events(const Vector<PyValue>& args, void* user_data) {
	GameScriptBridge* bridge = (GameScriptBridge*)user_data;
	if(!bridge || !bridge->GetGameScreen()) return PyValue::FromVector(Vector<PyValue>());

	const Vector<GameEvent>& queue = bridge->GetGameScreen()->eventQueue;
	Vector<PyValue> result;
	for(int i = 0; i < queue.GetCount(); i++) {
		PyValue ev = PyValue::Dict();
		ev.SetItem(PyValue(String("type")), PyValue(queue[i].type));
		ev.SetItem(PyValue(String("data")), PyValue((int64)queue[i].data));
		result.Add(ev);
	}
	return PyValue::FromVector(result);
}

// drain_events() -> list of {type, data} dicts, clears the queue
static PyValue py_drain_events(const Vector<PyValue>& args, void* user_data) {
	GameScriptBridge* bridge = (GameScriptBridge*)user_data;
	if(!bridge || !bridge->GetGameScreen()) return PyValue::FromVector(Vector<PyValue>());

	Vector<GameEvent>& queue = bridge->GetGameScreen()->eventQueue;
	Vector<PyValue> result;
	for(int i = 0; i < queue.GetCount(); i++) {
		PyValue ev = PyValue::Dict();
		ev.SetItem(PyValue(String("type")), PyValue(queue[i].type));
		ev.SetItem(PyValue(String("data")), PyValue((int64)queue[i].data));
		result.Add(ev);
	}
	queue.Clear();
	return PyValue::FromVector(result);
}

// clear_events() - discard all pending events
static PyValue py_clear_events(const Vector<PyValue>& args, void* user_data) {
	GameScriptBridge* bridge = (GameScriptBridge*)user_data;
	if(!bridge || !bridge->GetGameScreen()) return PyValue::None();
	bridge->GetGameScreen()->eventQueue.Clear();
	return PyValue::None();
}

// ============================================================================
// Track 2 - AI Planning: Pathfinding
// find_path(startCol, startRow, goalCol, goalRow) -> list of {col, row, move_type}
// ============================================================================

static PyValue py_find_path(const Vector<PyValue>& args, void* user_data) {
	GameScriptBridge* bridge = (GameScriptBridge*)user_data;
	if(!bridge || !bridge->GetGameScreen()) return PyValue::FromVector(Vector<PyValue>());
	if(args.GetCount() < 4) {
		LOG("ERROR: find_path requires 4 args: startCol, startRow, goalCol, goalRow");
		return PyValue::FromVector(Vector<PyValue>());
	}

	int sc = (int)args[0].AsInt();
	int sr = (int)args[1].AsInt();
	int gc = (int)args[2].AsInt();
	int gr = (int)args[3].AsInt();

	Pathfinder pf;
	pf.SetGameScreen(bridge->GetGameScreen());
	Vector<PathNode> path = pf.FindPath(sc, sr, gc, gr);

	static const char* moveTypeNames[] = { "walk", "jump", "fall" };

	Vector<PyValue> result;
	for(int i = 0; i < path.GetCount(); i++) {
		const PathNode& n = path[i];
		PyValue node = PyValue::Dict();
		node.SetItem(PyValue(String("col")),       PyValue((int64)n.col));
		node.SetItem(PyValue(String("row")),       PyValue((int64)n.row));
		node.SetItem(PyValue(String("move_type")), PyValue(String(moveTypeNames[n.moveType])));
		result.Add(node);
	}
	return PyValue::FromVector(result);
}



// ============================================================================
// Track 2 - AI Planning: Action Executor (Phase 2)
// execute_path(path_list, max_frames=3000) -> {success, frames, nodes, reason}
// navigate_to(col, row, max_frames=3000)  -> {success, frames, nodes, reason}
// ============================================================================

static PyValue MakeExecResult(const ActionExecutor::Result& r) {
	PyValue result = PyValue::Dict();
	result.SetItem(PyValue(String("success")), PyValue((int64)(r.success ? 1 : 0)));
	result.SetItem(PyValue(String("frames")),  PyValue((int64)r.framesUsed));
	result.SetItem(PyValue(String("nodes")),   PyValue((int64)r.nodesCompleted));
	result.SetItem(PyValue(String("reason")),  PyValue(r.reason));
	return result;
}

static PyValue py_execute_path(const Vector<PyValue>& args, void* user_data) {
	GameScriptBridge* bridge = (GameScriptBridge*)user_data;
	if(!bridge || !bridge->GetGameScreen()) return PyValue::None();
	if(args.GetCount() < 1) return PyValue::None();

	int maxFrames = 3000;
	if(args.GetCount() >= 2) maxFrames = (int)args[1].AsInt();

	// Parse path list: [{col, row, move_type}, ...]
	Vector<PathNode> path;
	const PyValue& plist = args[0];
	for(int i = 0; i < plist.GetCount(); i++) {
		const PyValue& nd = plist.GetItem(i);
		PathNode pn;
		pn.col = (int)nd.GetItem(PyValue(String("col"))).AsInt();
		pn.row = (int)nd.GetItem(PyValue(String("row"))).AsInt();
		String mt = nd.GetItem(PyValue(String("move_type"))).GetStr().ToString();
		if(mt == "jump") pn.moveType = MOVE_JUMP;
		else if(mt == "fall") pn.moveType = MOVE_FALL;
		else pn.moveType = MOVE_WALK;
		path.Add(pn);
	}

	ActionExecutor exec;
	return MakeExecResult(exec.ExecutePath(bridge->GetGameScreen(), path, maxFrames));
}

static PyValue py_navigate_to(const Vector<PyValue>& args, void* user_data) {
	GameScriptBridge* bridge = (GameScriptBridge*)user_data;
	if(!bridge || !bridge->GetGameScreen()) return PyValue::None();
	if(args.GetCount() < 2) {
		LOG("ERROR: navigate_to requires (col, row [, max_frames])");
		return PyValue::None();
	}

	int goalCol   = (int)args[0].AsInt();
	int goalRow   = (int)args[1].AsInt();
	int maxFrames = 3000;
	if(args.GetCount() >= 3) maxFrames = (int)args[2].AsInt();

	// Compute start tile from player center
	const Player& p = bridge->GetGameScreen()->player;
	int startCol = (int)(p.GetCenter().x / 14);
	int startRow = (int)(p.GetCenter().y / 14);

	Pathfinder pf;
	pf.SetGameScreen(bridge->GetGameScreen());
	Vector<PathNode> path = pf.FindPath(startCol, startRow, goalCol, goalRow);

	if(path.IsEmpty()) {
		PyValue result = PyValue::Dict();
		result.SetItem(PyValue(String("success")), PyValue((int64)0));
		result.SetItem(PyValue(String("frames")),  PyValue((int64)0));
		result.SetItem(PyValue(String("nodes")),   PyValue((int64)0));
		result.SetItem(PyValue(String("reason")),  PyValue(String("no_path")));
		return result;
	}

	ActionExecutor exec;
	return MakeExecResult(exec.ExecutePath(bridge->GetGameScreen(), path, maxFrames));
}

// ============================================================================
// Track 2 - AI Planning: Navigation Graph (Phase 1, Task 2)
// build_nav_graph() -> {walkable, components, edges, cols, rows}
// is_reachable(c1, r1, c2, r2) -> bool
// get_component_id(col, row) -> int
// ============================================================================

static NavGraph s_navGraph;

static PyValue py_build_nav_graph(const Vector<PyValue>& args, void* user_data) {
	GameScriptBridge* bridge = (GameScriptBridge*)user_data;
	if(!bridge || !bridge->GetGameScreen()) return PyValue::None();
	s_navGraph.Build(bridge->GetGameScreen());
	PyValue result = PyValue::Dict();
	result.SetItem(PyValue(String("walkable")),   PyValue((int64)s_navGraph.GetWalkableCount()));
	result.SetItem(PyValue(String("components")), PyValue((int64)s_navGraph.GetComponentCount()));
	result.SetItem(PyValue(String("edges")),      PyValue((int64)s_navGraph.GetEdgeCount()));
	result.SetItem(PyValue(String("cols")),       PyValue((int64)s_navGraph.GetCols()));
	result.SetItem(PyValue(String("rows")),       PyValue((int64)s_navGraph.GetRows()));
	return result;
}

static PyValue py_is_reachable(const Vector<PyValue>& args, void* user_data) {
	if(args.GetCount() < 4) return PyValue((int64)0);
	int c1 = (int)args[0].AsInt(), r1 = (int)args[1].AsInt();
	int c2 = (int)args[2].AsInt(), r2 = (int)args[3].AsInt();
	return PyValue((int64)(s_navGraph.IsReachable(c1, r1, c2, r2) ? 1 : 0));
}

static PyValue py_get_component_id(const Vector<PyValue>& args, void* user_data) {
	if(args.GetCount() < 2) return PyValue((int64)-1);
	int col = (int)args[0].AsInt(), row = (int)args[1].AsInt();
	return PyValue((int64)s_navGraph.GetComponentId(col, row));
}

// ============================================================================
// GameScriptBridge Implementation
// ============================================================================

GameScriptBridge::GameScriptBridge() {
	gameScreen = nullptr;
}

void GameScriptBridge::RegisterGameAPI() {
	VectorMap<PyValue, PyValue>& globals = vm.GetGlobals();

	// Query functions
	globals.GetAdd("get_player_position") =
		PyValue::Function("get_player_position", py_get_player_position, this);
	globals.GetAdd("get_player_velocity") =
		PyValue::Function("get_player_velocity", py_get_player_velocity, this);
	globals.GetAdd("get_player_lives") =
		PyValue::Function("get_player_lives", py_get_player_lives, this);
	globals.GetAdd("get_player_score") =
		PyValue::Function("get_player_score", py_get_player_score, this);
	globals.GetAdd("get_player_state") =
		PyValue::Function("get_player_state", py_get_player_state, this);
	globals.GetAdd("is_player_on_ground") =
		PyValue::Function("is_player_on_ground", py_is_player_on_ground, this);

	// Manipulation functions
	globals.GetAdd("set_player_position") =
		PyValue::Function("set_player_position", py_set_player_position, this);
	globals.GetAdd("set_player_velocity") =
		PyValue::Function("set_player_velocity", py_set_player_velocity, this);
	globals.GetAdd("add_player_score") =
		PyValue::Function("add_player_score", py_add_player_score, this);

	// Enemy query functions
	globals.GetAdd("get_enemy_count") =
		PyValue::Function("get_enemy_count", py_get_enemy_count, this);
	globals.GetAdd("get_enemy") =
		PyValue::Function("get_enemy", py_get_enemy, this);
	globals.GetAdd("get_all_enemies") =
		PyValue::Function("get_all_enemies", py_get_all_enemies, this);

	// Enemy manipulation functions
	globals.GetAdd("kill_enemy") =
		PyValue::Function("kill_enemy", py_kill_enemy, this);
	globals.GetAdd("clear_enemies") =
		PyValue::Function("clear_enemies", py_clear_enemies, this);

	// Collision/physics query functions (Task 3)
	globals.GetAdd("get_tile_type") =
		PyValue::Function("get_tile_type", py_get_tile_type, this);
	globals.GetAdd("is_tile_solid") =
		PyValue::Function("is_tile_solid", py_is_tile_solid, this);
	globals.GetAdd("is_tile_wall") =
		PyValue::Function("is_tile_wall", py_is_tile_wall, this);
	globals.GetAdd("world_to_tile") =
		PyValue::Function("world_to_tile", py_world_to_tile, this);
	globals.GetAdd("tile_to_world") =
		PyValue::Function("tile_to_world", py_tile_to_world, this);

	// Level/map state functions (Task 4)
	globals.GetAdd("get_level_dimensions") =
		PyValue::Function("get_level_dimensions", py_get_level_dimensions, this);
	globals.GetAdd("get_level_path") =
		PyValue::Function("get_level_path", py_get_level_path, this);
	globals.GetAdd("get_game_state") =
		PyValue::Function("get_game_state", py_get_game_state, this);
	globals.GetAdd("get_droplet_count") =
		PyValue::Function("get_droplet_count", py_get_droplet_count, this);
	globals.GetAdd("get_droplet_remaining") =
		PyValue::Function("get_droplet_remaining", py_get_droplet_remaining, this);

	// Input injection functions (Phase 3)
	globals.GetAdd("set_key") =
		PyValue::Function("set_key", py_set_key, this);
	globals.GetAdd("clear_keys") =
		PyValue::Function("clear_keys", py_clear_keys, this);
	globals.GetAdd("get_keys") =
		PyValue::Function("get_keys", py_get_keys, this);
	globals.GetAdd("tick_game") =
		PyValue::Function("tick_game", py_tick_game, this);
	globals.GetAdd("tick_game_frames") =
		PyValue::Function("tick_game_frames", py_tick_game_frames, this);
	// Event queue (Phase 5)
	globals.GetAdd("get_events") =
		PyValue::Function("get_events", py_get_events, this);
	globals.GetAdd("drain_events") =
		PyValue::Function("drain_events", py_drain_events, this);
	globals.GetAdd("clear_events") =
		PyValue::Function("clear_events", py_clear_events, this);
	// AI pathfinding (Track 2)
	globals.GetAdd("find_path") =
		PyValue::Function("find_path", py_find_path, this);
	// Action executor (Track 2, Phase 2)
	globals.GetAdd("execute_path") =
		PyValue::Function("execute_path", py_execute_path, this);
	globals.GetAdd("navigate_to") =
		PyValue::Function("navigate_to", py_navigate_to, this);
	// Navigation graph (Track 2, Task 2)
	globals.GetAdd("build_nav_graph") =
		PyValue::Function("build_nav_graph", py_build_nav_graph, this);
	globals.GetAdd("is_reachable") =
		PyValue::Function("is_reachable", py_is_reachable, this);
	globals.GetAdd("get_component_id") =
		PyValue::Function("get_component_id", py_get_component_id, this);
}

bool GameScriptBridge::ExecuteScript(const String& scriptPath) {
	if(!FileExists(scriptPath)) {
		LOG("ERROR: Script not found: " << scriptPath);
		return false;
	}

	String source = LoadFile(scriptPath);
	if(source.IsEmpty()) {
		LOG("ERROR: Failed to load script: " << scriptPath);
		return false;
	}

	try {
		Tokenizer tokenizer;
		tokenizer.SkipPythonComments(true);
		if(!tokenizer.Process(source, scriptPath)) {
			LOG("ERROR: Failed to tokenize script");
			return false;
		}
		tokenizer.CombineTokens();

		PyCompiler compiler(tokenizer.GetTokens());
		Vector<PyIR> ir;
		compiler.Compile(ir);

		vm.SetIR(ir);
		vm.Run();
		return true;
	}
	catch(const Exc& e) {
		LOG("ERROR: Script exception: " << e);
		return false;
	}
}
