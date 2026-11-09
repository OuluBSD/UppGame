#include "SweetSandy.h"

NAMESPACE_UPP

const Vector<TrackDef>& GetTrackCatalog()
{
	static Vector<TrackDef> catalog = [] {
		Vector<TrackDef> v;
		TrackDef& t = v.Add();
		t.id = "default";
		t.name = "Default Track";
		t.heightmap_path = "share/sweetsandy/heightmaps/default.png";
		t.world_size = 200;
		t.height_scale = 12;
		t.kind = TERRAIN_ROLLING;

		TrackDef& o = v.Add();
		o.id = "obstacle_course";
		o.name = "Obstacle Course";
		o.heightmap_path = "share/sweetsandy/heightmaps/obstacle_course.png";
		o.world_size = 200;
		o.kind = TERRAIN_OBSTACLE_COURSE;

		// Island Terrain phase, Task 4.
		TrackDef& i = v.Add();
		i.id = "island";
		i.name = "Island (experimental)";
		i.world_size = 100000; // 100km - IslandTerrainParams::island_radius = 50000
		i.height_scale = 150;  // IslandTerrainParams' own default, made explicit here
		i.kind = TERRAIN_ISLAND;
		return v;
	}();
	return catalog;
}

const Vector<VehicleDef>& GetVehicleCatalog()
{
	static Vector<VehicleDef> catalog = [] {
		Vector<VehicleDef> v;
		VehicleDef& c = v.Add();
		c.id = "sand_dummy";
		c.name = "SandDummy";
		return v;
	}();
	return catalog;
}

const TrackDef* FindTrack(const String& id)
{
	for(const TrackDef& t : GetTrackCatalog())
		if(t.id == id)
			return &t;
	return NULL;
}

const VehicleDef* FindVehicle(const String& id)
{
	for(const VehicleDef& v : GetVehicleCatalog())
		if(v.id == id)
			return &v;
	return NULL;
}

END_UPP_NAMESPACE
