#ifndef _SweetSandy_BuildingMeshBuilder_h_
#define _SweetSandy_BuildingMeshBuilder_h_

// Buildings phase, Task 2: turns one PlannedBuilding (Task 1) into a
// real triangle mesh - walls + roof, reusing VegMesh as-is (see
// Manager/2-plan/.../008-buildings/001-planning.md's own "reuse
// VegMesh, don't rename" decision).
class BuildingMeshBuilder {
public:
	// Two separate meshes, not one merged mesh - walls and roof get
	// different tints at draw time (Task 3), and VegMesh carries no
	// per-vertex/per-triangle color of its own (matches
	// VegTreeInstance's own separate branches/leaves meshes for the
	// identical reason: bark vs. leaf tint).
	//
	// ground_y is the world Y the footprint's own local y=0 sits at
	// (Terrain::HeightAt at the building's own center, sampled by the
	// caller - this class stays Terrain-agnostic, matching
	// RoadCreatorMeshGenerator/VegChunkGenerator's own established
	// convention). Both output meshes are in the building's own local
	// space (footprint center at local x=z=0, y=ground_y) - world
	// position/yaw placement happens at draw time (Task 3), matching
	// exactly how DrawTreeMeshGL's own tree meshes are positioned via
	// glTranslated/glRotated.
	static void Build(const PlannedBuilding& b, double ground_y, VegMesh& walls, VegMesh& roof);
};

#endif
