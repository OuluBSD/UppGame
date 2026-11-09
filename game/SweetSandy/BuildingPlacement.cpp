#include "SweetSandy.h"

NAMESPACE_UPP

namespace {

// Meters. Originally 7.0-16.0 ("sized against measured real block
// AREAS") - too large in practice: real blocks are frequently narrow,
// curvy-street-following strips, not compact squares that fill their
// own bounding box (a block's own AREA says nothing about its own
// WIDTH at any given point) - a first playtest run with real generated
// blocks accepted almost no candidates, most rejected for a footprint
// corner landing outside the block or inside the road setback. Shrunk
// after measuring real per-candidate rejection reasons directly (see
// 003-task3-worldbuilder-integration.md's own retuning note), not
// guessed a second time.
const double kMinFootprint = 5.0;
const double kMaxFootprint = 11.0;

// Meters between candidate placement points (jittered grid, same shape
// VegChunkGenerator uses for trees). Originally set to 20.0 (coarser
// than Vegetation's own 14.0m, reasoning "a building footprint is a
// bigger fraction of the grid cell than a tree canopy") - wrong in
// practice, found via real playtest data, not just the hand-built
// self-test blocks (which were 60-65m square, much bigger than a real
// block and so never exposed this): real generated blocks measure
// ~15-27m per side (~77-734m² per --citytest's own CityDump), so a 20m
// grid gave most blocks at most one candidate cell, and that one
// candidate's own full-cell jitter (±10m) very often landed outside a
// real block's own irregular (road-curvature-following) interior -
// see 003-task3-worldbuilder-integration.md's own note on this. 10.0
// gives even a modest block several candidate cells to try.
const double kBuildingGridSpacing = 8.0;

// Meters a footprint corner must clear the nearest road - covers the
// road's own painted width (Road Network's own kRoadWidth=4.0, so a
// road extends ~2.0m from its own centerline) plus a small real
// margin, not just "on the interior side of the block boundary graph
// edge" (the block boundary follows the road CENTERLINE, which sits
// inside the road's own width - see 001-planning.md's own note).
// Originally 3.5 - retuned down after measuring real generator output
// (see 003-task3-worldbuilder-integration.md's own retuning note): a
// real city block is frequently narrow enough that 3.5m of setback on
// both sides left little to no usable interior at all.
const double kBuildingSetback = 2.2;

// A block polygon with a smaller area than this isn't worth trying to
// place a building in (CityBlockFinder's own walk can produce a
// sliver face at an acute-angle junction).
const double kMinBlockArea = 60.0;
// Defensive guard against an implausibly large block polygon - not
// observed to trigger against real measured areas (see
// 001-planning.md), kept anyway since a block's own shape is generated
// content, not a hand-authored invariant this code should trust
// blindly.
const double kMaxBlockArea = 5000.0;

const double kFloorHeight = 3.0; // meters per floor

double FootprintRadius(double width, double depth)
{
	return 0.5 * sqrt(width * width + depth * depth);
}

} // anonymous namespace

Vector<PlannedBuilding> PlanBuildingsForBlock(const Vector<Vec2>& block,
                                               const Vector<Vector<Vec2>>& road_centerlines,
                                               dword base_seed)
{
	Vector<PlannedBuilding> result;

	double area = fabs(CityShoelaceArea(block));
	if(area < kMinBlockArea || area > kMaxBlockArea)
		return result;

	double min_x = 1e300, max_x = -1e300, min_z = 1e300, max_z = -1e300;
	for(Vec2 p : block) {
		min_x = min(min_x, p.x); max_x = max(max_x, p.x);
		min_z = min(min_z, p.y); max_z = max(max_z, p.y);
	}

	SeedRandom(base_seed);

	for(double gx = min_x + kBuildingGridSpacing * 0.5; gx < max_x; gx += kBuildingGridSpacing) {
		for(double gz = min_z + kBuildingGridSpacing * 0.5; gz < max_z; gz += kBuildingGridSpacing) {
			double x = gx + RandomRange(-kBuildingGridSpacing * 0.5, kBuildingGridSpacing * 0.5);
			double z = gz + RandomRange(-kBuildingGridSpacing * 0.5, kBuildingGridSpacing * 0.5);
			Vec2 center(x, z);

			PlannedBuilding b;
			b.center = center;
			b.yaw = Randomf() * 2 * M_PI;
			b.width = RandomRange(kMinFootprint, kMaxFootprint);
			b.depth = RandomRange(kMinFootprint, kMaxFootprint);
			// 85% low-rise [1,4], 15% tall [5,10] - see the task doc's
			// own "simple, not a fitted real-world distribution" note.
			b.floor_count = Randomf() < 0.85 ? 1 + (int)(Randomf() * 4) : 5 + (int)(Randomf() * 6);
			b.height = b.floor_count * kFloorHeight;
			b.gabled_roof = Randomf() < 0.4;
			// Plain, desaturated concrete/brick/tan palette - architectural,
			// not organic, so a much narrower hue range than VegTreePresets'
			// own bark/leaf tints.
			b.wall_tint = Color(180 + (int)(Randomf() * 50), 170 + (int)(Randomf() * 45), 150 + (int)(Randomf() * 45));
			b.roof_tint = Color(90 + (int)(Randomf() * 40), 70 + (int)(Randomf() * 35), 65 + (int)(Randomf() * 35));

			// Footprint corners, local (before yaw) -> world, for the
			// containment/setback checks below.
			double hw = b.width * 0.5, hd = b.depth * 0.5;
			Vec2 local_corners[4] = { Vec2(-hw, -hd), Vec2(hw, -hd), Vec2(hw, hd), Vec2(-hw, hd) };
			double cy = cos(b.yaw), sy = sin(b.yaw);
			Vec2 corners[4];
			bool ok = true;
			for(int i = 0; i < 4; i++) {
				Vec2 lc = local_corners[i];
				Vec2 wc = center + Vec2(lc.x * cy - lc.y * sy, lc.x * sy + lc.y * cy);
				corners[i] = wc;
				if(!PointInPolygon(block, wc) || DistanceToNearestRoad(wc, road_centerlines) < kBuildingSetback) {
					ok = false;
					break;
				}
			}
			if(!ok)
				continue;

			// Building-to-building overlap within this same block - see
			// 001-task1-building-placement.md's own "Building-to-building
			// overlap" note.
			double radius = FootprintRadius(b.width, b.depth);
			bool overlaps = false;
			for(const PlannedBuilding& other : result)
				if((b.center - other.center).Length() < radius + FootprintRadius(other.width, other.depth)) {
					overlaps = true;
					break;
				}
			if(overlaps)
				continue;

			result.Add(b);
		}
	}

	return result;
}

END_UPP_NAMESPACE
