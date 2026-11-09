#include "TilemapCollision.h"

NAMESPACE_UPP

TilemapCollisionSystem::TilemapCollisionSystem() {
}

TilemapCollisionSystem::~TilemapCollisionSystem() {
}

bool TilemapCollisionSystem::Initialize() {
    return true;
}

void TilemapCollisionSystem::SetMap(std::shared_ptr<TmxMap> map) {
    this->map = map;
    spatialGridValid = false;
}

void TilemapCollisionSystem::SetTileCollisionProperties(int globalTileId, const TileCollisionProperties& props) {
    tileCollisionProperties.GetAdd(globalTileId) = props;
}

TileCollisionProperties TilemapCollisionSystem::GetTileCollisionProperties(int globalTileId) const {
    const TileCollisionProperties* props = tileCollisionProperties.Get(globalTileId);
    return props ? *props : TileCollisionProperties();
}

void TilemapCollisionSystem::SetTilesetCollisionProperties(int tilesetIndex, 
                                                          std::function<TileCollisionProperties(int)> propertyFunc) {
    if (tilesetIndex >= tilesetCollisionFuncs.GetCount()) {
        tilesetCollisionFuncs.SetCount(tilesetIndex + 1);
    }
    tilesetCollisionFuncs[tilesetIndex] = propertyFunc;
}

bool TilemapCollisionSystem::CheckCollision(const Point2& position, double width, double height) const {
    if (!map) return false;
    
    Rect bounds((int)position.x, (int)position.y, (int)(position.x + width), (int)(position.y + height));
    return CheckCollision(bounds);
}

bool TilemapCollisionSystem::CheckCollision(const Rect& bounds) const {
    if (!map) return false;
    
    // Calculate which tiles intersect with the bounds
    int startTileX = (int)floor(bounds.left / map->GetTileWidth());
    int startTileY = (int)floor(bounds.top / map->GetTileHeight());
    int endTileX = (int)ceil(bounds.right / map->GetTileWidth());
    int endTileY = (int)ceil(bounds.bottom / map->GetTileHeight());
    
    // Check each layer for collisions
    for (int layerIdx = 0; layerIdx < map->GetLayers().GetCount(); layerIdx++) {
        const MapLayer& layer = map->GetLayers()[layerIdx];
        
        // Only check visible layers (or all layers if we're not being selective)
        for (int y = max(0, startTileY); y < min(layer.height, endTileY); y++) {
            for (int x = max(0, startTileX); x < min(layer.width, endTileX); x++) {
                const Tile* tile = map->GetTileAt(layerIdx, x, y);
                if (tile && tile->id >= 0) {
                    const TileCollisionProperties* props = GetTileCollisionProps(tile->id);
                    if (props && props->isSolid) {
                        // Get the tile's collision rectangle
                        Rect tileRect = GetEffectiveCollisionRect(x, y, layerIdx);
                        
                        // Check if the bounds intersect with the tile
                        if (bounds.Intersects(tileRect)) {
                            return true;
                        }
                    }
                }
            }
        }
    }
    
    return false;
}

const TileCollisionProperties* TilemapCollisionSystem::GetCollisionTileAt(int x, int y, int layerIndex) const {
    if (!map) return nullptr;
    
    const Tile* tile = map->GetTileAt(layerIndex, x, y);
    if (!tile || tile->id < 0) return nullptr;
    
    return GetTileCollisionProps(tile->id);
}

Rect TilemapCollisionSystem::GetTileCollisionRect(int tileX, int tileY, int layerIndex) const {
    if (!map) return Rect(0, 0, 0, 0);
    
    const Tile* tile = map->GetTileAt(layerIndex, tileX, tileY);
    if (!tile) return Rect(0, 0, 0, 0);
    
    const TileCollisionProperties* props = GetTileCollisionProps(tile->id);
    if (!props) return Rect(0, 0, 0, 0);
    
    // Return the effective collision rectangle
    if (!props->collisionRect.IsEmpty()) {
        // Use custom collision rectangle
        return Rect(
            tileX * map->GetTileWidth() + props->collisionRect.left,
            tileY * map->GetTileHeight() + props->collisionRect.top,
            tileX * map->GetTileWidth() + props->collisionRect.right,
            tileY * map->GetTileHeight() + props->collisionRect.bottom
        );
    } else {
        // Use full tile rectangle
        return Rect(
            tileX * map->GetTileWidth(),
            tileY * map->GetTileHeight(),
            (tileX + 1) * map->GetTileWidth(),
            (tileY + 1) * map->GetTileHeight()
        );
    }
}

Vector<Rect> TilemapCollisionSystem::GetCollidingTiles(const Rect& bounds) const {
    Vector<Rect> collidingTiles;
    
    if (!map) return collidingTiles;
    
    // Calculate which tiles intersect with the bounds
    int startTileX = (int)floor(bounds.left / map->GetTileWidth());
    int startTileY = (int)floor(bounds.top / map->GetTileHeight());
    int endTileX = (int)ceil(bounds.right / map->GetTileWidth());
    int endTileY = (int)ceil(bounds.bottom / map->GetTileHeight());
    
    // Check each layer for collisions
    for (int layerIdx = 0; layerIdx < map->GetLayers().GetCount(); layerIdx++) {
        const MapLayer& layer = map->GetLayers()[layerIdx];
        
        for (int y = max(0, startTileY); y < min(layer.height, endTileY); y++) {
            for (int x = max(0, startTileX); x < min(layer.width, endTileX); x++) {
                const Tile* tile = map->GetTileAt(layerIdx, x, y);
                if (tile && tile->id >= 0) {
                    const TileCollisionProperties* props = GetTileCollisionProps(tile->id);
                    if (props && props->isSolid) {
                        Rect tileRect = GetEffectiveCollisionRect(x, y, layerIdx);
                        
                        if (bounds.Intersects(tileRect)) {
                            collidingTiles.Add(tileRect);
                        }
                    }
                }
            }
        }
    }
    
    return collidingTiles;
}

TilemapCollisionSystem::CollisionResult 
TilemapCollisionSystem::CheckEntityCollision(const Rect& entityBounds, const Point2& velocity) const {
    CollisionResult result;
    
    if (!map) return result;
    
    // Calculate which tiles might collide with the entity
    int startTileX = (int)floor(entityBounds.left / map->GetTileWidth());
    int startTileY = (int)floor(entityBounds.top / map->GetTileHeight());
    int endTileX = (int)ceil(entityBounds.right / map->GetTileWidth());
    int endTileY = (int)ceil(entityBounds.bottom / map->GetTileHeight());
    
    // Expand search area based on velocity
    if (velocity.x > 0) endTileX++;
    if (velocity.x < 0) startTileX--;
    if (velocity.y > 0) endTileY++;
    if (velocity.y < 0) startTileY--;
    
    // Check for collisions
    for (int layerIdx = 0; layerIdx < map->GetLayers().GetCount(); layerIdx++) {
        const MapLayer& layer = map->GetLayers()[layerIdx];
        
        for (int y = max(0, startTileY); y < min(layer.height, endTileY); y++) {
            for (int x = max(0, startTileX); x < min(layer.width, endTileX); x++) {
                const Tile* tile = map->GetTileAt(layerIdx, x, y);
                if (tile && tile->id >= 0) {
                    const TileCollisionProperties* props = GetTileCollisionProps(tile->id);
                    if (props && props->isSolid) {
                        Rect tileRect = GetEffectiveCollisionRect(x, y, layerIdx);
                        
                        if (entityBounds.Intersects(tileRect)) {
                            result.collided = true;
                            result.collisionTiles.Add(Point(x, y));
                            result.collisionProperties.Add(*props);
                            
                            // Calculate minimum translation vector
                            // This is a simplified calculation - a full implementation would be more complex
                            double overlapX = 0, overlapY = 0;
                            
                            if (velocity.x > 0) {
                                // Moving right, colliding with left side of tile
                                overlapX = entityBounds.right - tileRect.left;
                            } else if (velocity.x < 0) {
                                // Moving left, colliding with right side of tile
                                overlapX = tileRect.right - entityBounds.left;
                            }
                            
                            if (velocity.y > 0) {
                                // Moving down, colliding with top of tile
                                overlapY = entityBounds.bottom - tileRect.top;
                            } else if (velocity.y < 0) {
                                // Moving up, colliding with bottom of tile
                                overlapY = tileRect.bottom - entityBounds.top;
                            }
                            
                            // Choose the smaller overlap
                            if (abs(overlapX) < abs(overlapY)) {
                                result.minimumTranslationVector.x = -overlapX;
                            } else {
                                result.minimumTranslationVector.y = -overlapY;
                            }
                        }
                    }
                }
            }
        }
    }
    
    return result;
}

bool TilemapCollisionSystem::CheckSlopeCollision(const Point2& position, double width, double height, 
                                               Point2& normal) const {
    if (!map) return false;
    
    // Calculate which tiles intersect with the bounds
    Rect bounds((int)position.x, (int)position.y, (int)(position.x + width), (int)(position.y + height));
    int startTileX = (int)floor(bounds.left / map->GetTileWidth());
    int startTileY = (int)floor(bounds.top / map->GetTileHeight());
    int endTileX = (int)ceil(bounds.right / map->GetTileWidth());
    int endTileY = (int)ceil(bounds.bottom / map->GetTileHeight());
    
    for (int layerIdx = 0; layerIdx < map->GetLayers().GetCount(); layerIdx++) {
        const MapLayer& layer = map->GetLayers()[layerIdx];
        
        for (int y = max(0, startTileY); y < min(layer.height, endTileY); y++) {
            for (int x = max(0, startTileX); x < min(layer.width, endTileX); x++) {
                const Tile* tile = map->GetTileAt(layerIdx, x, y);
                if (tile && tile->id >= 0) {
                    const TileCollisionProperties* props = GetTileCollisionProps(tile->id);
                    if (props && props->isSlope) {
                        // For this implementation, we'll just return the first slope collision
                        // and its normal
                        normal = Point2(cos(props->slopeAngle), sin(props->slopeAngle));
                        return true;
                    }
                }
            }
        }
    }
    
    return false;
}

bool TilemapCollisionSystem::CheckOneWayPlatformCollision(const Point2& position, 
                                                        double width, double height,
                                                        const Point2& velocity) const {
    if (!map) return false;
    
    // Calculate which tiles intersect with the bounds
    Rect bounds((int)position.x, (int)position.y, (int)(position.x + width), (int)(position.y + height));
    int startTileX = (int)floor(bounds.left / map->GetTileWidth());
    int startTileY = (int)floor(bounds.top / map->GetTileHeight());
    int endTileX = (int)ceil(bounds.right / map->GetTileWidth());
    int endTileY = (int)ceil(bounds.bottom / map->GetTileHeight());
    
    for (int layerIdx = 0; layerIdx < map->GetLayers().GetCount(); layerIdx++) {
        const MapLayer& layer = map->GetLayers()[layerIdx];
        
        for (int y = max(0, startTileY); y < min(layer.height, endTileY); y++) {
            for (int x = max(0, startTileX); x < min(layer.width, endTileX); x++) {
                const Tile* tile = map->GetTileAt(layerIdx, x, y);
                if (tile && tile->id >= 0) {
                    const TileCollisionProperties* props = GetTileCollisionProps(tile->id);
                    if (props && props->isOneWay) {
                        // One-way platforms only collide when moving in the same direction
                        // as the platform allows
                        if (velocity.y >= 0) {  // Moving down or not moving vertically
                            // Check if we're above the platform
                            if (position.y + height <= y * map->GetTileHeight() + map->GetTileHeight() / 2) {
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }
    
    return false;
}

TilemapCollisionSystem::RaycastResult 
TilemapCollisionSystem::Raycast(const Point2& start, const Point2& direction, 
                              double maxDistance) const {
    RaycastResult result;
    if (!map) return result;
    
    // Simple raycasting implementation using Bresenham-like algorithm
    Point2 rayPos = start;
    Point2 rayDir = direction.Normalize();
    
    // Use a fixed step size
    double stepSize = 1.0;
    double distance = 0.0;
    
    while (distance < maxDistance) {
        // Calculate which tile we're in
        int tileX = (int)floor(rayPos.x / map->GetTileWidth());
        int tileY = (int)floor(rayPos.y / map->GetTileHeight());
        
        // Check if we're within map bounds
        if (tileX >= 0 && tileX < map->GetWidth() && 
            tileY >= 0 && tileY < map->GetHeight()) {
            
            // Check each layer for collision
            for (int layerIdx = 0; layerIdx < map->GetLayers().GetCount(); layerIdx++) {
                const Tile* tile = map->GetTileAt(layerIdx, tileX, tileY);
                if (tile && tile->id >= 0) {
                    const TileCollisionProperties* props = GetTileCollisionProps(tile->id);
                    if (props && props->isSolid) {
                        result.hit = true;
                        result.hitPoint = rayPos;
                        result.tileCoords = Point(tileX, tileY);
                        
                        // Calculate a basic normal based on the direction of the ray
                        if (abs(rayDir.x) > abs(rayDir.y)) {
                            result.normal = Point2(-Sign(rayDir.x), 0);  // Vertical collision
                        } else {
                            result.normal = Point2(0, -Sign(rayDir.y));  // Horizontal collision
                        }
                        
                        return result;
                    }
                }
            }
        }
        
        // Move along the ray
        rayPos.x += rayDir.x * stepSize;
        rayPos.y += rayDir.y * stepSize;
        distance += stepSize;
    }
    
    return result;
}

Vector<Rect> TilemapCollisionSystem::GetCollisionBounds(int startX, int startY, int endX, int endY, 
                                                      int layerIndex) const {
    Vector<Rect> bounds;
    
    if (!map) return bounds;
    
    const MapLayer& layer = map->GetLayers()[layerIndex];
    
    for (int y = max(0, startY); y < min(layer.height, endY); y++) {
        for (int x = max(0, startX); x < min(layer.width, endX); x++) {
            const Tile* tile = map->GetTileAt(layerIndex, x, y);
            if (tile && tile->id >= 0) {
                const TileCollisionProperties* props = GetTileCollisionProps(tile->id);
                if (props && props->isSolid) {
                    bounds.Add(GetEffectiveCollisionRect(x, y, layerIndex));
                }
            }
        }
    }
    
    return bounds;
}

void TilemapCollisionSystem::RebuildSpatialPartitioning() {
    if (!map || !spatialPartitioning) return;
    
    UpdateSpatialGrid();
    spatialGridValid = true;
}

Point2 TilemapCollisionSystem::WorldToTileCoordinates(double worldX, double worldY) const {
    if (!map) return Point2(0, 0);
    
    return Point2(
        floor(worldX / map->GetTileWidth()),
        floor(worldY / map->GetTileHeight())
    );
}

Point2 TilemapCollisionSystem::TileToWorldCoordinates(int tileX, int tileY) const {
    if (!map) return Point2(0, 0);
    
    return Point2(
        tileX * map->GetTileWidth(),
        tileY * map->GetTileHeight()
    );
}

bool TilemapCollisionSystem::IsTileCollidable(int globalTileId) const {
    const TileCollisionProperties* props = GetTileCollisionProps(globalTileId);
    return props && props->isSolid;
}

const TileCollisionProperties* TilemapCollisionSystem::GetTileCollisionProps(int globalTileId) const {
    // First check specific tile properties
    const TileCollisionProperties* props = tileCollisionProperties.Get(globalTileId);
    if (props) return props;
    
    // If not found, check if it matches any tileset pattern
    if (!map) return nullptr;
    
    // Find which tileset contains this tile
    const Vector<Tileset>& tilesets = map->GetTilesets();
    for (int i = tilesets.GetCount() - 1; i >= 0; i--) {  // Check in reverse order
        if (globalTileId >= tilesets[i].firstGid) {
            // Found the tileset, check if it has a collision function
            if (i < tilesetCollisionFuncs.GetCount() && tilesetCollisionFuncs[i]) {
                // This is where we would create and cache the property, but for now just return nullptr
                // A full implementation would call the function and cache the result
            }
            break;
        }
    }
    
    // Return default properties (non-collidable)
    static TileCollisionProperties defaultProps;
    defaultProps.isSolid = false;
    return &defaultProps;
}

void TilemapCollisionSystem::UpdateSpatialGrid() {
    if (!map || !spatialPartitioning) return;
    
    int gridWidth = (map->GetWidth() + gridSize - 1) / gridSize;
    int gridHeight = (map->GetHeight() + gridSize - 1) / gridSize;
    
    spatialGrid.SetCount(gridWidth);
    for (int x = 0; x < gridWidth; x++) {
        spatialGrid[x].SetCount(gridHeight);
        for (int y = 0; y < gridHeight; y++) {
            spatialGrid[x][y].Clear();
        }
    }
    
    // Add collidable tiles to the spatial grid
    for (int layerIdx = 0; layerIdx < map->GetLayers().GetCount(); layerIdx++) {
        const MapLayer& layer = map->GetLayers()[layerIdx];
        for (int y = 0; y < layer.height; y++) {
            for (int x = 0; x < layer.width; x++) {
                const Tile* tile = layer.tiles[y * layer.width + x];
                if (tile && tile->id >= 0 && IsTileCollidable(tile->id)) {
                    int gridX = x / gridSize;
                    int gridY = y / gridSize;
                    
                    if (gridX < gridWidth && gridY < gridHeight) {
                        spatialGrid[gridX][gridY].Add(y * layer.width + x);
                    }
                }
            }
        }
    }
}

Rect TilemapCollisionSystem::GetEffectiveCollisionRect(int tileX, int tileY, int layerIndex) const {
    if (!map) return Rect(0, 0, 0, 0);
    
    const Tile* tile = map->GetTileAt(layerIndex, tileX, tileY);
    if (!tile) return Rect(0, 0, 0, 0);
    
    const TileCollisionProperties* props = GetTileCollisionProps(tile->id);
    if (!props) return Rect(0, 0, 0, 0);
    
    // Return the effective collision rectangle
    if (!props->collisionRect.IsEmpty()) {
        // Use custom collision rectangle
        return Rect(
            tileX * map->GetTileWidth() + props->collisionRect.left,
            tileY * map->GetTileHeight() + props->collisionRect.top,
            tileX * map->GetTileWidth() + props->collisionRect.right,
            tileY * map->GetTileHeight() + props->collisionRect.bottom
        );
    } else {
        // Use full tile rectangle
        return Rect(
            tileX * map->GetTileWidth(),
            tileY * map->GetTileHeight(),
            (tileX + 1) * map->GetTileWidth(),
            (tileY + 1) * map->GetTileHeight()
        );
    }
}

// TilemapPhysicsBody implementation
TilemapPhysicsBody::TilemapPhysicsBody() : gravity(980.0), mass(1.0) {
}

void TilemapPhysicsBody::Move(const Point2& delta) {
    position.x += delta.x;
    position.y += delta.y;
    
    // If we have a collision system, check for collisions
    if (collisionSystem) {
        Rect bounds = GetBounds();
        auto collisionResult = collisionSystem->CheckEntityCollision(bounds, delta);
        
        if (collisionResult.collided) {
            // Apply collision response
            position.x += collisionResult.minimumTranslationVector.x;
            position.y += collisionResult.minimumTranslationVector.y;
            
            // Call collision callbacks
            if (onCollision) {
                onCollision(collisionResult.collisionTiles);
            }
            
            // Update grounded state based on collision direction
            if (collisionResult.minimumTranslationVector.y < 0 && delta.y > 0) {
                isGrounded = true;
                if (onGrounded) onGrounded();
            } else if (collisionResult.minimumTranslationVector.y > 0 && delta.y < 0) {
                if (onCeiling) onCeiling();
            } else if (collisionResult.minimumTranslationVector.x != 0) {
                if (onWall) onWall();
            }
        } else {
            isGrounded = false;
        }
    }
}

void TilemapPhysicsBody::Update(double deltaTime) {
    if (collisionSystem) {
        // Apply gravity if not grounded
        if (!isGrounded) {
            velocity.y += gravity * deltaTime;
        }
        
        // Calculate movement
        Point2 movement = velocity * deltaTime;
        
        // Move the body
        Move(movement);
    }
}

END_UPP_NAMESPACE