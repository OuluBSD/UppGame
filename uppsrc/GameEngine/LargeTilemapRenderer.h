#ifndef UPP_LARGE_TILEMAP_RENDERER_H
#define UPP_LARGE_TILEMAP_RENDERER_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <GameLib/GameLib.h>
#include <GameEngine/GameEngine.h>
#include <GameEngine/TilemapRenderer.h>
#include <GameEngine/TilemapAnimations.h>

NAMESPACE_UPP

// Chunk-based tilemap for large maps
struct TilemapChunk {
    int chunkX, chunkY;                    // Chunk coordinates
    Vector<Tile> tiles;                    // Tiles in this chunk
    Image renderedChunk;                   // Pre-rendered image of the chunk
    bool isDirty = true;                   // Whether the chunk needs re-rendering
    bool isRendered = false;               // Whether the chunk has been rendered
    
    // Constructor
    TilemapChunk(int x, int y, int chunkSize) : chunkX(x), chunkY(y) {
        tiles.SetCount(chunkSize * chunkSize);
    }
};

// Optimized renderer for large tilemaps using chunking
class LargeTilemapRenderer {
public:
    LargeTilemapRenderer();
    virtual ~LargeTilemapRenderer();

    // Initialize the renderer with specific parameters
    bool Initialize(int chunkSize = 32, int maxChunksInMemory = 256);

    // Set the map to render
    void SetMap(std::shared_ptr<AnimatedTilemap> map);
    std::shared_ptr<AnimatedTilemap> GetMap() const { return map; }

    // Render the large tilemap
    void Render(Draw& draw, const Rect& viewport, 
               int cameraX = 0, int cameraY = 0, 
               double scale = 1.0);

    // Update animations for the tilemap
    void UpdateAnimations(double deltaTime);

    // Get/set rendering options
    void SetChunkSize(int size) { chunkSize = max(8, min(size, 64)); } // Limit chunk size
    int GetChunkSize() const { return chunkSize; }

    void SetMaxChunksInMemory(int max) { maxChunksInMemory = max; }
    int GetMaxChunksInMemory() const { return maxChunksInMemory; }

    void SetEnableFrustumCulling(bool enable) { enableFrustumCulling = enable; }
    bool IsFrustumCullingEnabled() const { return enableFrustumCulling; }

    void SetEnableChunkCaching(bool enable) { enableChunkCaching = enable; }
    bool IsChunkCachingEnabled() const { return enableChunkCaching; }

    void SetEnableLOD(bool enable) { enableLOD = enable; }
    bool IsLODEnabled() const { return enableLOD; }

    // Force re-render all chunks
    void InvalidateAllChunks();

    // Get rendering statistics
    int GetActiveChunksCount() const { return activeChunks.GetCount(); }
    int GetCachedChunksCount() const { return chunks.GetCount(); }
    int GetChunksRenderedLastFrame() const { return chunksRenderedLastFrame; }

    // Preload chunks around a specific position
    void PreloadChunks(int centerX, int centerY, int radius = 2);

    // Unload distant chunks to save memory
    void UnloadDistantChunks(int centerX, int centerY, int radius = 5);

private:
    std::shared_ptr<AnimatedTilemap> map;
    
    // Chunking system
    int chunkSize = 32;
    int maxChunksInMemory = 256;
    HashMap<Point, int> chunkMap;  // Map from chunk coordinates to index in chunks vector
    Vector<TilemapChunk> chunks;
    
    // Rendering options
    bool enableFrustumCulling = true;
    bool enableChunkCaching = true;
    bool enableLOD = true;
    
    // For animation updates
    AnimatedTilemapRenderer animationRenderer;
    
    // Statistics
    int chunksRenderedLastFrame = 0;
    
    // Find or create a chunk at the given coordinates
    int GetChunkIndex(int chunkX, int chunkY, bool createIfNotExists = true);
    
    // Get a chunk
    TilemapChunk* GetChunk(int chunkX, int chunkY);
    
    // Render a single chunk
    void RenderChunk(int chunkIndex, int layerIndex, int tilesetIndex);
    
    // Determine which chunks are visible in the viewport
    Vector<Point> GetVisibleChunks(const Rect& viewport, int cameraX, int cameraY) const;
    
    // Calculate chunk coordinates from world coordinates
    Point WorldToChunkCoords(double worldX, double worldY) const;
    
    // Calculate world coordinates from chunk and tile coordinates
    Point2 ChunkToWorldCoords(int chunkX, int chunkY) const;
    
    // Evict least recently used chunks when memory limit is reached
    void EvictLRUChunks();
    
    // Update chunk render status
    void UpdateChunkStatus();
};

// Quadtree for spatial indexing of large tilemaps
class TilemapQuadtree {
public:
    TilemapQuadtree();
    explicit TilemapQuadtree(const Rect& bounds, int maxObjectsPerNode = 10, int maxDepth = 5);
    virtual ~TilemapQuadtree();

    // Insert an object (tile bounds) into the quadtree
    void Insert(const Rect& bounds, int tileId);
    
    // Query objects that might intersect with the given bounds
    Vector<int> Query(const Rect& bounds) const;
    
    // Clear all objects from the quadtree
    void Clear();
    
    // Get tree statistics
    int GetObjectCount() const { return objectCount; }
    int GetNodeCount() const { return nodeCount; }

private:
    struct Node {
        Rect bounds;
        Vector<std::pair<Rect, int>> objects;  // bounds and tile ID
        Node* children[4];  // NW, NE, SW, SE
        bool isDivided;
        int maxObjects;
        int maxDepth;
        int currentDepth;
        
        Node(const Rect& b, int maxObjs, int maxD, int curDepth);
        ~Node();
        
        void Subdivide();
        int GetChildIndex(const Rect& bounds) const;
    };
    
    Node* root;
    int maxObjectsPerNode;
    int maxDepth;
    int objectCount;
    int nodeCount;
    
    bool IsWithinBounds(const Rect& a, const Rect& b) const;
    Vector<int> QueryRecursive(const Rect& queryBounds, const Node* node) const;
};

END_UPP_NAMESPACE

#endif