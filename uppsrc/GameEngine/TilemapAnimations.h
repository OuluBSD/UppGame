#ifndef UPP_TILEMAP_ANIMATIONS_H
#define UPP_TILEMAP_ANIMATIONS_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <GameLib/GameLib.h>
#include <GameEngine/GameEngine.h>
#include <GameEngine/TilemapRenderer.h>
#include <GameEngine/TmxMapLoader.h>

NAMESPACE_UPP

// Animation frame definition
struct AnimationFrame {
    int tileId;      // Tile ID for this frame
    double duration; // Duration in seconds
};

// Animated tile definition
struct AnimatedTile {
    Vector<AnimationFrame> frames;
    int currentFrame = 0;
    double timeElapsed = 0.0;
    
    // Get the tile ID for the current frame
    int GetCurrentTileId() const {
        if (frames.IsEmpty()) return -1;
        return frames[currentFrame].tileId;
    }
    
    // Advance the animation by the given time
    void Update(double deltaTime) {
        timeElapsed += deltaTime;
        
        // Check if we should advance to the next frame
        if (!frames.IsEmpty()) {
            if (timeElapsed >= frames[currentFrame].duration) {
                timeElapsed = fmod(timeElapsed, frames[currentFrame].duration);
                currentFrame = (currentFrame + 1) % frames.GetCount();
            }
        }
    }
};

// Object in a tilemap object layer
struct TilemapObject {
    String name;
    String type;
    int id = -1;
    Rect bounds;           // Position and size of the object
    Point2 position;       // Position of the object
    double width = 0;
    double height = 0;
    double rotation = 0;   // Rotation in degrees
    bool visible = true;
    Color color = White(); // Color of the object (for shapes)
    Point2 startPoint;     // For polyline/polygon objects
    Vector<Point2> points; // For polyline/polygon objects
    String properties;     // Additional properties
    
    // Type of object
    enum class Type {
        RECTANGLE,
        ELLIPSE,
        POLYGON,
        POLYLINE,
        TILE,
        TEXT
    } objectType = Type::RECTANGLE;
};

// Object layer in a tilemap
struct ObjectLayer {
    String name;
    Vector<TilemapObject> objects;
    double opacity = 1.0;
    bool visible = true;
    Color color;           // Tint color for all objects in this layer
    Point2 offset = Point2(0, 0);  // Layer offset
};

// Enhanced tilemap with animation and object layer support
class AnimatedTilemap : public TmxMap {
public:
    AnimatedTilemap();
    virtual ~AnimatedTilemap() = default;

    // Animated tile management
    void AddAnimatedTile(int globalTileId, const AnimatedTile& animation);
    void RemoveAnimatedTile(int globalTileId);
    bool HasAnimatedTile(int globalTileId) const;
    AnimatedTile GetAnimatedTile(int globalTileId) const;
    void UpdateAnimations(double deltaTime);

    // Object layer management
    void AddObjectLayer(const ObjectLayer& layer);
    int GetObjectLayerCount() const { return objectLayers.GetCount(); }
    const ObjectLayer& GetObjectLayer(int index) const { return objectLayers[index]; }
    ObjectLayer& GetObjectLayer(int index) { return objectLayers[index]; }

    // Get all objects of a specific type
    Vector<const TilemapObject*> GetObjectsByType(const String& type) const;
    Vector<const TilemapObject*> GetObjectsByName(const String& name) const;

    // Tile ID retrieval for current animation frames
    int GetAnimatedTileId(int globalTileId) const;

private:
    HashMap<int, AnimatedTile> animatedTiles;
    Vector<ObjectLayer> objectLayers;
    CriticalSection animationCs;  // For thread safety during animation updates
};

// Enhanced renderer for animated tiles and object layers
class AnimatedTilemapRenderer : public TilemapRenderer {
public:
    AnimatedTilemapRenderer();
    virtual ~AnimatedTilemapRenderer();

    // Set the animated map to render
    void SetAnimatedMap(std::shared_ptr<AnimatedTilemap> map);
    std::shared_ptr<AnimatedTilemap> GetAnimatedMap() const;
    
    // Update animations (call each frame)
    void UpdateAnimations(double deltaTime);
    
    // Render animated tilemap
    void Render(Draw& draw, const Rect& viewport, 
               int cameraX = 0, int cameraY = 0, 
               double scale = 1.0);
    
    // Render object layers
    void RenderObjectLayers(Draw& draw, const Rect& viewport,
                           int cameraX = 0, int cameraY = 0,
                           double scale = 1.0);

    // Get/set animation update settings
    void SetUpdateAnimations(bool update) { updateAnimationsEnabled = update; }
    bool IsUpdateAnimationsEnabled() const { return updateAnimationsEnabled; }

    void SetMaxAnimationUpdatesPerFrame(int max) { maxAnimationUpdatesPerFrame = max; }
    int GetMaxAnimationUpdatesPerFrame() const { return maxAnimationUpdatesPerFrame; }

private:
    std::shared_ptr<AnimatedTilemap> animatedMap;
    bool updateAnimationsEnabled = true;
    int maxAnimationUpdatesPerFrame = 100;  // Limit on animation updates per frame
    double lastAnimationTime = 0.0;

    // Render a single animated tile
    void RenderAnimatedTile(Draw& draw, int globalTileId, int screenX, int screenY, 
                           double scale);
    
    // Render a single object
    void RenderObject(Draw& draw, const TilemapObject& object, 
                     int cameraX, int cameraY, double scale);
    
    // Render different object types
    void RenderRectangle(Draw& draw, const TilemapObject& object, 
                        int cameraX, int cameraY, double scale);
    void RenderEllipse(Draw& draw, const TilemapObject& object, 
                      int cameraX, int cameraY, double scale);
    void RenderPolygon(Draw& draw, const TilemapObject& object, 
                      int cameraX, int cameraY, double scale);
    void RenderPolyline(Draw& draw, const TilemapObject& object, 
                       int cameraX, int cameraY, double scale);
    void RenderText(Draw& draw, const TilemapObject& object, 
                   int cameraX, int cameraY, double scale);
};

// Animation system for managing tilemap animations
class AnimationSystem {
public:
    AnimationSystem();
    virtual ~AnimationSystem();

    // Register an animated tilemap with the system
    void RegisterTilemap(std::shared_ptr<AnimatedTilemap> tilemap);
    void UnregisterTilemap(std::shared_ptr<AnimatedTilemap> tilemap);
    void ClearTilemaps();

    // Update all registered animations
    void Update(double deltaTime);

    // Get/set global animation properties
    void SetGlobalAnimationSpeed(double speed) { animationSpeed = speed; }
    double GetGlobalAnimationSpeed() const { return animationSpeed; }

    // Pausing animations
    void SetPaused(bool paused) { isPaused = paused; }
    bool IsPaused() const { return isPaused; }

    // Get animation statistics
    int GetRegisteredTilemapsCount() const { return tilemaps.GetCount(); }
    int GetTotalAnimatedTilesCount() const;

private:
    Vector<std::shared_ptr<AnimatedTilemap>> tilemaps;
    double animationSpeed = 1.0;
    bool isPaused = false;
};

END_UPP_NAMESPACE

#endif