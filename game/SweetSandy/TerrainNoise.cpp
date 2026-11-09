#include "SweetSandy.h"
#include <Core/MathNumeric/MathNumeric.h>

NAMESPACE_UPP

namespace {

const double kF2 = 0.5 * (sqrt(3.0) - 1.0);
const double kG2 = (3.0 - sqrt(3.0)) / 6.0;

// 8 gradient directions for 2D simplex noise - standard Gustavson set.
const int kGrad2[8][2] = {
	{1, 1}, {-1, 1}, {1, -1}, {-1, -1},
	{1, 0}, {-1, 0}, {0, 1}, {0, -1},
};

} // anonymous namespace

SimplexNoise2D::SimplexNoise2D(dword seed)
{
	byte p[256];
	for(int i = 0; i < 256; i++)
		p[i] = (byte)i;

	// A genuine Fisher-Yates shuffle, not Core's own
	// RandomNumberGenerator::Permutation (that one fills each slot with
	// an independent Random(num) draw - can repeat/omit values, which
	// would bias gradient selection here; simplex noise needs a real
	// bijection over 0..255).
	RandomNumberGenerator rng;
	rng.Seed(seed);
	for(int i = 255; i > 0; i--) {
		int j = (int)rng.Random(i + 1);
		byte t = p[i]; p[i] = p[j]; p[j] = t;
	}

	// Duplicated so perm[ii + perm[jj]] (ii, jj up to 255, perm[..] up
	// to 255, i1/j1 up to 1) never needs a wrap-around mask beyond a
	// plain array index - every index used in Sample() stays < 512.
	for(int i = 0; i < 512; i++)
		perm[i] = p[i & 255];
}

double SimplexNoise2D::Sample(double xin, double yin) const
{
	double s = (xin + yin) * kF2;
	int i = (int)floor(xin + s);
	int j = (int)floor(yin + s);
	double t = (i + j) * kG2;
	double X0 = i - t;
	double Y0 = j - t;
	double x0 = xin - X0;
	double y0 = yin - Y0;

	int i1, j1;
	if(x0 > y0) { i1 = 1; j1 = 0; }
	else        { i1 = 0; j1 = 1; }

	double x1 = x0 - i1 + kG2;
	double y1 = y0 - j1 + kG2;
	double x2 = x0 - 1.0 + 2.0 * kG2;
	double y2 = y0 - 1.0 + 2.0 * kG2;

	int ii = i & 255;
	int jj = j & 255;
	int gi0 = perm[ii + perm[jj]] & 7;
	int gi1 = perm[ii + i1 + perm[jj + j1]] & 7;
	int gi2 = perm[ii + 1 + perm[jj + 1]] & 7;

	auto Corner = [](double x, double y, int gi) {
		double t = 0.5 - x * x - y * y;
		if(t < 0)
			return 0.0;
		t *= t;
		return t * t * (kGrad2[gi][0] * x + kGrad2[gi][1] * y);
	};

	double n0 = Corner(x0, y0, gi0);
	double n1 = Corner(x1, y1, gi1);
	double n2 = Corner(x2, y2, gi2);

	// Standard normalization constant for this formulation - keeps the
	// result within roughly [-1,1] (verified empirically in the
	// self-test, not just asserted from theory).
	return 70.0 * (n0 + n1 + n2);
}

double OctavedSimplex(const SimplexNoise2D& noise, double x, double y,
                       int octaves, double base_frequency, double lacunarity, double persistence)
{
	double freq = base_frequency;
	double amplitude = 1.0;
	double sum = 0.0;
	for(int o = 0; o < octaves; o++) {
		sum += noise.Sample(x * freq, y * freq) * amplitude;
		freq *= lacunarity;
		amplitude *= persistence;
	}
	return sum;
}

double OctavedRidge(const SimplexNoise2D& noise, double x, double y,
                     int octaves, double base_frequency, double lacunarity, double persistence)
{
	double freq = base_frequency;
	double amplitude = 1.0;
	double sum = 0.0;
	double weight = 1.0;
	for(int o = 0; o < octaves; o++) {
		double v = 1.0 - fabs(noise.Sample(x * freq, y * freq));
		v *= v;
		v *= weight;
		weight = Clampd(v, 0.0, 1.0);
		sum += v * amplitude;
		freq *= lacunarity;
		amplitude *= persistence;
	}
	return sum;
}

END_UPP_NAMESPACE
