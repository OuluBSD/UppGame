#include "Umbrella.h"

#include <CtrlLib/CtrlLib.h>
#include <Draw/Draw.h>
#include <Core/Core.h>
#include <plugin/png/png.h>  // For image handling if needed

using namespace Upp;

// Forward declaration
class LevelLoader;

// Basic LevelData structure
struct LevelData {
    String name;
    int width;
    int height;
    Vector<Vector<int>> tiles;  // 2D array of tile IDs
    Vector<Point> spawnPoints;  // Player/enemy spawn points
    Vector<String> entities;    // Entities in the level
    
    LevelData() : width(0), height(0) {}
};

// Level Loader Class - Basic Implementation
class LevelLoader {
private:
    // Use a simpler approach with just basic types that U++ handles well
    Vector<int> tileIds;
    Vector<String> tileNames;
    String currentLevelPath;
    
public:
    LevelLoader();
    ~LevelLoader();
    
    // Load a level from file
    bool LoadLevel(const String& filePath, LevelData& levelData);
    
    // Save a level to file
    bool SaveLevel(const String& filePath, const LevelData& levelData);
    
    // Load tileset
    bool LoadTileSet(const String& tileSetPath);
    
    // Get tile info by ID
    String GetTileName(int tileId) const;
    bool IsValidTileId(int tileId) const;
    
    // Utility functions
    bool IsValidPosition(const LevelData& level, int x, int y) const;
    int GetTileAt(const LevelData& level, int x, int y) const;
    bool SetTileAt(LevelData& level, int x, int y, int tileId);
    
    // Getters
    const Vector<int>& GetTileIds() const { return tileIds; }
    const Vector<String>& GetTileNames() const { return tileNames; }
    const String& GetCurrentLevelPath() const { return currentLevelPath; }
};

// Implementation of LevelLoader
LevelLoader::LevelLoader() {
    // Initialize with some basic tiles
    tileIds.SetCount(10);
    tileNames.SetCount(10);
    
    tileIds[0] = 0; tileNames[0] = "Empty";
    tileIds[1] = 1; tileNames[1] = "Grass";
    tileIds[2] = 2; tileNames[2] = "Stone";
    tileIds[3] = 3; tileNames[3] = "Water";
    tileIds[4] = 4; tileNames[4] = "Dirt";
    tileIds[5] = 5; tileNames[5] = "Sand";
    tileIds[6] = 6; tileNames[6] = "Wood";
    tileIds[7] = 7; tileNames[7] = "Brick";
    tileIds[8] = 8; tileNames[8] = "Ice";
    tileIds[9] = 9; tileNames[9] = "Lava";
}

LevelLoader::~LevelLoader() {
    // Cleanup if needed
}

bool LevelLoader::LoadLevel(const String& filePath, LevelData& levelData) {
    currentLevelPath = filePath;
    
    // This is a simplified implementation - in a real scenario, 
    // this would parse a JSON or custom format file
    
    // For demonstration, create a simple level
    levelData.name = "Sample Level";
    levelData.width = 20;
    levelData.height = 15;
    
    // Resize the tiles matrix
    levelData.tiles.SetCount(levelData.height);
    for(int i = 0; i < levelData.height; i++) {
        levelData.tiles[i].SetCount(levelData.width, 0);  // Fill with empty tiles
    }
    
    // Add some sample tiles
    for(int x = 5; x < 10; x++) {
        for(int y = 5; y < 8; y++) {
            levelData.tiles[y][x] = 1;  // Grass tile
        }
    }
    
    // Add a stone area
    for(int x = 12; x < 18; x++) {
        levelData.tiles[7][x] = 2;  // Stone tile
    }
    
    // Add water
    for(int x = 2; x < 5; x++) {
        levelData.tiles[10][x] = 3;  // Water tile
    }
    
    // Add some spawn points
    levelData.spawnPoints.Add(Point(1, 1));
    levelData.spawnPoints.Add(Point(15, 5));
    
    // Add some entities
    levelData.entities.Add("PlayerStart");
    levelData.entities.Add("Enemy:Goomba");
    
    return true;
}

bool LevelLoader::SaveLevel(const String& filePath, const LevelData& levelData) {
    // This would implement saving to a file format
    // For now, just simulate success
    LOG("Saving level to: " + filePath);
    LOG("Level size: " + IntStr(levelData.width) + "x" + IntStr(levelData.height));
    
    // In a real implementation, this would serialize the level data to a file
    // Could be JSON, binary format, or custom text format
    
    return true;
}

bool LevelLoader::LoadTileSet(const String& tileSetPath) {
    // This would load a tileset from a file
    // For now, just add some more tiles to our basic set
    int currentCount = tileIds.GetCount();
    tileIds.SetCount(currentCount + 2);
    tileNames.SetCount(currentCount + 2);
    
    tileIds[currentCount] = currentCount; tileNames[currentCount] = "CustomTile1";
    tileIds[currentCount + 1] = currentCount + 1; tileNames[currentCount + 1] = "CustomTile2";
    
    LOG("Loaded tileset from: " + tileSetPath);
    return true;
}

String LevelLoader::GetTileName(int tileId) const {
    if(tileId >= 0 && tileId < tileNames.GetCount()) {
        return tileNames[tileId];
    }
    return "Unknown";
}

bool LevelLoader::IsValidTileId(int tileId) const {
    return tileId >= 0 && tileId < tileIds.GetCount();
}

bool LevelLoader::IsValidPosition(const LevelData& level, int x, int y) const {
    return x >= 0 && x < level.width && y >= 0 && y < level.height;
}

int LevelLoader::GetTileAt(const LevelData& level, int x, int y) const {
    if(IsValidPosition(level, x, y)) {
        return level.tiles[y][x];
    }
    return -1;  // Invalid position
}

bool LevelLoader::SetTileAt(LevelData& level, int x, int y, int tileId) {
    if(IsValidPosition(level, x, y)) {
        level.tiles[y][x] = tileId;
        return true;
    }
    return false;
}

// Global instance for easy access
LevelLoader& GetLevelLoader() {
    static LevelLoader loader;
    return loader;
}