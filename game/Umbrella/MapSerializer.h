#ifndef _Umbrella_MapSerializer_h_
#define _Umbrella_MapSerializer_h_

#include "LayerManager.h"
#include "Droplet.h"
#include "Enemy.h"
#include "Pickup.h"

using namespace Upp;

class MapSerializer {
public:
	// Load map from JSON file
	static bool LoadFromFile(const String& filePath, LayerManager& layerMgr);

	// Load droplet spawn points from JSON file
	static bool LoadDropletSpawns(const String& filePath, Array<DropletSpawnPoint>& spawns);

	// Load enemy spawn points from JSON file
	static bool LoadEnemySpawns(const String& filePath, Array<EnemySpawnPoint>& spawns);

	// Load pickup spawn points from JSON file
	static bool LoadPickupSpawns(const String& filePath, Array<PickupSpawnPoint>& spawns);

	// Save map to JSON file
	static bool SaveToFile(const String& filePath, const LayerManager& layerMgr,
	                       const Array<EnemySpawnPoint>* enemySpawns = nullptr,
	                       const Array<DropletSpawnPoint>* dropletSpawns = nullptr,
	                       const Array<PickupSpawnPoint>* pickupSpawns = nullptr);

private:
	// Helper: Load tile indices from JSON array
	static Vector<int> LoadTileIndices(const ValueArray& jsonArray);

	// Helper: Convert linear index to (col, row)
	static Point IndexToColRow(int index, int columns);
};

#endif
