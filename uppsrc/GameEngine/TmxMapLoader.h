#ifndef UPP_TMX_MAP_LOADER_H
#define UPP_TMX_MAP_LOADER_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <GameLib/GameLib.h>
#include <GameEngine/GameEngine.h>
#include <GameEngine/VFS.h>

NAMESPACE_UPP

// Tilemap-related structures
struct Tile {
    int id = -1;           // Tile ID
    int gid = 0;           // Global ID (includes flipping flags)
    bool flippedHorizontally = false;
    bool flippedVertically = false;
    bool flippedDiagonally = false;
};

struct Tileset {
    String name;
    String imageSource;
    int firstGid = 0;      // First global ID in this tileset
    int tileWidth = 0;
    int tileHeight = 0;
    int spacing = 0;       // Spacing between tiles in the image
    int margin = 0;        // Margin around the tileset image
    Image image;
};

struct MapLayer {
    String name;
    int width = 0;
    int height = 0;
    Vector<Tile> tiles;    // Flattened 2D array of tiles
    double opacity = 1.0;
    bool visible = true;
    Point2 offset = Point2(0, 0);  // Layer offset
};

struct ObjectGroup {
    String name;
    Vector<Value> objects; // Objects in the group (various types)
    Color color;          // Color to tint objects
    bool visible = true;
    double opacity = 1.0;
    Point2 offset = Point2(0, 0);
};

// TMX Map class representing a loaded tilemap
class TmxMap {
public:
    TmxMap();
    virtual ~TmxMap() = default;

    // Map properties
    String GetOrientation() const { return orientation; }
    String GetRenderOrder() const { return renderOrder; }
    int GetWidth() const { return width; }
    int GetHeight() const { return height; }
    int GetTileWidth() const { return tileWidth; }
    int GetTileHeight() const { return tileHeight; }
    
    // Get loaded data
    const Vector<Tileset>& GetTilesets() const { return tilesets; }
    const Vector<MapLayer>& GetLayers() const { return layers; }
    const Vector<ObjectGroup>& GetObjectGroups() const { return objectGroups; }
    
    // Add data
    void AddTileset(const Tileset& tileset) { tilesets.Add(tileset); }
    void AddLayer(const MapLayer& layer) { layers.Add(layer); }
    void AddObjectGroup(const ObjectGroup& group) { objectGroups.Add(group); }

    // Get a tile at specific coordinates from a layer
    const Tile* GetTileAt(int layerIndex, int x, int y) const;
    
    // Get global tile ID for a specific tile
    int GetGlobalTileId(int localTileId, int tilesetIndex) const;

private:
    String orientation = "orthogonal"; // orthogonal, isometric, staggered
    String renderOrder = "right-down"; // right-down, left-down, right-up, left-up
    int width = 0;
    int height = 0;
    int tileWidth = 0;
    int tileHeight = 0;
    
    Vector<Tileset> tilesets;
    Vector<MapLayer> layers;
    Vector<ObjectGroup> objectGroups;
};

// TMX Map loader class
class TmxMapLoader {
public:
    TmxMapLoader();
    virtual ~TmxMapLoader() = default;

    // Load a TMX map from file
    std::shared_ptr<TmxMap> LoadMap(const String& filepath);
    
    // Load a TMX map using the VFS
    std::shared_ptr<TmxMap> LoadMapFromVFS(const String& vfsPath, std::shared_ptr<VFS> vfs);

    // Set/get VFS for file access
    void SetVFS(std::shared_ptr<VFS> vfs) { this->vfs = vfs; }
    std::shared_ptr<VFS> GetVFS() const { return vfs; }

private:
    std::shared_ptr<VFS> vfs;

    // Parse XML content of TMX file
    bool ParseXML(const String& xmlContent, std::shared_ptr<TmxMap> map);

    // Parse tileset from XML element
    Tileset ParseTileset(XmlParser& parser);

    // Parse layer from XML element
    MapLayer ParseLayer(XmlParser& parser, int mapWidth, int mapHeight);

    // Parse object group from XML element
    ObjectGroup ParseObjectGroup(XmlParser& parser);

    // Decode tile data (handles CSV, base64, and gzip encoding)
    Vector<Tile> DecodeTileData(const String& data, const String& encoding, 
                               const String& compression, int width, int height);
};

END_UPP_NAMESPACE

#endif