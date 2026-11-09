#ifndef _SweetSandy_PlayScreen_h_
#define _SweetSandy_PlayScreen_h_

// The actual driving demo: owns one PhysicsWorld + Terrain + Vehicle, steps
// physics on a fixed-rate timer, and renders with immediate-mode GL.
class PlayScreen : public GLCtrl {
	PhysicsWorld pw;
	Terrain      terrain;
	Vehicle      vehicle;
	WorldBuilder city; // City Layout phase: streams/generates road+block content around the vehicle
	TrafficManager traffic;
	PedestrianManager pedestrians;
	VectorMap<WorldChunkCoord, bool> traffic_chunks;
	VectorMap<WorldChunkCoord, bool> pedestrian_chunks;
	bool         running = false;

	// Vegetation phase, Task 4: one entry per currently-loaded chunk that
	// has been given real trunk colliders - incremental (not
	// rebuild-everything like Terrain::hf_geom) since WorldOctree already
	// reports per-chunk load/unload individually. See Tick()'s own
	// comment for the create/destroy logic.
	VectorMap<WorldChunkCoord, Vector<PhysGeomId>> veg_colliders;
	// Buildings phase, Task 3: identical per-chunk incremental lifecycle
	// as veg_colliders above, one box collider per building.
	VectorMap<WorldChunkCoord, Vector<PhysGeomId>> building_colliders;
	// Unified Ground phase, Task 4: one heightfield collider per loaded
	// chunk (sampling the same Terrain::HeightAt every chunk's own
	// ground mesh vertices use, so height stays continuous even though
	// the collider doesn't literally share the mesh's own asphalt/
	// grass-boundary topology - see that task's own "why not a real
	// trimesh" note) - replaces the old single fixed-origin
	// terrain.BuildOdeHeightfield() entirely; same incremental per-
	// chunk lifecycle as veg_colliders/building_colliders, just one
	// geom per chunk instead of one per instance.
	VectorMap<WorldChunkCoord, PhysGeomId> ground_colliders;

	bool key_throttle = false, key_brake = false, key_left = false, key_right = false;
	int  debug_frames_painted = 0; // logs GL debug info for the first few frames only
	// Every draw call of one frame - see PlayScreen.cpp's own comment and
	// `--pedpic`'s character on/off pair.
	void DrawSceneGL(const Vec3& eye);
	String pending_framebuffer_dump; // see RequestFramebufferDump
	bool   pending_ped_check = false; // see RequestPedestrianOnScreenCheck
	String pending_ped_onpath;        // see RequestPedestrianOnScreenCheck

	// Map Editor phase, Task 6: this run's own terrain-paint action
	// history - see 012-map-editor/006-task6-brush-painting-ui.md.
	// Session-long, alongside terrain/city above - not reset by
	// anything short of a fresh StartRun.
	ActionGraph          paint_graph;
	PaintBaseHeightCache paint_base_cache;
	bool                 paint_mode = false;       // toggled by F2 - see Key()
	bool                 paint_hit_valid = false;   // whether paint_hit is a real raycast hit this frame
	Vec3                 paint_hit;                 // last hover/click raycast hit, world space - drives the brush-ring preview
	Vec3                 paint_drag_last;            // last point actually painted, for LeftDrag's own distance throttle

	// Map Editor phase, Task 8: adjustable at runtime via the mouse
	// wheel (plain = radius, Ctrl = strength) - see MouseWheel() and
	// this task's own planning doc. brush_strength is a magnitude only;
	// PaintAt still applies the sign from Shift at click/drag time.
	double brush_radius = 15.0;
	double brush_strength = 2.0;
	static constexpr double kMinBrushRadius = 4.0, kMaxBrushRadius = 60.0, kBrushRadiusStep = 2.0;
	static constexpr double kMinBrushStrength = 0.25, kMaxBrushStrength = 20.0, kBrushStrengthStep = 0.5;

	// Paints one dab (AppendTerrainPaint) at world_pos and immediately
	// compiles + invalidates so it's visible next frame - see this
	// task's own "every dab visible immediately" design note.
	void PaintAt(Vec3 world_pos, bool lower);

	// Map Editor phase, Task 7: the kGridRes=9 sample-grid + PhysCreate
	// Heightfield + PhysGeomSetPosition sequence Tick()'s own per-chunk
	// loop already ran inline - factored out so RebuildGroundCollidersNear
	// can call the identical sequence instead of duplicating it (see
	// 012-map-editor/007-task7-collider-staleness.md on why that
	// matters). Returns 0 for a degenerate (zero-area) chunk, same as
	// the inline version already did.
	PhysGeomId BuildGroundCollider(const WorldChunk& chunk);
	// Destroys and rebuilds the ground collider for every chunk that
	// already has one AND whose bounds overlap [center-half_extent,
	// center+half_extent] (world x/z) - so a live collider reflects a
	// Terrain change (e.g. a paint edit) immediately, not just at the
	// chunk's next load. A chunk with no collider yet is left alone -
	// it'll build correctly, from already-current Terrain data, the
	// first time it loads.
	void RebuildGroundCollidersNear(Vec2 center, Vec2 half_extent);

	// Unified Ground phase, Task 1: leaf textures, indexed by
	// (int)VegTreePreset - loaded lazily on first GLPaint (needs a real
	// GL context, which doesn't exist yet in the constructor - see
	// GLPaint's own loading guard) and never touched by any headless
	// self-test mode (--selftest/--geomtest/etc. never call GLPaint).
	GLuint leaf_tex[2] = { 0, 0 };

	void Tick();

public:
	typedef PlayScreen CLASSNAME;

	Event<> WhenBack;

	PlayScreen();
	~PlayScreen();

	// (Re)initializes the world/terrain/vehicle for a fresh run and starts
	// the physics timer. Call before showing this screen.
	void StartRun(const TrackDef& track, const VehicleDef& veh);
	void StopRun();

	void GLPaint() override;
	bool Key(dword key, int count) override;
	// Map Editor phase, Task 6: mouse-driven terrain painting - active
	// only while paint_mode is on (F2). See this task's own planning
	// doc for the interaction model (hover previews, click paints,
	// drag paints more, Shift lowers).
	void MouseMove(Point p, dword keyflags) override;
	void LeftDown(Point p, dword keyflags) override;
	void LeftDrag(Point p, dword keyflags) override;
	// Map Editor phase, Task 8: plain wheel adjusts brush_radius,
	// Ctrl+wheel adjusts brush_strength - active only while paint_mode
	// is on, same as the other mouse overrides above.
	void MouseWheel(Point p, int zdelta, dword keyflags) override;

	// One-shot: saves the *next* painted frame's framebuffer to path (see
	// GLDebug.h's GLSaveFramebufferPNG - reads only this app's own
	// rendered pixels, not the desktop). Used by --citypic.
	void RequestFramebufferDump(const String& path) { pending_framebuffer_dump = path; }

	// One-shot: on the next painted frame, project the nearest real character's
	// feet and head into screen space and log both. This is the numeric half
	// of "does the pedestrian look right" - a screenshot shows a figure, this
	// shows that the figure is 1.7 m tall, standing on the ground, on screen,
	// and at the size its distance implies. Used by --pedpic.
	void RequestPedestrianOnScreenCheck(const String& ab_prefix = Null)
	{
		pending_ped_check = true;
		// Non-empty writes the in-frame character on/off pair; see
		// PedestrianManager::SetDrawCharacter.
		pending_ped_onpath = ab_prefix;
	}

	// Read-only diagnostics access - used by --lodpic to log vehicle
	// position and loaded-chunk LOD breakdown alongside each captured
	// frame, so a screenshot isn't the only evidence for "did LOD
	// streaming actually happen while driving".
	Vec3 GetVehiclePosition() const { return vehicle.GetChassisPosition(); }
	const WorldBuilder& GetCity() const { return city; }
	// Tesseract integration, Task 3D3: the pedestrians, so a picture/diagnostic
	// mode can ask whether the real skinned character is loaded and how the
	// level of detail split the last frame.
	const PedestrianManager& GetPedestrians() const { return pedestrians; }
	// Island Terrain phase, Task 4: read-only access for --islandpic's
	// own diagnostic logging (HeightAt/GetWaterLevel at the vehicle's
	// current position).
	const Terrain& GetTerrain() const { return terrain; }

	// Map Editor phase, Task 6: public (not just internal to mouse
	// event handling) for --painttest/--paintpic's own diagnostic use -
	// same "diagnostic access for a test/pic flag" precedent
	// GetVehiclePosition/GetTerrain above already established. Rebuilds
	// GLPaint's own fixed chase-camera basis analytically from the
	// vehicle's current position (see this task's own planning doc -
	// no live GL matrix query needed) and raycasts it against terrain.
	bool RaycastFromScreen(Point p, Vec3& hit_out) const;

	// Map Editor phase, Task 7: test-only diagnostic access for
	// --collidertest - lets it probe the REAL live ODE ground-collider
	// geom directly (PhysCollide against a small sphere, same
	// established pattern RunFlatHeightfieldTest already uses) rather
	// than only checking Terrain::HeightAt or the rendered mesh, both
	// already proven by earlier tasks and not what this one is about.
	// Returns 0 if no chunk covering world_pos currently has one.
	PhysGeomId FindGroundColliderAt(Vec2 world_pos) const;
	// Map Editor phase, Task 7: test-only - lets --collidertest build
	// its own probe sphere in the same physics space the real ground
	// colliders live in.
	PhysSpaceId GetPhysSpace() const { return pw.space; }

	// Map Editor phase, Task 8: read-only access for --brushtest.
	double GetBrushRadius() const   { return brush_radius; }
	double GetBrushStrength() const { return brush_strength; }

	// Map Editor phase, Task 9: test-only - blocks until `city`'s own
	// background worker has installed every generation request submitted
	// so far (see WorldBuilder::WaitIdle). Lets --painttest measure a
	// real click's own (fast) return latency separately from the (much
	// slower) regeneration it kicks off, through the real LeftDown UI
	// path - see this task's own planning doc, verification point 3.
	// Never called from the real gameplay path.
	void WaitCityIdle() { city.WaitIdle(); }
};

#endif
