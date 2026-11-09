// Automated tests for GameScreen - run with bin/Umbrella --test
#include "Umbrella.h"
#include "GameScreen.h"
#include "MapSerializer.h"

using namespace Upp;

bool RunGameScreenTests() {
	int test_count = 0;
	int test_passed = 0;

	LOG("========================================");
	LOG("GameScreen Automated Tests");
	LOG("========================================");

	// Test 1: Level Loading
	{
		LOG("Test 1: Level Loading");
		test_count++;
		String testLevel = "share/mods/umbrella/levels/world1-stage1.json";

		if(!FileExists(testLevel)) {
			LOG("  ✗ FAILED: Test level not found: " << testLevel);
		} else {
			LayerManager layerMgr;
			bool loaded = MapSerializer::LoadFromFile(testLevel, layerMgr);

			if(!loaded) {
				LOG("  ✗ FAILED: Could not load level");
			} else if(layerMgr.GetLayerCount() != 4) {
				LOG("  ✗ FAILED: Expected 4 layers, got " << layerMgr.GetLayerCount());
			} else {
				Layer* terrainLayer = layerMgr.FindLayerByType(LAYER_TERRAIN);
				if(!terrainLayer) {
					LOG("  ✗ FAILED: Terrain layer not found");
				} else {
					const MapGrid& grid = terrainLayer->GetGrid();
					if(grid.GetColumns() == 0 || grid.GetRows() == 0) {
						LOG("  ✗ FAILED: Grid has zero dimensions");
					} else {
						LOG("  ✓ PASSED");
						test_passed++;
					}
				}
			}
		}
	}

	// Test 2: Camera Coordinate Transforms
	{
		LOG("Test 2: Camera Coordinate Transforms");
		test_count++;

		GameScreen screen;
		screen.zoom = 2.0f;
		screen.cameraOffset = Point(100, 50);

		Point worldPos(150, 100);
		Point screenPos = screen.WorldToScreen(worldPos);

		// Test that ScreenToWorld is the inverse of WorldToScreen
		Point worldPosBack = screen.ScreenToWorld(screenPos);
		if(worldPosBack.x != worldPos.x || worldPosBack.y != worldPos.y) {
			LOG("  ✗ FAILED: ScreenToWorld inverse failed - world(" << worldPos.x << "," << worldPos.y
			    << ") -> screen(" << screenPos.x << "," << screenPos.y
			    << ") -> world(" << worldPosBack.x << "," << worldPosBack.y << ")");
		} else {
			LOG("  ✓ PASSED");
			test_passed++;
		}
	}

	// Test 3: Camera Clamping
	{
		LOG("Test 3: Camera Clamping");
		test_count++;

		GameScreen screen;
		screen.SetRect(0, 0, 800, 600);
		screen.zoom = 2.0f;
		screen.levelColumns = 50;
		screen.levelRows = 30;
		screen.gridSize = 14;

		Point targetPos(10000, 10000);  // Way beyond level
		screen.UpdateCamera(targetPos);

		int levelWidth = screen.levelColumns * screen.gridSize;
		int levelHeight = screen.levelRows * screen.gridSize;
		int viewWidth = 800 / screen.zoom;
		int viewHeight = 600 / screen.zoom;

		if(screen.cameraOffset.x < 0 || screen.cameraOffset.y < 0) {
			LOG("  ✗ FAILED: Camera went negative");
		} else if(screen.cameraOffset.x > levelWidth - viewWidth ||
		          screen.cameraOffset.y > levelHeight - viewHeight) {
			LOG("  ✗ FAILED: Camera exceeded level bounds");
		} else {
			LOG("  ✓ PASSED");
			test_passed++;
		}
	}

	// Test 4: Layer Management
	{
		LOG("Test 4: Layer Management");
		test_count++;

		LayerManager layerMgr;
		layerMgr.InitializeDefaultLayers(10, 10);

		bool allPassed = true;
		for(int i = 0; i < layerMgr.GetLayerCount(); i++) {
			Layer& layer = layerMgr.GetLayer(i);
			if(!layer.IsVisible()) {
				LOG("  ✗ FAILED: Layer " << i << " not visible by default");
				allPassed = false;
				break;
			}

			layer.SetVisible(false);
			if(layer.IsVisible()) {
				LOG("  ✗ FAILED: SetVisible(false) didn't work");
				allPassed = false;
				break;
			}

			layer.SetVisible(true);
			if(!layer.IsVisible()) {
				LOG("  ✗ FAILED: SetVisible(true) didn't work");
				allPassed = false;
				break;
			}
		}

		if(allPassed) {
			LOG("  ✓ PASSED");
			test_passed++;
		}
	}

	// Test 5: Tile Operations
	{
		LOG("Test 5: Tile Operations");
		test_count++;

		LayerManager layerMgr;
		layerMgr.InitializeDefaultLayers(50, 30);

		Layer* terrainLayer = layerMgr.FindLayerByType(LAYER_TERRAIN);
		if(!terrainLayer) {
			LOG("  ✗ FAILED: Terrain layer not found");
		} else {
			terrainLayer->GetGrid().SetTile(10, 10, TILE_WALL);
			terrainLayer->GetGrid().SetTile(20, 15, TILE_FULLBLOCK);

			if(terrainLayer->GetGrid().GetTile(10, 10) != TILE_WALL) {
				LOG("  ✗ FAILED: SetTile/GetTile didn't work for TILE_WALL");
			} else if(terrainLayer->GetGrid().GetTile(20, 15) != TILE_FULLBLOCK) {
				LOG("  ✗ FAILED: SetTile/GetTile didn't work for TILE_FULLBLOCK");
			} else if(terrainLayer->GetGrid().GetTile(5, 5) != TILE_EMPTY) {
				LOG("  ✗ FAILED: Empty tile should be TILE_EMPTY");
			} else {
				LOG("  ✓ PASSED");
				test_passed++;
			}
		}
	}

	LOG("========================================");
	LOG("Results: " << test_passed << "/" << test_count << " tests passed");
	LOG("========================================");

	if(test_passed == test_count) {
		LOG("✓ ALL TESTS PASSED");
		return true;
	} else {
		LOG("✗ SOME TESTS FAILED");
		return false;
	}
}
