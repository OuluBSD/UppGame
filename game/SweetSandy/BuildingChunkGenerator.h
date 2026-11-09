#ifndef _SweetSandy_BuildingChunkGenerator_h_
#define _SweetSandy_BuildingChunkGenerator_h_

// Buildings phase, Task 3: the real IBuildingGenerator, deciding where
// buildings go and building them (Tasks 1-2) for every chunk - see
// Manager/2-plan/.../008-buildings/003-task3-worldbuilder-integration.md.
// Runs after ICityLayoutGenerator for the same chunk (needs
// content.blocks/content.road_centerlines, already filled in by then -
// see WorldBuilder::Update).
class BuildingChunkGenerator : public IBuildingGenerator {
	String world_seed;
	Terrain& terrain; // owned by whoever constructs this (PlayScreen) - same threading convention RoadCreatorMeshGenerator/VegChunkGenerator already use

public:
	BuildingChunkGenerator(const String& world_seed, Terrain& terrain) : world_seed(world_seed), terrain(terrain) {}

	void Generate(const WorldChunk& chunk, WorldChunkContent& content) override;
};

#endif
