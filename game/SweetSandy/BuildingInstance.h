#ifndef _SweetSandy_BuildingInstance_h_
#define _SweetSandy_BuildingInstance_h_

// Buildings phase, Task 3: one placed, already-built building - see
// Manager/2-plan/.../008-buildings/003-task3-worldbuilder-
// integration.md. Mirrors VegTreeInstance.h's own shape exactly
// (materialized mesh(es) + the extra fields a collider/renderer needs
// that the mesh triangles alone don't cheaply give back).
struct BuildingInstance : Moveable<BuildingInstance> {
	Vec3 position;                // world-space; y is ground height at placement time (Terrain::HeightAt)
	double yaw = 0;
	double width = 0, depth = 0, height = 0; // footprint + height, meters - needed for the collider without re-deriving from mesh triangles (matches VegTreeInstance's own trunk_radius/trunk_height precedent)
	Color wall_tint = White();
	Color roof_tint = White();
	VegMesh walls, roof;          // separate meshes, not merged - see BuildingMeshBuilder::Build's own signature

	void Serialize(Stream& s)
	{
		s % position % yaw % width % depth % height % walls % roof;
		// Color has no Stream::operator% overload in this U++ tree (only
		// a legacy operator/ - see Core/Color.h) - round-tripped via its
		// own raw dword instead, matching VegTreeInstance::Serialize
		// exactly.
		dword wall_raw = wall_tint.GetRaw();
		dword roof_raw = roof_tint.GetRaw();
		s % wall_raw % roof_raw;
		if(s.IsLoading()) {
			wall_tint = Color::FromRaw(wall_raw);
			roof_tint = Color::FromRaw(roof_raw);
		}
	}
};

#endif
