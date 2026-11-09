#ifndef _SweetSandy_GroundChunkGenerator_h_
#define _SweetSandy_GroundChunkGenerator_h_

// Unified Ground phase, Task 4: the real IGroundGenerator, filling
// content.ground for every chunk - one precise grass mesh per city
// block (Task 3's own GroundMeshBuilder::Build), plus a coarse flat
// fallback for whatever this chunk's own area isn't already covered by
// a block's own grass or (approximately, via road proximity) by
// asphalt - see Manager/2-plan/.../009-unified-ground/
// 004-task4-worldbuilder-integration.md. Fully deterministic given
// (chunk, content.blocks/road_centerlines, terrain) - no RNG/seed
// needed, unlike the other chunk generators.
class GroundChunkGenerator : public IGroundGenerator {
	Terrain& terrain; // owned by whoever constructs this (PlayScreen) - same threading convention every other chunk generator already uses

public:
	explicit GroundChunkGenerator(Terrain& terrain) : terrain(terrain) {}

	void Generate(const WorldChunk& chunk, WorldChunkContent& content) override;
};

#endif
