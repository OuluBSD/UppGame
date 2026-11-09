#ifndef _SweetSandy_CitySeparationGrid_h_
#define _SweetSandy_CitySeparationGrid_h_

// City Layout phase, Task 2: uniform-grid spatial hash used to enforce a
// minimum separation distance between streamline seed/sample points, and
// to look up nearby points when joining dangling streamline ends. Port
// of MapGenerator's GridStorage (src/ts/impl/grid_storage.ts) - see
// Manager/2-plan/.../003-city-layout/002-task2-streamlines.md.
//
// Simplification from the source: `vectorFarFromVectors` skips a sample
// only if it's the exact same JS object as the query point (reference
// identity) - not reproducible in C++ for by-value Vec2s, and not
// needed here since every caller queries with a point that isn't itself
// already stored in the grid yet (a fresh candidate seed, or a fresh
// streamline sample about to be added). Every other quirk (out-of-bounds
// samples silently landing in grid cell (0,0) rather than being
// rejected, matching the source's commented-out error log) is kept
// faithfully.
class CitySeparationGrid {
	Vec2 origin;
	Vec2 world_dimensions;
	double dsep;
	double dsep_sq;
	int grid_w = 1, grid_h = 1;
	Vector<Vector<Vec2>> cells; // row-major, index = gy * grid_w + gx

	void GetCellCoords(Vec2 world_v, int& gx, int& gy) const;
	bool CellOutOfBounds(int gx, int gy) const;
	Vector<Vec2>& Cell(int gx, int gy)             { return cells[gy * grid_w + gx]; }
	const Vector<Vec2>& Cell(int gx, int gy) const { return cells[gy * grid_w + gx]; }

public:
	CitySeparationGrid(Vec2 world_dimensions, Vec2 origin, double dsep);

	void AddSample(Vec2 v);
	void AddPolyline(const Vector<Vec2>& line);
	void AddAll(const CitySeparationGrid& other);

	// True if v is at least sqrt(d_sq) away from every existing sample
	// (default: the grid's own dsep - pass a smaller dtest^2 to allow
	// tighter integration-step spacing than seed spacing).
	bool IsValidSample(Vec2 v) const               { return IsValidSample(v, dsep_sq); }
	bool IsValidSample(Vec2 v, double d_sq) const;

	Vector<Vec2> GetNearbyPoints(Vec2 v, double distance) const;
};

#endif
