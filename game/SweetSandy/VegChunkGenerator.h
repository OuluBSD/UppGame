#ifndef _SweetSandy_VegChunkGenerator_h_
#define _SweetSandy_VegChunkGenerator_h_

// Vegetation phase, Task 4: the real IVegetationGenerator, deciding
// where trees go and building them (Tasks 1-3) for every chunk - see
// Manager/2-plan/.../005-vegetation/004-task4-worldbuilder-
// integration.md. Runs after ICityLayoutGenerator for the same chunk
// (needs content.road_centerlines, already filled in by then - see
// WorldBuilder::Update).
class VegChunkGenerator : public IVegetationGenerator {
	String world_seed;
	Terrain& terrain; // owned by whoever constructs this (PlayScreen) - same threading pattern RoadCreatorMeshGenerator already established, WorldBuilder itself stays terrain-agnostic

public:
	VegChunkGenerator(const String& world_seed, Terrain& terrain) : world_seed(world_seed), terrain(terrain) {}

	void Generate(const WorldChunk& chunk, WorldChunkContent& content) override;
};

#endif
