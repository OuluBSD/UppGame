#include "SweetSandy.h"

NAMESPACE_UPP

CitySeparationGrid::CitySeparationGrid(Vec2 world_dimensions_, Vec2 origin_, double dsep_)
	: origin(origin_), world_dimensions(world_dimensions_), dsep(dsep_)
{
	dsep_sq = dsep * dsep;
	grid_w = max(1, (int)ceil(world_dimensions.x / dsep));
	grid_h = max(1, (int)ceil(world_dimensions.y / dsep));
	cells.SetCount(grid_w * grid_h);
}

bool CitySeparationGrid::CellOutOfBounds(int gx, int gy) const
{
	return gx < 0 || gy < 0 || gx >= grid_w || gy >= grid_h;
}

void CitySeparationGrid::GetCellCoords(Vec2 world_v, int& gx, int& gy) const
{
	Vec2 v = world_v - origin; // worldToGrid, still in world units
	if(v.x < 0 || v.y < 0 || v.x >= world_dimensions.x || v.y >= world_dimensions.y) {
		// Out of bounds: the source silently treats this as cell (0,0)
		// (its error log is commented out) - kept faithfully.
		gx = 0;
		gy = 0;
		return;
	}
	gx = (int)floor(v.x / dsep);
	gy = (int)floor(v.y / dsep);
}

void CitySeparationGrid::AddSample(Vec2 v)
{
	int gx, gy;
	GetCellCoords(v, gx, gy);
	Cell(gx, gy).Add(v);
}

void CitySeparationGrid::AddPolyline(const Vector<Vec2>& line)
{
	for(Vec2 v : line)
		AddSample(v);
}

void CitySeparationGrid::AddAll(const CitySeparationGrid& other)
{
	for(int i = 0; i < other.cells.GetCount(); i++)
		for(Vec2 v : other.cells[i])
			AddSample(v);
}

bool CitySeparationGrid::IsValidSample(Vec2 v, double d_sq) const
{
	int cgx, cgy;
	GetCellCoords(v, cgx, cgy);

	for(int dx = -1; dx <= 1; dx++) {
		for(int dy = -1; dy <= 1; dy++) {
			int gx = cgx + dx, gy = cgy + dy;
			if(CellOutOfBounds(gx, gy))
				continue;
			for(Vec2 sample : Cell(gx, gy))
				if((sample - v).LengthSq() < d_sq)
					return false;
		}
	}
	return true;
}

Vector<Vec2> CitySeparationGrid::GetNearbyPoints(Vec2 v, double distance) const
{
	int radius = (int)ceil(distance / dsep - 0.5);
	int cgx, cgy;
	GetCellCoords(v, cgx, cgy);

	Vector<Vec2> out;
	for(int dx = -radius; dx <= radius; dx++) {
		for(int dy = -radius; dy <= radius; dy++) {
			int gx = cgx + dx, gy = cgy + dy;
			if(CellOutOfBounds(gx, gy))
				continue;
			for(Vec2 sample : Cell(gx, gy))
				out.Add(sample);
		}
	}
	return out;
}

END_UPP_NAMESPACE
