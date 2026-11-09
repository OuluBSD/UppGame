#include "SweetSandy.h"

NAMESPACE_UPP

Vector<PaintStrokeEntry> LoadPaintStrokes(const String& path)
{
	Vector<PaintStrokeEntry> out;
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
		PaintStrokeEntry e;
		e.kind = f[0];
		e.cx = ScanDouble(~f[1]);
		e.cz = ScanDouble(~f[2]);
		e.radius = ScanDouble(~f[3]);
		e.delta = ScanDouble(~f[4]);
		if(f.GetCount() >= 6)
			e.slope_limit = ScanDouble(~f[5]);
		if(f.GetCount() >= 7)
			e.droplets = (int)ScanDouble(~f[6]);
		if(f.GetCount() >= 8)
			e.seed = (dword)(int)ScanDouble(~f[7]);
		out.Add(e);
	}
	return out;
}

void SavePaintStrokes(const String& path, const Vector<PaintStrokeEntry>& strokes)
{
	String out;
	out << "# SweetSandy hand-authored terrain-paint strokes - kind cx cz radius delta slope_limit droplets seed\n";
	out << "# kind: raise (delta=additive offset) | flatten (delta=absolute target world-Y) | erode (droplets/seed used, delta/slope_limit ignored)\n";
	out << "# Generated/maintained by --mapedit paint|flatten|erode - see TerrainPaintStore.h. Order matters (replayed in order).\n";
	for(const PaintStrokeEntry& e : strokes)
		out << e.kind << " " << e.cx << " " << e.cz << " " << e.radius << " " << e.delta << " " << e.slope_limit
		    << " " << e.droplets << " " << (int)e.seed << "\n";
	SaveFile(path, out);
}

void LoadIslandTerrainWithPaint(Terrain& terrain)
{
	const TrackDef* track = FindTrack("island");
	ASSERT_(track, "LoadIslandTerrainWithPaint: no 'island' track in the catalog");
	terrain.Load(*track);

	Vector<PaintStrokeEntry> strokes = LoadPaintStrokes(kTerrainPaintStorePath);
	if(strokes.IsEmpty())
		return;
	ActionGraph paint_graph;
	PaintBaseHeightCache paint_base_cache;
	for(const PaintStrokeEntry& s : strokes) {
		if(s.kind == "erode") {
			TerrainErosionBrush eb;
			eb.cx = s.cx; eb.cz = s.cz; eb.radius = s.radius; eb.droplets = s.droplets; eb.seed = s.seed;
			AppendTerrainErosion(paint_graph, terrain, paint_base_cache, eb);
			continue;
		}
		TerrainPaintBrush b;
		b.cx = s.cx; b.cz = s.cz; b.radius = s.radius; b.delta = s.delta; b.slope_limit = s.slope_limit;
		b.flatten = (s.kind == "flatten");
		AppendTerrainPaint(paint_graph, terrain, paint_base_cache, b);
	}
	CompileTerrainPaintToTerrain(paint_graph, terrain);
	LOG("LoadIslandTerrainWithPaint: replayed " << strokes.GetCount() << " terrain-paint stroke(s)");
}

END_UPP_NAMESPACE
