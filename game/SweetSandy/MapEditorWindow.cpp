#include "SweetSandy.h"

NAMESPACE_UPP

MapEditorWindow::MapEditorWindow()
{
	Title("SweetSandy Map Editor");
	SetEditMode(false);
	// Map Editor phase, Task 10: a freshly-constructed Edit3D starts
	// with zero scenes (GeomProject::Clear's own empty val.sub) -
	// GeomWorldState::GetActiveScene() asserts active_scene >= 0, so
	// without this, painting the (empty, harmless) scene is the best
	// case and a Panic (release builds: silent corruption - see
	// AGENTS.md's own note on ASSERT/Panic being debug-only here) once
	// BakeWorldNear below tries to add real content is the worst case.
	// LoadEmptyProject() specifically, not LoadTestProject/LoadScene3D -
	// Task 1's own note on why those are avoided here still applies;
	// LoadEmptyProject only calls CreateDefaultInit (prj->Clear() +
	// prj->AddScene()) + CreateDefaultPostInit (a no-op with no camera
	// object yet), neither of which touches any external file or the
	// known-crashing demo scene.
	LoadEmptyProject();

	// Map Editor phase, Task 12: GeomTransform is the one Geom* type in
	// uppsrc/Scene3D/Core/Geom.h never registered via that file's own
	// INITIALIZE(X) macro (every other type - GeomObject, GeomScene,
	// GeomCamera, etc. - is, at static-init time, immune to call-order
	// issues). AsTypeHash<T>() always calls TypedStringHasher<T>(0) (a
	// NULL name); TypedStringHasher<T>'s own static Hasher is built
	// exactly once, from whichever call happens *first* - normally
	// GeomObject::GetTransform/FindTransform's own internal priming
	// line runs first (real name, "GeomTransform"), before anything
	// calls AsTypeHash<GeomTransform>() with a null one. But this
	// window's own LoadScene3D (Task 12's --mapedit) is, in a fresh
	// process, sometimes the very first VfsValue-heavy code to run at
	// all - and its own generic node-type dispatch calls
	// AsTypeHash<GeomTransform>() (via IsVfsType checks against every
	// known type, GeomTransform included, even for a project with zero
	// GeomTransform nodes in it) *before* anything of this project's
	// own ever calls GetTransform/FindTransform - hits the null-name
	// case, "PANIC: Type has not been set yet" (a real, reproducible
	// process hang - Panic() shows a modal dialog nothing can click in
	// a scripted run). Priming it here, unconditionally, on every
	// construction - real cost is one hash + one map insert - closes
	// this off regardless of what a caller does next.
	TYPED_STRING_HASHER(GeomTransform);
}

void MapEditorWindow::SetEditMode(bool b)
{
	edit_mode = b;
	// Task 1 scope: hide every dock that exposes editing tools/data-
	// entry in view mode. Scene tree + 4-view grid + timeline stay
	// visible in both modes - browsing the map read-only is useful on
	// its own. No working tool is wired up behind edit mode yet.
	if(dock_tools)   dock_tools->Show(b);
	if(dock_props)   dock_props->Show(b);
	if(dock_script)  dock_script->Show(b);
	if(dock_texture) dock_texture->Show(b);
	if(dock_pool)    dock_pool->Show(b);
	if(dock_assets)  dock_assets->Show(b);
	if(dock_video)   dock_video->Show(b);
	tool.Show(b);
}

bool MapEditorWindow::Key(dword key, int count)
{
	if(key == K_F2) {
		SetEditMode(!edit_mode);
		return true;
	}
	// Map Editor phase, Task 10: F3 (re)bakes the world around the
	// origin - a fixed, simple starting point (not the camera's current
	// position) since on first press there's no meaningful "current
	// position" to bake around yet; re-baking around wherever the
	// camera has flown to is real future work, not attempted here.
	if(key == K_F3 && !(key & K_KEYUP)) {
		BakeWorldNear(Vec2(0, 0), 300.0);
		return true;
	}
	// Map Editor phase, Task 12: F5 spawns a new marker at the free-fly
	// camera's own current ground point (its X/Z, at real terrain
	// height - not literally under the camera's feet in 3D, since the
	// camera is usually well above the ground) - "drop something near
	// where I'm looking" is a more useful default than a fixed world
	// position, given the whole point is to place things while flying
	// around. Only meaningful after at least one BakeWorldNear (city_
	// created guards Terrain::HeightAt being called before terrain.Load
	// has ever run).
	if(key == K_F5 && !(key & K_KEYUP) && city_created) {
		GeomCamera& cam = state->GetFocus();
		SpawnMarker(Vec3(cam.position[0], 0, cam.position[2])); // y resolved inside SpawnMarker via real terrain height
		return true;
	}
	return Edit3D::Key(key, count);
}

void MapEditorWindow::SpawnMarker(Vec3 position)
{
	EnsureTerrainLoaded();
	double ground_y = terrain.HeightAt(position.x, position.z);
	Vec3 spawn_pos(position.x, ground_y + 1.0, position.z); // +1m so it doesn't clip into the ground mesh
	marker_count++;
	String name = "marker_" + AsString(marker_count);
	const Color kMarkerTint(255, 0, 220); // bright magenta - distinct from both the orange start gate and every procedural tint
	GeomScene& scene = prj->GetScene(0);
	AddMarkerObject(scene, name, spawn_pos, kMarkerTint);
	state->UpdateObjects(); // Editor.cpp's own established convention after any scene-graph/transform change - see WorldGeomBridge.h's own AddMarkerObject comment
	GeomObject* obj = scene.FindObject(name);
	if(obj) {
		v0.selected_obj = obj;
		SetEditMode(true);
		SetEditTool(TOOL_OBJ_MOVE);
	}
	LOG("MapEditorWindow::SpawnMarker: " << name << " at (" << spawn_pos.x << "," << spawn_pos.y << "," << spawn_pos.z
	    << ") - selected, TOOL_OBJ_MOVE active, ready to drag");
	RefrehRenderers();
}

void MapEditorWindow::EnsureTerrainLoaded()
{
	if(terrain_loaded)
		return;
	// Map Editor phase, Task 13/15: "do the terrain first, using that
	// terrain generation algorithm with that erosion" - loads the
	// island track and replays every persisted terrain-paint stroke
	// (TerrainPaintStore.h) via this phase's own real, unmodified brush
	// system (Tasks 3-4) - "terrain loaded" means "terrain loaded and
	// shaped" everywhere this method is called from (BakeWorldNear
	// included), not just here, and always *before* any city/road/
	// building content generates on top of it. Factored out
	// (LoadIslandTerrainWithPaint) so a caller that only needs a real,
	// shaped Terrain - no Edit3D/GeomScene/WorldBuilder involved at all
	// - doesn't need a MapEditorWindow to get one (Task 15's own
	// heightmap export is exactly that caller).
	LoadIslandTerrainWithPaint(terrain);
	terrain_loaded = true;
}

void MapEditorWindow::BakeWorldNear(Vec2 center, double radius)
{
	if(!city_created) {
		const TrackDef* track = FindTrack("island");
		ASSERT_(track, "MapEditorWindow::BakeWorldNear: no 'island' track in the catalog");
		EnsureTerrainLoaded();
		// Own cache root, distinct from any live PlayScreen session's
		// own (see this class's own header comment on why this
		// WorldBuilder is never shared) - "_mapeditor_v1" rather than
		// PlayScreen::CITY_GEN_VERSION so a change to either one's own
		// generator version never silently serves the other's cache.
		city.Create(track->id + "_mapeditor_v1");
		// Single LOD level covering the whole bake radius - this is a
		// one-shot bake, not real-time distance-based streaming, so the
		// multi-ring LOD scheme PlayScreen's own Tick-driven Update
		// needs has no purpose here (see this task's own planning doc).
		Vector<WorldOctree::LodLevel> levels;
		{ WorldOctree::LodLevel l; l.cell_size = 30; l.load_distance = radius; levels.Add(l); }
		city.SetLodLevels(levels);
		city.SetErosionGenerator(One<ITerrainErosionGenerator>(new ErosionChunkGenerator(terrain)));
		city.SetCityLayoutGenerator(One<ICityLayoutGenerator>(new MapGeneratorCityLayoutGenerator(track->id, levels[0].cell_size)));
		city.SetRoadMeshGenerator(One<IRoadMeshGenerator>(new RoadCreatorMeshGenerator(terrain)));
		city.SetVegetationGenerator(One<IVegetationGenerator>(new VegChunkGenerator(track->id, terrain)));
		city.SetBuildingGenerator(One<IBuildingGenerator>(new BuildingChunkGenerator(track->id, terrain)));
		city.SetGroundGenerator(One<IGroundGenerator>(new GroundChunkGenerator(terrain)));
		city_created = true;
	}

	city.Update(Vec3(center.x, 0, center.y));
	// Blocking here is fine - see this task's own planning doc, "Bake"
	// is a manual, one-shot editor action, not the real-time gameplay
	// path Task 9's own async worker exists to keep smooth.
	city.WaitIdle();

	// Full reset, not an incremental add - a "Bake" replaces whatever
	// this window's own previous bake produced rather than accumulating
	// alongside it (see this task's own planning doc on why a bounded,
	// one-shot bake was chosen over live-streaming sync).
	prj->Clear();
	GeomScene& scene = prj->AddScene();
	state->active_scene = 0;

	last_bake_stats = BuildWorldGeomObjects(scene, city, terrain);
	LOG("MapEditorWindow::BakeWorldNear: center=(" << center.x << "," << center.y << ") radius=" << radius
	    << " chunks=" << last_bake_stats.chunks << " objects=" << last_bake_stats.objects
	    << " meshes=" << last_bake_stats.meshes << " triangles=" << last_bake_stats.triangles);

	// Map Editor phase, Task 11: the first hand-authored (not
	// procedurally generated) landmark - see WorldGeomBridge.h's own
	// comment. At the vehicle's own spawn point (matches PlayScreen::
	// StartRun's own Vec3(5, ..., 5) exactly), so it's immediately
	// findable in either window. Known limitation, not yet solved:
	// re-baking wipes it along with the procedural content (prj->Clear()
	// above) - real persistence (save/load to a project file, kept
	// separate from what a re-bake regenerates) is future work, not
	// attempted here; this proves the "author real content, see it in
	// the editor" loop works at all, which nothing before this task did.
	AddStartGateLandmark(scene, terrain, Vec2(5, 5));

	// Positions the free-fly camera (VIEWMODE_PERSPECTIVE's own
	// CAMSRC_FOCUS, already fully interactive - arrow keys/mouse-drag,
	// see EditRendererBase - no new camera-input code needed) directly
	// above the baked area's centre, looking straight down - a top-down
	// "bird's eye" starting view, not a claim about the "right" angle
	// for actual inspection. Chosen specifically to sidestep the two
	// possible build-time forward-axis conventions in uppsrc/Geometry/
	// Matrix.h (VEC_FWD is +Z or -Z depending on one): looking straight
	// down is the same rotation either way, since only the *horizontal*
	// facing axis is ambiguous, not vertical - confirmed empirically
	// too (an angled-back identity-orientation placement, tried first,
	// rendered a blank background: identity's own default forward
	// wasn't pointed at the content at all). The same interactive
	// rotate/move controls this bake exists to make useful take it from
	// here - see this task's own planning doc, verification point 2.
	GeomCamera& cam = state->GetFocus();
	double ground_y = terrain.HeightAt(center.x, center.y);
	cam.position = vec3((float)center.x, (float)(ground_y + radius * 1.5 + 40), (float)center.y);
	cam.orientation = AxisAngleQuat(vec3(1, 0, 0), -(float)(M_PI / 2));
}

END_UPP_NAMESPACE
