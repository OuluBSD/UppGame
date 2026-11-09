#include "TilemapAnimations.h"

NAMESPACE_UPP

AnimatedTilemap::AnimatedTilemap() {
}

void AnimatedTilemap::AddAnimatedTile(int globalTileId, const AnimatedTile& animation) {
    Mutex::Lock lock(animationCs);
    animatedTiles.GetAdd(globalTileId) = animation;
}

void AnimatedTilemap::RemoveAnimatedTile(int globalTileId) {
    Mutex::Lock lock(animationCs);
    animatedTiles.RemoveKey(globalTileId);
}

bool AnimatedTilemap::HasAnimatedTile(int globalTileId) const {
    Mutex::Lock lock(animationCs);
    return animatedTiles.Find(globalTileId) >= 0;
}

AnimatedTile AnimatedTilemap::GetAnimatedTile(int globalTileId) const {
    Mutex::Lock lock(animationCs);
    const AnimatedTile* anim = animatedTiles.Get(globalTileId);
    return anim ? *anim : AnimatedTile();
}

void AnimatedTilemap::UpdateAnimations(double deltaTime) {
    Mutex::Lock lock(animationCs);
    
    for (auto& animation : animatedTiles) {
        animation.value.Update(deltaTime);
    }
}

void AnimatedTilemap::AddObjectLayer(const ObjectLayer& layer) {
    objectLayers.Add(layer);
}

Vector<const TilemapObject*> AnimatedTilemap::GetObjectsByType(const String& type) const {
    Vector<const TilemapObject*> result;
    
    for (const auto& layer : objectLayers) {
        for (const auto& obj : layer.objects) {
            if (obj.type == type) {
                result.Add(&obj);
            }
        }
    }
    
    return result;
}

Vector<const TilemapObject*> AnimatedTilemap::GetObjectsByName(const String& name) const {
    Vector<const TilemapObject*> result;
    
    for (const auto& layer : objectLayers) {
        for (const auto& obj : layer.objects) {
            if (obj.name == name) {
                result.Add(&obj);
            }
        }
    }
    
    return result;
}

int AnimatedTilemap::GetAnimatedTileId(int globalTileId) const {
    Mutex::Lock lock(animationCs);
    const AnimatedTile* anim = animatedTiles.Get(globalTileId);
    return anim ? anim->GetCurrentTileId() : globalTileId;
}

AnimatedTilemapRenderer::AnimatedTilemapRenderer() {
}

AnimatedTilemapRenderer::~AnimatedTilemapRenderer() {
}

void AnimatedTilemapRenderer::SetAnimatedMap(std::shared_ptr<AnimatedTilemap> map) {
    animatedMap = map;
    SetMap(std::static_pointer_cast<TmxMap>(map));
}

std::shared_ptr<AnimatedTilemap> AnimatedTilemapRenderer::GetAnimatedMap() const {
    return animatedMap;
}

void AnimatedTilemapRenderer::UpdateAnimations(double deltaTime) {
    if (animatedMap && updateAnimationsEnabled) {
        animatedMap->UpdateAnimations(deltaTime * 1000); // Convert to milliseconds if needed
    }
}

void AnimatedTilemapRenderer::Render(Draw& draw, const Rect& viewport, 
                                    int cameraX, int cameraY, 
                                    double scale) {
    if (!animatedMap) return;
    
    // Update animations if enabled
    if (updateAnimationsEnabled) {
        UpdateAnimations(0.016); // Assuming ~60 FPS, but in a real implementation 
                                 // this would use actual delta time from the main loop
    }
    
    // Render the base tilemap
    TilemapRenderer::Render(draw, viewport, cameraX, cameraY, scale);
    
    // Additional animated tile rendering could happen here if needed
    // For now, the animation update happens in the UpdateAnimations call above
}

void AnimatedTilemapRenderer::RenderObjectLayers(Draw& draw, const Rect& viewport,
                                                int cameraX, int cameraY,
                                                double scale) {
    if (!animatedMap) return;
    
    // Iterate through each object layer
    for (int layerIdx = 0; layerIdx < animatedMap->GetObjectLayerCount(); layerIdx++) {
        const ObjectLayer& layer = animatedMap->GetObjectLayer(layerIdx);
        
        if (!layer.visible) continue;
        
        // Render each object in the layer
        for (const auto& obj : layer.objects) {
            if (obj.visible) {
                // Check if object is within the viewport
                Rect objRect(
                    (int)(obj.position.x * scale - cameraX),
                    (int)(obj.position.y * scale - cameraY),
                    (int)((obj.position.x + obj.width) * scale - cameraX),
                    (int)((obj.position.y + obj.height) * scale - cameraY)
                );
                
                // Simple visibility check
                if (viewport.Intersects(objRect)) {
                    RenderObject(draw, obj, cameraX, cameraY, scale);
                }
            }
        }
    }
}

void AnimatedTilemapRenderer::RenderObject(Draw& draw, const TilemapObject& object, 
                                          int cameraX, int cameraY, double scale) {
    switch (object.objectType) {
        case TilemapObject::Type::RECTANGLE:
            RenderRectangle(draw, object, cameraX, cameraY, scale);
            break;
        case TilemapObject::Type::ELLIPSE:
            RenderEllipse(draw, object, cameraX, cameraY, scale);
            break;
        case TilemapObject::Type::POLYGON:
            RenderPolygon(draw, object, cameraX, cameraY, scale);
            break;
        case TilemapObject::Type::POLYLINE:
            RenderPolyline(draw, object, cameraX, cameraY, scale);
            break;
        case TilemapObject::Type::TEXT:
            RenderText(draw, object, cameraX, cameraY, scale);
            break;
        default:
            // TILE type would be rendered as part of the tilemap itself
            break;
    }
}

void AnimatedTilemapRenderer::RenderRectangle(Draw& draw, const TilemapObject& object, 
                                            int cameraX, int cameraY, double scale) {
    Rect rect(
        (int)(object.position.x * scale - cameraX),
        (int)(object.position.y * scale - cameraY),
        (int)((object.position.x + object.width) * scale - cameraX),
        (int)((object.position.y + object.height) * scale - cameraY)
    );
    
    draw.DrawRect(rect, object.color);
}

void AnimatedTilemapRenderer::RenderEllipse(Draw& draw, const TilemapObject& object, 
                                          int cameraX, int cameraY, double scale) {
    // Draw an ellipse using multiple line segments
    int centerX = (int)((object.position.x + object.width/2) * scale - cameraX);
    int centerY = (int)((object.position.y + object.height/2) * scale - cameraY);
    int radiusX = (int)(object.width/2 * scale);
    int radiusY = (int)(object.height/2 * scale);
    
    // Draw using multiple lines to approximate the ellipse
    Vector<Point> points;
    const int segments = 32;
    for (int i = 0; i <= segments; i++) {
        double angle = 2 * M_PI * i / segments;
        int x = centerX + (int)(cos(angle) * radiusX);
        int y = centerY + (int)(sin(angle) * radiusY);
        points.Add(Point(x, y));
    }
    
    draw.DrawPolyline(points, 1, object.color);
}

void AnimatedTilemapRenderer::RenderPolygon(Draw& draw, const TilemapObject& object, 
                                          int cameraX, int cameraY, double scale) {
    Vector<Point> points;
    for (const auto& pt : object.points) {
        points.Add(Point(
            (int)(pt.x * scale - cameraX),
            (int)(pt.y * scale - cameraY)
        ));
    }
    
    if (points.GetCount() >= 3) {
        draw.DrawPolygon(points, object.color);
    }
}

void AnimatedTilemapRenderer::RenderPolyline(Draw& draw, const TilemapObject& object, 
                                           int cameraX, int cameraY, double scale) {
    Vector<Point> points;
    for (const auto& pt : object.points) {
        points.Add(Point(
            (int)(pt.x * scale - cameraX),
            (int)(pt.y * scale - cameraY)
        ));
    }
    
    if (points.GetCount() >= 2) {
        draw.DrawPolyline(points, 1, object.color);
    }
}

void AnimatedTilemapRenderer::RenderText(Draw& draw, const TilemapObject& object, 
                                       int cameraX, int cameraY, double scale) {
    int x = (int)(object.position.x * scale - cameraX);
    int y = (int)(object.position.y * scale - cameraY);
    
    // For now, we'll just draw the object name as text
    // In a real implementation, this would handle actual text properties
    draw.DrawText(x, y, object.name, StdFont(12), object.color);
}

AnimationSystem::AnimationSystem() {
}

AnimationSystem::~AnimationSystem() {
    ClearTilemaps();
}

void AnimationSystem::RegisterTilemap(std::shared_ptr<AnimatedTilemap> tilemap) {
    if (tilemap) {
        tilemaps.Add(tilemap);
    }
}

void AnimationSystem::UnregisterTilemap(std::shared_ptr<AnimatedTilemap> tilemap) {
    if (tilemap) {
        for (int i = 0; i < tilemaps.GetCount(); i++) {
            if (tilemaps[i].get() == tilemap.get()) {
                tilemaps.Remove(i);
                break;
            }
        }
    }
}

void AnimationSystem::ClearTilemaps() {
    tilemaps.Clear();
}

void AnimationSystem::Update(double deltaTime) {
    if (isPaused) return;
    
    // Update animations for all registered tilemaps
    for (auto& tilemap : tilemaps) {
        tilemap->UpdateAnimations(deltaTime * animationSpeed);
    }
}

int AnimationSystem::GetTotalAnimatedTilesCount() const {
    int total = 0;
    for (const auto& tilemap : tilemaps) {
        // In a real implementation, we'd need a method to get the count of animated tiles
        // For now, we'll just return a placeholder
        total += 0; // Placeholder - would need actual count from the tilemap
    }
    return total;
}

END_UPP_NAMESPACE