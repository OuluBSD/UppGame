#include "../Umbrella.h"
#include "../GameScreen.h"
#include "../MapSerializer.h"

using namespace Upp;

// Simple test framework
int test_count = 0;
int test_passed = 0;

#define TEST(name) \
	void test_##name(); \
	struct TestRegistrar_##name { \
		TestRegistrar_##name() { \
			LOG("Running test: " #name); \
			test_count++; \
			try { \
				test_##name(); \
				test_passed++; \
				LOG("  ✓ PASSED"); \
			} catch(const char* msg) { \
				LOG("  ✗ FAILED: " << msg); \
			} \
		} \
	} test_registrar_##name; \
	void test_##name()

#define ASSERT(cond) \
	if(!(cond)) { \
		throw "Assertion failed: " #cond; \
	}

#define ASSERT_EQ(a, b) \
	if((a) != (b)) { \
		throw Format("Assertion failed: %s != %s (got %d vs %d)", #a, #b, (int)(a), (int)(b)); \
	}

// Test 1: Level Loading
TEST(LevelLoading) {
	String testLevel = "share/mods/umbrella/levels/world1-stage1.json";

	ASSERT(FileExists(testLevel));

	LayerManager layerMgr;
	bool loaded = MapSerializer::LoadFromFile(testLevel, layerMgr);
	ASSERT(loaded);

	// Verify layers were created
	ASSERT(layerMgr.GetLayerCount() == 4);

	// Verify terrain layer exists and has data
	Layer* terrainLayer = layerMgr.FindLayerByType(LAYER_TERRAIN);
	ASSERT(terrainLayer != nullptr);

	const MapGrid& grid = terrainLayer->GetGrid();
	ASSERT(grid.GetColumns() > 0);
	ASSERT(grid.GetRows() > 0);
}

// Test 2: Camera Coordinate Transforms
TEST(CameraTransforms) {
	// Create a test GameScreen (don't show window)
	GameScreen screen;

	// Manually set camera and zoom
	screen.zoom = 2.0f;
	screen.cameraOffset = Point(100, 50);

	// Test WorldToScreen
	Point worldPos(150, 100);
	Point screenPos = screen.WorldToScreen(worldPos);

	// Expected: (150-100)*2 = 100, (100-50)*2 = 100
	ASSERT_EQ(screenPos.x, 100);
	ASSERT_EQ(screenPos.y, 100);

	// Test ScreenToWorld (inverse)
	Point worldPosBack = screen.ScreenToWorld(screenPos);
	ASSERT_EQ(worldPosBack.x, worldPos.x);
	ASSERT_EQ(worldPosBack.y, worldPos.y);
}

// Test 3: Camera Clamping
TEST(CameraClamping) {
	GameScreen screen;
	screen.SetRect(0, 0, 800, 600);  // Window size
	screen.zoom = 2.0f;
	screen.levelColumns = 50;
	screen.levelRows = 30;
	screen.gridSize = 14;

	// Try to move camera beyond level bounds
	Point targetPos(10000, 10000);  // Way beyond level
	screen.UpdateCamera(targetPos);

	// Camera should be clamped to level bounds
	int levelWidth = screen.levelColumns * screen.gridSize;
	int levelHeight = screen.levelRows * screen.gridSize;
	int viewWidth = 800 / screen.zoom;
	int viewHeight = 600 / screen.zoom;

	ASSERT(screen.cameraOffset.x >= 0);
	ASSERT(screen.cameraOffset.y >= 0);
	ASSERT(screen.cameraOffset.x <= levelWidth - viewWidth);
	ASSERT(screen.cameraOffset.y <= levelHeight - viewHeight);
}

// Test 4: Tile Rendering Bounds
TEST(TileRenderingBounds) {
	LayerManager layerMgr;
	layerMgr.InitializeDefaultLayers(50, 30);

	// Add some test tiles
	Layer* terrainLayer = layerMgr.FindLayerByType(LAYER_TERRAIN);
	ASSERT(terrainLayer != nullptr);

	terrainLayer->GetGrid().SetTile(10, 10, TILE_WALL);
	terrainLayer->GetGrid().SetTile(20, 15, TILE_FULLBLOCK);

	// Verify tiles were set
	ASSERT(terrainLayer->GetGrid().GetTile(10, 10) == TILE_WALL);
	ASSERT(terrainLayer->GetGrid().GetTile(20, 15) == TILE_FULLBLOCK);
	ASSERT(terrainLayer->GetGrid().GetTile(5, 5) == TILE_EMPTY);
}

// Test 5: Layer Visibility
TEST(LayerVisibility) {
	LayerManager layerMgr;
	layerMgr.InitializeDefaultLayers(10, 10);

	for(int i = 0; i < layerMgr.GetLayerCount(); i++) {
		Layer& layer = layerMgr.GetLayer(i);
		ASSERT(layer.IsVisible() == true);  // Default visible

		layer.SetVisible(false);
		ASSERT(layer.IsVisible() == false);

		layer.SetVisible(true);
		ASSERT(layer.IsVisible() == true);
	}
}

// Main test runner
CONSOLE_APP_MAIN
{
	StdLogSetup(LOG_COUT|LOG_FILE);

	LOG("========================================");
	LOG("Umbrella GameScreen Tests");
	LOG("========================================");

	// Tests are automatically registered and run via static constructors

	LOG("========================================");
	LOG("Results: " << test_passed << "/" << test_count << " tests passed");
	LOG("========================================");

	if(test_passed == test_count) {
		LOG("✓ ALL TESTS PASSED");
		SetExitCode(0);
	} else {
		LOG("✗ SOME TESTS FAILED");
		SetExitCode(1);
	}
}
