#include "SweetSandy.h"

NAMESPACE_UPP

namespace {
const char* const kLandmarksPath = "share/sweetsandy/levels/landmarks.txt";
}

// Map Editor phase, Task 14: graceful "ERROR: ..." returns here, not
// ASSERT_ - this function now also runs inside a long-lived
// --mapedit-server process (MapEditServer/main.cpp's own dispatch
// loop), where a bad command must never crash the whole server out
// from under every other client. The standalone one-shot --mapedit CLI
// path (main.cpp) still gets the same fail-fast behavior it always
// had, just via checking this function's own returned "ERROR" prefix
// and asserting there instead of inside this shared function.
String ExecuteMapEditCommand(AppShell& win, const Vector<String>& args)
{
	if(args.IsEmpty())
		return "ERROR: empty command";
	String sub = args[0];
	RealizeDirectory(GetFileFolder(kLandmarksPath));

	// Map Editor phase, Task 15: "do a lot lower resolution... and/or
	// smaller map, then export and view manually the heightmap" - the
	// user's own request, after Task 9/10's own "was that spike a mesh
	// bug or a real hillside" investigation had to settle the question
	// indirectly (a top-down 3D render, camera angle and lighting both
	// getting in the way) - a literal grayscale heightmap is the
	// direct way to look at what Terrain::HeightAt actually produced,
	// with no city/building/vegetation content or GL rendering pass in
	// the way at all. Checked before `ed` is fetched below - this is
	// the one subcommand that needs no Edit3D/GeomScene/WorldBuilder
	// involvement whatsoever, just LoadIslandTerrainWithPaint
	// (TerrainPaintStore.h, Task 15) + direct Terrain::HeightAt
	// sampling, so it stays cheap regardless of how expensive `render`
	// itself is.
	if(sub == "heightmap") {
		if(args.GetCount() < 2)
			return "ERROR: heightmap needs: out_path [cx cz half_extent resolution]";
		String out_path = args[1];
		double cx = args.GetCount() > 2 ? ScanDouble(~args[2]) : 0;
		double cz = args.GetCount() > 3 ? ScanDouble(~args[3]) : 0;
		// Defaults deliberately small/coarse (a 200m-square area,
		// 10m/sample -> 21x21) - the user's own explicit ask, both for
		// speed and because a coarser grid is *easier* to read by eye
		// than an oversampled, visually-smooth one that hides
		// individual erosion features.
		double half_extent = args.GetCount() > 4 ? ScanDouble(~args[4]) : 100.0;
		double resolution = args.GetCount() > 5 ? ScanDouble(~args[5]) : 10.0;
		if(resolution <= 0)
			return "ERROR: heightmap resolution must be positive";

		Terrain terrain;
		LoadIslandTerrainWithPaint(terrain);

		int n = (int)(2 * half_extent / resolution) + 1;
		if(n < 2)
			n = 2;
		Vector<float> heights;
		heights.SetCount(n * n);
		float minh = 1e9f, maxh = -1e9f;
		// Row 0 = world z = cz-half_extent (the "south" edge), row
		// n-1 = cz+half_extent ("north") - not flipped like a GL
		// framebuffer capture (GLDebug.h's own GLSaveFramebufferPNG),
		// since this was never rendered through a camera in the first
		// place - it's a direct top-down sample grid, world-space row
		// order throughout.
		for(int gz = 0; gz < n; gz++) {
			for(int gx = 0; gx < n; gx++) {
				double wx = cx - half_extent + gx * resolution;
				double wz = cz - half_extent + gz * resolution;
				float h = (float)terrain.HeightAt(wx, wz);
				heights[gz * n + gx] = h;
				minh = min(minh, h);
				maxh = max(maxh, h);
			}
		}
		// Only meaningful when IsIsland() (Terrain::GetWaterLevel's own
		// contract - see Terrain.h) - true for every terrain this
		// command ever loads (LoadIslandTerrainWithPaint always loads
		// the "island" track). A sample at or below this is drawn blue
		// (darker with depth) instead of grayscale, so the coastline
		// this task's own investigation already relied on (see this
		// task's own report - a suspicious-looking flat area turned
		// out to be real coastline, confirmed via a wider view) is
		// directly visible in the export itself, not just inferable
		// from "this patch happens to be dark".
		double water_level = terrain.GetWaterLevel();
		int water_count = 0;
		for(int i = 0; i < heights.GetCount(); i++)
			if(heights[i] <= water_level)
				water_count++;

		// Each sample becomes a solid block of pixels, not one bare
		// pixel - a genuinely coarse n x n sample grid (n=21 at the
		// defaults above) would otherwise export as a technically-
		// correct but practically unviewable 21x21 PNG. This keeps the
		// real, deliberately coarse sampling (no interpolation between
		// blocks - each block is one real Terrain::HeightAt call, not
		// smoothed) while making the file something actually worth
		// opening.
		const int px = 12;
		ImageBuffer ib(n * px, n * px);
		float range = maxh - minh;
		for(int gz = 0; gz < n; gz++) {
			for(int gx = 0; gx < n; gx++) {
				float h = heights[gz * n + gx];
				RGBA color;
				if(h <= water_level) {
					// Deep water darker, shoreline (h near
					// water_level) brighter blue - depth relative to
					// this export's own sampled range, not an absolute
					// scale (this island has no real bathymetry data
					// below sea level to speak of, just whatever the
					// same height function returns there).
					float depth_range = (float)water_level - minh;
					float t = depth_range > 0.0001f ? Clamp(((float)water_level - h) / depth_range, 0.0f, 1.0f) : 0.0f;
					color.r = (byte)(20 + (1 - t) * 40);
					color.g = (byte)(60 + (1 - t) * 80);
					color.b = (byte)(140 + (1 - t) * 100);
					color.a = 255;
				}
				else {
					byte v = range > 0.0001f ? (byte)Clamp((h - minh) / range * 255.0f, 0.0f, 255.0f) : (byte)128;
					color.r = color.g = color.b = v;
					color.a = 255;
				}
				for(int by = 0; by < px; by++) {
					RGBA *dst = ib[gz * px + by];
					for(int bx = 0; bx < px; bx++)
						dst[gx * px + bx] = color;
				}
			}
		}
		Image img(ib);
		if(!PNGEncoder().SaveFile(out_path, img))
			return "ERROR: failed to save " + out_path;
		return "OK heightmap " + AsString(n) + "x" + AsString(n) + " samples (" + AsString(n * px) + "x" + AsString(n * px)
		     + " px) saved " + out_path + " (min=" + AsString(minh) + " max=" + AsString(maxh) + " range=" + AsString(range)
		     + " water_level=" + AsString(water_level) + " underwater_samples=" + AsString(water_count) + "/" + AsString(n * n) + ")";
	}

	MapEditorWindow& ed = win.GetMapEditor();

	if(sub == "add-marker") {
		if(args.GetCount() < 5)
			return "ERROR: add-marker needs: name x height_above_ground z [r g b]";
		Vector<LandmarkEntry> entries = LoadLandmarks(kLandmarksPath);
		LandmarkEntry e;
		e.kind = "marker";
		e.name = args[1];
		e.x = ScanDouble(~args[2]);
		e.y = ScanDouble(~args[3]);
		e.z = ScanDouble(~args[4]);
		if(args.GetCount() > 7) {
			e.r = (int)ScanDouble(~args[5]);
			e.g = (int)ScanDouble(~args[6]);
			e.b = (int)ScanDouble(~args[7]);
		}
		for(int i = entries.GetCount() - 1; i >= 0; i--)
			if(entries[i].name == e.name)
				entries.Remove(i); // re-adding the same name updates it, not duplicates it
		entries.Add(e);
		SaveLandmarks(kLandmarksPath, entries);
		return "OK add-marker '" + e.name + "' at (" + AsString(e.x) + "," + AsString(e.y) + "," + AsString(e.z) + ")";
	}

	if(sub == "add-gate") {
		if(args.GetCount() < 3)
			return "ERROR: add-gate needs: name x z";
		Vector<LandmarkEntry> entries = LoadLandmarks(kLandmarksPath);
		LandmarkEntry e;
		e.kind = "gate";
		e.name = args[1];
		e.x = ScanDouble(~args[2]);
		e.z = ScanDouble(~args[3]);
		for(int i = entries.GetCount() - 1; i >= 0; i--)
			if(entries[i].name == e.name)
				entries.Remove(i);
		entries.Add(e);
		SaveLandmarks(kLandmarksPath, entries);
		return "OK add-gate '" + e.name + "' at (" + AsString(e.x) + "," + AsString(e.z) + ")";
	}

	if(sub == "move") {
		if(args.GetCount() < 4)
			return "ERROR: move needs: name dx dy dz";
		Vector<LandmarkEntry> entries = LoadLandmarks(kLandmarksPath);
		String name = args[1];
		int i = -1;
		for(int k = 0; k < entries.GetCount(); k++)
			if(entries[k].name == name) { i = k; break; }
		if(i < 0)
			return "ERROR: no landmark named '" + name + "' - see 'list'";
		entries[i].x += ScanDouble(~args[2]);
		entries[i].y += ScanDouble(~args[3]);
		entries[i].z += ScanDouble(~args[4]);
		SaveLandmarks(kLandmarksPath, entries);
		return "OK move '" + name + "' -> (" + AsString(entries[i].x) + "," + AsString(entries[i].y) + "," + AsString(entries[i].z) + ")";
	}

	if(sub == "remove") {
		if(args.GetCount() < 2)
			return "ERROR: remove needs: name";
		Vector<LandmarkEntry> entries = LoadLandmarks(kLandmarksPath);
		String name = args[1];
		int before = entries.GetCount();
		for(int i = entries.GetCount() - 1; i >= 0; i--)
			if(entries[i].name == name)
				entries.Remove(i);
		if(entries.GetCount() == before)
			return "ERROR: no landmark named '" + name + "' - see 'list'";
		SaveLandmarks(kLandmarksPath, entries);
		return "OK remove '" + name + "'";
	}

	if(sub == "paint") {
		if(args.GetCount() < 5)
			return "ERROR: paint needs: cx cz radius delta [slope_limit]";
		Vector<PaintStrokeEntry> strokes = LoadPaintStrokes(kTerrainPaintStorePath);
		PaintStrokeEntry e; // kind defaults to "raise"
		e.cx = ScanDouble(~args[1]);
		e.cz = ScanDouble(~args[2]);
		e.radius = ScanDouble(~args[3]);
		e.delta = ScanDouble(~args[4]);
		if(args.GetCount() > 5)
			e.slope_limit = ScanDouble(~args[5]);
		strokes.Add(e);
		SavePaintStrokes(kTerrainPaintStorePath, strokes);
		return "OK paint stroke #" + AsString(strokes.GetCount() - 1) + " center=(" + AsString(e.cx) + "," + AsString(e.cz)
		     + ") radius=" + AsString(e.radius) + " delta=" + AsString(e.delta);
	}

	// Map Editor phase, Task 17: "manual flat city area with brush to
	// terrain" - blends toward an absolute target world-Y instead of
	// adding an offset (TerrainPaintBrush::flatten - see that field's
	// own comment). target_height is a world-Y, not an offset above
	// ground - pass the value a prior 'heightmap' export already showed
	// for the area, or Terrain::HeightAt(cx,cz) itself if leveling
	// exactly to the current center height.
	if(sub == "flatten") {
		if(args.GetCount() < 5)
			return "ERROR: flatten needs: cx cz radius target_height [slope_limit]";
		Vector<PaintStrokeEntry> strokes = LoadPaintStrokes(kTerrainPaintStorePath);
		PaintStrokeEntry e;
		e.kind = "flatten";
		e.cx = ScanDouble(~args[1]);
		e.cz = ScanDouble(~args[2]);
		e.radius = ScanDouble(~args[3]);
		e.delta = ScanDouble(~args[4]); // the target world-Y - see TerrainPaintBrush::flatten
		if(args.GetCount() > 5)
			e.slope_limit = ScanDouble(~args[5]);
		strokes.Add(e);
		SavePaintStrokes(kTerrainPaintStorePath, strokes);
		return "OK flatten stroke #" + AsString(strokes.GetCount() - 1) + " center=(" + AsString(e.cx) + "," + AsString(e.cz)
		     + ") radius=" + AsString(e.radius) + " target_height=" + AsString(e.delta);
	}

	// Map Editor phase, Task 17: "erosion again (slight), to make flat
	// city area natural" - a real ErodeHeightmap touch-up (see
	// TerrainErosionBrush's own comment), not a fake roughening pass.
	// droplets defaults low (150) - "slight" per the user's own request.
	if(sub == "erode") {
		if(args.GetCount() < 4)
			return "ERROR: erode needs: cx cz radius [droplets] [seed]";
		Vector<PaintStrokeEntry> strokes = LoadPaintStrokes(kTerrainPaintStorePath);
		PaintStrokeEntry e;
		e.kind = "erode";
		e.cx = ScanDouble(~args[1]);
		e.cz = ScanDouble(~args[2]);
		e.radius = ScanDouble(~args[3]);
		if(args.GetCount() > 4)
			e.droplets = (int)ScanDouble(~args[4]);
		if(args.GetCount() > 5)
			e.seed = (dword)(int)ScanDouble(~args[5]);
		strokes.Add(e);
		SavePaintStrokes(kTerrainPaintStorePath, strokes);
		return "OK erode stroke #" + AsString(strokes.GetCount() - 1) + " center=(" + AsString(e.cx) + "," + AsString(e.cz)
		     + ") radius=" + AsString(e.radius) + " droplets=" + AsString(e.droplets) + " seed=" + AsString((int)e.seed);
	}

	// Map Editor phase, Task 19: no stroke-removal tool existed before
	// this - a real gap found while iterating on this task's own bridge
	// test terrain (a bad first attempt at an artificial pond/river left
	// scars with no way to clean them up). Index matches 'list''s own
	// numbering exactly (0-based, in replay order) - removing shifts
	// every later index down by one, same as any array removal.
	if(sub == "remove-stroke") {
		if(args.GetCount() < 2)
			return "ERROR: remove-stroke needs: index (see 'list')";
		int idx = (int)ScanDouble(~args[1]);
		Vector<PaintStrokeEntry> strokes = LoadPaintStrokes(kTerrainPaintStorePath);
		if(idx < 0 || idx >= strokes.GetCount())
			return "ERROR: stroke index " + AsString(idx) + " out of range (0.." + AsString(strokes.GetCount() - 1) + ") - see 'list'";
		strokes.Remove(idx);
		SavePaintStrokes(kTerrainPaintStorePath, strokes);
		return "OK remove-stroke #" + AsString(idx) + " (" + AsString(strokes.GetCount()) + " remaining)";
	}

	// Map Editor phase, Task 18/19: "join roads from city to rural
	// places and to other cities" (Task 18) - "focus on road path
	// finder next, which tries to keep the gradient limited... bridges
	// should be quite orthogonal to water crossings... different types
	// of road [30/40/50/60/80/100/120 km/h]" (Task 19, the user's own
	// words). Runs RoadPathfinder::FindRoadPath immediately, not just
	// at render/export time - a route that can't actually be built
	// (too steep, or a water crossing too wide for a single span) is
	// reported as an ERROR right here, not silently skipped later (see
	// MaterializeRoadLinks's own "logged and skipped" fallback, which
	// exists for a terrain edit made *after* a link was saved, not for
	// this).
	if(sub == "connect-road") {
		if(args.GetCount() < 6)
			return "ERROR: connect-road needs: name x1 z1 x2 z2 road_class(30|40|50|60|80|100|120)";
		RoadClassParams cls;
		if(!ParseRoadClass(args[6], cls))
			return "ERROR: unknown road_class '" + args[6] + "' - expected 30|40|50|60|80|100|120";
		RoadLinkEntry e;
		e.name = args[1];
		e.x1 = ScanDouble(~args[2]);
		e.z1 = ScanDouble(~args[3]);
		e.x2 = ScanDouble(~args[4]);
		e.z2 = ScanDouble(~args[5]);
		e.road_class = args[6];

		ed.EnsureTerrainLoaded();
		RoadPathResult path = FindRoadPath(ed.GetTerrain(), Vec2(e.x1, e.z1), Vec2(e.x2, e.z2), cls);
		if(!path.ok)
			return "ERROR: connect-road '" + e.name + "' - " + path.error;

		Vector<RoadLinkEntry> links = LoadRoadLinks(kRoadLinkStorePath);
		for(int i = links.GetCount() - 1; i >= 0; i--)
			if(links[i].name == e.name)
				links.Remove(i); // re-adding the same name updates it, not duplicates it - same convention as add-marker
		links.Add(e);
		SaveRoadLinks(kRoadLinkStorePath, links);
		return "OK connect-road '" + e.name + "' class=" + e.road_class + " points=" + AsString(path.points.GetCount())
		     + " bridge_points=" + AsString(path.bridge_point_count) + " max_grade_achieved="
		     + AsString(path.max_grade_pct_achieved) + "% min_curve_radius_achieved="
		     + AsString(path.min_curve_radius_achieved) + "m";
	}

	if(sub == "remove-road") {
		if(args.GetCount() < 2)
			return "ERROR: remove-road needs: name";
		Vector<RoadLinkEntry> links = LoadRoadLinks(kRoadLinkStorePath);
		String name = args[1];
		int before = links.GetCount();
		for(int i = links.GetCount() - 1; i >= 0; i--)
			if(links[i].name == name)
				links.Remove(i);
		if(links.GetCount() == before)
			return "ERROR: no road link named '" + name + "' - see 'list'";
		SaveRoadLinks(kRoadLinkStorePath, links);
		return "OK remove-road '" + name + "'";
	}

	// Map Editor phase, Task 20: "fencing requires zoning first" - the
	// user's own correction to Task 19's "fencing not attempted" gap.
	// A zone is a hand-authored circular area (ZoneStore.h) - fences
	// are never authored directly, they're computed from roads+zones
	// (see 'render'/'export' below, and FenceGenerator.h's own top
	// comment for why).
	if(sub == "add-zone") {
		if(args.GetCount() < 6)
			return "ERROR: add-zone needs: name type(residential|commercial|industrial|rural|park) cx cz radius";
		Vector<ZoneEntry> zones = LoadZones(kZoneStorePath);
		ZoneEntry e;
		e.name = args[1];
		e.type = args[2];
		e.cx = ScanDouble(~args[3]);
		e.cz = ScanDouble(~args[4]);
		e.radius = ScanDouble(~args[5]);
		for(int i = zones.GetCount() - 1; i >= 0; i--)
			if(zones[i].name == e.name)
				zones.Remove(i); // re-adding the same name updates it, not duplicates it - same convention as add-marker/connect-road
		zones.Add(e);
		SaveZones(kZoneStorePath, zones);
		return "OK add-zone '" + e.name + "' type=" + e.type + " center=(" + AsString(e.cx) + "," + AsString(e.cz)
		     + ") radius=" + AsString(e.radius) + (ZoneNeedsProtection(e.type) ? " (needs-protection)" : "");
	}

	if(sub == "remove-zone") {
		if(args.GetCount() < 2)
			return "ERROR: remove-zone needs: name";
		Vector<ZoneEntry> zones = LoadZones(kZoneStorePath);
		String name = args[1];
		int before = zones.GetCount();
		for(int i = zones.GetCount() - 1; i >= 0; i--)
			if(zones[i].name == name)
				zones.Remove(i);
		if(zones.GetCount() == before)
			return "ERROR: no zone named '" + name + "' - see 'list'";
		SaveZones(kZoneStorePath, zones);
		return "OK remove-zone '" + name + "'";
	}

	if(sub == "list") {
		String out;
		Vector<LandmarkEntry> entries = LoadLandmarks(kLandmarksPath);
		for(const LandmarkEntry& e : entries)
			out << "[" << e.kind << "] " << e.name << " pos=(" << e.x << "," << e.y << "," << e.z
			    << ") color=(" << e.r << "," << e.g << "," << e.b << ")\n";
		out << entries.GetCount() << " landmark(s) total\n";
		Vector<RoadLinkEntry> links = LoadRoadLinks(kRoadLinkStorePath);
		for(const RoadLinkEntry& e : links)
			out << "[road] " << e.name << " (" << e.x1 << "," << e.z1 << ") -> (" << e.x2 << "," << e.z2
			    << ") class=" << e.road_class << "\n";
		out << links.GetCount() << " road link(s) total\n";
		Vector<ZoneEntry> zones = LoadZones(kZoneStorePath);
		for(const ZoneEntry& e : zones)
			out << "[zone] " << e.name << " type=" << e.type << " center=(" << e.cx << "," << e.cz << ") radius=" << e.radius
			    << (ZoneNeedsProtection(e.type) ? " (needs-protection)" : "") << "\n";
		out << zones.GetCount() << " zone(s) total\n";
		ed.EnsureTerrainLoaded();
		Vector<FenceSegment> fences = ComputeFences(links, zones, ed.GetTerrain());
		out << fences.GetCount() << " fence segment(s) computed (roads x zones collision - see FenceGenerator.h)\n";
		Vector<PaintStrokeEntry> strokes = LoadPaintStrokes(kTerrainPaintStorePath);
		for(int i = 0; i < strokes.GetCount(); i++) {
			const PaintStrokeEntry& s = strokes[i];
			out << "[" << s.kind << " " << i << "] center=(" << s.cx << "," << s.cz << ") radius=" << s.radius;
			if(s.kind == "erode")
				out << " droplets=" << s.droplets << " seed=" << (int)s.seed;
			else
				out << " value=" << s.delta << " slope_limit=" << s.slope_limit;
			out << "\n";
		}
		out << strokes.GetCount() << " terrain-paint stroke(s) total";
		return "OK\n" + out;
	}

	if(sub == "render") {
		if(args.GetCount() < 3)
			return "ERROR: render needs: yz|xz|xy|persp out_path [cx cz]";
		String view_arg = args[1];
		ViewMode view_mode = VIEWMODE_PERSPECTIVE;
		if(view_arg == "yz") view_mode = VIEWMODE_YZ;
		else if(view_arg == "xz") view_mode = VIEWMODE_XZ;
		else if(view_arg == "xy") view_mode = VIEWMODE_XY;
		else if(view_arg == "persp") view_mode = VIEWMODE_PERSPECTIVE;
		else return "ERROR: unknown view '" + view_arg + "' - expected yz|xz|xy|persp";
		String out_path = args[2];
		bool closeup = args.GetCount() > 4;
		double closeup_x = closeup ? ScanDouble(~args[3]) : 0;
		double closeup_z = closeup ? ScanDouble(~args[4]) : 0;

		// See MapEditorWindow::BakeWorldNear's own comment - the
		// city_created guard inside it means this is a cheap scene
		// rebuild (no real generation work), not a full re-bake, once
		// this same `ed` has already baked once in this process (true
		// for every --mapedit-server call after its first "render").
		// Map Editor phase, Task 17: bake AROUND the closeup point when
		// one was given, not always (0,0) - a real bug found while
		// looking at a render of hand-authored content 500m from the
		// origin: the camera moved to the right place, but the world
		// itself was never generated that far out (WorldOctree::
		// UpdateStreaming only loads within `radius` of whatever center
		// city.Update() is called with), so the render showed nothing
		// there at all.
		ed.BakeWorldNear(closeup ? Vec2(closeup_x, closeup_z) : Vec2(0, 0), 300.0);
		Vector<LandmarkEntry> entries = LoadLandmarks(kLandmarksPath);
		GeomScene& scene = ed.prj->GetScene(ed.state->active_scene);
		MaterializeLandmarks(scene, ed.GetTerrain(), entries);
		Vector<RoadLinkEntry> road_links = LoadRoadLinks(kRoadLinkStorePath);
		MaterializeRoadLinks(scene, ed.GetTerrain(), road_links);
		Vector<ZoneEntry> zones = LoadZones(kZoneStorePath);
		for(const ZoneEntry& z : zones)
			MaterializeZoneBoundary(scene, ed.GetTerrain(), z);
		MaterializeFences(scene, ed.GetTerrain(), ComputeFences(road_links, zones, ed.GetTerrain()));
		ed.RefreshObjects();

		GeomCamera& cam = ed.state->GetFocus();
		if(closeup) {
			double gy = ed.GetTerrain().HeightAt(closeup_x, closeup_z);
			cam.position = vec3((float)closeup_x, (float)(gy + 25), (float)closeup_z);
			cam.orientation = AxisAngleQuat(vec3(1, 0, 0), -(float)(M_PI / 2));
		}
		if(view_mode != VIEWMODE_PERSPECTIVE) {
			cam.scale = 350.0f;
			if(view_mode == VIEWMODE_XY || view_mode == VIEWMODE_YZ)
				cam.position[1] = (float)(ed.GetTerrain().HeightAt(0, 0) + 100.0);
		}
		Image img;
		RenderSceneV2Headless(ed.render_ctx, Size(1024, 640), nullptr, &img, nullptr, false, view_mode, &cam, false);
		PNGEncoder().SaveFile(out_path, img);
		return "OK render saved " + out_path + " (landmarks=" + AsString(entries.GetCount())
		     + " HeightAt(" + AsString(closeup_x) + "," + AsString(closeup_z) + ")="
		     + AsString(ed.GetTerrain().HeightAt(closeup_x, closeup_z)) + ")";
	}

	// Map Editor phase, Task 17: "export 512x512 (or smaller) images...
	// doesn't need to be RGB light, but heightmap, road mask, etc." -
	// the user's own explicit ask for a `kind` selector, alongside
	// 'heightmap' (Task 15, unchanged) and 'render' (Task 14, a full GL
	// perspective/ortho shot). Rasterizes straight from WorldChunkContent
	// (road_centerlines/buildings/vegetation) - the same real generated
	// data BuildWorldGeomObjects turns into 3D geometry, just top-down
	// 2D dots/lines instead, so it stays readable at a glance and small
	// (a fixed 512x512, never larger, per the user's own size cap).
	if(sub == "export") {
		if(args.GetCount() < 3)
			return "ERROR: export needs: roadmask|buildingmask|vegmask|zonemask|citymask out_path [cx cz half_extent]";
		String kind = args[1];
		String out_path = args[2];
		double cx = args.GetCount() > 3 ? ScanDouble(~args[3]) : 0;
		double cz = args.GetCount() > 4 ? ScanDouble(~args[4]) : 0;
		double half_extent = args.GetCount() > 5 ? ScanDouble(~args[5]) : 80.0;
		bool want_road = kind == "roadmask" || kind == "citymask";
		bool want_building = kind == "buildingmask" || kind == "citymask";
		bool want_veg = kind == "vegmask";
		// Map Editor phase, Task 20: zones/fences shown for their own
		// dedicated 'zonemask' (nothing else in the way) as well as
		// folded into 'citymask' (they're inherently road-context, same
		// reasoning road links already got folded in for Task 18).
		bool want_zone = kind == "zonemask" || kind == "citymask";
		if(!want_road && !want_building && !want_veg && !want_zone)
			return "ERROR: unknown export kind '" + kind + "' - expected roadmask|buildingmask|vegmask|zonemask|citymask";

		// Padded so content generated just outside [cx-half_extent,
		// cx+half_extent] (a chunk straddling the boundary) still bakes
		// - the raster below clips to the exact requested extent anyway.
		ed.BakeWorldNear(Vec2(cx, cz), half_extent + 40.0);

		const int px = 512; // the user's own explicit cap - never larger
		ImageBuffer ib(px, px);
		RGBA bg; bg.r = bg.g = bg.b = 245; bg.a = 255; // near-white "nothing here" background, so real marks stand out
		for(int y = 0; y < px; y++) {
			RGBA *row = ib[y];
			for(int x = 0; x < px; x++)
				row[x] = bg;
		}

		auto ToPixel = [&](double wx, double wz, int& out_x, int& out_y) {
			double u = (wx - (cx - half_extent)) / (2 * half_extent);
			// Row 0 = world z = cz-half_extent ("south"), matching
			// heightmap's own un-flipped world-space row order above.
			double v = (wz - (cz - half_extent)) / (2 * half_extent);
			out_x = (int)Clamp(u * px, 0.0, (double)(px - 1));
			out_y = (int)Clamp(v * px, 0.0, (double)(px - 1));
		};
		auto PlotDot = [&](double wx, double wz, RGBA color, int r) {
			int cxp, cyp;
			ToPixel(wx, wz, cxp, cyp);
			for(int dy = -r; dy <= r; dy++) {
				int py = cyp + dy;
				if(py < 0 || py >= px)
					continue;
				RGBA *row = ib[py];
				for(int dx = -r; dx <= r; dx++) {
					int pxx = cxp + dx;
					if(pxx < 0 || pxx >= px || dx * dx + dy * dy > r * r)
						continue;
					row[pxx] = color;
				}
			}
		};
		auto PlotLine = [&](Vec2 a, Vec2 b, RGBA color) {
			int x0, y0, x1, y1;
			ToPixel(a.x, a.y, x0, y0);
			ToPixel(b.x, b.y, x1, y1);
			int steps = max(abs(x1 - x0), abs(y1 - y0));
			if(steps == 0) steps = 1;
			for(int s = 0; s <= steps; s++) {
				double t = (double)s / steps;
				int px_i = (int)round(x0 + (x1 - x0) * t);
				int py_i = (int)round(y0 + (y1 - y0) * t);
				if(px_i >= 0 && px_i < px && py_i >= 0 && py_i < px)
					ib[py_i][px_i] = color;
			}
		};

		int road_segments = 0, buildings = 0, vegetation = 0;
		for(int i = 0; i < ed.GetCity().GetOctree().GetLoadedChunkCount(); i++) {
			const WorldChunk& chunk = ed.GetCity().GetOctree().GetLoadedChunk(i);
			const WorldChunkContent *content = ed.GetCity().FindContent(chunk.coord);
			if(!content)
				continue; // still generating - BakeWorldNear already WaitIdle()'d, so this should not happen in practice
			if(want_road) {
				RGBA road_c; road_c.r = 60; road_c.g = 60; road_c.b = 200; road_c.a = 255;
				for(const Vector<Vec2>& line : content->road_centerlines) {
					for(int k = 0; k + 1 < line.GetCount(); k++) {
						PlotLine(line[k], line[k + 1], road_c);
						road_segments++;
					}
				}
			}
			if(want_building) {
				RGBA b_c; b_c.r = 200; b_c.g = 40; b_c.b = 40; b_c.a = 255;
				for(const BuildingInstance& b : content->buildings) {
					PlotDot(b.position.x, b.position.z, b_c, 3);
					buildings++;
				}
			}
			if(want_veg) {
				RGBA v_c; v_c.r = 30; v_c.g = 140; v_c.b = 30; v_c.a = 255;
				for(const VegTreeInstance& t : content->vegetation) {
					PlotDot(t.position.x, t.position.z, v_c, 2);
					vegetation++;
				}
			}
		}
		// Map Editor phase, Task 18/19: hand-authored road links drawn on
		// top, in their own bright-cyan color (matching AddRoadLinkObject/
		// MaterializeRoadLinks' own 3D tint) - distinct from the blue
		// procedural road_centerlines above, so a citymask/roadmask export
		// shows at a glance which connections are generated vs. which one
		// this task just hand-placed. Runs the SAME FindRoadPath a real
		// render/materialize would (Task 19) - not the raw endpoints -
		// so the mask shows the actual curved/bridged route, not a
		// straight line that may not even be where the real road goes.
		// Bridge points drawn in a brighter yellow so a water crossing
		// is visible at a glance in the mask too, not just in a render.
		int road_links = 0, road_link_bridge_points = 0;
		if(want_road) {
			RGBA link_c; link_c.r = 0; link_c.g = 200; link_c.b = 220; link_c.a = 255;
			RGBA bridge_c; bridge_c.r = 255; bridge_c.g = 220; bridge_c.b = 0; bridge_c.a = 255;
			for(const RoadLinkEntry& e : LoadRoadLinks(kRoadLinkStorePath)) {
				RoadClassParams cls;
				if(!ParseRoadClass(e.road_class, cls))
					continue;
				RoadPathResult path = FindRoadPath(ed.GetTerrain(), Vec2(e.x1, e.z1), Vec2(e.x2, e.z2), cls);
				if(!path.ok)
					continue;
				for(int k = 0; k + 1 < path.points.GetCount(); k++) {
					bool bridge = path.points[k].bridge || path.points[k + 1].bridge;
					PlotLine(path.points[k].pos, path.points[k + 1].pos, bridge ? bridge_c : link_c);
					if(bridge)
						road_link_bridge_points++;
				}
				road_links++;
			}
		}
		// Map Editor phase, Task 20: zone boundaries (thin rings, tinted
		// by type - matches MaterializeZoneBoundary's own 3D tint
		// exactly, ZoneStore.cpp's own ZoneTint) and computed fences
		// (thicker lines, colored by kind - matches MaterializeFences'
		// own tint) - drawn last so they sit on top of everything else,
		// since "did the fence actually land where the road and zone
		// collide" is exactly what this export exists to let a caller
		// check at a glance.
		int zones_drawn = 0, fence_segments = 0;
		if(want_zone) {
			Vector<ZoneEntry> zones = LoadZones(kZoneStorePath);
			for(const ZoneEntry& z : zones) {
				RGBA zc;
				if(z.type == "residential")     { zc.r = 230; zc.g = 160; zc.b = 40; }
				else if(z.type == "park")       { zc.r = 40;  zc.g = 200; zc.b = 90; }
				else if(z.type == "commercial") { zc.r = 160; zc.g = 60;  zc.b = 200; }
				else if(z.type == "industrial") { zc.r = 140; zc.g = 140; zc.b = 60; }
				else                             { zc.r = 150; zc.g = 150; zc.b = 150; }
				zc.a = 255;
				const int kSamples = 48;
				Vec2 prev(z.cx + z.radius, z.cz);
				for(int i = 1; i <= kSamples; i++) {
					double ang = 2 * M_PI * i / kSamples;
					Vec2 cur(z.cx + cos(ang) * z.radius, z.cz + sin(ang) * z.radius);
					PlotLine(prev, cur, zc);
					prev = cur;
				}
				zones_drawn++;
			}
			RGBA wall_c; wall_c.r = 90; wall_c.g = 90; wall_c.b = 95; wall_c.a = 255;
			RGBA dirt_c; dirt_c.r = 120; dirt_c.g = 85; dirt_c.b = 50; dirt_c.a = 255;
			for(const FenceSegment& f : ComputeFences(LoadRoadLinks(kRoadLinkStorePath), zones, ed.GetTerrain())) {
				RGBA fc = f.kind == "wall" ? wall_c : dirt_c;
				for(int k = 0; k + 1 < f.points.GetCount(); k++)
					PlotLine(f.points[k], f.points[k + 1], fc);
				fence_segments++;
			}
		}
		Image img(ib);
		if(!PNGEncoder().SaveFile(out_path, img))
			return "ERROR: failed to save " + out_path;
		return "OK export " + kind + " saved " + out_path + " (" + AsString(px) + "x" + AsString(px)
		     + " px, half_extent=" + AsString(half_extent) + ") road_segments=" + AsString(road_segments)
		     + " road_links=" + AsString(road_links) + " road_link_bridge_segments=" + AsString(road_link_bridge_points)
		     + " buildings=" + AsString(buildings) + " vegetation=" + AsString(vegetation)
		     + " zones=" + AsString(zones_drawn) + " fence_segments=" + AsString(fence_segments);
	}

	return "ERROR: unknown subcommand '" + sub
	     + "' - expected add-marker|add-gate|move|remove|paint|flatten|erode|remove-stroke|connect-road|remove-road|add-zone|remove-zone|heightmap|export|list|render";
}

END_UPP_NAMESPACE
