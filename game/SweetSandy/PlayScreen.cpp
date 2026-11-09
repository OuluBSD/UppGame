#include "SweetSandy.h"

NAMESPACE_UPP

// Wedge-shaped shell (the light "mirrors/roof/doors" part 2 of the body,
// see Vehicle::Create): a box whose front (low, toward +z - matches the
// front wheels' hub_offset in Vehicle::Create) is shorter than the back,
// with a single slanted "windshield/hood" face between - "vähän kiilamainen
// ja aerodynaaminen" per the request. sx/sz are the full footprint (same as
// part 1 below it); y_bottom/y_top_front/y_top_rear are in the chassis
// body's local frame.
static void DrawWedgeShell(double sx, double sz, double y_bottom, double y_top_front, double y_top_rear)
{
	double x = sx * 0.5, z = sz * 0.5;

	glBegin(GL_QUADS);
		glNormal3d(0, -1, 0);
		glVertex3d(-x, y_bottom, -z); glVertex3d(x, y_bottom, -z);
		glVertex3d(x, y_bottom,  z);  glVertex3d(-x, y_bottom,  z);
	glEnd();

	glBegin(GL_QUADS); // rear, vertical, full height
		glNormal3d(0, 0, -1);
		glVertex3d(-x, y_bottom, -z);   glVertex3d(-x, y_top_rear, -z);
		glVertex3d( x, y_top_rear, -z); glVertex3d( x, y_bottom, -z);
	glEnd();

	glBegin(GL_QUADS); // front, vertical, low
		glNormal3d(0, 0, 1);
		glVertex3d(-x, y_bottom, z);     glVertex3d(x, y_bottom, z);
		glVertex3d( x, y_top_front, z);  glVertex3d(-x, y_top_front, z);
	glEnd();

	{
		Vec3 slope(0, y_top_front - y_top_rear, sz); // rear-top -> front-top, along the slant
		Vec3 n = slope.Cross(Vec3(1, 0, 0)).Normalized();
		glBegin(GL_QUADS); // the slanted "windshield/hood" face
			glNormal3d(n.x, n.y, n.z);
			glVertex3d(-x, y_top_rear, -z);  glVertex3d(x, y_top_rear, -z);
			glVertex3d( x, y_top_front, z);  glVertex3d(-x, y_top_front, z);
		glEnd();
	}

	glBegin(GL_QUADS); // left
		glNormal3d(-1, 0, 0);
		glVertex3d(-x, y_bottom, -z);    glVertex3d(-x, y_top_rear, -z);
		glVertex3d(-x, y_top_front, z);  glVertex3d(-x, y_bottom, z);
	glEnd();

	glBegin(GL_QUADS); // right
		glNormal3d(1, 0, 0);
		glVertex3d(x, y_bottom, -z);   glVertex3d(x, y_bottom, z);
		glVertex3d(x, y_top_front, z); glVertex3d(x, y_top_rear, -z);
	glEnd();
}

// Cylinder centered on the origin, symmetry axis along local Z - matches
// Vehicle's wheel body orientation directly (see Vehicle::Create).
static void DrawWheelCylinder(double radius, double length)
{
	const int SEG = 14;
	double hz = length * 0.5;
	glBegin(GL_QUAD_STRIP);
	for(int i = 0; i <= SEG; i++) {
		double a = 2 * M_PI * i / SEG;
		double cx = cos(a), cy = sin(a);
		glNormal3d(cx, cy, 0);
		glVertex3d(radius * cx, radius * cy,  hz);
		glVertex3d(radius * cx, radius * cy, -hz);
	}
	glEnd();
	for(int side = 0; side < 2; side++) {
		double z = side ? -hz : hz;
		glBegin(GL_TRIANGLE_FAN);
		glNormal3d(0, 0, side ? -1 : 1);
		glVertex3d(0, 0, z);
		for(int i = 0; i <= SEG; i++) {
			double a = 2 * M_PI * i / SEG * (side ? -1 : 1);
			glVertex3d(radius * cos(a), radius * sin(a), z);
		}
		glEnd();
	}
}

// City Layout + Road Network phase content: the real triangulated road
// mesh (Road Network Tasks 1-4) and city-block outlines (City Layout
// Task 4) for every currently-loaded chunk. Points are already world-
// space x/z (see MapGeneratorCityLayoutGenerator::Generate), so no per-
// chunk offset is needed - just look up ground height and draw. Blocks
// are still an unlit wireframe preview (no building mesh exists yet -
// that's a later phase per 002-world-builder/001-planning.md); roads are
// now real flat-colored geometry, not a wireframe placeholder.
static void DrawCityGL(const WorldBuilder& city, const Terrain& terrain)
{
	// Unified Ground phase, Task 4: small upward nudge, kept as a defensive
	// margin against the asphalt/grass boundary not being bit-identical
	// (GroundMeshBuilder insets each block by kRoadWidth*0.5 rather than
	// literally sharing RoadMesh's own ribbon-edge vertices - see that
	// task's own "block inset, not boundary trace" scope note) - without
	// it, a tiny float mismatch right at the shared edge could z-fight.
	const double y_offset = 0.08;

	glDisable(GL_LIGHTING);

	// Road Network phase, Task 4: the real triangulated road mesh
	// (ribbons + junction fans, Tasks 1-3), replacing the flat
	// centerline wireframe City Layout originally drew here. Flat-
	// colored, no per-vertex normals computed (lighting is already
	// disabled for this whole function) - matches "flat asphalt color,
	// no texture system yet" from the phase's own planning doc.
	glColor3ub(60, 60, 64); // dark asphalt-like gray
	for(int i = 0; i < city.GetOctree().GetLoadedChunkCount(); i++) {
		const WorldChunk& chunk = city.GetOctree().GetLoadedChunk(i);
		const WorldChunkContent* content = city.FindContent(chunk.coord);
		if(!content)
			continue;
		const RoadMesh& mesh = content->road_mesh;
		if(mesh.triangles.IsEmpty())
			continue;
		glBegin(GL_TRIANGLES);
		for(int t = 0; t < mesh.triangles.GetCount(); t++) {
			Vec2 v = mesh.vertices[mesh.triangles[t]].pos;
			glVertex3d(v.x, terrain.HeightAt(v.x, v.y) + y_offset, v.y);
		}
		glEnd();
	}

	// Block outlines are drawn nudged toward each polygon's own average
	// point instead of at their true (road-bounding) vertices - a block's
	// boundary traces the same edge as the road that bounds it, so drawn
	// at the literal vertices the two line sets nearly coincide and are
	// hard to tell apart ("siirrä korttelin ääriviiva hieman sisemmäs
	// tiestä"). This is a simple per-vertex shrink-toward-average, not a
	// true parallel/uniform polygon inset (which would need real offset
	// geometry - awkward for a possibly non-convex face, in particular
	// the graph's "outer"/unbounded one - see CityBlockFinder.h) - good
	// enough to visually separate the two line sets in this wireframe
	// preview, not a claim about where a real building footprint would
	// start.
	const double inset_fraction = 0.12;
	glColor3ub(110, 220, 220); // light cyan, distinct from both terrain and the vehicle's own colors
	for(int i = 0; i < city.GetOctree().GetLoadedChunkCount(); i++) {
		const WorldChunk& chunk = city.GetOctree().GetLoadedChunk(i);
		const WorldChunkContent* content = city.FindContent(chunk.coord);
		if(!content)
			continue;
		for(const Vector<Vec2>& poly : content->blocks) {
			Vec2 avg = CityPolygonAverage(poly);
			glBegin(GL_LINE_LOOP);
			for(Vec2 v : poly) {
				Vec2 p = v + (avg - v) * inset_fraction;
				glVertex3d(p.x, terrain.HeightAt(p.x, p.y) + y_offset, p.y);
			}
			glEnd();
		}
	}

	glEnable(GL_LIGHTING);
}

// Vegetation phase, Task 4: one flat-tinted, lit VegMesh (branches or
// leaves). Unlike DrawCityGL's own unlit road/block wireframe, VegMesh
// carries real per-vertex normals (Vegetation Tasks 1/3), so lit
// rendering is a strict upgrade here - requires GL_LIGHTING already
// enabled (GLPaint's own default, still on when this runs right after
// DrawCityGL leaves it re-enabled) rather than touching that state
// itself. Renamed from DrawTreeMeshGL (Buildings phase, Task 3): no
// longer tree-specific, now also draws each building's own walls/roof
// meshes - the exact "second real consumer" this project's own
// promotion precedent already established elsewhere in this phase
// (SandyMath.h's CityShoelaceArea/DistanceToNearestRoad).
// Unified Ground phase, Task 1: optional tex (0 = untextured, the
// original behavior every building/branch call still uses) binds a GL
// texture and hard alpha-cutouts it (GL_ALPHA_TEST, not GL_BLEND - no
// back-to-front sort needed for overlapping leaf quads in this
// immediate-mode renderer). tint still multiplies the texture's own
// color (default GL_MODULATE texture env), so per-tree tint variety
// survives on top of the real leaf texture.
static void DrawFlatTintedMeshGL(const VegMesh& mesh, Color tint, GLuint tex = 0)
{
	if(mesh.triangles.IsEmpty())
		return;
	if(tex) {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, tex);
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.5f);
	}
	glColor3ub(tint.GetR(), tint.GetG(), tint.GetB());
	glBegin(GL_TRIANGLES);
	for(int idx : mesh.triangles) {
		const VegMeshVertex& v = mesh.vertices[idx];
		glNormal3d(v.normal.x, v.normal.y, v.normal.z);
		glTexCoord2d(v.uv.x, v.uv.y); // no-op when untextured, harmless
		glVertex3d(v.pos.x, v.pos.y, v.pos.z);
	}
	glEnd();
	if(tex) {
		glDisable(GL_ALPHA_TEST);
		glDisable(GL_TEXTURE_2D);
	}
}

// Every loaded chunk's real tree geometry (VegTreeInstance.branches/
// leaves, Vegetation Tasks 1-3). No Terrain& needed here, unlike
// DrawCityGL - a tree instance's own position.y already has ground
// height resolved at placement time (VegChunkGenerator), it's not a
// bare 2D point like road_centerlines/blocks are.
static void DrawVegetationGL(const WorldBuilder& city, const GLuint leaf_tex[2])
{
	for(int i = 0; i < city.GetOctree().GetLoadedChunkCount(); i++) {
		const WorldChunk& chunk = city.GetOctree().GetLoadedChunk(i);
		const WorldChunkContent* content = city.FindContent(chunk.coord);
		if(!content)
			continue;
		for(const VegTreeInstance& tree : content->vegetation) {
			glPushMatrix();
			glTranslated(tree.position.x, tree.position.y, tree.position.z);
			glRotated(tree.yaw * 180.0 / M_PI, 0, 1, 0);
			DrawFlatTintedMeshGL(tree.branches, tree.bark_tint);
			DrawFlatTintedMeshGL(tree.leaves, tree.leaf_tint, leaf_tex[(int)tree.preset]);
			glPopMatrix();
		}
	}
}

// Buildings phase, Task 3: every loaded chunk's real building geometry
// (BuildingInstance.walls/roof, Tasks 1-2). Same per-instance
// glPushMatrix/glTranslated/glRotated placement pattern as
// DrawVegetationGL above - a building instance's own position.y already
// has ground height resolved at placement time (BuildingChunkGenerator),
// same as a tree instance's.
static void DrawBuildingsGL(const WorldBuilder& city)
{
	for(int i = 0; i < city.GetOctree().GetLoadedChunkCount(); i++) {
		const WorldChunk& chunk = city.GetOctree().GetLoadedChunk(i);
		const WorldChunkContent* content = city.FindContent(chunk.coord);
		if(!content)
			continue;
		for(const BuildingInstance& b : content->buildings) {
			glPushMatrix();
			glTranslated(b.position.x, b.position.y, b.position.z);
			glRotated(b.yaw * 180.0 / M_PI, 0, 1, 0);
			DrawFlatTintedMeshGL(b.walls, b.wall_tint);
			DrawFlatTintedMeshGL(b.roof, b.roof_tint);
			glPopMatrix();
		}
	}
}

// Unified Ground phase, Task 4: every loaded chunk's own real ground
// mesh (GroundChunkGenerator's own per-block grass + coarse fallback
// fill) - replaces terrain.DrawGL()'s old single fixed-origin rolling
// terrain entirely. Ground mesh vertices are already in world space
// (unlike DrawVegetationGL/DrawBuildingsGL's own per-instance local
// meshes), so no glPushMatrix/glTranslated is needed here.
static void DrawGroundGL(const WorldBuilder& city)
{
	Color grass_tint(100, 170, 80);
	for(int i = 0; i < city.GetOctree().GetLoadedChunkCount(); i++) {
		const WorldChunk& chunk = city.GetOctree().GetLoadedChunk(i);
		const WorldChunkContent* content = city.FindContent(chunk.coord);
		if(!content)
			continue;
		for(const VegMesh& mesh : content->ground)
			DrawFlatTintedMeshGL(mesh, grass_tint);
	}
}

PlayScreen::PlayScreen()
{
	WantFocus();
}

PlayScreen::~PlayScreen()
{
	StopRun();
}

void PlayScreen::StartRun(const TrackDef& track, const VehicleDef& veh)
{
	StopRun();

	pw.Create();
	terrain.Load(track);
	// Unified Ground phase, Task 4: no terrain.BuildOdeHeightfield() here
	// any more - Terrain is now a height *function* only (Terrain::HeightAt,
	// read by GroundMeshBuilder/GroundChunkGenerator and this file's own
	// per-chunk ground_colliders below), never its own rendered or
	// collidable object. See that phase's own planning doc for why: the
	// user reported seeing Terrain's old rolling-hill DrawGL() rendered
	// underneath the real asphalt simultaneously, which was never the
	// intent - asphalt and grass now come from the same unified per-chunk
	// ground system instead.

	// Island Terrain phase, Task 4: nudged off the exact origin - found
	// the hard way that (0,0) sits precisely at the shared corner of
	// four separately-built per-chunk ground_colliders (Unified Ground
	// Task 4's own LOD0 cell_size=30 tiling always has a chunk corner
	// at the world origin), and a car spawned exactly there gets
	// inconsistent per-wheel collision across those four independently
	// sampled heightfields - each wheel can end up on a different tile,
	// and on island's much steeper terrain (height_scale=150 vs. the
	// small tracks' own gentle 12) that showed up as a slow, ~1-second
	// partial freefall instead of a normal settle (the small tracks
	// never surfaced this - their own minimal relief made the same
	// corner seam small enough to be unnoticeable). A transient corner
	// crossing *while already moving* is not the same failure mode - a
	// car passes through a boundary in well under a car-length's worth
	// of travel time, nothing sits parked on the seam - so this is
	// fixed narrowly (don't spawn exactly on a chunk-grid corner)
	// rather than redesigning the whole per-chunk collider tiling to
	// overlap at edges.
	Vec3 spawn(5, terrain.HeightAt(5, 5) + 0.3, 5);
	vehicle.Create(pw.world, pw.space, veh, spawn);

	// City Layout phase: stream/generate road+block content around the
	// vehicle as it drives. track.id is the generator's world_seed (see
	// MapGeneratorCityLayoutGenerator.h) - the normal "changing track/
	// seed regenerates everything" case. The cache's seed_version is
	// track.id *plus* CITY_GEN_VERSION, not track.id alone: WorldChunkCache
	// has no way to know the generator's own algorithm/parameters changed
	// unless that's reflected in the key, and serving stale pre-change
	// content from a previous run is exactly what happened the first time
	// this wasn't done (see CITY_GEN_VERSION's comment).
	// Cell sizes are chosen relative to MapGeneratorCityLayoutGenerator's
	// street spacing (15m - see that file), not picked independently:
	// each cell is comfortably bigger than one street spacing (>=2x) so a
	// chunk contains more than a single lonely street ("kaupungin koko
	// suhteessa autoon vaikutti väärältä" - the reported city-vs-car
	// scale bug was street spacing scaling off the chunk's own size
	// instead of a fixed real-world meter value; this is the other half
	// of that fix - chunks now sized *for* a fixed street spacing,
	// instead of the street spacing being sized *for* arbitrary chunks).
	//
	// Retuned smaller than the earlier 90/140/200 (--worldscale
	// diagnostic, added to investigate "octree LOD doesn't seem to
	// stream with player position"): those distances were already
	// "tightened to fit Terrain::world_size instead of dwarfing it" per
	// an earlier fix, but even the *outermost* ring (200m) still
	// exceeded Terrain's own centre-to-corner radius (~141m for
	// world_size=200) - meaning the entire drivable area sat inside a
	// single LOD ring's radius the whole time, so every position (even
	// the spawn point) already saw all three LOD levels' content
	// simultaneously, and driving around barely changed that mix
	// (confirmed with --worldscale: 8/14/14/12/14/14/12/14 loaded
	// chunks across viewer distances 0-161m, essentially flat). Retuned
	// so the *outermost* ring (90m) sits safely inside Terrain's own
	// half-width (100m) - now driving from the centre toward the edge
	// crosses all three LOD boundaries for real, and which chunks are
	// loaded genuinely depends on where the vehicle is, confirmed the
	// same way (see AGENTS.md's own entry for the re-measured numbers).
	//
	// Cell sizes are also now exact integer multiples of levels[0]'s own
	// (30, 60=2x, 90=3x) - not just individually "comfortably bigger
	// than one street spacing" as before, but aligned so every coarser
	// chunk's own bounds line up exactly with a whole number of lod-0
	// cells and no partial/ambiguous overlap at the boundary. This
	// matters now that MapGeneratorCityLayoutGenerator derives a coarser
	// chunk's own content by assembling the lod-0 cells it covers (see
	// that file's own header comment - "same layout at every LOD",
	// following a real report that different LOD levels previously
	// showed unrelated, crossing street networks over the same area).
	city.Create(track.id + "_" + CITY_GEN_VERSION);
	Vector<WorldOctree::LodLevel> levels;
	{ WorldOctree::LodLevel l; l.cell_size = 30; l.load_distance = 30; levels.Add(l); }
	{ WorldOctree::LodLevel l; l.cell_size = 60; l.load_distance = 60; levels.Add(l); }
	{ WorldOctree::LodLevel l; l.cell_size = 90; l.load_distance = 90; levels.Add(l); }
	city.SetLodLevels(levels);
	// Terrain Erosion phase, Task 3: runs *first* (WorldBuilder::Update's
	// own ordering) - a no-op for the small hand-authored tracks
	// (ErosionChunkGenerator checks terrain.IsIsland() itself), so this
	// is always wired up rather than conditioned on the track kind here.
	city.SetErosionGenerator(One<ITerrainErosionGenerator>(new ErosionChunkGenerator(terrain)));
	// finest_cell_size must be levels[0]'s own cell_size (not a separate
	// number) - see MapGeneratorCityLayoutGenerator.h's own note on why.
	city.SetCityLayoutGenerator(One<ICityLayoutGenerator>(new MapGeneratorCityLayoutGenerator(track.id, levels[0].cell_size)));
	// Road Network phase, Task 4: runs after the city layout generator for
	// the same chunk (WorldBuilder::Update's own ordering), turning its
	// road_centerlines into a real mesh and flattening terrain under it
	// (Tasks 1-3). Bound to this PlayScreen's own terrain instance - see
	// RoadCreatorMeshGenerator.h's own note on why Terrain isn't owned by
	// WorldBuilder itself.
	city.SetRoadMeshGenerator(One<IRoadMeshGenerator>(new RoadCreatorMeshGenerator(terrain)));
	// Vegetation phase, Task 4: runs after the city layout generator too
	// (needs content.road_centerlines to avoid placing trees on roads -
	// see WorldBuilder::Update's own ordering). Same terrain-threading
	// pattern as RoadCreatorMeshGenerator just above.
	city.SetVegetationGenerator(One<IVegetationGenerator>(new VegChunkGenerator(track.id, terrain)));
	// Buildings phase, Task 3: runs after the city layout generator too
	// (needs content.blocks/content.road_centerlines - see WorldBuilder::
	// Update's own ordering). Same terrain-threading pattern as
	// RoadCreatorMeshGenerator/VegChunkGenerator above.
	city.SetBuildingGenerator(One<IBuildingGenerator>(new BuildingChunkGenerator(track.id, terrain)));
	// Unified Ground phase, Task 4: runs after the city layout/building
	// generators too (needs content.blocks - see WorldBuilder::Update's own
	// ordering). Same terrain-threading pattern as the other generators
	// above; this is now the *only* place Terrain's heightmap ends up
	// visible or collidable in the running game (via GroundMeshBuilder
	// reading Terrain::HeightAt), never through Terrain's own DrawGL/
	// BuildOdeHeightfield any more.
	city.SetGroundGenerator(One<IGroundGenerator>(new GroundChunkGenerator(terrain)));
	traffic.Create(pw.space, terrain, track.id);
	pedestrians.Create(pw.space, terrain, track.id);
	traffic_chunks.Clear();
	pedestrian_chunks.Clear();

	key_throttle = key_brake = key_left = key_right = false;
	debug_frames_painted = 0;
	running = true;
	SetTimeCallback(-16, THISBACK(Tick));
}

void PlayScreen::StopRun()
{
	if(!running)
		return;
	KillTimeCallback();
	for(int i = traffic_chunks.GetCount() - 1; i >= 0; i--)
		traffic.DespawnForChunk(traffic_chunks.GetKey(i));
	for(int i = pedestrian_chunks.GetCount() - 1; i >= 0; i--)
		pedestrians.DespawnForChunk(pedestrian_chunks.GetKey(i));
	traffic_chunks.Clear();
	pedestrian_chunks.Clear();

	// Map Editor phase, Task 9: must happen before this function returns
	// - StartRun's very next steps after calling StopRun() include
	// terrain.Load(track), which reassigns several of Terrain's own
	// fields with no lock at all (see WorldBuilder::Stop's own comment).
	// KillTimeCallback above only stops *this* thread from calling
	// city.Update() again; city's own background worker thread, once
	// started, runs independently of the timer and must be joined
	// explicitly.
	city.Stop();
	pw.Destroy(); // destroys the whole physics space, including every tree/building collider geom below
	veg_colliders.Clear(); // ...so this bookkeeping must be dropped too, or it would hold dangling PhysGeomIds
	building_colliders.Clear();
	ground_colliders.Clear();
	running = false;
}

void PlayScreen::Tick()
{
	if(!running)
		return;
	VehicleInput in;
	in.throttle = key_throttle ? 1.0 : 0.0;
	in.brake    = key_brake ? 1.0 : 0.0;
	in.steer    = (key_left ? -1.0 : 0.0) + (key_right ? 1.0 : 0.0);

	const double dt = 1.0 / 60;
	vehicle.Step(in, dt);
	pw.Step(dt);

	// Streams/generates city content around the vehicle every tick, same
	// rate as physics - Map Editor phase, Task 9: generation itself now
	// runs on city's own background worker thread (see
	// 012-map-editor/009-task9-async-chunk-regeneration.md), so this call
	// no longer hitches; a newly-entered chunk's content simply isn't
	// there yet for the first several ticks, same as a chunk this
	// PlayScreen has never visited before ever staying empty until its
	// first Update() call happens to land after the worker finishes it.
	Vector<WorldChunkCoord> newly_installed = city.Update(vehicle.GetChassisPosition());

	VectorMap<WorldChunkCoord, bool> still_loaded;
	for(int i = 0; i < city.GetOctree().GetLoadedChunkCount(); i++) {
		const WorldChunk& chunk = city.GetOctree().GetLoadedChunk(i);
		still_loaded.Add(chunk.coord, true);
	}

	for(WorldChunkCoord coord : newly_installed) {
		if(traffic_chunks.Find(coord) >= 0)
			traffic.DespawnForChunk(coord);
		if(pedestrian_chunks.Find(coord) >= 0)
			pedestrians.DespawnForChunk(coord);
		const WorldChunkContent* content = city.FindContent(coord);
		if(content) {
			traffic.SpawnForChunk(coord, *content);
			pedestrians.SpawnForChunk(coord, *content);
		}
		traffic_chunks.Add(coord, true);
		pedestrian_chunks.Add(coord, true);
	}
	for(int i = traffic_chunks.GetCount() - 1; i >= 0; i--) {
		if(still_loaded.Find(traffic_chunks.GetKey(i)) >= 0)
			continue;
		traffic.DespawnForChunk(traffic_chunks.GetKey(i));
		traffic_chunks.Remove(i);
	}
	for(int i = pedestrian_chunks.GetCount() - 1; i >= 0; i--) {
		if(still_loaded.Find(pedestrian_chunks.GetKey(i)) >= 0)
			continue;
		pedestrians.DespawnForChunk(pedestrian_chunks.GetKey(i));
		pedestrian_chunks.Remove(i);
	}
	traffic.Tick(dt);
	pedestrians.Tick(dt);

	// Map Editor phase, Task 9: a chunk can now appear in the octree's
	// own loaded set well before city.FindContent() has anything for it
	// (generation is in flight on the worker thread). The three
	// incremental collider lifecycles below all key off "have I already
	// built (possibly empty) colliders for this coord", so left alone
	// they would permanently freeze in the empty state they saw on the
	// very first tick a chunk loaded - never retried once real content
	// actually arrives. Fixed by dropping any existing collider entry
	// for a coord `Update()` just reports as newly installed (destroying
	// whatever - possibly empty, possibly rough pre-erosion for
	// ground_colliders - geoms it already built), so each lifecycle
	// block's own "not present yet -> build" branch naturally rebuilds
	// it from the now-final content a few lines below.
	for(WorldChunkCoord coord : newly_installed) {
		int vi = veg_colliders.Find(coord);
		if(vi >= 0) {
			for(PhysGeomId geom : veg_colliders[vi])
				PhysDestroyGeom(geom);
			veg_colliders.Remove(vi);
		}
		int bi = building_colliders.Find(coord);
		if(bi >= 0) {
			for(PhysGeomId geom : building_colliders[bi])
				PhysDestroyGeom(geom);
			building_colliders.Remove(bi);
		}
		int gi = ground_colliders.Find(coord);
		if(gi >= 0) {
			if(ground_colliders[gi])
				PhysDestroyGeom(ground_colliders[gi]);
			ground_colliders.Remove(gi);
		}
	}

	// Vegetation phase, Task 4: incremental per-chunk tree-collider
	// lifecycle - create colliders for chunks that just gained content,
	// destroy colliders for chunks that unloaded. Piggybacks on the same
	// loaded-chunk-coord set WorldBuilder::Update itself already prunes
	// `content` against, rather than tracking a second independent
	// notion of "which chunks are current". Deliberately NOT
	// Terrain::BuildOdeHeightfield's own "destroy everything, rebuild
	// from scratch" pattern (Road Network Task 4) - WorldOctree already
	// reports per-chunk load/unload individually here (unlike the one
	// shared heightfield), and a full rebuild of every tree geom on
	// every single chunk change would scale badly once per-chunk tree
	// counts add up.
	{
		VectorMap<WorldChunkCoord, bool> still_loaded;
		for(int i = 0; i < city.GetOctree().GetLoadedChunkCount(); i++) {
			const WorldChunk& chunk = city.GetOctree().GetLoadedChunk(i);
			still_loaded.Add(chunk.coord, true);
			if(veg_colliders.Find(chunk.coord) >= 0)
				continue; // already has colliders (or already known to have none)

			Vector<PhysGeomId> geoms;
			const WorldChunkContent* content = city.FindContent(chunk.coord);
			if(content)
				for(const VegTreeInstance& tree : content->vegetation) {
					PhysGeomId geom = PhysCreateCylinder(pw.space, tree.trunk_radius, tree.trunk_height);
					// PhysCreateCylinder's length axis defaults to local Z;
					// rotate it to stand upright along world Y. No body
					// (never moves - matches Terrain's own static
					// heightfield geom).
					PhysGeomSetRotationAxisAngle(geom, Vec3(1, 0, 0), -M_PI / 2);
					// Cylinder is centered on its own local origin, so
					// offset up by half its height to put the *base* at
					// the tree's own ground position.
					PhysGeomSetPosition(geom, Vec3(tree.position.x, tree.position.y + tree.trunk_height * 0.5, tree.position.z));
					geoms.Add(geom);
				}
			veg_colliders.Add(chunk.coord, pick(geoms));
		}
		for(int i = veg_colliders.GetCount() - 1; i >= 0; i--) {
			if(still_loaded.Find(veg_colliders.GetKey(i)) >= 0)
				continue;
			for(PhysGeomId geom : veg_colliders[i])
				PhysDestroyGeom(geom);
			veg_colliders.Remove(i);
		}
	}

	// Buildings phase, Task 3: identical incremental per-chunk collider
	// lifecycle as the veg_colliders block above, one static box collider
	// per building (never moves, matches Terrain's own static heightfield
	// geom and TrafficManager's own kinematic-not-dynamic car colliders).
	{
		VectorMap<WorldChunkCoord, bool> still_loaded;
		for(int i = 0; i < city.GetOctree().GetLoadedChunkCount(); i++) {
			const WorldChunk& chunk = city.GetOctree().GetLoadedChunk(i);
			still_loaded.Add(chunk.coord, true);
			if(building_colliders.Find(chunk.coord) >= 0)
				continue; // already has colliders (or already known to have none)

			Vector<PhysGeomId> geoms;
			const WorldChunkContent* content = city.FindContent(chunk.coord);
			if(content)
				for(const BuildingInstance& b : content->buildings) {
					PhysGeomId geom = PhysCreateBox(pw.space, b.width, b.height, b.depth);
					PhysGeomSetRotationAxisAngle(geom, Vec3(0, 1, 0), b.yaw);
					// Box is centered on its own local origin, so offset up by
					// half its height to put the *base* at the building's own
					// ground position (same convention as the tree cylinder
					// collider above).
					PhysGeomSetPosition(geom, Vec3(b.position.x, b.position.y + b.height * 0.5, b.position.z));
					geoms.Add(geom);
				}
			building_colliders.Add(chunk.coord, pick(geoms));
		}
		for(int i = building_colliders.GetCount() - 1; i >= 0; i--) {
			if(still_loaded.Find(building_colliders.GetKey(i)) >= 0)
				continue;
			for(PhysGeomId geom : building_colliders[i])
				PhysDestroyGeom(geom);
			building_colliders.Remove(i);
		}
	}

	// Unified Ground phase, Task 4: one heightfield collider per loaded
	// chunk, same incremental create-on-load/destroy-on-unload lifecycle as
	// veg_colliders/building_colliders above - replaces the old single
	// fixed-origin terrain.BuildOdeHeightfield() rebuild entirely (removed
	// above), so physics collision now covers every chunk the vehicle can
	// actually reach, not just Terrain's own original fixed footprint.
	// Samples the same Terrain::HeightAt every chunk's own ground mesh
	// vertices use (GroundMeshBuilder), so the collider stays height-
	// continuous with what's drawn even though it doesn't literally share
	// that mesh's own asphalt/grass-boundary triangulation - a coarse
	// per-chunk grid is enough for driving-game collision (no per-material
	// friction is modeled - see 004-task4-worldbuilder-integration.md's
	// "why not trimesh" note).
	{
		VectorMap<WorldChunkCoord, bool> still_loaded;
		for(int i = 0; i < city.GetOctree().GetLoadedChunkCount(); i++) {
			const WorldChunk& chunk = city.GetOctree().GetLoadedChunk(i);
			still_loaded.Add(chunk.coord, true);
			if(ground_colliders.Find(chunk.coord) >= 0)
				continue; // already has a collider (or already known to have none)
			ground_colliders.Add(chunk.coord, BuildGroundCollider(chunk));
		}
		for(int i = ground_colliders.GetCount() - 1; i >= 0; i--) {
			if(still_loaded.Find(ground_colliders.GetKey(i)) >= 0)
				continue;
			if(ground_colliders[i])
				PhysDestroyGeom(ground_colliders[i]);
			ground_colliders.Remove(i);
		}
	}

	Refresh();
}

bool PlayScreen::Key(dword key, int count)
{
	bool up = key & K_KEYUP;
	dword k = key & ~K_KEYUP;
	const KeyBindings& kb = GetKeyBindings();

	if(k == kb.throttle)    { key_throttle = !up; return true; }
	if(k == kb.brake)       { key_brake = !up; return true; }
	if(k == kb.steer_left)  { key_left = !up; return true; }
	if(k == kb.steer_right) { key_right = !up; return true; }
	if(k == kb.back) {
		if(!up)
			WhenBack();
		return true;
	}
	if(k == K_F2) {
		if(!up) {
			paint_mode = !paint_mode;
			LOG("PlayScreen: terrain paint mode " << (paint_mode ? "ON" : "OFF"));
		}
		return true;
	}
	return GLCtrl::Key(key, count);
}

bool PlayScreen::RaycastFromScreen(Point p, Vec3& hit_out) const
{
	Size sz = GetSize();
	if(sz.cx <= 0 || sz.cy <= 0)
		return false;

	// Must match GLPaint's own fixed chase camera exactly (eye/target/
	// up/fov/aspect) - see this task's own planning doc on why this is
	// rebuilt analytically from the vehicle's current position instead
	// of querying live GL matrices.
	Vec3 car = vehicle.GetChassisPosition();
	Vec3 eye = car + Vec3(0, 6, -12);
	Vec3 forward = (car - eye).Normalized();
	Vec3 world_up(0, 1, 0);
	Vec3 right = forward.Cross(world_up).Normalized();
	Vec3 up = right.Cross(forward).Normalized();

	double fov_y = 60.0 * (M_PI / 180.0);
	double tan_half = tan(fov_y * 0.5);
	double aspect = (double)sz.cx / sz.cy;
	double ndc_x = (2.0 * p.x / sz.cx) - 1.0;
	double ndc_y = 1.0 - (2.0 * p.y / sz.cy); // screen y grows downward, NDC y grows upward
	Vec3 ray_dir = (forward + right * (ndc_x * tan_half * aspect) + up * (ndc_y * tan_half)).Normalized();

	return terrain.RaycastHeightAt(eye, ray_dir, 400.0, hit_out);
}

void PlayScreen::PaintAt(Vec3 world_pos, bool lower)
{
	TerrainPaintBrush brush;
	brush.cx = world_pos.x;
	brush.cz = world_pos.z;
	brush.radius = brush_radius;
	brush.delta = lower ? -brush_strength : brush_strength;
	// Generous - an interactive tool silently rejecting a dab because
	// it happened to be a little steep isn't useful feedback yet;
	// surfacing fit_check failures in the UI is future work, not
	// attempted here.
	brush.slope_limit = 1000.0;
	AppendTerrainPaint(paint_graph, terrain, paint_base_cache, brush);
	CompileTerrainPaintToTerrain(paint_graph, terrain);
	InvalidateWorldChunksForPaint(paint_graph, city);
	// Map Editor phase, Task 7: without this, an already-loaded chunk's
	// live ODE ground collider stays built from the pre-edit heights
	// (PhysCreateHeightfield copies its data once, at build time) even
	// though Terrain::HeightAt and the rendered mesh both already
	// reflect the edit - see 012-map-editor/007-task7-collider-staleness.md.
	RebuildGroundCollidersNear(Vec2(world_pos.x, world_pos.z), Vec2(brush_radius, brush_radius));
	paint_drag_last = world_pos;
}

PhysGeomId PlayScreen::BuildGroundCollider(const WorldChunk& chunk)
{
	double world_size_x = chunk.half_extent.x * 2;
	double world_size_z = chunk.half_extent.z * 2;
	if(world_size_x <= 0 || world_size_z <= 0)
		return 0;
	const int kGridRes = 9; // samples per axis - coarse and cheap; only needs to match Terrain's own height function, not the visual ground mesh's triangle density
	Vector<float> heights;
	heights.SetCount(kGridRes * kGridRes);
	for(int gz = 0; gz < kGridRes; gz++)
		for(int gx = 0; gx < kGridRes; gx++) {
			double wx = chunk.center.x - chunk.half_extent.x + world_size_x * gx / (kGridRes - 1);
			double wz = chunk.center.z - chunk.half_extent.z + world_size_z * gz / (kGridRes - 1);
			heights[gz * kGridRes + gx] = (float)terrain.HeightAt(wx, wz);
		}
	PhysGeomId geom = PhysCreateHeightfield(pw.space, heights.Begin(), kGridRes, kGridRes, world_size_x, world_size_z, 1.0, 0.0);
	PhysGeomSetPosition(geom, Vec3(chunk.center.x, 0, chunk.center.z));
	return geom;
}

void PlayScreen::RebuildGroundCollidersNear(Vec2 center, Vec2 half_extent)
{
	for(int i = 0; i < ground_colliders.GetCount(); i++) {
		const WorldChunkCoord& coord = ground_colliders.GetKey(i);
		const WorldChunk* chunk = city.GetOctree().FindChunk(coord);
		if(!chunk)
			continue; // defensive - Tick() keeps ground_colliders in sync with loaded chunks every frame
		if(fabs(chunk->center.x - center.x) > chunk->half_extent.x + half_extent.x)
			continue;
		if(fabs(chunk->center.z - center.y) > chunk->half_extent.z + half_extent.y)
			continue;
		if(ground_colliders[i])
			PhysDestroyGeom(ground_colliders[i]);
		ground_colliders[i] = BuildGroundCollider(*chunk);
	}
}

PhysGeomId PlayScreen::FindGroundColliderAt(Vec2 world_pos) const
{
	PhysGeomId best = 0;
	int best_lod = 0x7fffffff;
	for(int i = 0; i < city.GetOctree().GetLoadedChunkCount(); i++) {
		const WorldChunk& chunk = city.GetOctree().GetLoadedChunk(i);
		if(fabs(chunk.center.x - world_pos.x) > chunk.half_extent.x)
			continue;
		if(fabs(chunk.center.z - world_pos.y) > chunk.half_extent.z)
			continue;
		if(chunk.coord.lod >= best_lod)
			continue;
		int gi = ground_colliders.Find(chunk.coord);
		if(gi < 0)
			continue;
		best = ground_colliders[gi];
		best_lod = chunk.coord.lod;
	}
	return best;
}

void PlayScreen::MouseMove(Point p, dword keyflags)
{
	if(paint_mode)
		paint_hit_valid = RaycastFromScreen(p, paint_hit);
	GLCtrl::MouseMove(p, keyflags);
}

void PlayScreen::LeftDown(Point p, dword keyflags)
{
	if(paint_mode) {
		Vec3 hit;
		if(RaycastFromScreen(p, hit)) {
			paint_hit_valid = true;
			paint_hit = hit;
			PaintAt(hit, keyflags & K_SHIFT);
		}
		return;
	}
	GLCtrl::LeftDown(p, keyflags);
}

void PlayScreen::LeftDrag(Point p, dword keyflags)
{
	if(paint_mode) {
		Vec3 hit;
		if(RaycastFromScreen(p, hit)) {
			paint_hit_valid = true;
			paint_hit = hit;
			if((hit - paint_drag_last).Length() >= brush_radius * 0.5)
				PaintAt(hit, keyflags & K_SHIFT);
		}
		return;
	}
	GLCtrl::LeftDrag(p, keyflags);
}

void PlayScreen::MouseWheel(Point p, int zdelta, dword keyflags)
{
	if(paint_mode) {
		// Sign only, one fixed step per callback - see this task's own
		// planning doc on why sub-notch precision isn't worth it here.
		int dir = zdelta > 0 ? 1 : zdelta < 0 ? -1 : 0;
		if(dir != 0) {
			if(keyflags & K_CTRL) {
				brush_strength = Clampd(brush_strength + dir * kBrushStrengthStep, kMinBrushStrength, kMaxBrushStrength);
				LOG("PlayScreen: brush strength = " << brush_strength);
			}
			else {
				brush_radius = Clampd(brush_radius + dir * kBrushRadiusStep, kMinBrushRadius, kMaxBrushRadius);
				LOG("PlayScreen: brush radius = " << brush_radius);
			}
		}
		return;
	}
	GLCtrl::MouseWheel(p, zdelta, keyflags);
}

// Every draw call of one frame, in one place: the world, the traffic, the
// pedestrians and the player's own car. Split out of `GLPaint` so `--pedpic`
// can render the *same* frame twice - once with the skinned character and
// once with the flat figure it replaces - and diff the two pictures. Two
// runs would not do: the car has driven on between them.
void PlayScreen::DrawSceneGL(const Vec3& eye)
{
	DrawGroundGL(city);
	DrawCityGL(city, terrain);
	DrawVegetationGL(city, leaf_tex);
	DrawBuildingsGL(city);
	traffic.DrawGL();
	pedestrians.DrawGL(eye);

	// Island Terrain phase, Task 3+4: drawn after every opaque ground/
	// city/vegetation/building pass above (translucency needs whatever
	// it blends over already in the color/depth buffer) and only for
	// the island track - the small hand-authored tracks aren't
	// ocean-shaped and don't have a real water_level concept.
	if(terrain.IsIsland())
		DrawWaterGL(eye, terrain.GetWaterLevel(), 600.0, Color(60, 110, 180), 200);

	// Map Editor phase, Task 6: ground-hugging brush-ring preview - the
	// user's only feedback for where a dab will land and how big it is
	// before committing one. Drawn after water so it's never hidden
	// under the translucent blend.
	if(paint_mode && paint_hit_valid) {
		glDisable(GL_LIGHTING);
		glColor3ub(255, 200, 0);
		const int segs = 32;
		const double ring_y_offset = 0.15;
		glBegin(GL_LINE_LOOP);
		for(int i = 0; i < segs; i++) {
			double a = 2 * M_PI * i / segs;
			double wx = paint_hit.x + cos(a) * brush_radius;
			double wz = paint_hit.z + sin(a) * brush_radius;
			glVertex3d(wx, terrain.HeightAt(wx, wz) + ring_y_offset, wz);
		}
		glEnd();
	}

	glPushMatrix();
	double chassis_m[16];
	PhysBodyGetGLMatrix(vehicle.GetChassisBody(), chassis_m);
	glMultMatrixd(chassis_m);

	// Two-part body, same local layout Vehicle::Create used for mass/geom:
	// body-local y=0 is part 1's (heavy tub) bottom, part 2 (light shell,
	// wedge-shaped) stacks directly on top of it - each its own color.
	const VehicleDef& vdef = vehicle.GetDef();
	double part1_height = vdef.chassis_part1_height;
	double part2_height = vdef.chassis_part2_height;
	double part1_center_y = part1_height * 0.5;
	double part1_top_y = part1_height;
	double part2_top_front_y = part1_top_y + part2_height * 0.35; // low nose
	double part2_top_rear_y = part1_top_y + part2_height;         // full-height cabin/roof at the back
	Vec3 csz = vehicle.GetChassisSize(); // x/z footprint (y is part1+part2, not needed here)

	glColor3ub(150, 40, 30); // part 1: darker rust-red structural tub
	glPushMatrix();
		glTranslated(0, part1_center_y, 0);
		DrawUnitBox(csz.x, part1_height, csz.z);
	glPopMatrix();

	glColor3ub(225, 190, 60); // part 2: lighter sand-yellow shell
	DrawWedgeShell(csz.x, csz.z, part1_top_y, part2_top_front_y, part2_top_rear_y);

	glPopMatrix();

	for(int i = 0; i < Vehicle::WHEEL_COUNT; i++) {
		glPushMatrix();
		double wheel_m[16];
		PhysBodyGetGLMatrix(vehicle.GetWheelBody(i), wheel_m);
		glMultMatrixd(wheel_m);
		glColor3ub(25, 25, 25);
		DrawWheelCylinder(vehicle.GetWheelRadius(), vehicle.GetWheelWidth());
		glPopMatrix();
	}

}

void PlayScreen::GLPaint()
{
	debug_frames_painted++;
	Size sz = GetSize();
	bool dump = debug_frames_painted <= 3; // first few frames only - see AGENTS.md
	if(dump)
		LOG("PlayScreen::GLPaint: frame=" << debug_frames_painted
		    << " widget_size=" << sz.cx << "x" << sz.cy << " running=" << running);

	if(sz.cx <= 0 || sz.cy <= 0 || !running) {
		if(dump)
			LOG("PlayScreen::GLPaint: skipped (zero size or not running) - "
			    "zero size means this Ctrl was never laid out by its parent");
		return;
	}

	// Unified Ground phase, Task 1: lazily created (needs a real GL
	// context, which doesn't exist earlier than this) and only ever
	// once - every self-test/diagnostic mode that never opens a real
	// window never reaches GLPaint at all, so this never runs headless.
	if(!leaf_tex[0]) {
		leaf_tex[(int)VegTreePreset::Oak] = LoadGLTexture("share/sweetsandy/textures/leaves/oak.png");
		leaf_tex[(int)VegTreePreset::Pine] = LoadGLTexture("share/sweetsandy/textures/leaves/pine.png");
	}

	glShadeModel(GL_SMOOTH);
	glClearColor(0.55f, 0.75f, 0.95f, 1.0f);
	glClearDepth(1.0);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	GLfloat light_pos[4] = { 0.3f, 1.0f, 0.4f, 0.0f };
	glLightfv(GL_LIGHT0, GL_POSITION, light_pos);

	glViewport(0, 0, (GLsizei)sz.cx, (GLsizei)sz.cy);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, (double)sz.cx / sz.cy, 0.5, 500.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Fixed offset chase camera behind and above the chassis. Does not yet
	// track chassis yaw - good enough to see the suspension/driving demo;
	// a heading-following camera is a natural Task 6 follow-up.
	Vec3 car = vehicle.GetChassisPosition();
	Vec3 eye = car + Vec3(0, 6, -12);
	gluLookAt(eye.x, eye.y, eye.z, car.x, car.y, car.z, 0, 1, 0);

	if(dump) {
		LOG("PlayScreen::GLPaint: camera eye=(" << eye.x << "," << eye.y << "," << eye.z
		    << ") target(chassis)=(" << car.x << "," << car.y << "," << car.z << ")");
		GLLogProjectedPoint("chassis center", car.x, car.y, car.z);
		Vec3 gz = Vec3(0, terrain.HeightAt(0, 0), 0);
		GLLogProjectedPoint("terrain origin", gz.x, gz.y, gz.z);

		int road_count = 0, block_count = 0, building_count = 0, ground_mesh_count = 0;
		for(int i = 0; i < city.GetOctree().GetLoadedChunkCount(); i++) {
			const WorldChunkContent* c = city.FindContent(city.GetOctree().GetLoadedChunk(i).coord);
			if(c) { road_count += c->road_centerlines.GetCount(); block_count += c->blocks.GetCount(); building_count += c->buildings.GetCount(); ground_mesh_count += c->ground.GetCount(); }
		}
		LOG("PlayScreen::GLPaint: city loaded_chunks=" << city.GetOctree().GetLoadedChunkCount()
		    << " roads=" << road_count << " blocks=" << block_count << " buildings=" << building_count
		    << " ground_meshes=" << ground_mesh_count);
	}

	DrawSceneGL(eye);

	if(dump) {
		glFinish();
		GLFrameStats st = GLReadFrameStats(sz.cx, sz.cy, MakeRGBA(140, 191, 242), 20);
		GLLogFrameStats("PlayScreen", st);
	}

	// The on-screen size of the nearest real character, measured rather than
	// eyeballed. A pedestrian that is drawn at the wrong scale, floating off
	// the ground or off screen all show up here as numbers, which is the same
	// evidence a screenshot gives - and it survives a headless run.
	if(pending_ped_check) {
		pending_ped_check = false;
		String ped_on_path = pending_ped_onpath;
		pending_ped_onpath.Clear();
		GLdouble mv[16], pr[16];
		GLint vp[4];
		glGetDoublev(GL_MODELVIEW_MATRIX, mv);
		glGetDoublev(GL_PROJECTION_MATRIX, pr);
		glGetIntegerv(GL_VIEWPORT, vp);
		double height_m = pedestrians.GetPedHeightMeters();
		int onscreen_count = 0, best = -1;
		double best_d = 0, best_px = 0, best_feet[2] = {0, 0}, best_head[2] = {0, 0};
		double best_gx = 0, best_gy = 0, best_gz = 0, best_heading = 0, best_depth = 0;
		for(int i = 0; i < pedestrians.GetPedestrianCount(); i++) {
			const Pedestrian& ped = pedestrians.GetPedestrian(i);
			double gx = ped.follower.position.x, gz = ped.follower.position.y;
			double gy = terrain.HeightAt(gx, gz);
			GLdouble feet[3], head[3];
			gluProject(gx, gy, gz, mv, pr, vp, &feet[0], &feet[1], &feet[2]);
			gluProject(gx, gy + height_m, gz, mv, pr, vp, &head[0], &head[1], &head[2]);
			// "On screen" is what a person would say about the figure: inside
			// the viewport and in front of the camera. The nearest pedestrian is
			// very often behind the chase camera, so picking it blind would
			// report a figure that is not in the picture at all.
			if(head[2] <= 0 || head[2] >= 1) continue;
			if(head[0] < vp[0] || head[0] > vp[0] + vp[2] ||
			   feet[1] < vp[1] || feet[1] > vp[1] + vp[3]) continue;
			onscreen_count++;
			double d = sqrt((gx - eye.x) * (gx - eye.x) + (gy - eye.y) * (gy - eye.y) +
			                (gz - eye.z) * (gz - eye.z));
			if(best < 0 || d < best_d) {
				best = i;
				best_d = d;
				best_px = head[1] - feet[1]; // GL window y grows upward, so the head is the higher number
				best_feet[0] = feet[0]; best_feet[1] = feet[1];
				best_head[0] = head[0]; best_head[1] = head[1];
				best_gx = gx; best_gy = gy; best_gz = gz;
				best_heading = ped.follower.heading;
				best_depth = head[2];
			}
		}
		// The in-frame A/B: capture the frame with the character drawn, then
		// redraw *only the pedestrians* with the flat figure and capture that.
		// Same matrices, same frame, same everything else, so a pixel diff of
		// the two is exactly the character and nothing else. (The two frames
		// two runs produce are not comparable - the car has driven on.)
		if(!ped_on_path.IsEmpty()) {
			String off_path = ped_on_path + ".off.png";
			String on_path = ped_on_path + ".on.png";
			ped_on_path.Clear();
			// Each capture is a **complete** re-render of the same frame, not
			// an extra draw on top of it: drawing the flat figure over the
			// character that is already in the buffer would leave the
			// character visible in both pictures and the diff would show only
			// the lines.
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			DrawSceneGL(eye);
			glFinish();
			GLSaveFramebufferPNG(vp[2], vp[3], on_path);
			pedestrians.SetDrawCharacter(false);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			DrawSceneGL(eye);
			glFinish();
			GLSaveFramebufferPNG(vp[2], vp[3], off_path);
			pedestrians.SetDrawCharacter(true);
			LOG("PedPic: wrote the in-frame pair " << on_path << " / " << off_path
			    << " - diff them and the character is everything that moved");
		}

		if(best < 0) {
			LOG("PedScreenCheck: " << onscreen_count << " pedestrians on screen - none of the "
			    << pedestrians.GetPedestrianCount() << " is in view, so nothing to measure");
		} else {
			// The pixels a 1.7 m figure must occupy at this distance, from the
			// projection itself: 2*tan(fov/2)*d is the world height the
			// viewport covers at that depth. A character drawn at the wrong
			// scale, or off the ground, or inside out, misses this by a lot.
			double expected_px = height_m * vp[3] / (2.0 * 0.5773502692 * best_d);
			LOG("PedScreenCheck: " << onscreen_count << " of " << pedestrians.GetPedestrianCount()
			    << " pedestrians on screen; nearest is #" << best << " at " << Format("%.1f", best_d)
			    << "m, world(" << Format("%.1f", best_gx) << "," << Format("%.1f", best_gy) << ","
			    << Format("%.1f", best_gz) << ") heading " << Format("%.0f", best_heading * 180.0 / M_PI)
			    << "deg -> screen feet(" << Format("%.0f,%.0f", best_feet[0], best_feet[1])
			    << ") head(" << Format("%.0f,%.0f", best_head[0], best_head[1]) << ") -> the figure spans "
			    << Format("%.1f", best_px) << "px, where a standing " << Format("%.2f", height_m)
			    << "m figure spans " << Format("%.1f", expected_px) << "px at that distance (ratio "
			    << Format("%.3f", best_px / expected_px) << " - the idle clip crouches and bobs, so this "
			    << "is above 1 by design), depth " << Format("%.3f", best_depth));
		}
	}

	if(!pending_framebuffer_dump.IsEmpty()) {
		glFinish();
		GLSaveFramebufferPNG(sz.cx, sz.cy, pending_framebuffer_dump);
		pending_framebuffer_dump.Clear();
	}
}

END_UPP_NAMESPACE
