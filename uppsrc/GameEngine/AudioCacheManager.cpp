#include "AudioCacheManager.h"

NAMESPACE_UPP

AudioCacheManager::AudioCacheManager() 
    : maxCacheSize(50 * 1024 * 1024) // 50MB default
    , enabled(true)
    , backgroundThreadRunning(false) {
}

AudioCacheManager::~AudioCacheManager() {
    backgroundThreadRunning = false;
    if (backgroundThread.IsOpen()) {
        backgroundThread.Wait();
    }
}

void AudioCacheManager::Initialize(size_t maxCacheSizeMB) {
    maxCacheSize = maxCacheSizeMB * 1024 * 1024;
    backgroundThreadRunning = true;
    backgroundThread.Run([this] { BackgroundLoader(); });
}

bool AudioCacheManager::Preload(const String& path, std::shared_ptr<VFS> vfs) {
    if (!enabled) return false;
    
    // Check if already cached
    if (cache.Find(path) >= 0) {
        // Update access info
        auto& entry = cache.GetAdd(path);
        entry.lastAccessed = GetSysTime();
        entry.accessCount++;
        return true;
    }
    
    SoundData soundData;
    bool loaded = false;
    
    if (vfs) {
        // Load from VFS
        soundData = vfs->LoadSound(path);
        loaded = soundData.samples.GetCount() > 0;
    } else {
        // Load from regular file system
        WAVDecoder decoder;
        loaded = decoder.LoadFile(path, soundData);
    }
    
    if (loaded) {
        // Add to cache
        cache.GetAdd(path) = AudioCacheEntry(soundData);
        
        // Prune if necessary
        PruneCache();
        return true;
    }
    
    return false;
}

void AudioCacheManager::Preload(const Vector<String>& paths, std::shared_ptr<VFS> vfs) {
    for (const auto& path : paths) {
        Preload(path, vfs);
    }
}

SoundData AudioCacheManager::Get(const String& path) {
    if (!enabled) return SoundData();
    
    int idx = cache.Find(path);
    if (idx >= 0) {
        // Update access info
        auto& entry = cache.GetAdd(path);
        entry.lastAccessed = GetSysTime();
        entry.accessCount++;
        return entry.data;
    }
    
    // If not cached, load it (but don't cache if it exceeds limits)
    SoundData soundData;
    if (std::shared_ptr<VFS> vfs = nullptr) { // This is a placeholder - in real implementation, you'd want to pass VFS properly
        // For now, try direct file access
        WAVDecoder decoder;
        decoder.LoadFile(path, soundData);
    }
    
    return soundData;
}

bool AudioCacheManager::IsCached(const String& path) const {
    return cache.Find(path) >= 0;
}

void AudioCacheManager::Remove(const String& path) {
    cache.RemoveKey(path);
}

void AudioCacheManager::Clear() {
    cache.Clear();
}

size_t AudioCacheManager::GetCacheSizeBytes() const {
    size_t total = 0;
    for (const auto& pair : cache) {
        // Calculate approximate memory usage of the SoundData
        total += pair.value.data.samples.GetCount() * sizeof(int16); // 16-bit samples
        total += sizeof(pair.value); // overhead of cache entry
    }
    return total;
}

void AudioCacheManager::PruneCache() {
    if (GetCacheSizeBytes() <= maxCacheSize) return;
    
    // Create a list of entries with their access info
    struct CacheEntryInfo {
        String path;
        Time lastAccessed;
        int accessCount;
    };
    
    Vector<CacheEntryInfo> entries;
    for (const auto& pair : cache) {
        CacheEntryInfo info;
        info.path = pair.key;
        info.lastAccessed = pair.value.lastAccessed;
        info.accessCount = pair.value.accessCount;
        entries.Add(info);
    }
    
    // Sort by access criteria (LRU: least recently used first)
    entries.Sort([](const CacheEntryInfo& a, const CacheEntryInfo& b) {
        return a.lastAccessed < b.lastAccessed; // Oldest first
    });
    
    // Remove entries until we're under the limit
    for (const auto& entry : entries) {
        if (GetCacheSizeBytes() <= maxCacheSize) break;
        
        cache.RemoveKey(entry.path);
    }
}

void AudioCacheManager::Update() {
    // Process any items in the preload queue
    Vector<String> itemsToProcess;
    {
        Mutex::Lock lock(queueCS);
        itemsToProcess <<= preloadQueue;
        preloadQueue.Clear();
    }
    
    for (const auto& path : itemsToProcess) {
        Preload(path); // Add to cache
    }
}

void AudioCacheManager::BackgroundLoader() {
    while (backgroundThreadRunning) {
        Sleep(100); // Sleep a bit to avoid busy waiting
        
        Update(); // Process any queued preload operations
    }
}

// AudioPreloader implementation
AudioPreloader::AudioPreloader(std::shared_ptr<AudioCacheManager> cacheManager)
    : cacheManager(cacheManager)
    , preloadCount(0)
    , completedCount(0)
    , preloading(false)
    , cancelled(false) {
}

AudioPreloader::~AudioPreloader() {
    CancelPreloading();
    if (preloadThread.IsOpen()) {
        preloadThread.Wait();
    }
}

void AudioPreloader::QueuePreload(const String& path, std::shared_ptr<VFS> vfs) {
    Mutex::Lock lock(cs);
    preloadQueue.Add(path);
}

void AudioPreloader::QueuePreload(const Vector<String>& paths, std::shared_ptr<VFS> vfs) {
    Mutex::Lock lock(cs);
    for (const auto& path : paths) {
        preloadQueue.Add(path);
    }
}

void AudioPreloader::StartPreloading() {
    Mutex::Lock lock(cs);
    if (preloadQueue.GetCount() == 0 || preloading) return;
    
    preloadCount = preloadQueue.GetCount();
    completedCount = 0;
    cancelled = false;
    preloading = true;
    
    preloadThread.Run([this] { PreloadThread(); });
}

double AudioPreloader::GetProgress() const {
    if (preloadCount == 0) return 0.0;
    return (double)completedCount / preloadCount;
}

void AudioPreloader::CancelPreloading() {
    Mutex::Lock lock(cs);
    cancelled = true;
}

void AudioPreloader::PreloadThread() {
    Vector<String> queueCopy;
    {
        Mutex::Lock lock(cs);
        queueCopy <<= preloadQueue;
    }
    
    for (int i = 0; i < queueCopy.GetCount() && !cancelled; i++) {
        if (cacheManager) {
            cacheManager->Preload(queueCopy[i]);
        }
        
        Mutex::Lock lock(cs);
        completedCount++;
    }
    
    Mutex::Lock lock(cs);
    preloading = false;
    preloadQueue.Clear();
}

END_UPP_NAMESPACE