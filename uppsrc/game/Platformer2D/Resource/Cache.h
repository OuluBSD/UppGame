#ifndef _Platformer2D_Resource_Cache_h_
#define _Platformer2D_Resource_Cache_h_

#include "Resource.h"

NAMESPACE_UPP

// Chunk type for cache indexing
enum class ChunkType : uint16_t
{
	UNKNOWN = 0,
	GRAPHICS = 1,
	AUDIO = 2,
	MAP = 3,
	FONT = 4,
	PALETTE = 5,
	SPRITE = 6,
	TILE8 = 7,
	TILE8M = 8,
	MUSIC = 9,
	SOUND = 10,
};

// ============================================================================
// Cache Entry Implementation
// ============================================================================

class CacheEntryImpl : public CacheEntry
{
public:
	CacheEntryImpl(int chunkNum_, size_t size_, void* data_, bool owned_ = true)
		: chunkNum(chunkNum_), size(size_), data(data_), owned(owned_), purgeable(true), refCount(0) {}
	
	~CacheEntryImpl()
	{
		if (owned && data)
			MemoryFree(data);
	}
	
	size_t GetSize() const override { return size; }
	const void* GetData() const override { return data; }
	void* GetData() override { return data; }
	bool IsPurgeable() const override { return purgeable; }
	void SetPurgeable(bool p) override { purgeable = p; }
	int GetRefCount() const override { return refCount; }
	void AddRef() override { ++refCount; }
	void Release() override { if (--refCount <= 0 && purgeable) g_resourceCache->PurgeEntry(this); }
	
	int chunkNum;
	size_t size;
	void* data;
	bool owned;
	bool purgeable;
	int refCount;
};

// ============================================================================
// Resource Cache Implementation
// ============================================================================

class ResourceCache : public IResourceCache
{
public:
	ResourceCache();
	~ResourceCache();
	
	CacheEntry* GetChunk(int chunkNum) override;
	CacheEntry* LockChunk(int chunkNum) override;
	void UnlockChunk(int chunkNum) override;
	void MarkChunk(int chunkNum) override;
	void ClearMarks() override;
	void CacheMarks(int priority = 0) override;
	void PurgeUnmarked() override;
	size_t GetTotalMemory() const override;
	void SetMaxMemory(size_t bytes) override;
	
	// Internal
	void PurgeEntry(CacheEntry* entry) override;
	CacheEntryImpl* LoadAndCacheChunk(int chunkNum);
	
private:
	struct ChunkInfo
	{
		uint32_t offset = 0;
		uint32_t length = 0;
		uint16_t compressedLength = 0;
		ChunkType type = ChunkType::UNKNOWN;
		bool compressed = false;
		bool marked = false;
	};
	
	Index<CacheEntryImpl*> cache; // chunkNum -> entry
	Vector<ChunkInfo> chunkIndex;      // chunkNum -> info (for GAMEMAPS/EGAGRAPH/AUDIO)
	size_t maxMemory = 4 * 1024 * 1024; // 4MB default
	size_t currentMemory = 0;
	int currentPriority = 0;
};

// Global cache accessor
inline IResourceCache* GetResourceCache()
{
	static ResourceCache instance;
	return static_cast<IResourceCache*>(&instance);
}

END_UPP_NAMESPACE

#endif // _Platformer2D_Resource_Cache_h_