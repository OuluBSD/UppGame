#include "SweetSandy.h"

NAMESPACE_UPP

IslandTerrain::IslandTerrain(dword seed, const IslandTerrainParams& params)
	: noise(seed), params(params)
{
}

double IslandTerrain::FalloffAt(double world_x, double world_z) const
{
	double nx = world_x / params.island_radius;
	double nz = world_z / params.island_radius;
	double t = max(fabs(nx), fabs(nz));
	return 1.0 - t;
}

double IslandTerrain::HeightAt(double world_x, double world_z) const
{
	double base = OctavedSimplex(noise, world_x, world_z, params.octaves,
	                              params.base_frequency, params.lacunarity, params.persistence);
	double ridge = OctavedRidge(noise, world_x, world_z, params.octaves,
	                             params.base_frequency, params.lacunarity, params.persistence);
	double combined = (base + ridge) * 0.5;

	double h = combined * params.height_scale * FalloffAt(world_x, world_z);
	return Clampd(h, params.water_level, params.max_height);
}

END_UPP_NAMESPACE
