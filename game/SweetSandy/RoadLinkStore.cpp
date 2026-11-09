#include "SweetSandy.h"

NAMESPACE_UPP

Vector<RoadLinkEntry> LoadRoadLinks(const String& path)
{
	Vector<RoadLinkEntry> out;
	String content = LoadFile(path);
	if(content.IsEmpty())
		return out;
	for(const String& line : Split(~content, '\n', false)) {
		String t = TrimBoth(line);
		if(t.IsEmpty() || t[0] == '#')
			continue;
		Vector<String> f = Split(~t, ' ', true);
		if(f.GetCount() < 5)
			continue; // malformed - skip, same "one bad line doesn't take down the rest" reasoning as LoadLandmarks
		RoadLinkEntry e;
		e.name = f[0];
		e.x1 = ScanDouble(~f[1]);
		e.z1 = ScanDouble(~f[2]);
		e.x2 = ScanDouble(~f[3]);
		e.z2 = ScanDouble(~f[4]);
		if(f.GetCount() >= 6)
			e.road_class = f[5];
		out.Add(e);
	}
	return out;
}

void SaveRoadLinks(const String& path, const Vector<RoadLinkEntry>& entries)
{
	String out;
	out << "# SweetSandy hand-authored road links - name x1 z1 x2 z2 road_class\n";
	out << "# road_class: 30|40|50|60|80|100|120 (km/h) - see RoadClass.h\n";
	out << "# Generated/maintained by --mapedit connect-road - see RoadLinkStore.h.\n";
	for(const RoadLinkEntry& e : entries)
		out << e.name << " " << e.x1 << " " << e.z1 << " " << e.x2 << " " << e.z2 << " " << e.road_class << "\n";
	SaveFile(path, out);
}

namespace {
const Color kRoadLinkTint(0, 200, 220); // bright cyan - distinct from asphalt gray, grass green, and every landmark tint already in use
} // anonymous namespace

void MaterializeRoadLinks(GeomScene& scene, const Terrain& terrain, const Vector<RoadLinkEntry>& entries)
{
	for(const RoadLinkEntry& e : entries) {
		RoadClassParams cls;
		if(!ParseRoadClass(e.road_class, cls)) {
			LOG("MaterializeRoadLinks: '" << e.name << "' has unknown road_class '" << e.road_class << "' - skipped");
			continue;
		}
		RoadPathResult path = FindRoadPath(terrain, Vec2(e.x1, e.z1), Vec2(e.x2, e.z2), cls);
		if(!path.ok) {
			LOG("MaterializeRoadLinks: '" << e.name << "' - " << path.error << " - skipped");
			continue;
		}
		AddRoadLinkObject(scene, "roadlink_" + e.name, path.points, cls.width, terrain, kRoadLinkTint);
	}
}

END_UPP_NAMESPACE
