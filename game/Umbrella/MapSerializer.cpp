#include "Umbrella.h"
#include "MapSerializer.h"

using namespace Upp;

bool MapSerializer::LoadFromFile(const String& filePath, LayerManager& layerMgr) {
	// Load JSON file
	String jsonText = LoadFile(filePath);
	if(jsonText.IsEmpty()) {
		Exclamation("Failed to load file: " + filePath);
		return false;
	}

	// Parse JSON
	Value json = ParseJSON(jsonText);
	if(json.IsError()) {
		Exclamation("Failed to parse JSON: " + filePath);
		return false;
	}

	// Check required fields
	if(json["columns"].IsVoid() || json["rows"].IsVoid()) {
		Exclamation("Missing required fields (columns, rows) in: " + filePath);
		return false;
	}

	// Extract grid dimensions
	int columns = json["columns"];
	int rows = json["rows"];
	int gridSize = json["gridSize"].IsVoid() ? 14 : (int)json["gridSize"];
	int mapCols = json["mapCols"].IsVoid() ? 32 : (int)json["mapCols"];
	int mapRows = json["mapRows"].IsVoid() ? 24 : (int)json["mapRows"];

	// Clear and reinitialize layer manager
	layerMgr.ClearAllLayers();
	layerMgr.InitializeDefaultLayers(columns, rows);

	// Get layers
	Layer* terrainLayer = layerMgr.FindLayerByType(LAYER_TERRAIN);
	Layer* backgroundLayer = layerMgr.FindLayerByType(LAYER_BACKGROUND);

	if(!terrainLayer || !backgroundLayer) {
		Exclamation("Failed to find default layers");
		return false;
	}

	// Set map area
	terrainLayer->GetGrid().SetMapArea(mapCols, mapRows);
	backgroundLayer->GetGrid().SetMapArea(mapCols, mapRows);

	// Load walls
	if(!json["walls"].IsVoid()) {
		const ValueArray& wallArray = json["walls"];
		Vector<int> wallIndices = LoadTileIndices(wallArray);
		for(int index : wallIndices) {
			Point pt = IndexToColRow(index, columns);
			terrainLayer->GetGrid().SetTile(pt.x, pt.y, TILE_WALL);
		}
	}

	// Load fullBlocks
	if(!json["fullBlocks"].IsVoid()) {
		const ValueArray& blockArray = json["fullBlocks"];
		Vector<int> blockIndices = LoadTileIndices(blockArray);
		for(int index : blockIndices) {
			Point pt = IndexToColRow(index, columns);
			terrainLayer->GetGrid().SetTile(pt.x, pt.y, TILE_FULLBLOCK);
		}
	}

	// Load background
	if(!json["background"].IsVoid()) {
		const ValueArray& bgArray = json["background"];
		Vector<int> bgIndices = LoadTileIndices(bgArray);
		for(int index : bgIndices) {
			Point pt = IndexToColRow(index, columns);
			backgroundLayer->GetGrid().SetTile(pt.x, pt.y, TILE_BACKGROUND);
		}
	}

	// TODO: Load spawns (entity layer - future task)

	return true;
}

bool MapSerializer::LoadDropletSpawns(const String& filePath, Array<DropletSpawnPoint>& spawns) {
	// Load JSON file
	String jsonText = LoadFile(filePath);
	if(jsonText.IsEmpty()) {
		return false;  // File doesn't exist or empty
	}

	// Parse JSON
	Value json = ParseJSON(jsonText);
	if(json.IsError()) {
		return false;
	}

	// Check if droplets array exists
	if(json["droplets"].IsVoid()) {
		return true;  // No droplets defined, but not an error
	}

	// Parse droplet spawn points
	const ValueArray& dropletArray = json["droplets"];
	for(int i = 0; i < dropletArray.GetCount(); i++) {
		const ValueMap& dropletData = dropletArray[i];

		DropletSpawnPoint spawn;

		// Required fields
		if(dropletData["col"].IsVoid() || dropletData["row"].IsVoid()) {
			LOG("Warning: Droplet spawn missing col/row, skipping");
			continue;
		}

		spawn.col = dropletData["col"];
		spawn.row = dropletData["row"];

		// Mode (default: RAINBOW)
		if(!dropletData["mode"].IsVoid()) {
			String modeStr = dropletData["mode"];
			if(modeStr == "ICE") spawn.mode = DROPLET_ICE;
			else if(modeStr == "FIRE") spawn.mode = DROPLET_FIRE;
			else spawn.mode = DROPLET_RAINBOW;
		}

		// Direction (default: 1)
		spawn.direction = dropletData["direction"].IsVoid() ? 1 : (int)dropletData["direction"];

		// Interval (default: 2000ms)
		spawn.intervalMs = dropletData["intervalMs"].IsVoid() ? 2000 : (int)dropletData["intervalMs"];

		// Enabled (default: true)
		spawn.enabled = dropletData["enabled"].IsVoid() ? true : (bool)dropletData["enabled"];

		spawns.Add(pick(spawn));
	}

	LOG("Loaded " << spawns.GetCount() << " droplet spawn points from " << filePath);
	return true;
}

bool MapSerializer::LoadEnemySpawns(const String& filePath, Array<EnemySpawnPoint>& spawns) {
	// Load JSON file
	String jsonText = LoadFile(filePath);
	if(jsonText.IsEmpty()) {
		return false;  // File doesn't exist or empty
	}

	// Parse JSON
	Value json = ParseJSON(jsonText);
	if(json.IsError()) {
		return false;
	}

	// Check if enemies array exists
	if(json["enemies"].IsVoid()) {
		return true;  // No enemies defined, but not an error
	}

	// Parse enemy spawn points
	const ValueArray& enemyArray = json["enemies"];
	for(int i = 0; i < enemyArray.GetCount(); i++) {
		const ValueMap& enemyData = enemyArray[i];

		EnemySpawnPoint spawn;

		// Required fields
		if(enemyData["col"].IsVoid() || enemyData["row"].IsVoid()) {
			LOG("Warning: Enemy spawn missing col/row, skipping");
			continue;
		}

		spawn.col = enemyData["col"];
		spawn.row = enemyData["row"];

		// Type (default: PATROLLER)
		if(!enemyData["type"].IsVoid()) {
			String typeStr = enemyData["type"];
			if(typeStr == "JUMPER") spawn.type = ENEMY_JUMPER;
			else if(typeStr == "SHOOTER") spawn.type = ENEMY_SHOOTER;
			else if(typeStr == "FLYER") spawn.type = ENEMY_FLYER;
			else spawn.type = ENEMY_PATROLLER;
		}

		// Facing (default: 1 = right)
		spawn.facing = enemyData["facing"].IsVoid() ? 1 : (int)enemyData["facing"];

		spawns.Add(pick(spawn));
	}

	LOG("Loaded " << spawns.GetCount() << " enemy spawn points from " << filePath);
	return true;
}

bool MapSerializer::LoadPickupSpawns(const String& filePath, Array<PickupSpawnPoint>& spawns) {
	String jsonText = LoadFile(filePath);
	if(jsonText.IsEmpty()) return false;

	Value json = ParseJSON(jsonText);
	if(json.IsError()) return false;

	if(json["pickups"].IsVoid()) return true;

	const ValueArray& pickupArray = json["pickups"];
	for(int i = 0; i < pickupArray.GetCount(); i++) {
		const ValueMap& data = pickupArray[i];

		if(data["col"].IsVoid() || data["row"].IsVoid()) {
			LOG("Warning: Pickup spawn missing col/row, skipping");
			continue;
		}

		PickupSpawnPoint spawn;
		spawn.col = data["col"];
		spawn.row = data["row"];

		if(!data["type"].IsVoid()) {
			String typeStr = data["type"];
			if(typeStr == "GEM") spawn.type = PU_GEM;
			else if(typeStr == "LIGHTNING") spawn.type = PU_LIGHTNING;
			else if(typeStr == "SPEED") spawn.type = PU_SPEED;
			else spawn.type = PU_HEART;
		}

		spawns.Add(pick(spawn));
	}

	LOG("Loaded " << spawns.GetCount() << " pickup spawn points from " << filePath);
	return true;
}

Vector<int> MapSerializer::LoadTileIndices(const ValueArray& jsonArray) {
	Vector<int> indices;

	for(int i = 0; i < jsonArray.GetCount(); i++) {
		indices.Add(jsonArray[i]);
	}

	return indices;
}

Point MapSerializer::IndexToColRow(int index, int columns) {
	return Point(index % columns, index / columns);
}

bool MapSerializer::SaveToFile(const String& filePath, const LayerManager& layerMgr,
                                const Array<EnemySpawnPoint>* enemySpawns,
                                const Array<DropletSpawnPoint>* dropletSpawns,
                                const Array<PickupSpawnPoint>* pickupSpawns) {
	// Get layers
	const Layer* terrainLayer = layerMgr.FindLayerByType(LAYER_TERRAIN);
	const Layer* backgroundLayer = layerMgr.FindLayerByType(LAYER_BACKGROUND);

	if(!terrainLayer || !backgroundLayer) {
		Exclamation("Failed to find layers for saving");
		return false;
	}

	const MapGrid& terrainGrid = terrainLayer->GetGrid();
	const MapGrid& backgroundGrid = backgroundLayer->GetGrid();

	int columns = terrainGrid.GetColumns();
	int rows = terrainGrid.GetRows();
	int mapCols = terrainGrid.GetMapCols();
	int mapRows = terrainGrid.GetMapRows();

	// Collect tile indices
	Vector<int> wallIndices, fullBlockIndices, backgroundIndices;

	for(int row = 0; row < rows; row++) {
		for(int col = 0; col < columns; col++) {
			// Check terrain layer
			TileType terrainTile = terrainGrid.GetTile(col, row);
			if(terrainTile == TILE_WALL) {
				wallIndices.Add(row * columns + col);
			}
			else if(terrainTile == TILE_FULLBLOCK) {
				fullBlockIndices.Add(row * columns + col);
			}

			// Check background layer
			TileType bgTile = backgroundGrid.GetTile(col, row);
			if(bgTile == TILE_BACKGROUND) {
				backgroundIndices.Add(row * columns + col);
			}
		}
	}

	// Build JSON object using ValueMap
	ValueMap jsonObj;
	jsonObj.Add("columns", columns);
	jsonObj.Add("rows", rows);
	jsonObj.Add("gridSize", 14);
	jsonObj.Add("mapCols", mapCols);
	jsonObj.Add("mapRows", mapRows);

	// Add wall indices
	ValueArray wallArray;
	for(int idx : wallIndices)
		wallArray.Add(idx);
	jsonObj.Add("walls", wallArray);

	// Add background indices
	ValueArray bgArray;
	for(int idx : backgroundIndices)
		bgArray.Add(idx);
	jsonObj.Add("background", bgArray);

	// Add fullBlock indices
	ValueArray blockArray;
	for(int idx : fullBlockIndices)
		blockArray.Add(idx);
	jsonObj.Add("fullBlocks", blockArray);

	// Add enemy spawns if provided
	if(enemySpawns && enemySpawns->GetCount() > 0) {
		ValueArray enemiesArray;
		for(const EnemySpawnPoint& spawn : *enemySpawns) {
			ValueMap spawnObj;
			spawnObj.Add("col", spawn.col);
			spawnObj.Add("row", spawn.row);
			spawnObj.Add("facing", spawn.facing);

			// Convert type enum to string
			String typeStr;
			switch(spawn.type) {
				case ENEMY_PATROLLER: typeStr = "PATROLLER"; break;
				case ENEMY_JUMPER: typeStr = "JUMPER"; break;
				case ENEMY_SHOOTER: typeStr = "SHOOTER"; break;
				case ENEMY_FLYER: typeStr = "FLYER"; break;
			}
			spawnObj.Add("type", typeStr);

			enemiesArray.Add(spawnObj);
		}
		jsonObj.Add("enemies", enemiesArray);
	}

	// Add droplet spawns if provided
	if(dropletSpawns && dropletSpawns->GetCount() > 0) {
		ValueArray dropletsArray;
		for(const DropletSpawnPoint& spawn : *dropletSpawns) {
			ValueMap spawnObj;
			spawnObj.Add("col", spawn.col);
			spawnObj.Add("row", spawn.row);
			spawnObj.Add("direction", spawn.direction);
			spawnObj.Add("intervalMs", spawn.intervalMs);
			spawnObj.Add("enabled", spawn.enabled);

			// Convert mode enum to string
			String modeStr;
			switch(spawn.mode) {
				case DROPLET_RAINBOW: modeStr = "RAINBOW"; break;
				case DROPLET_ICE: modeStr = "ICE"; break;
				case DROPLET_FIRE: modeStr = "FIRE"; break;
			}
			spawnObj.Add("mode", modeStr);

			dropletsArray.Add(spawnObj);
		}
		jsonObj.Add("droplets", dropletsArray);
	}

	// Add pickup spawns if provided
	if(pickupSpawns && pickupSpawns->GetCount() > 0) {
		ValueArray pickupsArray;
		for(const PickupSpawnPoint& spawn : *pickupSpawns) {
			ValueMap spawnObj;
			spawnObj.Add("col", spawn.col);
			spawnObj.Add("row", spawn.row);

			String typeStr;
			switch(spawn.type) {
				case PU_HEART:     typeStr = "HEART"; break;
				case PU_GEM:       typeStr = "GEM"; break;
				case PU_LIGHTNING: typeStr = "LIGHTNING"; break;
				case PU_SPEED:     typeStr = "SPEED"; break;
			}
			spawnObj.Add("type", typeStr);

			pickupsArray.Add(spawnObj);
		}
		jsonObj.Add("pickups", pickupsArray);
	}

	// Convert to JSON string with nice formatting
	String jsonText = AsJSON(jsonObj, true);

	// Save to file
	if(!SaveFile(filePath, jsonText)) {
		Exclamation("Failed to write file: " + filePath);
		return false;
	}

	LOG("Saved map to: " << filePath);
	return true;
}
