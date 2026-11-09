#include "SweetSandy.h"

NAMESPACE_UPP

ErosionTileCoord ErosionTileCoordFor(Vec2 world_pos)
{
	ErosionTileCoord c;
	c.x = (int)floor(world_pos.x / kErosionTileSize);
	c.z = (int)floor(world_pos.y / kErosionTileSize);
	return c;
}

Vec2 ErosionTileOrigin(ErosionTileCoord coord)
{
	return Vec2(coord.x * kErosionTileSize, coord.z * kErosionTileSize);
}

double ErosionTile::HeightAt(double world_x, double world_z) const
{
	Vec2 origin = ErosionTileOrigin(coord);
	double cell_size = kErosionTileSize / (kErosionTileGridRes - 1);
	double gx = (world_x - origin.x) / cell_size;
	double gz = (world_z - origin.y) / cell_size;
	int ix0 = Clampi((int)floor(gx), 0, kErosionTileGridRes - 2);
	int iz0 = Clampi((int)floor(gz), 0, kErosionTileGridRes - 2);
	double fx = Clampd(gx - ix0, 0.0, 1.0);
	double fz = Clampd(gz - iz0, 0.0, 1.0);

	auto At = [this](int x, int z) { return (double)height[z * kErosionTileGridRes + x]; };
	double h00 = At(ix0, iz0),     h10 = At(ix0 + 1, iz0);
	double h01 = At(ix0, iz0 + 1), h11 = At(ix0 + 1, iz0 + 1);
	double h0 = h00 * (1 - fx) + h10 * fx;
	double h1 = h01 * (1 - fx) + h11 * fx;
	return h0 * (1 - fz) + h1 * fz;
}

String ErosionTile::Serialize() const
{
	ErosionTile& self = const_cast<ErosionTile&>(*this);
	StringStream ss;
	ss.Create();
	ss % self.coord.x % self.coord.z % self.height;
	return ss.GetResult();
}

ErosionTile ErosionTile::Deserialize(const String& data)
{
	ErosionTile t;
	StringStream ss;
	ss.Open(data);
	ss % t.coord.x % t.coord.z % t.height;
	return t;
}

ErosionTile BakeErosionTile(const IslandTerrain& island, ErosionTileCoord coord)
{
	ErosionTile tile;
	tile.coord = coord;
	tile.height.SetCount(kErosionTileGridRes * kErosionTileGridRes);
	Vec2 origin = ErosionTileOrigin(coord);
	double cell_size = kErosionTileSize / (kErosionTileGridRes - 1);
	for(int gz = 0; gz < kErosionTileGridRes; gz++)
		for(int gx = 0; gx < kErosionTileGridRes; gx++) {
			double wx = origin.x + gx * cell_size;
			double wz = origin.y + gz * cell_size;
			tile.height[gz * kErosionTileGridRes + gx] = (float)island.HeightAt(wx, wz);
		}
	return tile;
}

END_UPP_NAMESPACE
