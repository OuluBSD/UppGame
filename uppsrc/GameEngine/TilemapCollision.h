#ifndef UPP_TILEMAP_COLLISION_H
#define UPP_TILEMAP_COLLISION_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <GameLib/GameLib.h>
#include <GameEngine/GameEngine.h>
#include <GameEngine/TilemapRenderer.h>

NAMESPACE_UPP

// Collision properties for tiles
struct TileCollisionProperties {
    bool isSolid = false;              // Whether the tile blocks movement
    bool isOneWay = false;             // One-way platform (e.g., ladders, platforms you can jump through)
    bool isSlope = false;              // Whether the tile is sloped
    double slopeAngle = 0.0;           // Angle of the slope in radians
    Rect collisionRect;                // Custom collision rectangle within the tile
    String collisionType = "solid";    // Type of collision (solid, platform, ladder, etc.)
    
    // Constructor
    TileCollisionProperties() : collisionRect(Rect(0, 0, 0, 0)) {}
};

// Collision detection system for tilemaps
class TilemapCollisionSystem {
public:
    TilemapCollisionSystem();
    virtual ~TilemapCollisionSystem();

    // Initialize the collision system
    bool Initialize();

    // Set the map to use for collision detection
    void SetMap(std::shared_ptr<TmxMap> map);
    std::shared_ptr<TmxMap> GetMap() const { return map; }

    // Set collision properties for specific tiles
    void SetTileCollisionProperties(int globalTileId, const TileCollisionProperties& props);
    TileCollisionProperties GetTileCollisionProperties(int globalTileId) const;

    // Define collision properties for an entire tileset
    void SetTilesetCollisionProperties(int tilesetIndex, 
                                      std::function<TileCollisionProperties(int localTileId)> propertyFunc);

    // Collision detection methods
    bool CheckCollision(const Point2& position, double width, double height) const;
    bool CheckCollision(const Rect& bounds) const;

    // Get collision tile at position
    const TileCollisionProperties* GetCollisionTileAt(int x, int y, int layerIndex = 0) const;

    // Collision response methods
    Rect GetTileCollisionRect(int tileX, int tileY, int layerIndex = 0) const;
    Vector<Rect> GetCollidingTiles(const Rect& bounds) const;

    // Entity collision methods (for moving objects)
    struct CollisionResult {
        bool collided = false;
        Point2 minimumTranslationVector;  // How much to move to resolve collision
        Vector<Point> collisionTiles;     // Tile coordinates that were collided with
        Vector<TileCollisionProperties> collisionProperties;  // Properties of collided tiles
    };

    CollisionResult CheckEntityCollision(const Rect& entityBounds, 
                                        const Point2& velocity) const;

    // Specialized collision methods
    bool CheckSlopeCollision(const Point2& position, double width, double height, 
                            Point2& normal) const;
    bool CheckOneWayPlatformCollision(const Point2& position, 
                                     double width, double height,
                                     const Point2& velocity) const;

    // Raycasting for pathfinding or line-of-sight
    struct RaycastResult {
        bool hit = false;
        Point2 hitPoint;
        Point2 normal;  // Surface normal at the hit point
        Point tileCoords;  // Tile coordinates of the hit
    };

    RaycastResult Raycast(const Point2& start, const Point2& direction, 
                         double maxDistance) const;

    // Get collision bounds for an area
    Vector<Rect> GetCollisionBounds(int startX, int startY, int endX, int endY, 
                                   int layerIndex = 0) const;

    // Optimization: spatial partitioning
    void RebuildSpatialPartitioning();
    void SetSpatialPartitioningEnabled(bool enabled) { spatialPartitioning = enabled; }
    bool IsSpatialPartitioningEnabled() const { return spatialPartitioning; }

private:
    std::shared_ptr<TmxMap> map;
    
    // Collision properties for specific tiles
    HashMap<int, TileCollisionProperties> tileCollisionProperties;
    
    // Collision properties for entire tilesets
    Vector<std::function<TileCollisionProperties(int)>> tilesetCollisionFuncs;
    
    // Spatial partitioning grid for optimization
    Vector<Vector<Vector<int>>> spatialGrid;  // [gridX][gridY][tileIndices]
    int gridSize = 16;  // Size of each spatial partition in tiles
    bool spatialPartitioning = true;
    bool spatialGridValid = false;
    
    // Helper methods
    Point2 WorldToTileCoordinates(double worldX, double worldY) const;
    Point2 TileToWorldCoordinates(int tileX, int tileY) const;
    bool IsTileCollidable(int globalTileId) const;
    const TileCollisionProperties* GetTileCollisionProps(int globalTileId) const;
    void UpdateSpatialGrid();
    Rect GetEffectiveCollisionRect(int tileX, int tileY, int layerIndex) const;
};

// Physics body for entities that interact with tilemap collisions
class TilemapPhysicsBody {
public:
    TilemapPhysicsBody();
    virtual ~TilemapPhysicsBody() = default;

    // Set the collision system to use
    void SetCollisionSystem(std::shared_ptr<TilemapCollisionSystem> collisionSystem) {
        this->collisionSystem = collisionSystem;
    }

    // Position and bounds
    void SetPosition(const Point2& pos) { position = pos; }
    Point2 GetPosition() const { return position; }

    void SetSize(double width, double height) { 
        size = Point2(width, height); 
    }
    Point2.GetSize() const { return size; }

    void SetVelocity(const Point2& vel) { velocity = vel; }
    Point2 GetVelocity() const { return velocity; }

    // Movement and collision handling
    void Move(const Point2& delta);
    void Update(double deltaTime);

    // Collision callbacks
    std::function<void(const Vector<Point>& collisionTiles)> onCollision;
    std::function<void()> onGrounded;
    std::function<void()> onCeiling;
    std::function<void()> onWall;

    // Physics properties
    void SetGravity(double g) { gravity = g; }
    double GetGravity() const { return gravity; }

    void SetGrounded(bool grounded) { isGrounded = grounded; }
    bool IsGrounded() const { return isGrounded; }

    void SetMass(double m) { mass = m; }
    double GetMass() const { return mass; }

    // Get bounds
    Rect GetBounds() const { 
        return Rect((int)position.x, (int)position.y, 
                   (int)(position.x + size.x), (int)(position.y + size.y)); 
    }

private:
    Point2 position = Point2(0, 0);
    Point2 size = Point2(1, 1);
    Point2 velocity = Point2(0, 0);
    
    double gravity = 980.0;  // pixels per second squared (approximate)
    double mass = 1.0;
    
    bool isGrounded = false;
    
    std::shared_ptr<TilemapCollisionSystem> collisionSystem;
};

END_UPP_NAMESPACE

#endif