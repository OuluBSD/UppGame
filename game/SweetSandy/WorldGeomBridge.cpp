#include "SweetSandy.h"

NAMESPACE_UPP

namespace {

Material& GetOrAddTintMaterial(Model& mdl, VectorMap<dword, int>& tint_to_material_id, Color tint)
{
	dword raw = tint.GetRaw();
	int i = tint_to_material_id.Find(raw);
	if(i >= 0)
		return mdl.materials.Get(tint_to_material_id[i]);
	Material& m = mdl.AddMaterial();
	vec3 c((float)tint.GetR() / 255.0f, (float)tint.GetG() / 255.0f, (float)tint.GetB() / 255.0f);
	m.params->diffuse = c;
	m.params->base_clr_factor = vec4(c[0], c[1], c[2], 1.0f);
	tint_to_material_id.Add(raw, m.id);
	return m;
}

// Ground: already world-space (see WorldGeomBridge.h's own note) - every
// VegMesh in `parts` is merged into one Mesh, index offsets tracked by
// hand (Mesh has no built-in "append another mesh" helper).
void AddMergedWorldSpace(Model& mdl, const Vector<VegMesh>& parts, Color tint, VectorMap<dword, int>& tint_mat, int& triangles_out)
{
	int total_v = 0, total_t = 0;
	for(const VegMesh& part : parts) {
		total_v += part.vertices.GetCount();
		total_t += part.triangles.GetCount() / 3;
	}
	if(total_v == 0 || total_t == 0)
		return;
	Mesh& mesh = mdl.AddMesh();
	mesh.SetCountTriangles(total_v, total_t);
	int voff = 0, toff = 0;
	for(const VegMesh& part : parts) {
		int nv = part.vertices.GetCount();
		for(int i = 0; i < nv; i++) {
			const VegMeshVertex& v = part.vertices[i];
			mesh.SetVertCoord(voff + i, vec3((float)v.pos.x, (float)v.pos.y, (float)v.pos.z));
			mesh.SetVertNormal(voff + i, vec3((float)v.normal.x, (float)v.normal.y, (float)v.normal.z));
		}
		int nt = part.triangles.GetCount() / 3;
		for(int t = 0; t < nt; t++)
			mesh.SetTriangleIndices(toff + t, ivec3(voff + part.triangles[3 * t + 0],
			                                         voff + part.triangles[3 * t + 1],
			                                         voff + part.triangles[3 * t + 2]));
		voff += nv;
		toff += nt;
	}
	mesh.material = GetOrAddTintMaterial(mdl, tint_mat, tint).id;
	triangles_out += total_t;
}

// Buildings/vegetation: local-space source mesh, baked into world space
// here via translate(position) * rotateY(yaw) - the identical transform
// PlayScreen::DrawBuildingsGL/DrawVegetationGL apply at draw time
// (glTranslated then glRotated, yaw in radians converted to degrees
// there - same rotation, done here directly in radians on vec3 math
// instead of through the GL matrix stack). One Mesh per (instance,
// tinted part) - not merged across instances, since two buildings can
// have different wall/roof tints and a Mesh carries only one Material.
void AddLocalSpaceTransformed(Model& mdl, const VegMesh& part, Vec3 position, double yaw, Color tint,
                               VectorMap<dword, int>& tint_mat, int& triangles_out)
{
	int nv = part.vertices.GetCount();
	int nt = part.triangles.GetCount() / 3;
	if(nv == 0 || nt == 0)
		return;
	Mesh& mesh = mdl.AddMesh();
	mesh.SetCountTriangles(nv, nt);
	double c = cos(yaw), s = sin(yaw);
	for(int i = 0; i < nv; i++) {
		const Vec3& p = part.vertices[i].pos;
		double wx = p.x * c + p.z * s;
		double wz = -p.x * s + p.z * c;
		mesh.SetVertCoord(i, vec3((float)(position.x + wx), (float)(position.y + p.y), (float)(position.z + wz)));
		const Vec3& n = part.vertices[i].normal;
		double nx = n.x * c + n.z * s;
		double nz = -n.x * s + n.z * c;
		mesh.SetVertNormal(i, vec3((float)nx, (float)n.y, (float)nz));
	}
	for(int t = 0; t < nt; t++)
		mesh.SetTriangleIndices(t, ivec3(part.triangles[3 * t + 0], part.triangles[3 * t + 1], part.triangles[3 * t + 2]));
	mesh.material = GetOrAddTintMaterial(mdl, tint_mat, tint).id;
	triangles_out += nt;
}

// Road: 2D source vertices (Vec2 pos, no height - see RoadMesh.h's own
// convention) - world Y resolved via terrain.HeightAt at bridge time,
// matching PlayScreen::DrawCityGL's own y_offset exactly. `tint`
// parameterized (Task 18) - procedural roads keep the asphalt gray
// DrawCityGL itself uses, but a hand-authored road link
// (AddRoadLinkObject, below) needs a visually distinct color so it
// reads as "the thing I just added" against the generated network.
void AddRoadMesh(Model& mdl, const RoadMesh& road, const Terrain& terrain, VectorMap<dword, int>& tint_mat, int& triangles_out,
                  Color tint, double y_offset = 0.08)
{
	int nv = road.vertices.GetCount();
	int nt = road.triangles.GetCount() / 3;
	if(nv == 0 || nt == 0)
		return;
	Mesh& mesh = mdl.AddMesh();
	mesh.SetCountTriangles(nv, nt);
	for(int i = 0; i < nv; i++) {
		Vec2 p = road.vertices[i].pos;
		double wy = terrain.HeightAt(p.x, p.y) + y_offset;
		mesh.SetVertCoord(i, vec3((float)p.x, (float)wy, (float)p.y));
		mesh.SetVertNormal(i, vec3(0, 1, 0));
	}
	for(int t = 0; t < nt; t++)
		mesh.SetTriangleIndices(t, ivec3(road.triangles[3 * t + 0], road.triangles[3 * t + 1], road.triangles[3 * t + 2]));
	mesh.material = GetOrAddTintMaterial(mdl, tint_mat, tint).id;
	triangles_out += nt;
}

// Map Editor phase, Task 11: an axis-aligned box, centered at
// `center` with the given half-extents - the basic primitive
// AddStartGateLandmark builds its pillars/crossbeam from. 8 vertices,
// 12 triangles (2 per face, 6 faces), one flat face normal each (not
// smoothed - a landmark structure reads better with hard edges than
// the organic ground/tree meshes elsewhere in this bridge).
void AddBoxMesh(Model& mdl, vec3 center, vec3 half, Color tint, VectorMap<dword, int>& tint_mat, int& triangles_out)
{
	Mesh& mesh = mdl.AddMesh();
	mesh.SetCountTriangles(8, 12);
	float sx[8] = { -1, +1, +1, -1, -1, +1, +1, -1 };
	float sy[8] = { -1, -1, -1, -1, +1, +1, +1, +1 };
	float sz[8] = { -1, -1, +1, +1, -1, -1, +1, +1 };
	for(int i = 0; i < 8; i++)
		mesh.SetVertCoord(i, vec3(center[0] + sx[i] * half[0], center[1] + sy[i] * half[1], center[2] + sz[i] * half[2]));
	// Two triangles per face; winding doesn't matter for this
	// renderer's own lighting (RenderSceneV2Headless derives the face
	// normal from the triangle's own 3 vertices at render time - see
	// WorldGeomBridge.h's own note on why SetVertNormal below is best-
	// effort only, not load-bearing).
	static const int faces[6][4] = {
		{ 0, 1, 2, 3 }, // bottom (-Y)
		{ 4, 5, 6, 7 }, // top (+Y)
		{ 0, 1, 5, 4 }, // -Z
		{ 3, 2, 6, 7 }, // +Z
		{ 0, 3, 7, 4 }, // -X
		{ 1, 2, 6, 5 }, // +X
	};
	for(int f = 0; f < 6; f++) {
		mesh.SetTriangleIndices(f * 2 + 0, ivec3(faces[f][0], faces[f][1], faces[f][2]));
		mesh.SetTriangleIndices(f * 2 + 1, ivec3(faces[f][0], faces[f][2], faces[f][3]));
	}
	for(int i = 0; i < 8; i++)
		mesh.SetVertNormal(i, vec3(sx[i], sy[i], sz[i]));
	mesh.material = GetOrAddTintMaterial(mdl, tint_mat, tint).id;
	triangles_out += 12;
}

} // anonymous namespace

void AddStartGateLandmark(GeomScene& scene, const Terrain& terrain, Vec2 center)
{
	GeomObject& obj = scene.GetAddModel("landmark_start_gate");
	Model& mdl = obj.mdl.Create();
	VectorMap<dword, int> tint_mat;
	int unused_triangles = 0;
	const Color kGateTint(255, 140, 0); // bright orange - see this function's own header comment

	double ground_y = terrain.HeightAt(center.x, center.y);
	const double gap = 8.0;         // clear width between pillars - wide enough to drive a vehicle through
	const double pillar_w = 1.0, pillar_d = 1.0, pillar_h = 5.0;
	const double beam_h = 1.0;

	AddBoxMesh(mdl, vec3((float)(center.x - gap / 2 - pillar_w / 2), (float)(ground_y + pillar_h / 2), (float)center.y),
	           vec3((float)(pillar_w / 2), (float)(pillar_h / 2), (float)(pillar_d / 2)), kGateTint, tint_mat, unused_triangles);
	AddBoxMesh(mdl, vec3((float)(center.x + gap / 2 + pillar_w / 2), (float)(ground_y + pillar_h / 2), (float)center.y),
	           vec3((float)(pillar_w / 2), (float)(pillar_h / 2), (float)(pillar_d / 2)), kGateTint, tint_mat, unused_triangles);
	AddBoxMesh(mdl, vec3((float)center.x, (float)(ground_y + pillar_h + beam_h / 2), (float)center.y),
	           vec3((float)(gap / 2 + pillar_w), (float)(beam_h / 2), (float)(pillar_d / 2)), kGateTint, tint_mat, unused_triangles);
}

void AddMarkerObject(GeomScene& scene, const String& name, Vec3 position, Color tint)
{
	GeomObject& obj = scene.GetAddModel(name);
	Model& mdl = obj.mdl.Create();
	VectorMap<dword, int> tint_mat;
	int unused_triangles = 0;
	const double half_size = 1.0; // a 2m cube - big enough to click/see, small enough not to dominate the scene
	AddBoxMesh(mdl, vec3(0, 0, 0), vec3((float)half_size, (float)half_size, (float)half_size), tint, tint_mat, unused_triangles);
	// Centered on its own local origin (unlike AddStartGateLandmark's
	// pillars, which bake world position directly into vertices) -
	// this object's own GeomTransform (below) carries the position
	// instead, so it's immediately ready for TOOL_OBJ_MOVE's own
	// gizmo-drag code to read/write (see GeomTransform's own role in
	// GeomWorldState::UpdateObjects - a baked-in-vertices object has
	// nothing meaningful for a "move" to be relative to).
	GeomTransform& tr = obj.GetTransform();
	tr.position = vec3((float)position.x, (float)position.y, (float)position.z);
}

void AddRoadLinkObject(GeomScene& scene, const String& name, const Vector<RoadPathPoint>& path, double width,
                        const Terrain& terrain, Color tint)
{
	if(path.GetCount() < 2)
		return;
	GeomObject& obj = scene.GetAddModel(name);
	Model& mdl = obj.mdl.Create();
	VectorMap<dword, int> tint_mat;

	Vector<Vec2> centerline;
	for(const RoadPathPoint& p : path)
		centerline.Add(p.pos);
	RoadMesh road = RoadMeshBuilder::BuildRibbon(centerline, width);
	int nv = road.vertices.GetCount();
	int nt = road.triangles.GetCount() / 3;
	if(nv == 0 || nt == 0)
		return;

	// Task 19: per-point world-Y, not a single uniform rule - see this
	// function's own header comment. y_offset (both branches) is the
	// same small bump Task 18 added to avoid z-fighting against the
	// procedural network's own ground-hugging roads; on a bridge run
	// it's dwarfed by kBridgeClearance and kept only for consistency.
	const double y_offset = 0.16;
	const double kBridgeClearance = 3.0; // meters above the higher of the bridge run's own two ends - enough to clear typical water/terrain relief at this project's scale, not a real structural calculation
	// Matches RoadPathfinder.cpp's own kApproachRamp - a bridge run's
	// first/last this-many-meters is real land (the approach ramp, see
	// that file's own comment), so the deck ramps smoothly from real
	// ground height up to full clearance over that same distance,
	// rather than snapping straight to an elevated plateau right at
	// the run's own edge.
	const double kApproachRamp = 25.0;
	Vector<double> vertex_y;
	vertex_y.SetCount(path.GetCount());
	int i = 0;
	while(i < path.GetCount()) {
		if(!path[i].bridge) {
			vertex_y[i] = terrain.HeightAt(path[i].pos.x, path[i].pos.y) + y_offset;
			i++;
			continue;
		}
		int run_start = i;
		while(i < path.GetCount() && path[i].bridge)
			i++;
		int run_end = i; // exclusive
		double ground_start = terrain.HeightAt(path[run_start].pos.x, path[run_start].pos.y);
		double ground_end = terrain.HeightAt(path[run_end - 1].pos.x, path[run_end - 1].pos.y);
		double deck_y = max(ground_start, ground_end) + kBridgeClearance + y_offset;

		// Cumulative arc length from run_start, so distance-from-either-
		// end is real path distance, not just an index count (segments
		// aren't all the same length after Chaikin smoothing/relaxation).
		Vector<double> dist_from_start;
		dist_from_start.SetCount(run_end - run_start, 0.0);
		for(int k = run_start + 1; k < run_end; k++)
			dist_from_start[k - run_start] = dist_from_start[k - run_start - 1] + (path[k].pos - path[k - 1].pos).Length();
		double total = dist_from_start[run_end - run_start - 1];

		for(int k = run_start; k < run_end; k++) {
			double d_start = dist_from_start[k - run_start];
			double d_end = total - d_start;
			double t = Smooth01(min(d_start, d_end) / kApproachRamp); // 0 at either end, 1 once kApproachRamp meters from both
			double ground_anchor = d_start < d_end ? ground_start : ground_end;
			vertex_y[k] = (ground_anchor + y_offset) * (1 - t) + deck_y * t;
		}
	}

	Mesh& mesh = mdl.AddMesh();
	mesh.SetCountTriangles(nv, nt);
	for(int vi = 0; vi < nv; vi++) {
		int center_i = vi / 2; // BuildRibbon emits [left,right] per centerline point, in order - see RoadMeshBuilder.cpp
		Vec2 p = road.vertices[vi].pos;
		mesh.SetVertCoord(vi, vec3((float)p.x, (float)vertex_y[center_i], (float)p.y));
		mesh.SetVertNormal(vi, vec3(0, 1, 0));
	}
	for(int t = 0; t < nt; t++)
		mesh.SetTriangleIndices(t, ivec3(road.triangles[3 * t + 0], road.triangles[3 * t + 1], road.triangles[3 * t + 2]));
	mesh.material = GetOrAddTintMaterial(mdl, tint_mat, tint).id;
}

WorldGeomBridgeStats BuildWorldGeomObjects(GeomScene& scene, const WorldBuilder& city, const Terrain& terrain)
{
	WorldGeomBridgeStats stats;
	const Color kGrassTint(100, 170, 80); // matches DrawGroundGL's own grass_tint exactly

	for(int i = 0; i < city.GetOctree().GetLoadedChunkCount(); i++) {
		const WorldChunk& chunk = city.GetOctree().GetLoadedChunk(i);
		const WorldChunkContent* content = city.FindContent(chunk.coord);
		if(!content)
			continue; // still generating - skip; caller should WaitIdle() first for a complete bake

		String name = Format("chunk_%d_%d_%d_%d", chunk.coord.lod, chunk.coord.x, chunk.coord.y, chunk.coord.z);
		GeomObject& obj = scene.GetAddModel(name);
		Model& mdl = obj.mdl.Create();
		VectorMap<dword, int> tint_mat; // per-object (per-chunk) tint -> material id cache
		int mesh_count_before = mdl.meshes.GetCount();

		if(!content->ground.IsEmpty())
			AddMergedWorldSpace(mdl, content->ground, kGrassTint, tint_mat, stats.triangles);
		if(!content->road_mesh.triangles.IsEmpty())
			AddRoadMesh(mdl, content->road_mesh, terrain, tint_mat, stats.triangles, Color(60, 60, 64)); // matches DrawCityGL's own asphalt color
		for(const BuildingInstance& b : content->buildings) {
			AddLocalSpaceTransformed(mdl, b.walls, b.position, b.yaw, b.wall_tint, tint_mat, stats.triangles);
			AddLocalSpaceTransformed(mdl, b.roof, b.position, b.yaw, b.roof_tint, tint_mat, stats.triangles);
		}
		for(const VegTreeInstance& t : content->vegetation) {
			AddLocalSpaceTransformed(mdl, t.branches, t.position, t.yaw, t.bark_tint, tint_mat, stats.triangles);
			AddLocalSpaceTransformed(mdl, t.leaves, t.position, t.yaw, t.leaf_tint, tint_mat, stats.triangles);
		}

		if(mdl.meshes.GetCount() == mesh_count_before)
			continue; // this chunk produced no visible geometry at all (e.g. an all-empty area) - still counts as a loaded chunk, but not a meaningful object
		stats.chunks++;
		stats.objects++;
		stats.meshes += mdl.meshes.GetCount() - mesh_count_before;
	}
	return stats;
}

void AddFenceObject(GeomScene& scene, const String& name, const Vector<Vec2>& centerline, double width, double height,
                     const Terrain& terrain, Color tint)
{
	int n = centerline.GetCount();
	if(n < 2)
		return;
	GeomObject& obj = scene.GetAddModel(name);
	Model& mdl = obj.mdl.Create();
	VectorMap<dword, int> tint_mat;

	double half = width * 0.5;
	const double base_offset = 0.05;
	Vector<vec3> outer_b, outer_t, inner_b, inner_t;
	outer_b.SetCount(n); outer_t.SetCount(n); inner_b.SetCount(n); inner_t.SetCount(n);
	for(int i = 0; i < n; i++) {
		Vec2 fwd = i == 0 ? centerline[1] - centerline[0]
		         : i == n - 1 ? centerline[i] - centerline[i - 1]
		         : centerline[i + 1] - centerline[i - 1];
		Vec2 left = Vec2(-fwd.y, fwd.x).Normalized();
		Vec2 op = centerline[i] + left * half;
		Vec2 ip = centerline[i] - left * half;
		double oy = terrain.HeightAt(op.x, op.y) + base_offset;
		double iy = terrain.HeightAt(ip.x, ip.y) + base_offset;
		outer_b[i] = vec3((float)op.x, (float)oy, (float)op.y);
		outer_t[i] = vec3((float)op.x, (float)(oy + height), (float)op.y);
		inner_b[i] = vec3((float)ip.x, (float)iy, (float)ip.y);
		inner_t[i] = vec3((float)ip.x, (float)(iy + height), (float)ip.y);
	}

	Mesh& mesh = mdl.AddMesh();
	int nseg = n - 1;
	mesh.SetCountTriangles(n * 4, nseg * 6);
	for(int i = 0; i < n; i++) {
		int base = i * 4;
		mesh.SetVertCoord(base + 0, outer_b[i]);
		mesh.SetVertCoord(base + 1, outer_t[i]);
		mesh.SetVertCoord(base + 2, inner_b[i]);
		mesh.SetVertCoord(base + 3, inner_t[i]);
		// Best-effort only, like AddBoxMesh's own - RenderSceneV2Headless
		// derives the actual face normal from each triangle's own 3
		// vertices at render time.
		mesh.SetVertNormal(base + 0, vec3(0, 0, 1));
		mesh.SetVertNormal(base + 1, vec3(0, 0, 1));
		mesh.SetVertNormal(base + 2, vec3(0, 0, -1));
		mesh.SetVertNormal(base + 3, vec3(0, 0, -1));
	}
	int t = 0;
	for(int i = 0; i < nseg; i++) {
		int a = i * 4, b = (i + 1) * 4;
		mesh.SetTriangleIndices(t++, ivec3(a + 0, a + 1, b + 1)); // outer face
		mesh.SetTriangleIndices(t++, ivec3(a + 0, b + 1, b + 0));
		mesh.SetTriangleIndices(t++, ivec3(a + 2, b + 3, a + 3)); // inner face (reverse winding)
		mesh.SetTriangleIndices(t++, ivec3(a + 2, b + 2, b + 3));
		mesh.SetTriangleIndices(t++, ivec3(a + 1, a + 3, b + 3)); // top cap
		mesh.SetTriangleIndices(t++, ivec3(a + 1, b + 3, b + 1));
	}
	mesh.material = GetOrAddTintMaterial(mdl, tint_mat, tint).id;
}

END_UPP_NAMESPACE
