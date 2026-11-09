#ifndef UPP_TILEMAP_RENDERER_H
#define UPP_TILEMAP_RENDERER_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <GameLib/GameLib.h>
#include <GameEngine/GameEngine.h>
#include <GameEngine/TmxMapLoader.h>

NAMESPACE_UPP

// Tilemap renderer for efficiently rendering tile-based maps
class TilemapRenderer {
public:
    TilemapRenderer();
    virtual ~TilemapRenderer();

    // Initialize the tilemap renderer
    bool Initialize();

    // Set the map to render
    void SetMap(std::shared_ptr<TmxMap> map);
    std::shared_ptr<TmxMap> GetMap() const { return map; }

    // Render the tilemap
    void Render(Draw& draw, const Rect& viewport, 
               int cameraX = 0, int cameraY = 0, 
               double scale = 1.0);

    // Render a specific layer
    void RenderLayer(Draw& draw, int layerIndex, const Rect& viewport,
                    int cameraX = 0, int cameraY = 0, 
                    double scale = 1.0);

    // Get/set rendering options
    void SetEnableCulling(bool enabled) { cullingEnabled = enabled; }
    bool IsCullingEnabled() const { return cullingEnabled; }

    void SetMaxRenderDistance(int distance) { maxRenderDistance = distance; }
    int GetMaxRenderDistance() const { return maxRenderDistance; }

    // Get tile at screen position
    Point GetTileAtScreenPos(int screenX, int screenY, int cameraX = 0, int cameraY = 0, 
                            double scale = 1.0) const;

    // Get tile at map coordinates
    const Tile* GetTileAt(int layerIndex, int x, int y) const;

    // Get tile world position from tile coordinates
    Point2 GetWorldPosition(int tileX, int tileY) const;

    // Get tile coordinates from world position
    Point GetTileCoordinates(double worldX, double worldY) const;

    // Invalidate tile cache (call when map changes)
    void InvalidateCache();

    // Render with effects (for animated tiles, etc.)
    void RenderWithEffects(Draw& draw, const Rect& viewport,
                          int cameraX = 0, int cameraY = 0,
                          double scale = 1.0,
                          bool renderAnimatedTiles = true);

private:
    std::shared_ptr<TmxMap> map;
    
    // Rendering options
    bool cullingEnabled = true;
    int maxRenderDistance = 10;  // How many tiles beyond viewport to render
    
    // Tile cache for optimized rendering
    struct TileRenderCache {
        Image cachedImage;
        Rect sourceRect;
        Point2 position;
        bool valid = false;
    };
    
    Vector<TileRenderCache> tileCache;
    bool cacheValid = false;
    
    // Render a single tile
    void RenderTile(Draw& draw, const Tile& tile, int tilesetIndex, 
                   int screenX, int screenY, double scale);
    
    // Find tileset for a given global tile ID
    int FindTilesetForTile(int globalTileId) const;
    
    // Calculate which tiles are visible in the viewport
    Rect CalculateVisibleArea(const Rect& viewport, int cameraX, int cameraY, 
                             double scale) const;
};

// Animated tile support
struct AnimatedTile {
    Vector<int> frames;        // Tile IDs in animation sequence
    double frameDuration;      // Duration of each frame in seconds
    int currentFrame;          // Current frame index
    double timeElapsed;        // Time since last frame change
};

// Enhanced tilemap renderer with animation support
class AnimatedTilemapRenderer : public TilemapRenderer {
public:
    AnimatedTilemapRenderer();
    virtual ~AnimatedTilemapRenderer();

    // Add an animated tile sequence
    void AddAnimatedTile(int globalTileId, const AnimatedTile& animation);

    // Update animations (call each frame)
    void UpdateAnimations(double deltaTime);

    // Clear all animated tiles
    void ClearAnimations();

    // Render with animation support
    void Render(Draw& draw, const Rect& viewport,
               int cameraX = 0, int cameraY = 0,
               double scale = 1.0) override;

private:
    HashMap<int, AnimatedTile> animatedTiles;
    double animationTime = 0.0;
};

END_UPP_NAMESPACE

#endif