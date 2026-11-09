#ifndef UPP_AUDIO_CACHE_MANAGER_H
#define UPP_AUDIO_CACHE_MANAGER_H

#include <Core/Core.h>
#include <GameLib/GameLib.h>
#include <GameEngine/VFS.h>
#include <GameEngine/AudioSystem.h>
#include <plugin/wav/Wav.h>
#include <Thread/Thread.h>

NAMESPACE_UPP

// Audio cache entry
struct AudioCacheEntry {
    SoundData data;
    Time lastAccessed;
    int accessCount;
    
    AudioCacheEntry() : accessCount(0) {}
    AudioCacheEntry(const SoundData& data) : data(data), lastAccessed(GetSysTime()), accessCount(1) {}
};

// Audio cache manager for preloading and caching audio files
class AudioCacheManager {
public:
    AudioCacheManager();
    ~AudioCacheManager();
    
    // Initialize the cache manager
    void Initialize(size_t maxCacheSizeMB = 50); // Default 50MB cache
    
    // Preload an audio file by path
    bool Preload(const String& path, std::shared_ptr<VFS> vfs = nullptr);
    
    // Preload multiple audio files
    void Preload(const Vector<String>& paths, std::shared_ptr<VFS> vfs = nullptr);
    
    // Get cached audio data
    SoundData Get(const String& path);
    
    // Check if audio is cached
    bool IsCached(const String& path) const;
    
    // Remove from cache
    void Remove(const String& path);
    
    // Clear entire cache
    void Clear();
    
    // Get cache statistics
    size_t GetCacheSizeBytes() const;
    size_t GetCacheSizeMB() const { return GetCacheSizeBytes() / (1024 * 1024); }
    int GetCachedCount() const { return cache.GetCount(); }
    
    // Set cache size limit (in MB)
    void SetMaxCacheSize(size_t maxMB) { maxCacheSize = maxMB * 1024 * 1024; }
    size_t GetMaxCacheSize() const { return maxCacheSize; }
    
    // Enable/disable caching
    void SetEnabled(bool enabled) { this->enabled = enabled; }
    bool IsEnabled() const { return enabled; }
    
    // Prune cache based on LRU algorithm when it exceeds max size
    void PruneCache();
    
    // Update method to be called regularly (handles background loading, etc.)
    void Update();
    
private:
    HashMap<String, AudioCacheEntry> cache;
    size_t maxCacheSize; // in bytes
    bool enabled;
    
    // Background loading thread
    Thread backgroundThread;
    bool backgroundThreadRunning;
    Vector<String> preloadQueue;
    CriticalSection queueCS;
    
    // Background loading function
    void BackgroundLoader();
};

// Audio preloader for loading audio in background
class AudioPreloader {
public:
    AudioPreloader(std::shared_ptr<AudioCacheManager> cacheManager);
    ~AudioPreloader();
    
    // Queue an audio file for preloading
    void QueuePreload(const String& path, std::shared_ptr<VFS> vfs = nullptr);
    
    // Queue multiple audio files
    void QueuePreload(const Vector<String>& paths, std::shared_ptr<VFS> vfs = nullptr);
    
    // Start preloading queued files
    void StartPreloading();
    
    // Check if preloading is in progress
    bool IsPreloading() const { return preloading; }
    
    // Get progress (0.0 to 1.0)
    double GetProgress() const;
    
    // Cancel preloading
    void CancelPreloading();
    
private:
    std::shared_ptr<AudioCacheManager> cacheManager;
    Vector<String> preloadQueue;
    int preloadCount;
    int completedCount;
    bool preloading;
    bool cancelled;
    
    CriticalSection cs;
    Thread preloadThread;
    
    void PreloadThread();
};

END_UPP_NAMESPACE

#endif