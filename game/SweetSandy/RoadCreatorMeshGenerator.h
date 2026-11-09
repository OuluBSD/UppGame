#ifndef _SweetSandy_RoadCreatorMeshGenerator_h_
#define _SweetSandy_RoadCreatorMeshGenerator_h_

// Road Network phase, Task 4: the real IRoadMeshGenerator, wiring Tasks
// 1-3 together per chunk - see Manager/2-plan/.../004-road-network/
// 004-task4-worldbuilder-integration.md. Runs after ICityLayoutGenerator
// for the same chunk (needs content.road_centerlines as input, already
// filled in by then - see WorldBuilder::Update).
class RoadCreatorMeshGenerator : public IRoadMeshGenerator {
	Terrain& terrain;

public:
	// terrain is the specific Terrain instance whose heightmap gets
	// flattened under every generated road (Task 2) - owned by whoever
	// constructs this (PlayScreen), not by WorldBuilder itself, which
	// stays terrain-agnostic (works with generic WorldChunk/
	// WorldChunkContent data only).
	explicit RoadCreatorMeshGenerator(Terrain& terrain) : terrain(terrain) {}

	void Generate(const WorldChunk& chunk, WorldChunkContent& content) override;
};

#endif
