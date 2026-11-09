#include "SweetSandy.h"

NAMESPACE_UPP

Vector<ZoneEntry> LoadZones(const String& path)
{
	Vector<ZoneEntry> out;
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
		ZoneEntry e;
		e.name = f[0];
		e.type = f[1];
		e.cx = ScanDouble(~f[2]);
		e.cz = ScanDouble(~f[3]);
		e.radius = ScanDouble(~f[4]);
		out.Add(e);
	}
	return out;
}

void SaveZones(const String& path, const Vector<ZoneEntry>& entries)
{
	String out;
	out << "# SweetSandy hand-authored zones - name type cx cz radius\n";
	out << "# type: residential|commercial|industrial|rural|park - see ZoneStore.h\n";
	out << "# Generated/maintained by --mapedit add-zone - see ZoneStore.h.\n";
	for(const ZoneEntry& e : entries)
		out << e.name << " " << e.type << " " << e.cx << " " << e.cz << " " << e.radius << "\n";
	SaveFile(path, out);
}

bool ZoneNeedsProtection(const String& type)
{
	return type == "residential" || type == "park";
}

namespace {

Color ZoneTint(const String& type)
{
	if(type == "residential") return Color(230, 160, 40);  // warm amber - matches this task's own "needs protecting" zones being the visually prominent ones
	if(type == "park")        return Color(40, 200, 90);   // green
	if(type == "commercial")  return Color(160, 60, 200);  // purple
	if(type == "industrial")  return Color(140, 140, 60);  // dull olive
	return Color(150, 150, 150); // "rural"/unknown - neutral gray
}

} // anonymous namespace

void MaterializeZoneBoundary(GeomScene& scene, const Terrain& terrain, const ZoneEntry& zone)
{
	const int kSamples = 48;
	Vector<RoadPathPoint> ring;
	for(int i = 0; i <= kSamples; i++) { // <=: repeats point 0 at the end, closing the ring for BuildRibbon
		double ang = 2 * M_PI * i / kSamples;
		RoadPathPoint p;
		p.pos = Vec2(zone.cx + cos(ang) * zone.radius, zone.cz + sin(ang) * zone.radius);
		p.bridge = false; // a zone boundary always follows the ground - no water-crossing concept here
		ring.Add(p);
	}
	AddRoadLinkObject(scene, "zone_" + zone.name, ring, 1.0, terrain, ZoneTint(zone.type));
}

END_UPP_NAMESPACE
