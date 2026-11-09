#ifndef _SweetSandy_TerrainNoise_h_
#define _SweetSandy_TerrainNoise_h_

// Island Terrain phase, Task 1: seeded 2D simplex noise plus a fractal
// (octaved) sum and its "ridge" variant on top - the math foundation
// the island height function (Task 2) composes with a falloff map.
// Ported (algorithm, not code - the reference is a Blender add-on/DX11
// compute shader, neither portable as-is) from `TLabAltoh/
// realistic_terrain`'s own `noise.py` - see Manager/2-plan/.../
// 010-island-terrain/001-task1-noise-foundation.md for the full
// reasoning, including why this port uses standard frequency/amplitude
// naming instead of the reference's own (swapped) one.
class SimplexNoise2D {
	byte perm[512];

public:
	explicit SimplexNoise2D(dword seed);

	// Standard 2D simplex noise (Gustavson's algorithm) - deterministic
	// for a given seed, empirically bounded within roughly [-1,1].
	double Sample(double x, double y) const;
};

// Fractal sum: sum of noise.Sample(x*freq, y*freq)*amplitude across
// octaves, with freq *= lacunarity and amplitude *= persistence each
// step (standard terminology - frequency rises, amplitude falls).
// Output is NOT normalized to [0,1] - callers own that, since Task 2's
// island composition needs the raw signed sum before remapping.
double OctavedSimplex(const SimplexNoise2D& noise, double x, double y,
                       int octaves, double base_frequency, double lacunarity, double persistence);

// "Ridge" variant: each octave contributes (1-|sample|)^2, weighted by
// the *previous* octave's own (clamped [0,1]) contribution before being
// summed - this weighting (not just a plain (1-|n|)^2 sum) is what
// actually produces sharp ridgelines instead of smooth hills, per the
// reference's own octaved_ridge_noise. The reference also exposes a
// tunable weight-scale multiplier (default 1.0, effectively a no-op at
// its default) - fixed at 1.0 here for simplicity; revisit if a later
// task needs to tune ridge sharpness independently of octave count.
double OctavedRidge(const SimplexNoise2D& noise, double x, double y,
                     int octaves, double base_frequency, double lacunarity, double persistence);

#endif
