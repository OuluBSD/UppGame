#include "SweetSandy.h"

NAMESPACE_UPP

// Smoothstep-interpolated ramp between two heights over [z0,z1] - avoids the
// sharp kinks a plain linear ramp would leave at each segment boundary
// (those show up as small jolts when driving over them).
static double SmoothRamp(double z, double z0, double z1, double h0, double h1)
{
	double t = Clampd((z - z0) / (z1 - z0), 0.0, 1.0);
	t = t * t * (3 - 2 * t);
	return h0 + (h1 - h0) * t;
}

// A hand-laid-out course along +z: start flat, a small bump, a bigger bump,
// then a launch ramp into a real jump (a lip followed by a steep drop into a
// landing pit, so a car with enough speed leaves the ground before the
// descending slope catches up with it), then climb back out. Constant across
// x (full-width ramps), so it reads as "drive straight ahead over jumps and
// height differences" per the request - independent of def.height_scale,
// since these are already real meters, picked to suit SandDummy's scale
// (0.55m wheel radius, 1.4m ride height).
static double ObstacleCourseHeight(double world_z)
{
	double z = world_z;
	if(z < 8)   return 0;
	if(z < 18)  return SmoothRamp(z, 8, 18, 0, 0.8);       // bump 1 up (~4.6 degrees)
	if(z < 24)  return 0.8;                                 // bump 1 top
	if(z < 34)  return SmoothRamp(z, 24, 34, 0.8, 0);      // bump 1 down
	if(z < 46)  return 0;                                   // flat run-up (room to build speed)
	if(z < 61)  return SmoothRamp(z, 46, 61, 0, 2.0);      // launch ramp up (~7.6 degrees, gentle)
	if(z < 65)  return 2.0;                                 // launch lip
	if(z < 72)  return SmoothRamp(z, 65, 72, 2.0, -0.3);   // drop - the jump (softer landing than before)
	if(z < 81)  return -0.3;                                // landing pit floor
	if(z < 93)  return SmoothRamp(z, 81, 93, -0.3, 0);     // gentle climb out of the pit
	return 0;                                               // flat run-out
}

// Signed area x2 - sign tells which side of a->b point p is on. Used for
// point-in-triangle/quad tests below (Road Network phase, Task 2).
static double Cross2(Vec2 a, Vec2 b, Vec2 p)
{
	return (b - a).Cross(p - a);
}

static bool PointInTriangle(Vec2 p, Vec2 a, Vec2 b, Vec2 c)
{
	double d1 = Cross2(a, b, p);
	double d2 = Cross2(b, c, p);
	double d3 = Cross2(c, a, p);
	bool has_neg = d1 < 0 || d2 < 0 || d3 < 0;
	bool has_pos = d1 > 0 || d2 > 0 || d3 > 0;
	return !(has_neg && has_pos);
}

// Point-in-quad via the a-c diagonal - works for the mildly non-convex
// quads a sharp turn's ribbon segment can produce, not just convex ones.
// Corners must be given in boundary order (a->b->c->d->a), matching how
// RoadMeshBuilder pairs left/right vertices per centerline point.
static bool PointInQuad(Vec2 p, Vec2 a, Vec2 b, Vec2 c, Vec2 d)
{
	return PointInTriangle(p, a, b, c) || PointInTriangle(p, a, c, d);
}

double Terrain::HeightAtCell(int gx, int gz) const
{
	gx = Clampi(gx, 0, grid_w - 1);
	gz = Clampi(gz, 0, grid_d - 1);
	return height[gz * grid_w + gx];
}

void Terrain::Load(const TrackDef& def)
{
	// A fresh Load() means any earlier hf_geom's space is presumed gone
	// (see hf_geom's own comment in Terrain.h) - forget the stale ID
	// without touching the physics backend, never PhysDestroyGeom it here.
	hf_geom = 0;
	island.Clear();
	loaded_erosion_tiles.Clear();

	world_size = def.world_size;
	height_scale = max(1.0, def.height_scale);

	if(def.kind == TERRAIN_ISLAND) {
		// Island Terrain phase, Task 4: no baked grid at all - island's
		// own HeightAt/NormalAt is a pure function, safely evaluable
		// anywhere, which is the entire point (see `island`'s own
		// comment in Terrain.h). Seeded from the track id, same
		// convention every other seeded generator in this project
		// already uses (String::GetHashValue()).
		IslandTerrainParams params;
		params.island_radius = def.world_size * 0.5;
		params.height_scale = def.height_scale; // not clamped to the >=1.0 floor the baked-grid path uses above
		island_seed = (dword)def.id.GetHashValue();
		island.Create(island_seed, params);
		grid_w = grid_d = 0;
		height.Clear();
		cell_size = 1;
		// Terrain Erosion phase, Task 3: same seed_version convention
		// WorldChunkCache/ErosionTileCache's own callers already use
		// elsewhere (track id, not combined with a CITY_GEN_VERSION-style
		// suffix here - this cache's own on-disk shape hasn't changed
		// since Task 1, nothing to invalidate yet).
		erosion_cache.Open(def.id + "_" + EROSION_GEN_VERSION);
		LOG("Terrain::Load: island mode, seed='" << def.id << "' island_radius=" << params.island_radius);
		return;
	}

	Image img = StreamRaster::LoadFileAny(def.heightmap_path);
	if(img.GetSize().cx >= 2 && img.GetSize().cy >= 2) {
		grid_w = img.GetSize().cx;
		grid_d = img.GetSize().cy;
		height.SetCount(grid_w * grid_d);
		for(int z = 0; z < grid_d; z++) {
			const RGBA *row = img[z];
			for(int x = 0; x < grid_w; x++) {
				double gray = (row[x].r + row[x].g + row[x].b) / (3.0 * 255.0);
				height[z * grid_w + x] = (float)(gray * def.height_scale);
			}
		}
	}
	else if(def.kind == TERRAIN_OBSTACLE_COURSE) {
		LOG("Terrain::Load: heightmap '" << def.heightmap_path
		    << "' not found, generating procedural obstacle course");
		// Finer grid than the rolling terrain: ramps as short as 3-8m need
		// more than one sample to keep their shape and collide cleanly (a
		// coarse grid here would round the jump ramp into a blocky
		// staircase).
		grid_w = grid_d = 129;
		height.SetCount(grid_w * grid_d);
		height_scale = 4.0; // for TerrainColor's normalization only; see ObstacleCourseHeight
		double half = world_size * 0.5;
		for(int z = 0; z < grid_d; z++) {
			double world_z = (double)z / (grid_d - 1) * world_size - half;
			float h = (float)ObstacleCourseHeight(world_z);
			for(int x = 0; x < grid_w; x++)
				height[z * grid_w + x] = h;
		}
	}
	else {
		// Deterministic procedural fallback (no heightmap asset shipped yet
		// in this phase) - gentle rolling bumps, never fails to start.
		LOG("Terrain::Load: heightmap '" << def.heightmap_path
		    << "' not found, generating procedural rolling terrain");
		grid_w = grid_d = 65;
		height.SetCount(grid_w * grid_d);
		for(int z = 0; z < grid_d; z++) {
			double v = (double)z / (grid_d - 1);
			for(int x = 0; x < grid_w; x++) {
				double u = (double)x / (grid_w - 1);
				double h = 0.5 + 0.5 * sin(u * 9.0) * cos(v * 7.0)
				         + 0.25 * sin((u + v) * 17.0);
				h = Clampd((h + 1.75) / 3.5, 0.0, 1.0);
				height[z * grid_w + x] = (float)(h * def.height_scale);
			}
		}
	}
	cell_size = world_size / (grid_w - 1);
}

double Terrain::GetWaterLevel() const
{
	return island ? island->GetWaterLevel() : 0.0;
}

namespace {
// Terrain Erosion phase, Task 3: retuned from HydraulicErosionParams'
// own defaults (which mirror the reference's own Unity-demo values,
// implicitly assuming a heightmap normalized to a small range) for
// this project's real absolute-meter terrain (IslandTerrain's own
// height_scale, up to 150) - see 011-terrain-erosion/
// 003-task3-pipeline-wiring.md's own note, and this task's own report
// for the roughness-reduction measurement this value was chosen
// against, not just derived on paper.
HydraulicErosionParams IslandErosionParams()
{
	HydraulicErosionParams p;
	p.gravity = 0.2; // reference default 4.0 - see the comment above
	return p;
}

const int kErosionDropletsPerTile = 50000; // ~1.5s/tile per Task 2's own measured rate - see this task's own report

// Terrain Erosion phase, Task 4: each tile is eroded independently - a
// droplet's own path never crosses into a neighboring tile's own
// separate grid. Before erosion, adjacent tiles agree exactly at their
// shared edge (both sample the identical deterministic
// IslandTerrain::HeightAt there) - but erosion then moves material
// around *within* each tile with zero knowledge of what the
// neighboring tile's own droplets do at the very same boundary line,
// so post-erosion the two tiles' own edges can end up genuinely
// different heights - a real, physical cliff at every tile boundary,
// not just a cosmetic shading seam. Found the hard way: a real vehicle
// spawned near the origin (which sits on a tile-grid corner, same as
// the physics-chunk grid Island Terrain Task 4 already had to work
// around) rolled over in 0.14m instead of clearing 500m - see this
// task's own report. Fixed by blending the eroded result back toward
// the *raw* (pre-erosion) height within a margin of each tile's own
// edge, smoothstepped to exactly 0 right at the edge - since the raw
// function is the one thing guaranteed identical on both sides of any
// shared boundary, this guarantees two adjacent tiles meet at exactly
// the same height, eliminating the cliff (at some cost to how far
// erosion's own smoothing reaches toward each tile's own edge).
const int kErosionEdgeBlendMarginCells = 24; // ~192m at 8m/cell

void BlendErosionTileEdgeTowardRaw(Vector<float>& eroded, const Vector<float>& raw, int res)
{
	for(int gz = 0; gz < res; gz++)
		for(int gx = 0; gx < res; gx++) {
			int dist = min(min(gx, res - 1 - gx), min(gz, res - 1 - gz));
			if(dist >= kErosionEdgeBlendMarginCells)
				continue;
			double t = (double)dist / kErosionEdgeBlendMarginCells; // 0 exactly at the edge, 1 at the margin's own inner boundary
			t = t * t * (3 - 2 * t); // smoothstep - continuous first derivative too, not just continuous value
			int i = gz * res + gx;
			eroded[i] = (float)(raw[i] * (1 - t) + eroded[i] * t);
		}
}
}

void Terrain::EnsureErosionTile(ErosionTileCoord coord)
{
	if(!island)
		return;
	{
		RWMutex::ReadLock __(height_lock);
		if(loaded_erosion_tiles.Find(coord) >= 0)
			return; // already loaded - the fast, common path
	}

	// Map Editor phase, Task 9: the expensive part - deliberately
	// outside any lock. This is a purely local computation (touches no
	// shared Terrain state until the very end), and holding height_lock
	// across a multi-hundred-ms-to-low-seconds bake would block every
	// other reader (the main thread's own rendering/physics/picking
	// included) for that whole duration - exactly the stall this task
	// exists to move off the main thread, not relocate into a lock.
	ErosionTile tile;
	if(!erosion_cache.Load(coord, tile)) {
		// Logged unconditionally (not just under some verbose flag) -
		// a real, multi-hundred-millisecond-to-low-seconds computation
		// the first time any given tile is ever touched, worth always
		// being visible in the log. No longer a *synchronous* hitch as
		// of this task - see EnsureErosionTile's own callers.
		int64 t0 = msecs();
		tile = BakeErosionTile(*island, coord);
		// Vector<T>'s own copy constructor/assignment is deleted - a
		// plain element-wise copy instead (pre-erosion baseline, see
		// BlendErosionTileEdgeTowardRaw's own comment on why it's needed).
		Vector<float> raw_height;
		raw_height.SetCount(tile.height.GetCount());
		for(int i = 0; i < tile.height.GetCount(); i++)
			raw_height[i] = tile.height[i];
		ErodeHeightmap(tile.height, kErosionTileGridRes, kErosionDropletsPerTile,
		               IslandErosionParams(), CombineHash(island_seed, coord.x, coord.z));
		BlendErosionTileEdgeTowardRaw(tile.height, raw_height, kErosionTileGridRes);
		erosion_cache.Save(coord, tile);
		LOG("Terrain::EnsureErosionTile: baked+eroded+cached tile (" << coord.x << "," << coord.z
		    << ") in " << (msecs() - t0) << "ms");
	}

	// Install under a short write lock - re-checked first in case
	// another caller (the background worker's own erosion_gen and the
	// main thread's own ApplyPaintedHeight can both first-touch a tile
	// independently) already installed the same coordinate while this
	// one was baking; a real but rare, safe-to-lose race - the loser's
	// own redundant bake is simply discarded.
	RWMutex::WriteLock __(height_lock);
	if(loaded_erosion_tiles.Find(coord) < 0)
		loaded_erosion_tiles.Add(coord, pick(tile));
}

void Terrain::EnsureErosionCoverage(Vec2 center, Vec2 half_extent)
{
	if(!island)
		return;
	Vec2 corners[4] = {
		Vec2(center.x - half_extent.x, center.y - half_extent.y),
		Vec2(center.x + half_extent.x, center.y - half_extent.y),
		Vec2(center.x - half_extent.x, center.y + half_extent.y),
		Vec2(center.x + half_extent.x, center.y + half_extent.y),
	};
	for(Vec2 c : corners)
		EnsureErosionTile(ErosionTileCoordFor(c));
}

double Terrain::HeightAt(double world_x, double world_z) const
{
	// Map Editor phase, Task 9: spans this whole function (both
	// branches) - the background world-gen worker thread and the main
	// thread both call this constantly, concurrently with each other
	// and with EnsureErosionTile/ApplyPaintedHeight/FlattenUnderRibbon
	// (which write the same state from either thread) - see
	// height_lock's own comment in Terrain.h.
	RWMutex::ReadLock __(height_lock);

	if(island) {
		// Terrain Erosion phase, Task 3: prefer a loaded eroded tile over
		// island's own raw function wherever one covers this point - an
		// in-memory VectorMap lookup, cheap enough for this hot path
		// (unlike a disk-cache check or, worse, baking+eroding a tile on
		// demand from inside HeightAt itself - see EnsureErosionCoverage,
		// which a pipeline stage calls once per chunk *before* anything
		// samples HeightAt for it).
		int i = loaded_erosion_tiles.Find(ErosionTileCoordFor(Vec2(world_x, world_z)));
		if(i >= 0)
			return loaded_erosion_tiles[i].HeightAt(world_x, world_z);
		return island->HeightAt(world_x, world_z);
	}

	double gx = (world_x + world_size * 0.5) / cell_size;
	double gz = (world_z + world_size * 0.5) / cell_size;
	int ix0 = Clampi((int)floor(gx), 0, grid_w - 2);
	int iz0 = Clampi((int)floor(gz), 0, grid_d - 2);
	double fx = Clampd(gx - ix0, 0.0, 1.0);
	double fz = Clampd(gz - iz0, 0.0, 1.0);

	double h00 = HeightAtCell(ix0,     iz0);
	double h10 = HeightAtCell(ix0 + 1, iz0);
	double h01 = HeightAtCell(ix0,     iz0 + 1);
	double h11 = HeightAtCell(ix0 + 1, iz0 + 1);

	double h0 = h00 * (1 - fx) + h10 * fx;
	double h1 = h01 * (1 - fx) + h11 * fx;
	return h0 * (1 - fz) + h1 * fz;
}

Vec3 Terrain::NormalAt(double world_x, double world_z) const
{
	double e = cell_size * 0.5;
	double hL = HeightAt(world_x - e, world_z);
	double hR = HeightAt(world_x + e, world_z);
	double hD = HeightAt(world_x, world_z - e);
	double hU = HeightAt(world_x, world_z + e);
	return Vec3(hL - hR, 2 * e, hD - hU).Normalized();
}

static Color TerrainColor(double height01, double slope)
{
	// slope: 0 = flat, 1 = vertical. Low+flat -> sand, mid -> grass, steep/high -> rock.
	int sand_g  = 200, sand_b  = 150;
	int grass_g = 150, grass_b = 60;
	int rock    = 120;
	double t = Clampd(height01, 0.0, 1.0);
	int r = (int)(220 * (1 - t) + 90 * t);
	int g = (int)(sand_g * (1 - t) + grass_g * t);
	int b = (int)(sand_b * (1 - t) + grass_b * t);
	double s = Clampd(slope * 2.0, 0.0, 1.0);
	r = (int)(r * (1 - s) + rock * s);
	g = (int)(g * (1 - s) + rock * s);
	b = (int)(b * (1 - s) + rock * s);
	return Color(r, g, b);
}

void Terrain::DrawGL() const
{
	if(height.IsEmpty())
		return;
	double half = world_size * 0.5;
	glBegin(GL_TRIANGLES);
	for(int gz = 0; gz < grid_d - 1; gz++) {
		for(int gx = 0; gx < grid_w - 1; gx++) {
			double x0 = gx * cell_size - half,       x1 = (gx + 1) * cell_size - half;
			double z0 = gz * cell_size - half,       z1 = (gz + 1) * cell_size - half;
			double y00 = HeightAtCell(gx, gz),       y10 = HeightAtCell(gx + 1, gz);
			double y01 = HeightAtCell(gx, gz + 1),   y11 = HeightAtCell(gx + 1, gz + 1);

			Vec3 n0 = NormalAt(x0, z0);
			Vec3 n1 = NormalAt(x1, z1);
			double avg_y = (y00 + y10 + y01 + y11) * 0.25;
			double h01 = Clampd(avg_y / height_scale, 0.0, 1.0);
			double slope = 1.0 - (n0.y + n1.y) * 0.5;
			Color c = TerrainColor(h01, slope);
			glColor3ub(c.GetR(), c.GetG(), c.GetB());

			glNormal3d(n0.x, n0.y, n0.z);
			glVertex3d(x0, y00, z0);
			glVertex3d(x0, y01, z1);
			glVertex3d(x1, y10, z0);

			glNormal3d(n1.x, n1.y, n1.z);
			glVertex3d(x1, y10, z0);
			glVertex3d(x0, y01, z1);
			glVertex3d(x1, y11, z1);
		}
	}
	glEnd();
}

PhysGeomId Terrain::BuildOdeHeightfield(PhysSpaceId space)
{
	// Island Terrain phase, Task 4: no baked grid to build a heightfield
	// from when IsIsland() - never actually called for the island track
	// today (PlayScreen's own per-chunk ground_colliders, Unified Ground
	// Task 4, already covers physics for every track kind by sampling
	// HeightAt directly), guarded here defensively rather than left to
	// build a heightfield from an empty grid_w=grid_d=0.
	if(island) {
		LOG("Terrain::BuildOdeHeightfield: no-op in island mode");
		return 0;
	}

	if(hf_geom) {
		// A previous call's geom, still live in this same space (Road
		// Network Task 4: a chunk's roads flattened more of the grid, so
		// collision needs rebuilding to reflect it) - must be destroyed
		// before creating a new one, or the physics backend keeps
		// colliding against a geom whose own height data we're about to
		// replace below. PhysDestroyGeom frees the geom's own height
		// data too (Physics Abstraction Task 2) - no separate data-
		// destroy step needed here anymore.
		PhysDestroyGeom(hf_geom);
		hf_geom = 0;
	}

	hf_geom = PhysCreateHeightfield(space, height.Begin(), grid_w, grid_d,
	                                 world_size, world_size, 1.0, 0.0);
	// A placeable ODE heightfield is already centered on the geom's own
	// position by default (verified empirically with dGeomGetAABB - this is
	// not obvious from the header docs, which describe the LOCAL span as
	// [0,width]x[0,depth] but don't mention that span is centered on the
	// geom origin, not corner-anchored to it). Our world coordinates are
	// also centered on the origin, so no further translation is needed -
	// setting one here double-centers it (this was the root cause of the
	// "wheels sink through the ground" incident: for a 200m terrain, it put
	// the actual collision surface 100m away from where Terrain::HeightAt()
	// and the GL render think it is).
	return hf_geom;
}

void Terrain::FlattenUnderRibbon(const Vector<Vec2>& centerline, const RoadMesh& mesh)
{
	// Island Terrain phase, Task 4: no-op when IsIsland() - see the
	// header's own comment on `island` for why (no grid to mutate,
	// roads there simply follow the height function's own contour).
	if(island)
		return;
	if(centerline.GetCount() < 2 || height.IsEmpty())
		return;

	// The road's own height along its centerline, sampled from the
	// terrain's CURRENT (pre-deform) state before touching anything -
	// roads still follow the general lay of the land (a smoothly-varying
	// profile along their length), not a flat constant everywhere. Flat
	// only *across* the road's width at any given point along it, which
	// is a reasonable simplification (real roads are graded level across
	// their own width) - see the task doc's own framing.
	Vector<double> road_height;
	for(Vec2 p : centerline)
		road_height.Add(HeightAt(p.x, p.y)); // each call independently read-locked - must happen before the write lock below, never under it (a thread already holding the write lock re-entering HeightAt's own read lock would deadlock)

	// Map Editor phase, Task 9: one write lock spanning only the
	// mutation loop below (no further HeightAt calls inside it) - see
	// this function's own note just above on why the sampling above
	// must stay outside it.
	RWMutex::WriteLock __(height_lock);

	double half = world_size * 0.5;

	// Widen the flattened footprint by one grid spacing on each side so
	// Terrain::HeightAt's bilinear interpolation near the ribbon's *true*
	// edge never blends in an unflattened neighbouring grid point -
	// found via this task's own self-test: a road only a few grid
	// spacings wide showed the original bumpy terrain "bleeding through"
	// near its edges without this margin (the flattening only touched
	// grid points strictly inside the ribbon, but HeightAt's 2x2
	// interpolation neighbourhood for a query point near the edge could
	// still reach one grid point just outside it).
	double margin = cell_size;

	// One quad per centerline segment - corners start as RoadMeshBuilder's
	// own left/right vertex pairs at i and i+1 (RoadMesh.h: [2*i]=left,
	// [2*i+1]=right), so the *un*-widened shape always matches whatever
	// ribbon Task 1 built for the same centerline; only the widened copy
	// below decides which grid points get touched, not the target height.
	for(int i = 0; i + 1 < centerline.GetCount(); i++) {
		Vec2 a_left = mesh.vertices[i * 2].pos;
		Vec2 a_right = mesh.vertices[i * 2 + 1].pos;
		Vec2 b_left = mesh.vertices[(i + 1) * 2].pos;
		Vec2 b_right = mesh.vertices[(i + 1) * 2 + 1].pos;

		Vec2 dir_a = (a_left - centerline[i]).Normalized();
		Vec2 dir_b = (b_left - centerline[i + 1]).Normalized();
		Vec2 fwd = (centerline[i + 1] - centerline[i]).Normalized();
		// Widened along-length too (fwd), not just across-width (dir_a/
		// dir_b) - found via Road Network Task 4's own integration
		// self-test (RunRoadCreatorMeshSelfTest), which is the first test
		// to exercise a real, CURVED generated road: on a bend, two
		// consecutive segments' quads only meet exactly at their shared
		// vertex, leaving a wedge-shaped gap on the outer side of the
		// turn that the perpendicular-only margin above never covered
		// (RunTerrainFlattenSelfTest's own hand-built centerline is a
		// straight line, so it could never have caught this). Extending
		// each quad past its own endpoint along the segment direction
		// makes adjacent quads overlap near their shared vertex instead
		// of only touching it, closing that gap; points that fall past
		// t=0/t=1 in the interpolation below just clamp to the nearest
		// endpoint's height, which already matches the neighbouring
		// segment's own target there.
		Vec2 a_left_ex = a_left + dir_a * margin - fwd * margin;
		Vec2 a_right_ex = a_right - dir_a * margin - fwd * margin;
		Vec2 b_left_ex = b_left + dir_b * margin + fwd * margin;
		Vec2 b_right_ex = b_right - dir_b * margin + fwd * margin;

		double min_x = min(min(a_left_ex.x, a_right_ex.x), min(b_left_ex.x, b_right_ex.x));
		double max_x = max(max(a_left_ex.x, a_right_ex.x), max(b_left_ex.x, b_right_ex.x));
		double min_z = min(min(a_left_ex.y, a_right_ex.y), min(b_left_ex.y, b_right_ex.y));
		double max_z = max(max(a_left_ex.y, a_right_ex.y), max(b_left_ex.y, b_right_ex.y));

		// Grid-index bounding box, +-1 cell so a grid point right at the
		// (already-widened) quad's edge is never missed by rounding.
		int gx0 = Clampi((int)floor((min_x + half) / cell_size) - 1, 0, grid_w - 1);
		int gx1 = Clampi((int)ceil((max_x + half) / cell_size) + 1, 0, grid_w - 1);
		int gz0 = Clampi((int)floor((min_z + half) / cell_size) - 1, 0, grid_d - 1);
		int gz1 = Clampi((int)ceil((max_z + half) / cell_size) + 1, 0, grid_d - 1);

		Vec2 seg = centerline[i + 1] - centerline[i];
		double seg_len_sq = seg.LengthSq();

		for(int gz = gz0; gz <= gz1; gz++) {
			for(int gx = gx0; gx <= gx1; gx++) {
				double wx = gx * cell_size - half;
				double wz = gz * cell_size - half;
				Vec2 p(wx, wz);

				if(!PointInQuad(p, a_left_ex, b_left_ex, b_right_ex, a_right_ex))
					continue;

				double t = seg_len_sq > 1e-12 ? Clampd((p - centerline[i]).Dot(seg) / seg_len_sq, 0.0, 1.0) : 0.0;
				double h = road_height[i] * (1 - t) + road_height[i + 1] * t;
				height[gz * grid_w + gx] = (float)h;
			}
		}
	}
}

void Terrain::ApplyPaintedHeight(double world_x, double world_z, double new_height)
{
	if(!island)
		return;
	ErosionTileCoord coord = ErosionTileCoordFor(Vec2(world_x, world_z));
	EnsureErosionTile(coord); // its own locking, released before we proceed

	// Map Editor phase, Task 9: one continuous write lock from the
	// lookup through the mutation - `tile` below is a reference into
	// VectorMap's own storage, which a concurrent Add (the background
	// worker's own EnsureErosionTile, for a *different* coordinate)
	// could reallocate and invalidate if this weren't held for the
	// whole span.
	RWMutex::WriteLock __(height_lock);
	int i = loaded_erosion_tiles.Find(coord);
	if(i < 0)
		return; // defensive - EnsureErosionTile always loads it when island is set

	ErosionTile& tile = loaded_erosion_tiles[i];
	Vec2 origin = ErosionTileOrigin(coord);
	double cell_size = kErosionTileSize / (kErosionTileGridRes - 1);
	int lx = (int)round((world_x - origin.x) / cell_size);
	int lz = (int)round((world_z - origin.y) / cell_size);
	if(lx < 0 || lx >= kErosionTileGridRes || lz < 0 || lz >= kErosionTileGridRes)
		return;
	tile.height[lz * kErosionTileGridRes + lx] = (float)new_height;
}

bool Terrain::RaycastHeightAt(Vec3 origin, Vec3 dir, double max_dist, Vec3& hit_out) const
{
	const double step = 2.0;
	double prev_diff = origin.y - HeightAt(origin.x, origin.z);
	for(double t = step; t <= max_dist; t += step) {
		Vec3 p = origin + dir * t;
		double diff = p.y - HeightAt(p.x, p.z);
		if(diff <= 0 && prev_diff > 0) {
			// Bisect the last step for sub-step precision - the coarse
			// march above only needs to find *which* 2m segment the
			// surface is in.
			double t0 = t - step, t1 = t;
			for(int i = 0; i < 12; i++) {
				double tm = (t0 + t1) * 0.5;
				Vec3 pm = origin + dir * tm;
				double dm = pm.y - HeightAt(pm.x, pm.z);
				if(dm > 0)
					t0 = tm;
				else
					t1 = tm;
			}
			hit_out = origin + dir * ((t0 + t1) * 0.5);
			return true;
		}
		prev_diff = diff;
	}
	return false;
}

END_UPP_NAMESPACE
