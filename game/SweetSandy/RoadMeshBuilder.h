#ifndef _SweetSandy_RoadMeshBuilder_h_
#define _SweetSandy_RoadMeshBuilder_h_

// Road Network phase, Task 1: turns a road centerline polyline (City
// Layout Task 2's output) into a real triangulated RoadMesh. Port of
// RoadCreator's RoadSegment.CreateRoadMesh/GenerateMesh core loop, see
// Manager/2-plan/.../004-road-network/001-task1-ribbon-mesh.md.
class RoadMeshBuilder {
public:
	// width is constant along the whole centerline - City Layout's
	// streamlines carry no per-point width to taper from (unlike
	// RoadCreator's hand-authored start/end width), so there's nothing
	// to interpolate; revisit if that changes. centerline must have at
	// least 2 points.
	static RoadMesh BuildRibbon(const Vector<Vec2>& centerline, double width);
};

#endif
