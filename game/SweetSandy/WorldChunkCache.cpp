#include "SweetSandy.h"

NAMESPACE_UPP

static AppRegistry& WorldChunkCacheRegistry()
{
	static AppRegistry reg;
	static bool init = false;
	if(!init) {
		reg.Vendor("AiUpp").AppId("SweetSandy").Profile("default");
		init = true;
	}
	return reg;
}

void WorldChunkCache::Open(const String& seed_version)
{
	String root = AppendFileName(AppendFileName(WorldChunkCacheRegistry().GetCacheDir(), "world"), seed_version);
	cache.Open(root);
}

String WorldChunkCache::KeyFor(const WorldChunkCoord& c)
{
	return AppendFileName(Format("%d", c.lod), Format("%d_%d_%d.bin", c.x, c.y, c.z));
}

bool WorldChunkCache::Load(const WorldChunkCoord& c, String& data_out) const
{
	return cache.Load(KeyFor(c), data_out);
}

void WorldChunkCache::Save(const WorldChunkCoord& c, const String& data)
{
	cache.Save(KeyFor(c), data);
}

void WorldChunkCache::Invalidate(const WorldChunkCoord& c)
{
	cache.Delete(KeyFor(c));
}

END_UPP_NAMESPACE
