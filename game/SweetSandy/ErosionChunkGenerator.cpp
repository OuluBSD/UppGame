#include "SweetSandy.h"

NAMESPACE_UPP

void ErosionChunkGenerator::Generate(const WorldChunk& chunk, WorldChunkContent& content)
{
	if(!terrain.IsIsland())
		return;
	terrain.EnsureErosionCoverage(Vec2(chunk.center.x, chunk.center.z), Vec2(chunk.half_extent.x, chunk.half_extent.z));
}

END_UPP_NAMESPACE
