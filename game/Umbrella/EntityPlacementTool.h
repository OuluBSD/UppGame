#ifndef _Umbrella_EntityPlacementTool_h_
#define _Umbrella_EntityPlacementTool_h_

#include <Core/Core.h>
#include "Enemy.h"
#include "Droplet.h"
#include "Pickup.h"

using namespace Upp;

enum EntityPlacementMode {
	PLACEMENT_ADD,
	PLACEMENT_REMOVE
};

enum EntityToolType {
	ENTITY_ENEMY,
	ENTITY_DROPLET,
	ENTITY_PICKUP
};

class EntityPlacementTool {
protected:
	EntityPlacementMode mode;
	EntityToolType toolType;

public:
	EntityPlacementTool(EntityToolType type) : mode(PLACEMENT_ADD), toolType(type) {}
	virtual ~EntityPlacementTool() {}

	void SetMode(EntityPlacementMode m) { mode = m; }
	EntityPlacementMode GetMode() const { return mode; }
	EntityToolType GetToolType() const { return toolType; }

	virtual void Click(int col, int row) = 0;
	virtual void Render(Draw& w, int col, int row, Point offset, double zoom, int gridSize) = 0;
};

class EnemyPlacementTool : public EntityPlacementTool {
private:
	Array<EnemySpawnPoint>* enemySpawns;  // Reference to map's enemy spawn array
	EnemyType selectedType;
	int selectedFacing;

public:
	EnemyPlacementTool();

	void SetEnemySpawns(Array<EnemySpawnPoint>* spawns) { enemySpawns = spawns; }
	void SetSelectedType(EnemyType type) { selectedType = type; }
	EnemyType GetSelectedType() const { return selectedType; }
	void SetSelectedFacing(int facing) { selectedFacing = facing; }
	int GetSelectedFacing() const { return selectedFacing; }

	virtual void Click(int col, int row) override;
	virtual void Render(Draw& w, int col, int row, Point offset, double zoom, int gridSize) override;

private:
	int FindSpawnAt(int col, int row) const;
};

class DropletPlacementTool : public EntityPlacementTool {
private:
	Array<DropletSpawnPoint>* dropletSpawns;  // Reference to map's droplet spawn array
	DropletType selectedType;
	int selectedDirection;
	int selectedInterval;

public:
	DropletPlacementTool();

	void SetDropletSpawns(Array<DropletSpawnPoint>* spawns) { dropletSpawns = spawns; }
	void SetSelectedType(DropletType type) { selectedType = type; }
	DropletType GetSelectedType() const { return selectedType; }
	void SetSelectedDirection(int dir) { selectedDirection = dir; }
	int GetSelectedDirection() const { return selectedDirection; }
	void SetSelectedInterval(int interval) { selectedInterval = interval; }
	int GetSelectedInterval() const { return selectedInterval; }

	virtual void Click(int col, int row) override;
	virtual void Render(Draw& w, int col, int row, Point offset, double zoom, int gridSize) override;

private:
	int FindSpawnAt(int col, int row) const;
};

class PickupPlacementTool : public EntityPlacementTool {
private:
	Array<PickupSpawnPoint>* pickupSpawns;
	PickupType selectedType;

public:
	PickupPlacementTool();

	void SetPickupSpawns(Array<PickupSpawnPoint>* spawns) { pickupSpawns = spawns; }
	void SetSelectedType(PickupType type) { selectedType = type; }
	PickupType GetSelectedType() const { return selectedType; }

	virtual void Click(int col, int row) override;
	virtual void Render(Draw& w, int col, int row, Point offset, double zoom, int gridSize) override;

private:
	int FindSpawnAt(int col, int row) const;
};

#endif
