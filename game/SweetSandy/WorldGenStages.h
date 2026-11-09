#ifndef _SweetSandy_WorldGenStages_h_
#define _SweetSandy_WorldGenStages_h_

// World Builder phase, Task 3: pluggable generator-stage interfaces. Only
// the content type and the stages that actually have a caller
// (WorldBuilder) exist here (see AGENTS.md: no unused/speculative code).

// City Layout phase, Task 5 (road_centerlines/blocks) + Road Network
// phase, Task 4 (road_mesh) + Vegetation phase, Task 4 (vegetation):
// real content, filled in by MapGeneratorCityLayoutGenerator, then
// RoadCreatorMeshGenerator, then the real IVegetationGenerator in turn
// (WorldBuilder::Update runs each stage after the other, per chunk).
// world-space road centerlines, city-block polygons, a real
// triangulated road+junction mesh (Road Network Tasks 1-3), and a set
// of placed, already-built trees (Vegetation Tasks 1-3). No separate
// "has_city" flag - "did anything generate here" is just
// !road_centerlines.IsEmpty().
struct WorldChunkContent : Moveable<WorldChunkContent> {
	Vector<Vector<Vec2>> road_centerlines;
	Vector<Vector<Vec2>> blocks;
	RoadMesh road_mesh; // ribbons (Task 1) + junction fans (Task 3) merged into one mesh
	Vector<VegTreeInstance> vegetation; // stored materialized (built mesh), not just placement+seed - see 004-task4-worldbuilder-integration.md
	Vector<BuildingInstance> buildings; // Buildings phase, Task 3, see BuildingInstance.h - stored materialized (built mesh), same reasoning as vegetation above
	Vector<VegMesh> ground; // Unified Ground phase, Task 4: one mesh per block with real grass fill (GroundMeshBuilder::Build), plus whatever BuildFlatGroundGrid fills in for the rest of the chunk - covers every loaded chunk, not just ones with road content

	String Serialize() const;
	static WorldChunkContent Deserialize(const String& data);
};

// Terrain Erosion phase, Task 3: runs *first* in WorldBuilder::Update's
// own per-chunk order, before any stage that calls Terrain::HeightAt
// (that's every stage below except ICityLayoutGenerator itself, which
// has no terrain dependency). Writes nothing to `content` - it exists
// purely for its side effect on the Terrain& it was constructed with
// (ensuring the erosion tile(s) covering this chunk are loaded before
// anything samples HeightAt for it) - see ErosionChunkGenerator.h.
class ITerrainErosionGenerator {
public:
	virtual ~ITerrainErosionGenerator() {}
	virtual void Generate(const WorldChunk& chunk, WorldChunkContent& content) = 0;
};

class ICityLayoutGenerator {
public:
	virtual ~ICityLayoutGenerator() {}
	// Fills in content for one newly-loaded chunk.
	virtual void Generate(const WorldChunk& chunk, WorldChunkContent& content) = 0;
};

// Road Network phase, Task 4: runs after ICityLayoutGenerator for the
// same chunk (needs content.road_centerlines as input) - WorldBuilder::
// Update calls both in sequence before caching the result.
class IRoadMeshGenerator {
public:
	virtual ~IRoadMeshGenerator() {}
	virtual void Generate(const WorldChunk& chunk, WorldChunkContent& content) = 0;
};

// Vegetation phase, Task 4: name reserved since 002-world-builder/
// 001-planning.md. Runs after ICityLayoutGenerator for the same chunk
// (needs content.road_centerlines to avoid placing trees on roads);
// order relative to IRoadMeshGenerator doesn't matter functionally (it
// never reads road_mesh).
class IVegetationGenerator {
public:
	virtual ~IVegetationGenerator() {}
	virtual void Generate(const WorldChunk& chunk, WorldChunkContent& content) = 0;
};

// Buildings phase, Task 3. Runs after ICityLayoutGenerator for the same
// chunk (needs content.blocks/content.road_centerlines); order relative
// to IRoadMeshGenerator/IVegetationGenerator doesn't matter functionally
// (reads neither's own output).
class IBuildingGenerator {
public:
	virtual ~IBuildingGenerator() {}
	virtual void Generate(const WorldChunk& chunk, WorldChunkContent& content) = 0;
};

// Unified Ground phase, Task 4. Runs after ICityLayoutGenerator/
// IBuildingGenerator for the same chunk (needs content.blocks); order
// relative to IRoadMeshGenerator/IVegetationGenerator doesn't matter
// functionally (reads neither's own output).
class IGroundGenerator {
public:
	virtual ~IGroundGenerator() {}
	virtual void Generate(const WorldChunk& chunk, WorldChunkContent& content) = 0;
};

#endif
