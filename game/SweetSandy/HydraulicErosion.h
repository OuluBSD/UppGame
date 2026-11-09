#ifndef _SweetSandy_HydraulicErosion_h_
#define _SweetSandy_HydraulicErosion_h_

// Terrain Erosion phase, Task 2: droplet-based hydraulic erosion,
// ported (algorithm and structure, not just referenced) from
// SebLague/Hydraulic-Erosion's own Erosion.cs (MIT) - unlike the
// Island Terrain phase's own reference repo, that one's simulation
// core is plain, dependency-free C#, confirmed by reading the whole
// file. See Manager/2-plan/.../011-terrain-erosion/
// 002-task2-erosion-algorithm.md for the full port notes.
struct HydraulicErosionParams {
	int    erosion_radius = 3;
	double inertia = 0.05;                 // 0 = instantly follow the gradient, 1 = never turn
	double sediment_capacity_factor = 4.0;
	double min_sediment_capacity = 0.01;
	double erode_speed = 0.3;
	double deposit_speed = 0.3;
	double evaporate_speed = 0.01;
	double gravity = 4.0;
	int    max_droplet_lifetime = 30;
	double initial_water_volume = 1.0;
	double initial_speed = 1.0;
};

// Runs num_droplets independent droplet simulations over map (map_size x
// map_size, row-major z*map_size+x), mutating it in place. Deterministic
// for a given seed.
void ErodeHeightmap(Vector<float>& map, int map_size, int num_droplets,
                     const HydraulicErosionParams& params, dword seed);

// Simulates exactly one droplet starting at a caller-chosen (start_x,
// start_y) instead of a seeded random point - exposed so a hand-built
// test can verify the erosion/deposition direction on a known slope
// deterministically (ErodeHeightmap's own random spawn point can't be
// pinned down for a single-droplet correctness check). ErodeHeightmap
// itself is just this, called num_droplets times with random starts.
void SimulateOneDroplet(Vector<float>& map, int map_size, double start_x, double start_y,
                         const HydraulicErosionParams& params);

// Exposed for direct testing of the erosion brush's own weight
// normalization - not used by real callers (ErodeHeightmap/
// SimulateOneDroplet compute their own brush internally, for every
// cell, once per call).
void ComputeErosionBrushForCell(int map_size, int radius, int centre_x, int centre_y,
                                 Vector<int>& indices_out, Vector<double>& weights_out);

#endif
