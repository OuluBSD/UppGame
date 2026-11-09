#include "LargeTilemapRenderer.h"

NAMESPACE_UPP

LargeTilemapRenderer::LargeTilemapRenderer() {
}

LargeTilemapRenderer::~LargeTilemapRenderer() {
    chunks.Clear();
    chunkMap.Clear();
}

bool LargeTilemapRenderer::Initialize(int chunkSize, int maxChunksInMemory) {
    SetChunkSize(chunkSize);
    SetMaxChunksInMemory(maxChunksInMemory);
    return true;
}

void LargeTilemapRenderer::SetMap(std::shared_ptr<AnimatedTilemap> map) {
    this->map = map;
}

void LargeTilemapRenderer::Render(Draw& draw, const Rect& viewport, 
                                 int cameraX, int cameraY, 
                                 double scale) {
    if (!map) return;
    
    chunksRenderedLastFrame = 0;
    
    // Update animations if the map supports them
    if (map) {
        UpdateAnimations(0.016); // Assuming ~60 FPS, but in real implementation would use actual delta time
    }
    
    // Get visible chunks based on the viewport
    Vector<Point> visibleChunks = GetVisibleChunks(viewport, cameraX, cameraY);
    
    // Calculate the center chunk for LOD and culling purposes
    Point centerChunk = WorldToChunkCoords(cameraX, cameraY);
    
    for (const Point& chunkCoords : visibleChunks) {
        int chunkIndex = GetChunkIndex(chunkCoords.x, chunkCoords.y, true);
        if (chunkIndex < 0) continue;
        
        TilemapChunk* chunk = &chunks[chunkIndex];
        
        if (!chunk) continue;
        
        // Update chunk if it's dirty (needs re-rendering)
        if (chunk->isDirty) {
            for (int layerIdx = 0; layerIdx < map->GetLayers().GetCount(); layerIdx++) {
                // This is a simplified approach - in a real implementation, we would 
                // render the entire chunk to an image (texture) and then draw that
                for (int tilesetIdx = 0; tilesetIdx < map->GetTilesets().GetCount(); tilesetIdx++) {
                    RenderChunk(chunkIndex, layerIdx, tilesetIdx);
                }
            }
        }
        
        // Calculate the world position of this chunk
        int chunkWorldX = chunkCoords.x * chunkSize * map->GetTileWidth();
        int chunkWorldY = chunkCoords.y * chunkSize * map->GetTileHeight();
        
        // Apply camera offset and scale
        int screenX = (int)(chunkWorldX * scale - cameraX);
        int screenY = (int)(chunkWorldY * scale - cameraY);
        
        int chunkPixelWidth = chunkSize * map->GetTileWidth();
        int chunkPixelHeight = chunkSize * map->GetTileHeight();
        
        int destWidth = (int)(chunkPixelWidth * scale);
        int destHeight = (int)(chunkPixelHeight * scale);
        
        // Draw the pre-rendered chunk
        if (chunk->isRendered && chunk->renderedChunk && !chunk->renderedChunk.IsEmpty()) {
            draw.DrawImageRect(Rect(screenX, screenY, destWidth, destHeight), 
                             chunk->renderedChunk);
            chunksRenderedLastFrame++;
        }
    }
    
    // Evict least recently used chunks if we're exceeding memory limits
    EvictLRUChunks();
}

void LargeTilemapRenderer::UpdateAnimations(double deltaTime) {
    // Update animations in the map
    if (map) {
        map->UpdateAnimations(deltaTime);
    }
    
    // Mark all chunks as dirty since animations might have changed tiles
    for (auto& chunk : chunks) {
        chunk.isDirty = true;
    }
}

int LargeTilemapRenderer::GetChunkIndex(int chunkX, int chunkY, bool createIfNotExists) {
    Point key(chunkX, chunkY);
    int* index = chunkMap.Get(key);
    
    if (index) {
        return *index;
    }
    
    if (!createIfNotExists) {
        return -1;
    }
    
    // Create a new chunk
    if (chunks.GetCount() >= maxChunksInMemory) {
        // Need to evict some chunks first
        EvictLRUChunks();
    }
    
    int newIdx = chunks.GetCount();
    chunks.Add(TilemapChunk(chunkX, chunkY, chunkSize));
    chunkMap.GetAdd(key) = newIdx;
    
    return newIdx;
}

TilemapChunk* LargeTilemapRenderer::GetChunk(int chunkX, int chunkY) {
    int idx = GetChunkIndex(chunkX, chunkY, false);
    if (idx >= 0 && idx < chunks.GetCount()) {
        return &chunks[idx];
    }
    return nullptr;
}

void LargeTilemapRenderer::RenderChunk(int chunkIndex, int layerIndex, int tilesetIndex) {
    if (chunkIndex < 0 || chunkIndex >= chunks.GetCount()) return;
    if (!map) return;
    
    TilemapChunk& chunk = chunks[chunkIndex];
    const MapLayer& layer = map->GetLayers()[layerIndex];
    const Tileset& tileset = map->GetTilesets()[tilesetIndex];
    
    // Create image buffer for this chunk
    int chunkPixelWidth = chunkSize * map->GetTileWidth();
    int chunkPixelHeight = chunkSize * map->GetTileHeight();
    
    ImageBuffer chunkBuffer(chunkPixelWidth, chunkPixelHeight);
    
    // Render tiles in this chunk
    for (int y = 0; y < chunkSize && y < layer.height - chunk.chunkY * chunkSize; y++) {
        for (int x = 0; x < chunkSize && x < layer.width - chunk.chunkX * chunkSize; x++) {
            int tileX = chunk.chunkX * chunkSize + x;
            int tileY = chunk.chunkY * chunkSize + y;
            
            if (tileX >= layer.width || tileY >= layer.height) continue;
            
            const Tile* tile = &layer.tiles[tileY * layer.width + tileX];
            if (!tile || tile->id < 0) continue;
            
            // Get the current tile ID for animated tiles
            int currentTileId = tile->id;
            if (map->HasAnimatedTile(tile->id)) {
                currentTileId = map->GetAnimatedTileId(tile->id);
            }
            
            // Calculate source rectangle in the tileset image
            int tilesPerRow = tileset.image.GetWidth() / tileset.tileWidth;
            int tileXInSet = (currentTileId - tileset.firstGid) % tilesPerRow;
            int tileYInSet = (currentTileId - tileset.firstGid) / tilesPerRow;
            
            if (tileXInSet >= 0 && tileXInSet < tilesPerRow && 
                tileYInSet >= 0 && tileYInSet < tileset.image.GetHeight() / tileset.tileHeight) {
                
                Rect sourceRect(
                    tileXInSet * tileset.tileWidth + tileset.margin,
                    tileYInSet * tileset.tileHeight + tileset.margin,
                    tileset.tileWidth,
                    tileset.tileHeight
                );
                
                // Copy from tileset to chunk buffer
                for (int py = 0; py < tileset.tileHeight && (y * tileset.tileHeight + py) < chunkBuffer.GetHeight(); py++) {
                    for (int px = 0; px < tileset.tileWidth && (x * tileset.tileWidth + px) < chunkBuffer.GetWidth(); px++) {
                        int dstX = x * tileset.tileWidth + px;
                        int dstY = y * tileset.tileHeight + py;
                        
                        if (dstX < chunkBuffer.GetWidth() && dstY < chunkBuffer.GetHeight()) {
                            const RGBA* srcPixel = &tileset.image[tileYInSet * tileset.tileHeight + py][tileXInSet * tileset.tileWidth + px];
                            RGBA* dstPixel = &chunkBuffer[dstY][dstX];
                            
                            *dstPixel = *srcPixel;
                        }
                    }
                }
            }
        }
    }
    
    // Store the rendered chunk
    chunk.renderedChunk = Image(chunkBuffer);
    chunk.isDirty = false;
    chunk.isRendered = true;
}

Vector<Point> LargeTilemapRenderer::GetVisibleChunks(const Rect& viewport, int cameraX, int cameraY) const {
    Vector<Point> visibleChunks;
    
    if (!map) return visibleChunks;
    
    // Convert viewport coordinates to chunk coordinates
    Point topLeft = WorldToChunkCoords(viewport.left + cameraX, viewport.top + cameraY);
    Point bottomRight = WorldToChunkCoords(viewport.right + cameraX, viewport.bottom + cameraY);
    
    // Add a buffer of chunks around the visible area
    const int buffer = 1;
    topLeft.x -= buffer;
    topLeft.y -= buffer;
    bottomRight.x += buffer;
    bottomRight.y += buffer;
    
    // Add all chunks in the visible range
    for (int y = topLeft.y; y <= bottomRight.y; y++) {
        for (int x = topLeft.x; x <= bottomRight.x; x++) {
            // Check if this chunk is within the map bounds
            int tileX = x * chunkSize;
            int tileY = y * chunkSize;
            
            if (tileX < map->GetWidth() && tileY < map->GetHeight()) {
                visibleChunks.Add(Point(x, y));
            }
        }
    }
    
    return visibleChunks;
}

Point LargeTilemapRenderer::WorldToChunkCoords(double worldX, double worldY) const {
    if (!map) return Point(0, 0);
    
    int tileX = (int)floor(worldX / map->GetTileWidth());
    int tileY = (int)floor(worldY / map->GetTileHeight());
    
    return Point(
        (int)floor(tileX / chunkSize),
        (int)floor(tileY / chunkSize)
    );
}

Point2 LargeTilemapRenderer::ChunkToWorldCoords(int chunkX, int chunkY) const {
    if (!map) return Point2(0, 0);
    
    return Point2(
        chunkX * chunkSize * map->GetTileWidth(),
        chunkY * chunkSize * map->GetTileHeight()
    );
}

void LargeTilemapRenderer::EvictLRUChunks() {
    // Simple eviction: remove chunks that are furthest from the center
    // In a real implementation, we'd track access patterns to implement true LRU
    
    // For now, we'll just ensure we don't exceed maxChunksInMemory
    if (chunks.GetCount() <= maxChunksInMemory) return;
    
    // This is a simplified approach - in a real implementation, we'd have an LRU list
    // and remove chunks based on when they were last accessed
    
    // For demonstration purposes, we'll just remove some chunks
    while (chunks.GetCount() > maxChunksInMemory * 0.8) {  // Keep it under 80% of max
        // Remove the last chunk (simple approach)
        if (!chunks.IsEmpty()) {
            Point key = Point(chunks.Top().chunkX, chunks.Top().chunkY);
            chunkMap.RemoveKey(key);
            chunks.RemoveTop();
        }
    }
}

void LargeTilemapRenderer::InvalidateAllChunks() {
    for (auto& chunk : chunks) {
        chunk.isDirty = true;
    }
}

void LargeTilemapRenderer::PreloadChunks(int centerX, int centerY, int radius) {
    Point centerChunk = WorldToChunkCoords(centerX, centerY);
    
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            int chunkX = centerChunk.x + dx;
            int chunkY = centerChunk.y + dy;
            
            // Try to get/create the chunk to preload it
            GetChunkIndex(chunkX, chunkY, true);
        }
    }
}

void LargeTilemapRenderer::UnloadDistantChunks(int centerX, int centerY, int radius) {
    Point centerChunk = WorldToChunkCoords(centerX, centerY);
    
    // Go through all chunks and remove those outside the radius
    Vector<int> chunksToRemove;
    for (int i = 0; i < chunks.GetCount(); i++) {
        int dx = abs(chunks[i].chunkX - centerChunk.x);
        int dy = abs(chunks[i].chunkY - centerChunk.y);
        
        if (dx > radius || dy > radius) {
            Point key(chunks[i].chunkX, chunks[i].chunkY);
            chunksToRemove.Add(i);
        }
    }
    
    // Remove chunks in reverse order to maintain indices
    for (int i = chunksToRemove.GetCount() - 1; i >= 0; i--) {
        int idx = chunksToRemove[i];
        Point key(chunks[idx].chunkX, chunks[idx].chunkY);
        chunkMap.RemoveKey(key);
        chunks.Remove(idx);
    }
}

// TilemapQuadtree implementation
TilemapQuadtree::Node::Node(const Rect& b, int maxObjs, int maxD, int curDepth)
    : bounds(b), maxObjects(maxObjs), maxDepth(maxD), currentDepth(curDepth), isDivided(false) {
    for (int i = 0; i < 4; i++) {
        children[i] = nullptr;
    }
}

TilemapQuadtree::Node::~Node() {
    for (int i = 0; i < 4; i++) {
        delete children[i];
    }
}

void TilemapQuadtree::Node::Subdivide() {
    int halfWidth = bounds.Width() / 2;
    int halfHeight = bounds.Height() / 2;
    int x = bounds.left;
    int y = bounds.top;
    
    // Create four children: NW, NE, SW, SE
    children[0] = new Node(Rect(x, y, x + halfWidth, y + halfHeight), maxObjects, maxDepth, currentDepth + 1);
    children[1] = new Node(Rect(x + halfWidth, y, x + halfWidth * 2, y + halfHeight), maxObjects, maxDepth, currentDepth + 1);
    children[2] = new Node(Rect(x, y + halfHeight, x + halfWidth, y + halfHeight * 2), maxObjects, maxDepth, currentDepth + 1);
    children[3] = new Node(Rect(x + halfWidth, y + halfHeight, x + halfWidth * 2, y + halfHeight * 2), maxObjects, maxDepth, currentDepth + 1);
    
    isDivided = true;
}

int TilemapQuadtree::Node::GetChildIndex(const Rect& rect) const {
    int x = bounds.left + bounds.Width() / 2;
    int y = bounds.top + bounds.Height() / 2;
    
    bool topQuadrant = (rect.top < y && rect.bottom < y);
    bool bottomQuadrant = (rect.top > y);
    bool leftQuadrant = (rect.left < x && rect.right < x);
    bool rightQuadrant = (rect.left > x);
    
    if (topQuadrant && leftQuadrant) return 0;   // NW
    if (topQuadrant && rightQuadrant) return 1;  // NE
    if (bottomQuadrant && leftQuadrant) return 2; // SW
    if (bottomQuadrant && rightQuadrant) return 3; // SE
    
    return -1; // Does not fit in a single quadrant
}

TilemapQuadtree::TilemapQuadtree() : root(nullptr), maxObjectsPerNode(10), maxDepth(5), objectCount(0), nodeCount(1) {
}

TilemapQuadtree::TilemapQuadtree(const Rect& bounds, int maxObjectsPerNode, int maxDepth)
    : maxObjectsPerNode(maxObjectsPerNode), maxDepth(maxDepth), objectCount(0), nodeCount(1) {
    root = new Node(bounds, maxObjectsPerNode, maxDepth, 0);
}

TilemapQuadtree::~TilemapQuadtree() {
    delete root;
}

void TilemapQuadtree::Insert(const Rect& bounds, int tileId) {
    if (!root) return;
    
    // Insert in a recursive manner
    std::vector<Node*> nodesToVisit = {root};
    
    while (!nodesToVisit.empty()) {
        Node* current = nodesToVisit.back();
        nodesToVisit.pop_back();
        
        // If node is divided, check which child(ren) the bounds belong to
        if (current->isDivided) {
            int index = current->GetChildIndex(bounds);
            if (index != -1) {
                // Fits entirely in one child
                nodesToVisit.push_back(current->children[index]);
            } else {
                // Crosses multiple children, add to current node
                current->objects.Add(std::make_pair(bounds, tileId));
                
                // May need to redistribute objects if node gets too full
                if (current->objects.GetCount() > maxObjectsPerNode && current->currentDepth < maxDepth) {
                    current->Subdivide();
                    // Redistribute existing objects to children
                    Vector<std::pair<Rect, int>> tempObjects = current->objects;
                    current->objects.Clear();
                    
                    for (const auto& obj : tempObjects) {
                        Insert(obj.first, obj.second); // Re-insert using the new structure
                    }
                }
            }
        } else {
            // If node is not divided, add to current node
            current->objects.Add(std::make_pair(bounds, tileId));
            objectCount++;
            
            // Subdivide if too many objects and not at max depth
            if (current->objects.GetCount() > maxObjectsPerNode && current->currentDepth < maxDepth) {
                current->Subdivide();
                // Redistribute existing objects to children
                Vector<std::pair<Rect, int>> tempObjects = current->objects;
                current->objects.Clear();
                
                for (const auto& obj : tempObjects) {
                    Insert(obj.first, obj.second); // Re-insert using the new structure
                }
            }
        }
    }
}

Vector<int> TilemapQuadtree::Query(const Rect& bounds) const {
    Vector<int> result;
    if (!root) return result;
    
    std::vector<const Node*> nodesToVisit = {root};
    
    while (!nodesToVisit.empty()) {
        const Node* current = nodesToVisit.back();
        nodesToVisit.pop_back();
        
        // Check if query bounds intersect with node bounds
        if (!IsWithinBounds(bounds, current->bounds)) {
            continue;
        }
        
        // Add objects in this node that intersect with query bounds
        for (const auto& obj : current->objects) {
            if (bounds.Intersects(obj.first)) {
                result.Add(obj.second);
            }
        }
        
        // If node is divided, check children
        if (current->isDivided) {
            for (int i = 0; i < 4; i++) {
                nodesToVisit.push_back(current->children[i]);
            }
        }
    }
    
    return result;
}

void TilemapQuadtree::Clear() {
    delete root;
    root = new Node(Rect(0, 0, 10000, 10000), maxObjectsPerNode, maxDepth, 0); // Default bounds
    objectCount = 0;
    nodeCount = 1;
}

bool TilemapQuadtree::IsWithinBounds(const Rect& a, const Rect& b) const {
    return !(a.right < b.left || a.left > b.right || a.bottom < b.top || a.top > b.bottom);
}

END_UPP_NAMESPACE