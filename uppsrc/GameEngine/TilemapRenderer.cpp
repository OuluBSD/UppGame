#include "TilemapRenderer.h"

NAMESPACE_UPP

TilemapRenderer::TilemapRenderer() {
}

TilemapRenderer::~TilemapRenderer() {
}

bool TilemapRenderer::Initialize() {
    // In a real implementation, this would initialize GPU resources
    return true;
}

void TilemapRenderer::SetMap(std::shared_ptr<TmxMap> map) {
    this->map = map;
    InvalidateCache();
}

void TilemapRenderer::Render(Draw& draw, const Rect& viewport, 
                            int cameraX, int cameraY, 
                            double scale) {
    if (!map) return;

    // Render all visible layers
    for (int layerIdx = 0; layerIdx < map->GetLayers().GetCount(); layerIdx++) {
        const MapLayer& layer = map->GetLayers()[layerIdx];
        if (layer.visible) {
            RenderLayer(draw, layerIdx, viewport, cameraX, cameraY, scale);
        }
    }
}

void TilemapRenderer::RenderLayer(Draw& draw, int layerIndex, const Rect& viewport,
                                 int cameraX, int cameraY, 
                                 double scale) {
    if (!map || layerIndex < 0 || layerIndex >= map->GetLayers().GetCount()) return;

    const MapLayer& layer = map->GetLayers()[layerIndex];
    if (!layer.visible) return;

    // Calculate visible area
    Rect visibleArea = CalculateVisibleArea(viewport, cameraX, cameraY, scale);

    // Determine the range of tiles to render
    int startX = max(0, visibleArea.left);
    int endX = min(layer.width, visibleArea.right);
    int startY = max(0, visibleArea.top);
    int endY = min(layer.height, visibleArea.bottom);

    // Render each visible tile
    for (int y = startY; y < endY; y++) {
        for (int x = startX; x < endX; x++) {
            const Tile* tile = GetTileAt(layerIndex, x, y);
            if (tile && tile->id >= 0) {  // Only render if there's an actual tile
                // Calculate screen position
                int screenX = (int)(x * map->GetTileWidth() * scale - cameraX);
                int screenY = (int)(y * map->GetTileHeight() * scale - cameraY);

                // Find the appropriate tileset for this tile
                int tilesetIndex = FindTilesetForTile(tile->id);
                if (tilesetIndex >= 0) {
                    RenderTile(draw, *tile, tilesetIndex, screenX, screenY, scale);
                }
            }
        }
    }
}

void TilemapRenderer::RenderTile(Draw& draw, const Tile& tile, int tilesetIndex, 
                                int screenX, int screenY, double scale) {
    if (tilesetIndex < 0 || tilesetIndex >= map->GetTilesets().GetCount()) return;

    const Tileset& tileset = map->GetTilesets()[tilesetIndex];

    // Calculate source rectangle in the tileset image
    int tilesPerRow = tileset.image.GetWidth() / tileset.tileWidth;
    int tileX = (tile.id - tileset.firstGid) % tilesPerRow;
    int tileY = (tile.id - tileset.firstGid) / tilesPerRow;

    Rect sourceRect(
        tileX * tileset.tileWidth + tileset.margin,
        tileY * tileset.tileHeight + tileset.margin,
        tileset.tileWidth,
        tileset.tileHeight
    );

    // Apply flipping if necessary
    // For now, we'll render the tile as-is; flipping would require more complex rendering
    
    // Calculate destination size
    int destWidth = (int)(map->GetTileWidth() * scale);
    int destHeight = (int)(map->GetTileHeight() * scale);

    // Draw the tile
    if (tileset.image && !tileset.image.IsEmpty()) {
        draw.DrawImageRect(Rect(screenX, screenY, destWidth, destHeight), 
                         tileset.image, sourceRect);
    } else {
        // Fallback: draw a colored rectangle
        draw.DrawRect(Rect(screenX, screenY, destWidth, destHeight), 
                     Color(100, 100, 100));
    }
}

int TilemapRenderer::FindTilesetForTile(int globalTileId) const {
    if (!map) return -1;

    // Find the tileset that contains this global tile ID
    const Vector<Tileset>& tilesets = map->GetTilesets();
    for (int i = tilesets.GetCount() - 1; i >= 0; i--) {  // Check in reverse order
        if (globalTileId >= tilesets[i].firstGid) {
            return i;
        }
    }

    return -1; // Not found
}

Rect TilemapRenderer::CalculateVisibleArea(const Rect& viewport, int cameraX, int cameraY, 
                                          double scale) const {
    if (!map) return Rect(0, 0, 0, 0);

    // Convert viewport coordinates to tile coordinates
    int tileWidth = map->GetTileWidth();
    int tileHeight = map->GetTileHeight();

    int startTileX = max(0, (int)floor((cameraX) / (tileWidth * scale)) - maxRenderDistance);
    int startTileY = max(0, (int)floor((cameraY) / (tileHeight * scale)) - maxRenderDistance);
    int endTileX = min(map->GetWidth(), (int)ceil((cameraX + viewport.Width()) / (tileWidth * scale)) + maxRenderDistance);
    int endTileY = min(map->GetHeight(), (int)ceil((cameraY + viewport.Height()) / (tileHeight * scale)) + maxRenderDistance);

    return Rect(startTileX, startTileY, endTileX, endTileY);
}

Point TilemapRenderer::GetTileAtScreenPos(int screenX, int screenY, int cameraX, int cameraY, 
                                         double scale) const {
    if (!map) return Point(-1, -1);

    // Convert screen position to world position
    int worldX = (int)(screenX + cameraX);
    int worldY = (int)(screenY + cameraY);

    // Convert world position to tile coordinates
    int tileX = (int)floor(worldX / (map->GetTileWidth() * scale));
    int tileY = (int)floor(worldY / (map->GetTileHeight() * scale));

    // Check if coordinates are within map bounds
    if (tileX >= 0 && tileX < map->GetWidth() && 
        tileY >= 0 && tileY < map->GetHeight()) {
        return Point(tileX, tileY);
    }

    return Point(-1, -1); // Outside map bounds
}

const Tile* TilemapRenderer::GetTileAt(int layerIndex, int x, int y) const {
    if (!map) return nullptr;
    return map->GetTileAt(layerIndex, x, y);
}

Point2 TilemapRenderer::GetWorldPosition(int tileX, int tileY) const {
    if (!map) return Point2(0, 0);

    double worldX = tileX * map->GetTileWidth();
    double worldY = tileY * map->GetTileHeight();

    return Point2(worldX, worldY);
}

Point TilemapRenderer::GetTileCoordinates(double worldX, double worldY) const {
    if (!map) return Point(-1, -1);

    int tileX = (int)floor(worldX / map->GetTileWidth());
    int tileY = (int)floor(worldY / map->GetTileHeight());

    return Point(tileX, tileY);
}

void TilemapRenderer::InvalidateCache() {
    cacheValid = false;
    tileCache.Clear();
}

// AnimatedTilemapRenderer implementation
AnimatedTilemapRenderer::AnimatedTilemapRenderer() {
}

AnimatedTilemapRenderer::~AnimatedTilemapRenderer() {
}

void AnimatedTilemapRenderer::AddAnimatedTile(int globalTileId, const AnimatedTile& animation) {
    animatedTiles.GetAdd(globalTileId) = animation;
}

void AnimatedTilemapRenderer::UpdateAnimations(double deltaTime) {
    animationTime += deltaTime;

    for (auto& anim : animatedTiles) {
        AnimatedTile& tileAnim = anim.value;
        tileAnim.timeElapsed += deltaTime;

        // Change frame if needed
        if (tileAnim.timeElapsed >= tileAnim.frameDuration) {
            tileAnim.timeElapsed = fmod(tileAnim.timeElapsed, tileAnim.frameDuration);
            tileAnim.currentFrame = (tileAnim.currentFrame + 1) % tileAnim.frames.GetCount();
        }
    }
}

void AnimatedTilemapRenderer::ClearAnimations() {
    animatedTiles.Clear();
}

void AnimatedTilemapRenderer::Render(Draw& draw, const Rect& viewport,
                                    int cameraX, int cameraY,
                                    double scale) {
    // For this implementation, we'll just call the base render method
    // In a full implementation, we would replace animated tiles with their current frame
    TilemapRenderer::Render(draw, viewport, cameraX, cameraY, scale);
}

END_UPP_NAMESPACE