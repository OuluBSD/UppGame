#ifndef UPP_TEXTURE_STREAMING_H
#define UPP_TEXTURE_STREAMING_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <CtrlCore/CtrlCore.h>
#include <Geometry/Geometry.h>
#include <GameLib/GameLib.h>
#include <GameEngine/GameEngine.h>

NAMESPACE_UPP_BEGIN

// Texture streaming priorities
enum class TexturePriority {
    IMMEDIATE = 0,    // Load immediately, highest priority
    HIGH = 1,         // Load as soon as possible
    NORMAL = 2,       // Normal loading priority
    LOW = 3,          // Low priority loading
    BACKGROUND = 4    // Lowest priority, loaded in background
};

// Texture streaming states
enum class TextureStreamingState {
    NOT_LOADED,
    LOADING,
    LOADED,
    STREAMING_IN,
    STREAMING_OUT,
    UNLOADING,
    ERROR
};

// Texture handle for streaming management
class TextureHandle {
public:
    TextureHandle() : id(0), state(TextureStreamingState::NOT_LOADED), priority(TexturePriority::NORMAL) {}
    
    // Set texture data
    void SetImage(const Image& image) { 
        this->image = image; 
        state = TextureStreamingState::LOADED;
    }
    
    // Get texture data
    const Image& GetImage() const { return image; }
    
    // Get texture ID
    int GetId() const { return id; }
    
    // Get/set state
    TextureStreamingState GetState() const { return state; }
    void SetState(TextureStreamingState newState) { state = newState; }
    
    // Get/set priority
    TexturePriority GetPriority() const { return priority; }
    void SetPriority(TexturePriority newPriority) { priority = newPriority; }
    
    // Get texture properties
    Size GetSize() const { return image.GetSize(); }
    int GetWidth() const { return image.GetWidth(); }
    int GetHeight() const { return image.GetHeight(); }
    
    // Calculate memory usage
    size_t GetMemoryUsage() const { 
        Size sz = GetSize();
        return sz.cx * sz.cy * 4; // 4 bytes per pixel (RGBA)
    }
    
    // Set path for async loading
    void SetPath(const String& path) { this->path = path; }
    const String& GetPath() const { return path; }
    
    // Check if texture is loaded
    bool IsLoaded() const { return state == TextureStreamingState::LOADED; }
    bool IsLoading() const { return state == TextureStreamingState::LOADING; }
    bool IsReady() const { return IsLoaded() && !image.IsEmpty(); }

private:
    int id;
    Image image;
    TextureStreamingState state;
    TexturePriority priority;
    String path;
    static int next_id;
};

// Texture streaming system for managing texture loading/unloading
class TextureStreamingSystem {
public:
    TextureStreamingSystem();
    ~TextureStreamingSystem();
    
    // Initialize the system with memory budget and thread count
    void Initialize(uint64 memoryBudget = 256 * 1024 * 1024, int numThreads = 2); // 256MB, 2 threads default
    
    // Request a texture to be loaded
    SharedPtr<TextureHandle> RequestTexture(const String& path, TexturePriority priority = TexturePriority::NORMAL);
    
    // Request a texture by ID to be loaded
    SharedPtr<TextureHandle> RequestTexture(int textureId, TexturePriority priority = TexturePriority::NORMAL);
    
    // Register a texture for streaming management
    int RegisterTexture(const Image& image, TexturePriority priority = TexturePriority::NORMAL);
    
    // Unregister a texture (marks for unloading)
    void UnregisterTexture(int textureId);
    
    // Update the streaming system (call once per frame)
    void Update(double deltaTime);
    
    // Get texture by ID
    SharedPtr<TextureHandle> GetTexture(int textureId) const;
    
    // Get texture by path
    SharedPtr<TextureHandle> GetTexture(const String& path) const;
    
    // Get texture info
    bool IsTextureLoaded(int textureId) const;
    bool IsTextureLoading(int textureId) const;
    bool IsTextureReady(int textureId) const;
    
    // Get/set memory budget (total memory allowed for textures)
    void SetMemoryBudget(uint64 budget) { memory_budget = budget; }
    uint64 GetMemoryBudget() const { return memory_budget; }
    
    // Get current memory usage
    uint64 GetUsedMemory() const { return used_memory; }
    
    // Get texture count
    int GetTextureCount() const { return (int)texture_handles.size(); }
    
    // Set streaming distance parameters (for LOD-based streaming)
    void SetStreamingDistance(float minDistance, float maxDistance) {
        min_streaming_distance = minDistance;
        max_streaming_distance = maxDistance;
    }
    
    // Set camera position for distance-based streaming
    void SetCameraPosition(const Point3& position) { camera_position = position; }
    
    // Force immediate loading of all pending textures
    void LoadAllPendingTextures();
    
    // Unload textures to stay within budget
    void EnforceMemoryBudget();
    
    // Clear all textures
    void ClearAllTextures();
    
    // Get list of texture IDs
    Vector<int> GetTextureIds() const;
    
    // Get texture statistics
    struct TextureStats {
        int total_textures;
        int loaded_textures;
        int loading_textures;
        int unloaded_textures;
        uint64 used_memory;
        uint64 budget_memory;
    };
    
    TextureStats GetStats() const;
    
private:
    // Internal methods
    void ProcessLoadingQueue();
    void ProcessUnloadingQueue();
    void UpdateTexturePriorities();
    bool CanLoadMoreTextures() const;
    void LoadTextureFromFile(int textureId, const String& path);
    void UnloadTexture(int textureId);
    void SortLoadingQueue();
    
    // Data structures
    std::map<int, SharedPtr<TextureHandle>> texture_handles;
    std::map<String, int> path_to_id;
    std::vector<int> loading_queue; // Textures to load
    std::vector<int> unloading_queue; // Textures to unload
    std::map<int, String> pending_loads; // Async loading operations
    
    // Memory management
    uint64 memory_budget = 0;
    uint64 used_memory = 0;
    
    // Threading
    std::vector<Thread> loader_threads;
    int num_loader_threads = 0;
    bool running = false;
    
    // Streaming parameters
    float min_streaming_distance = 0.0f;
    float max_streaming_distance = 100.0f;
    Point3 camera_position;
    
    // Priority management
    int next_priority_update = 0;
};

// Texture cache to optimize frequently accessed textures
class TextureCache {
public:
    TextureCache();
    ~TextureCache();
    
    // Initialize cache with memory budget
    void Initialize(uint64 memoryBudget = 128 * 1024 * 1024); // 128MB default
    
    // Add texture to cache
    void AddTexture(const String& key, const Image& texture);
    
    // Get texture from cache
    Image GetTexture(const String& key) const;
    
    // Check if texture exists in cache
    bool HasTexture(const String& key) const;
    
    // Remove texture from cache
    void RemoveTexture(const String& key);
    
    // Clear cache
    void Clear();
    
    // Get cache statistics
    uint64 GetUsedMemory() const { return used_memory; }
    uint64 GetMemoryBudget() const { return memory_budget; }
    size_t GetCacheSize() const { return cache.size(); }
    
    // Get cache hit ratio
    double GetHitRatio() const;
    
    // Set/get memory budget
    void SetMemoryBudget(uint64 budget);
    
    // Enforce memory budget by removing least recently used textures
    void EnforceBudget();
    
private:
    struct CachedTexture {
        Image data;
        uint64 access_time;  // Timestamp of last access
        size_t memory_usage; // Memory used by this texture
        
        CachedTexture() : access_time(0), memory_usage(0) {}
        CachedTexture(const Image& img) : data(img), access_time(GetTickCount()), 
                                         memory_usage(img.GetWidth() * img.GetHeight() * 4) {}
    };
    
    std::map<String, CachedTexture> cache;
    std::vector<String> access_order; // For LRU implementation
    
    uint64 memory_budget = 0;
    uint64 used_memory = 0;
    
    mutable int hits = 0;
    mutable int misses = 0;
    
    mutable CriticalSection cs; // For thread safety
};

// Implementation
inline int TextureHandle::next_id = 0;

inline TextureStreamingSystem::TextureStreamingSystem() {
    // Initialize streaming system
}

inline TextureStreamingSystem::~TextureStreamingSystem() {
    ClearAllTextures();
    
    running = false;
    for (auto& thread : loader_threads) {
        if (thread.IsRunning()) {
            thread.Wait();
        }
    }
}

inline void TextureStreamingSystem::Initialize(uint64 memoryBudget, int numThreads) {
    memory_budget = memoryBudget;
    num_loader_threads = numThreads;
    
    running = true;
    
    // Create loader threads
    loader_threads.resize(num_loader_threads);
    for (int i = 0; i < num_loader_threads; i++) {
        loader_threads[i].Run([this, i]() {
            while (running) {
                ProcessLoadingQueue();
                Sleep(10); // Prevent busy waiting
            }
        });
    }
}

inline SharedPtr<TextureHandle> TextureStreamingSystem::RequestTexture(const String& path, TexturePriority priority) {
    auto it = path_to_id.find(path);
    if (it != path_to_id.end()) {
        int id = it->second;
        auto handle = GetTexture(id);
        if (handle) {
            handle->SetPriority(priority);
            return handle;
        }
    }
    
    // If texture isn't registered, register it first
    int id = RegisterTexture(Image(), priority);
    texture_handles[id]->SetPath(path);
    path_to_id[path] = id;
    
    // Add to loading queue
    loading_queue.push_back(id);
    texture_handles[id]->SetState(TextureStreamingState::LOADING);
    
    return texture_handles[id];
}

inline SharedPtr<TextureHandle> TextureStreamingSystem::RequestTexture(int textureId, TexturePriority priority) {
    auto handle = GetTexture(textureId);
    if (handle) {
        handle->SetPriority(priority);
    }
    return handle;
}

inline int TextureStreamingSystem::RegisterTexture(const Image& image, TexturePriority priority) {
    int id = next_id++;
    auto handle = MakeSharedPtr<TextureHandle>();
    handle->SetImage(image);
    handle->SetPriority(priority);
    handle->SetState(image.IsEmpty() ? TextureStreamingState::NOT_LOADED : TextureStreamingState::LOADED);
    
    texture_handles[id] = handle;
    
    if (!image.IsEmpty()) {
        used_memory += handle->GetMemoryUsage();
    }
    
    return id;
}

inline void TextureStreamingSystem::UnregisterTexture(int textureId) {
    auto it = texture_handles.find(textureId);
    if (it != texture_handles.end()) {
        // Remove from path mapping if exists
        for (auto pathIt = path_to_id.begin(); pathIt != path_to_id.end(); ) {
            if (pathIt->second == textureId) {
                pathIt = path_to_id.erase(pathIt);
            } else {
                ++pathIt;
            }
        }
        
        // Update memory usage
        if (it->second->IsLoaded()) {
            used_memory -= it->second->GetMemoryUsage();
        }
        
        texture_handles.erase(it);
    }
}

inline void TextureStreamingSystem::Update(double deltaTime) {
    // Update texture priorities based on distance or other factors
    UpdateTexturePriorities();
    
    // Process loading/unloading as needed
    ProcessLoadingQueue();
    EnforceMemoryBudget();
}

inline SharedPtr<TextureHandle> TextureStreamingSystem::GetTexture(int textureId) const {
    auto it = texture_handles.find(textureId);
    return (it != texture_handles.end()) ? it->second : nullptr;
}

inline SharedPtr<TextureHandle> TextureStreamingSystem::GetTexture(const String& path) const {
    auto it = path_to_id.find(path);
    if (it != path_to_id.end()) {
        return GetTexture(it->second);
    }
    return nullptr;
}

inline bool TextureStreamingSystem::IsTextureLoaded(int textureId) const {
    auto handle = GetTexture(textureId);
    return handle && handle->IsLoaded();
}

inline bool TextureStreamingSystem::IsTextureLoading(int textureId) const {
    auto handle = GetTexture(textureId);
    return handle && handle->IsLoading();
}

inline bool TextureStreamingSystem::IsTextureReady(int textureId) const {
    auto handle = GetTexture(textureId);
    return handle && handle->IsReady();
}

inline void TextureStreamingSystem::LoadAllPendingTextures() {
    // Load any textures that are not yet loaded
    for (auto& pair : texture_handles) {
        if (pair.second->GetState() == TextureStreamingState::NOT_LOADED ||
            pair.second->GetState() == TextureStreamingState::LOADING) {
            if (!pair.second->GetPath().IsEmpty()) {
                LoadTextureFromFile(pair.first, pair.second->GetPath());
            }
        }
    }
}

inline void TextureStreamingSystem::EnforceMemoryBudget() {
    if (used_memory <= memory_budget) {
        return; // Within budget
    }
    
    // Simple approach: unload lowest priority textures until budget is met
    Vector<int> unload_candidates;
    
    for (auto& pair : texture_handles) {
        if (pair.second->GetState() == TextureStreamingState::LOADED) {
            unload_candidates.Add(pair.first);
        }
    }
    
    // Sort by priority (lowest first)
    Sort(unload_candidates, [this](int a, int b) {
        return texture_handles[a]->GetPriority() > texture_handles[b]->GetPriority();
    });
    
    // Unload textures until within budget
    for (int id : unload_candidates) {
        if (used_memory <= memory_budget) break;
        
        auto handle = texture_handles[id];
        if (handle && handle->GetState() == TextureStreamingState::LOADED) {
            UnloadTexture(id);
        }
    }
}

inline void TextureStreamingSystem::ClearAllTextures() {
    for (auto& pair : texture_handles) {
        if (pair.second->IsLoaded()) {
            used_memory -= pair.second->GetMemoryUsage();
        }
    }
    
    texture_handles.clear();
    path_to_id.clear();
    loading_queue.clear();
    unloading_queue.clear();
    pending_loads.clear();
}

inline Vector<int> TextureStreamingSystem::GetTextureIds() const {
    Vector<int> ids;
    for (const auto& pair : texture_handles) {
        ids.Add(pair.first);
    }
    return ids;
}

inline TextureStreamingSystem::TextureStats TextureStreamingSystem::GetStats() const {
    TextureStats stats = {};
    stats.total_textures = GetTextureCount();
    
    for (const auto& pair : texture_handles) {
        switch (pair.second->GetState()) {
            case TextureStreamingState::LOADED:
                stats.loaded_textures++;
                break;
            case TextureStreamingState::LOADING:
                stats.loading_textures++;
                break;
            default:
                stats.unloaded_textures++;
                break;
        }
    }
    
    stats.used_memory = used_memory;
    stats.budget_memory = memory_budget;
    
    return stats;
}

inline void TextureStreamingSystem::ProcessLoadingQueue() {
    // Process textures in the loading queue
    for (size_t i = 0; i < loading_queue.size(); ) {
        int id = loading_queue[i];
        auto handle = GetTexture(id);
        
        if (handle && handle->GetState() == TextureStreamingState::LOADING) {
            // Check if it's already being loaded asynchronously
            if (pending_loads.find(id) != pending_loads.end()) {
                // Still loading, skip for now
                i++;
                continue;
            }
            
            // Start loading if the file path is set
            if (!handle->GetPath().IsEmpty()) {
                LoadTextureFromFile(id, handle->GetPath());
                loading_queue.erase(loading_queue.begin() + i);
            } else {
                i++;
            }
        } else {
            loading_queue.erase(loading_queue.begin() + i);
        }
    }
}

inline void TextureStreamingSystem::UpdateTexturePriorities() {
    // For distance-based priority updates (simplified)
    // In a real implementation, this would consider camera distance, screen space size, etc.
    if (++next_priority_update > 60) { // Update every 60 frames
        next_priority_update = 0;
    }
}

inline bool TextureStreamingSystem::CanLoadMoreTextures() const {
    return used_memory < memory_budget * 0.9; // Keep 10% headroom
}

inline void TextureStreamingSystem::LoadTextureFromFile(int textureId, const String& path) {
    auto handle = GetTexture(textureId);
    if (!handle) return;
    
    Image img = LoadImageFile(AsString(path));
    if (!img.IsEmpty()) {
        handle->SetImage(img);
        handle->SetState(TextureStreamingState::LOADED);
        
        // Update memory usage
        used_memory += handle->GetMemoryUsage();
    } else {
        handle->SetState(TextureStreamingState::ERROR);
    }
}

inline void TextureStreamingSystem::UnloadTexture(int textureId) {
    auto handle = GetTexture(textureId);
    if (handle && handle->IsLoaded()) {
        used_memory -= handle->GetMemoryUsage();
        handle->SetImage(Image()); // Clear image data
        handle->SetState(TextureStreamingState::NOT_LOADED);
    }
}

inline TextureCache::TextureCache() {
    // Initialize cache
}

inline TextureCache::~TextureCache() {
    Clear();
}

inline void TextureCache::Initialize(uint64 memoryBudget) {
    memory_budget = memoryBudget;
    Clear();
}

inline void TextureCache::AddTexture(const String& key, const Image& texture) {
    CS_LOCK(cs);
    
    auto it = cache.find(key);
    if (it != cache.end()) {
        // Update existing texture
        used_memory -= it->second.memory_usage;
    }
    
    CachedTexture cached(texture);
    cache[key] = cached;
    used_memory += cached.memory_usage;
    
    // Add to access order for LRU
    access_order.push_back(key);
    
    // Enforce budget
    EnforceBudget();
}

inline Image TextureCache::GetTexture(const String& key) const {
    CS_LOCK(cs);
    
    auto it = cache.find(key);
    if (it != cache.end()) {
        // Update access time and order
        it->second.access_time = GetTickCount();
        
        // Move to end of access order (most recently used)
        auto orderIt = std::find(access_order.begin(), access_order.end(), key);
        if (orderIt != access_order.end()) {
            access_order.erase(orderIt);
            access_order.push_back(key);
        }
        
        hits++;
        return it->second.data;
    }
    
    misses++;
    return Image(); // Not found
}

inline bool TextureCache::HasTexture(const String& key) const {
    CS_LOCK(cs);
    return cache.find(key) != cache.end();
}

inline void TextureCache::RemoveTexture(const String& key) {
    CS_LOCK(cs);
    
    auto it = cache.find(key);
    if (it != cache.end()) {
        used_memory -= it->second.memory_usage;
        cache.erase(it);
        
        auto orderIt = std::find(access_order.begin(), access_order.end(), key);
        if (orderIt != access_order.end()) {
            access_order.erase(orderIt);
        }
    }
}

inline void TextureCache::Clear() {
    CS_LOCK(cs);
    cache.clear();
    access_order.clear();
    used_memory = 0;
    hits = 0;
    misses = 0;
}

inline double TextureCache::GetHitRatio() const {
    CS_LOCK(cs);
    int total = hits + misses;
    return total > 0 ? (double)hits / total : 0.0;
}

inline void TextureCache::SetMemoryBudget(uint64 budget) {
    CS_LOCK(cs);
    memory_budget = budget;
    EnforceBudget();
}

inline void TextureCache::EnforceBudget() {
    CS_LOCK(cs);
    
    if (used_memory <= memory_budget) {
        return; // Within budget
    }
    
    // Remove least recently used textures until within budget
    while (used_memory > memory_budget && !access_order.empty()) {
        String key = access_order.front();
        access_order.erase(access_order.begin());
        
        auto it = cache.find(key);
        if (it != cache.end()) {
            used_memory -= it->second.memory_usage;
            cache.erase(it);
        }
    }
}

NAMESPACE_UPP_END

#endif