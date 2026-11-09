#ifndef _SweetSandy_ErosionTile_h_
#define _SweetSandy_ErosionTile_h_

// Terrain Erosion phase, Task 1: a finite, baked heightmap tile - the
// thing Task 2's droplet simulation actually mutates (it needs a real
// grid to operate on, unlike IslandTerrain::HeightAt's own pure,
// stateless function - see Manager/2-plan/.../011-terrain-erosion/
// 001-planning.md for the full reasoning).

struct ErosionTileCoord : Moveable<ErosionTileCoord> {
	int x = 0, z = 0; // tile index - world origin = (x,z) * kErosionTileSize

	bool operator==(const ErosionTileCoord& o) const { return x == o.x && z == o.z; }
};

inline hash_t GetHashValue(const ErosionTileCoord& c) { return CombineHash(c.x, c.z); }

const double kErosionTileSize = 2048.0; // meters, square
const int    kErosionTileGridRes = 257; // samples per axis - 8m/cell

// Which tile a world point falls in, and that tile's own world-space
// min corner (Vec2.x = world x, Vec2.y = world z - same convention
// every other Vec2 world-position in this project already uses).
ErosionTileCoord ErosionTileCoordFor(Vec2 world_pos);
Vec2 ErosionTileOrigin(ErosionTileCoord coord);

struct ErosionTile : Moveable<ErosionTile> {
	ErosionTileCoord coord;
	Vector<float> height; // kErosionTileGridRes^2, row-major (z*res+x) -
	                       // same convention Terrain::height and
	                       // PlayScreen's own ground_colliders sampling
	                       // already use

	// Bilinear, clamped to this tile's own bounds - same shape as
	// Terrain::HeightAt's own grid sampling, just over an external grid
	// instead of Terrain's own private member.
	double HeightAt(double world_x, double world_z) const;

	String Serialize() const;
	static ErosionTile Deserialize(const String& data);
};

// Materializes IslandTerrain::HeightAt into a fresh grid for this tile
// coordinate - no erosion applied (Task 2's own job).
ErosionTile BakeErosionTile(const IslandTerrain& island, ErosionTileCoord coord);

#endif
