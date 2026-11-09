#ifndef _SweetSandy_IslandTerrain_h_
#define _SweetSandy_IslandTerrain_h_

// Island Terrain phase, Task 2: a stateless height function over the
// whole island - no baked grid, no storage, safely evaluable at any
// (world_x, world_z) - see Manager/2-plan/.../010-island-terrain/
// 002-task2-island-height-function.md for the full composition
// reasoning, including why this can't reuse the reference repo's own
// dynamic min/max normalization (that needs a finite, pre-scanned
// grid; this needs to answer a single point query with no context).
struct IslandTerrainParams {
	int    octaves = 5;
	double base_frequency = 1.0 / 8000.0; // ~8km base feature size
	double lacunarity = 2.0;
	double persistence = 0.5;
	double height_scale = 150.0;    // meters
	double island_radius = 50000.0; // meters - half of a 100km island
	double water_level = 0.0;
	double max_height = 400.0;
};

class IslandTerrain {
	SimplexNoise2D noise; // shared source for both the plain and ridge
	                      // octave sums - same as the reference, which
	                      // differs only in post-processing, not the
	                      // underlying noise function
	IslandTerrainParams params;

public:
	IslandTerrain(dword seed, const IslandTerrainParams& params = IslandTerrainParams());

	// Chebyshev-distance falloff (island_radius away from center -> 0,
	// linearly continuing negative beyond it - see the task doc for why
	// this is the reference's own general power-falloff formula reduced
	// to a plain 1-t at its own default parameters). Exposed separately
	// from HeightAt so it's directly hand-verifiable in isolation.
	double FalloffAt(double world_x, double world_z) const;

	double HeightAt(double world_x, double world_z) const;

	double GetWaterLevel() const { return params.water_level; }
};

#endif
