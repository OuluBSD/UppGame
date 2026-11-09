#ifndef _SweetSandy_BuildingPlacement_h_
#define _SweetSandy_BuildingPlacement_h_

// Buildings phase, Task 1: pure geometry/placement logic - which spots
// inside a city block get a building, and what footprint/height/roof
// style each one gets. No meshing yet (Task 2) - see Manager/2-plan/
// .../008-buildings/001-task1-building-placement.md.
struct PlannedBuilding : Moveable<PlannedBuilding> {
	Vec2 center;
	double yaw = 0;               // random facing around Y, visual variety only
	double width = 0, depth = 0;  // footprint size, meters (local x/z before yaw)
	double height = 0;            // floor_count * kFloorHeight
	int floor_count = 0;
	bool gabled_roof = false;     // false = flat roof
	Color wall_tint = White();
	Color roof_tint = White();
};

// Plans buildings for one city block (a block's own polygon, as found
// by CityBlockFinder - implicitly closed, see PointInPolygon's own
// convention). road_centerlines is the whole chunk's own set (not just
// this block's boundary) - a candidate near a road on the OTHER side
// of this block must still be rejected if it's simply too close to
// that road, matching VegChunkGenerator's own road-avoidance scope.
// base_seed determines every placement decision deterministically (see
// the task doc's own verification section).
Vector<PlannedBuilding> PlanBuildingsForBlock(const Vector<Vec2>& block,
                                               const Vector<Vector<Vec2>>& road_centerlines,
                                               dword base_seed);

#endif
