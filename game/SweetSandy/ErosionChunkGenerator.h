#ifndef _SweetSandy_ErosionChunkGenerator_h_
#define _SweetSandy_ErosionChunkGenerator_h_

// Terrain Erosion phase, Task 3: runs first in WorldBuilder::Update's
// own per-chunk order - ensures every erosion tile overlapping this
// chunk is loaded (Terrain::EnsureErosionCoverage) before any later
// stage samples Terrain::HeightAt for it. A no-op when the terrain
// isn't in island mode (the small hand-authored tracks never need
// this).
class ErosionChunkGenerator : public ITerrainErosionGenerator {
	Terrain& terrain;

public:
	explicit ErosionChunkGenerator(Terrain& terrain) : terrain(terrain) {}

	void Generate(const WorldChunk& chunk, WorldChunkContent& content) override;
};

#endif
