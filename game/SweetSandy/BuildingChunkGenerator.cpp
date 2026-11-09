#include "SweetSandy.h"

NAMESPACE_UPP

namespace {

// Deterministic per (world_seed, chunk coord), same idea as
// MapGeneratorCityLayoutGenerator's/VegChunkGenerator's own per-chunk
// seeds - salted distinctly ("bld") so this generator's own RNG stream
// doesn't start at the exact same point another generator already
// reseeded the shared RNG to for this chunk.
dword DeterministicBuildingSeed(const String& world_seed, const WorldChunkCoord& coord)
{
	hash_t h = CombineHash(world_seed.GetHashValue(), coord.lod, coord.x, coord.y);
	h = CombineHash(h, coord.z);
	h = CombineHash(h, 'b', 'l', 'd'); // "building", trimmed to 3 chars - CombineHash tops out at 4 total args
	return (dword)h;
}

} // anonymous namespace

void BuildingChunkGenerator::Generate(const WorldChunk& chunk, WorldChunkContent& content)
{
	dword chunk_seed = DeterministicBuildingSeed(world_seed, chunk.coord);

	for(int block_index = 0; block_index < content.blocks.GetCount(); block_index++) {
		const Vector<Vec2>& block = content.blocks[block_index];
		// Per-block seed, not the shared chunk_seed directly - a chunk
		// can hold several blocks, and reusing one seed across all of
		// them would correlate their own jitter patterns (e.g. every
		// block's own first candidate landing at the identical relative
		// offset from its own grid cell).
		dword block_seed = (dword)CombineHash(chunk_seed, block_index);

		Vector<PlannedBuilding> planned = PlanBuildingsForBlock(block, content.road_centerlines, block_seed);
		for(const PlannedBuilding& p : planned) {
			double ground_y = terrain.HeightAt(p.center.x, p.center.y);

			BuildingInstance b;
			b.position = Vec3(p.center.x, ground_y, p.center.y);
			b.yaw = p.yaw;
			b.width = p.width;
			b.depth = p.depth;
			b.height = p.height;
			b.wall_tint = p.wall_tint;
			b.roof_tint = p.roof_tint;
			BuildingMeshBuilder::Build(p, 0.0, b.walls, b.roof); // local space (ground_y=0 here) - world position/yaw applied at draw time (PlayScreen)
			content.buildings.Add(pick(b));
		}
	}
}

END_UPP_NAMESPACE
