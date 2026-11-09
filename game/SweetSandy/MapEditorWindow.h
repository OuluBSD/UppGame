#ifndef _SweetSandy_MapEditorWindow_h_
#define _SweetSandy_MapEditorWindow_h_

// Map Editor phase, Task 1: SweetSandy's own thin wrapper around
// ModelerApp's Edit3D window, opened by AppShell on F1 (see
// AppShell::Key/ToggleMapEditor) as an independent top-level window -
// TopWindow::Open() (not Run()) is enough since it's serviced by the
// same GUI_APP_MAIN Run() loop AppShell's own window already runs
// (uppsrc/ModelerApp/main.cpp uses the identical pattern for its own
// secondary ExecutionWindow).
//
// Defaults to view-only (edit_mode=false): docks that expose editing
// tools/data-entry are hidden, only the scene tree, 4-view grid and
// timeline stay visible - still a useful read-only map viewer, per the
// user's own "F1 opens editor, or view-only if read-only" request. F2
// toggles edit mode while this window has focus - a real menu item/
// AppShell-level control is future work; this task only proves the
// embedding itself, no working editing tool is wired up yet.
//
// See Manager repo 012-map-editor/001-task1-modelerapp-embedding.md
// for why a freshly-constructed Edit3D (never fed a scene via
// LoadTestProject/LoadScene3D, exactly what this class does) is known
// safe - unlike uppsrc/ModelerApp's own built-in demo scene 0, which
// crashes for an unrelated, pre-existing, data-triggered reason.
class MapEditorWindow : public Edit3D {
public:
	typedef MapEditorWindow CLASSNAME;

	MapEditorWindow();

	void SetEditMode(bool b);
	bool IsEditMode() const { return edit_mode; }

	bool Key(dword key, int count) override;

	// Map Editor phase, Task 10: generates every chunk within `radius`
	// of `center` (a single-LOD-level, one-shot WorldBuilder::Update +
	// WaitIdle - blocking is fine here, this is a manual editor action,
	// not the real-time gameplay path Task 9 exists for) and converts
	// the result into real GeomObject/Model geometry in the active
	// scene (WorldGeomBridge.h), replacing whatever this window's own
	// previous bake produced. Positions the free-fly (VIEWMODE_
	// PERSPECTIVE, CAMSRC_FOCUS) camera above the baked area's centre,
	// looking down at a comfortable angle, ready to fly - see
	// 012-map-editor/010-task10-terrain-content-bridge.md. Uses this
	// window's own independent Terrain/WorldBuilder pair (below) -
	// deliberately never PlayScreen's own live instances, see that
	// doc's own "decoupled from any live PlayScreen" note.
	void BakeWorldNear(Vec2 center, double radius);
	// Test-only: the stats from the most recent BakeWorldNear call.
	WorldGeomBridgeStats GetLastBakeStats() const { return last_bake_stats; }
	// Test-only: for --mapeditorfly's own waypoint camera placement
	// (needs real ground height at arbitrary points - only meaningful
	// after at least one BakeWorldNear call).
	const Terrain& GetTerrain() const { return terrain; }
	// Map Editor phase, Task 17: read-only access to the same WorldBuilder
	// BakeWorldNear already filled in - the "export roadmask/buildingmask/
	// vegmask" subcommand (MapEditCommands.cpp) rasterizes straight from
	// its own WorldChunkContent (road_centerlines/buildings/vegetation),
	// not from the baked GeomScene, so it works even for content this
	// bridge doesn't merge into visible geometry (e.g. it wants raw
	// per-instance positions, not triangles).
	const WorldBuilder& GetCity() const { return city; }

	// Map Editor phase, Task 12: "wire up real editing so I can place
	// things by hand" - adds one new, uniquely-named marker object
	// (WorldGeomBridge.h's own AddMarkerObject) at `position`, selects
	// it (v0.selected_obj) and switches to TOOL_OBJ_MOVE, so a user can
	// immediately drag it into place with the mouse (Edit3D's own
	// already-existing gizmo-drag code - see AddMarkerObject's own
	// comment on why nothing new was needed for the move mechanism
	// itself, only something new to select-and-move). Forces edit_mode
	// on first (SetEditMode(true)) - the tool docks TOOL_OBJ_MOVE's own
	// UI lives in are hidden in view-only mode. Bound to F5.
	void SpawnMarker(Vec3 position);

	// Map Editor phase, Task 12: loads terrain.Load (cheap - a height
	// *function*, not the full procedural WorldBuilder pipeline
	// BakeWorldNear also sets up) if it hasn't run yet this session -
	// factored out of BakeWorldNear so --mapedit's own load-existing-
	// level subcommands (add-gate/move, which need real ground height
	// via Terrain::HeightAt but must NOT re-bake, since that would
	// silently discard whatever LoadScene3D just loaded) can get a
	// working Terrain without going through a full bake. Safe/cheap to
	// call repeatedly - a no-op after the first call.
	void EnsureTerrainLoaded();

	// Map Editor phase, Task 12: test-only accessors for
	// --mapeditoredittest - let it confirm SpawnMarker/a simulated drag
	// actually did what they claim (added the object, selected it,
	// armed the move tool, moved it) without needing a live mouse.
	// GetSelectedObject/GetEditTool simply expose Edit3D's own already-
	// public v0.selected_obj/edit_tool by name, for the same "don't
	// reach into base-class internals from a test" reason every other
	// test-only accessor in this class exists.
	GeomObject* FindObject(const String& name) { return prj->GetScene(0).FindObject(name); }
	GeomObject* GetSelectedObject() const { return v0.selected_obj; }
	EditTool GetEditTool() const { return edit_tool; }
	// Same GeomWorldState::UpdateObjects() call SpawnMarker/Editor.cpp's
	// own gizmo-drag code both make after changing a GeomTransform -
	// needed here so a test simulating a drag (write .position, then
	// this) sees the same real pickup the renderer would.
	void RefreshObjects() { state->UpdateObjects(); }

private:
	bool edit_mode = false;
	int marker_count = 0; // Task 12: each SpawnMarker call needs a unique object name - see AddMarkerObject's own comment

	// Map Editor phase, Task 10: this window's own world model, used
	// purely for BakeWorldNear - independent of any live PlayScreen
	// session (which owns its own Terrain/WorldBuilder pair entirely
	// separately). Constructed fresh on first bake.
	Terrain terrain;
	bool terrain_loaded = false; // Task 12: EnsureTerrainLoaded's own guard - separate from city_created, see that method's own comment
	WorldBuilder city;
	bool city_created = false;
	WorldGeomBridgeStats last_bake_stats;
};

#endif
