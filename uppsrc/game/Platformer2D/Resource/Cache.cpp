#include "Cache.h"
#include "Resource.h"
#include "Decompress.h"

NAMESPACE_UPP

// ============================================================================
// ResourceCache Implementation
// ============================================================================

ResourceCache::ResourceCache()
{
	chunkIndex.SetCount(10000); // Reasonable default for chunk count
}

ResourceCache::~ResourceCache()
{
	for (int i = 0; i < cache.GetCount(); ++i)
	{
		CacheEntryImpl* entry = cache[i];
		if (entry) delete entry;
	}
	cache.Clear();
}

CacheEntry* ResourceCache::GetChunk(int chunkNum)
{
	if (chunkNum < 0 || chunkNum >= cache.GetCount()) return nullptr;
	CacheEntryImpl* entry = cache[chunkNum];
	if (entry)
	{
		entry->AddRef();
		return entry;
	}
	
	// Try to load
	return LoadAndCacheChunk(chunkNum);
}

CacheEntry* ResourceCache::LockChunk(int chunkNum)
{
	CacheEntry* entry = GetChunk(chunkNum);
	if (entry)
	{
		CacheEntryImpl* impl = static_cast<CacheEntryImpl*>(entry);
		impl->SetPurgeable(false);
		impl->AddRef(); // Lock adds a reference
	}
	return entry;
}

void ResourceCache::UnlockChunk(int chunkNum)
{
	if (chunkNum >= 0 && chunkNum < cache.GetCount())
	{
		CacheEntryImpl* entry = cache[chunkNum];
		if (entry)
		{
			entry->SetPurgeable(true);
			entry->Release();
		}
	}
}

void ResourceCache::MarkChunk(int chunkNum)
{
	if (chunkNum >= 0 && chunkNum < chunkIndex.GetCount())
		chunkIndex[chunkNum].marked = true;
}

void ResourceCache::ClearMarks()
{
	for (auto& info : chunkIndex)
		info.marked = false;
}

void ResourceCache::CacheMarks(int priority)
{
	currentPriority = priority;
	// Load all marked chunks
	for (int i = 0; i < chunkIndex.GetCount(); ++i)
	{
		if (chunkIndex[i].marked)
			GetChunk(i);
	}
}

void ResourceCache::PurgeUnmarked()
{
	Vector<int> toRemove;
	for (int i = 0; i < cache.GetCount(); ++i)
	{
		CacheEntryImpl* entry = cache[i];
		if (entry && entry->IsPurgeable() && entry->GetRefCount() == 0)
		{
			// Check if marked for current priority
			if (entry->chunkNum < chunkIndex.GetCount() && chunkIndex[entry->chunkNum].marked)
				continue; // Keep marked chunks
			toRemove.Add(entry->chunkNum);
		}
	}
	
	for (int chunkNum : toRemove)
	{
		if (chunkNum >= 0 && chunkNum < cache.GetCount())
		{
			CacheEntryImpl* entry = cache[chunkNum];
			if (entry)
			{
				PurgeEntry(entry);
				cache.Remove(chunkNum);
			}
		}
	}
}

size_t ResourceCache::GetTotalMemory() const
{
	return currentMemory;
}

void ResourceCache::SetMaxMemory(size_t bytes)
{
	maxMemory = bytes;
	// Purge if over limit
	while (currentMemory > maxMemory)
		PurgeUnmarked();
}

void ResourceCache::PurgeEntry(CacheEntry* entry)
{
	if (!entry) return;
	CacheEntryImpl* impl = static_cast<CacheEntryImpl*>(entry);
	currentMemory -= impl->GetSize();
	delete impl;
}

CacheEntryImpl* ResourceCache::LoadAndCacheChunk(int chunkNum)
{
	// TODO: Actually load from GAMEMAPS/EGAGRAPH/AUDIO files
	// For now, return dummy entry
	LOG("ResourceCache::LoadAndCacheChunk: " << chunkNum << " - Not implemented");
	return nullptr;
}

// Global cache instance
IResourceCache* g_resourceCache = GetResourceCache();

END_UPP_NAMESPACE