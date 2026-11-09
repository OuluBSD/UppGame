#include "SweetSandy.h"

NAMESPACE_UPP

static AppRegistry& ErosionTileCacheRegistry()
{
	static AppRegistry reg;
	static bool init = false;
	if(!init) {
		reg.Vendor("AiUpp").AppId("SweetSandy").Profile("default");
		init = true;
	}
	return reg;
}

void ErosionTileCache::Open(const String& seed_version)
{
	String root = AppendFileName(AppendFileName(ErosionTileCacheRegistry().GetCacheDir(), "erosion"), seed_version);
	cache.Open(root);
}

String ErosionTileCache::KeyFor(const ErosionTileCoord& c)
{
	return Format("%d_%d.bin", c.x, c.z);
}

bool ErosionTileCache::Load(const ErosionTileCoord& c, ErosionTile& out) const
{
	String data;
	if(!cache.Load(KeyFor(c), data))
		return false;
	out = ErosionTile::Deserialize(data);
	return true;
}

void ErosionTileCache::Save(const ErosionTileCoord& c, const ErosionTile& tile)
{
	cache.Save(KeyFor(c), tile.Serialize());
}

END_UPP_NAMESPACE
