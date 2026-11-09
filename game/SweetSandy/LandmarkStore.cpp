#include "SweetSandy.h"

NAMESPACE_UPP

Vector<LandmarkEntry> LoadLandmarks(const String& path)
{
	Vector<LandmarkEntry> out;
	String content = LoadFile(path);
	if(content.IsEmpty())
		return out; // no file yet - not an error, see this function's own header comment
	for(const String& line : Split(~content, '\n', false)) {
		String t = TrimBoth(line);
		if(t.IsEmpty() || t[0] == '#')
			continue;
		Vector<String> f = Split(~t, ' ', true); // true: merge consecutive separators, so extra spaces don't produce empty fields
		if(f.GetCount() < 5)
			continue; // malformed line - skip rather than ASSERT, so a hand-edited file with one bad line doesn't take down every other entry
		LandmarkEntry e;
		e.kind = f[0];
		e.name = f[1];
		e.x = ScanDouble(~f[2]);
		e.y = ScanDouble(~f[3]);
		e.z = ScanDouble(~f[4]);
		if(f.GetCount() >= 8) {
			e.r = (int)ScanDouble(~f[5]);
			e.g = (int)ScanDouble(~f[6]);
			e.b = (int)ScanDouble(~f[7]);
		}
		out.Add(e);
	}
	return out;
}

void SaveLandmarks(const String& path, const Vector<LandmarkEntry>& entries)
{
	String out;
	out << "# SweetSandy hand-authored landmarks - kind name x y z [r g b]\n";
	out << "# Generated/maintained by --mapedit - see LandmarkStore.h.\n";
	for(const LandmarkEntry& e : entries)
		out << e.kind << " " << e.name << " " << e.x << " " << e.y << " " << e.z << " " << e.r << " " << e.g << " " << e.b << "\n";
	SaveFile(path, out);
}

void MaterializeLandmarks(GeomScene& scene, const Terrain& terrain, const Vector<LandmarkEntry>& entries)
{
	for(const LandmarkEntry& e : entries) {
		if(e.kind == "marker")
			AddMarkerObject(scene, e.name, Vec3(e.x, terrain.HeightAt(e.x, e.z) + e.y, e.z), Color(e.r, e.g, e.b));
		else if(e.kind == "gate")
			AddStartGateLandmark(scene, terrain, Vec2(e.x, e.z));
		else
			LOG("MaterializeLandmarks: unknown kind '" << e.kind << "' for '" << e.name << "' - skipped");
	}
}

END_UPP_NAMESPACE
