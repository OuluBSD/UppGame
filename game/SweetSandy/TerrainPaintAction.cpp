#include "SweetSandy.h"

NAMESPACE_UPP

const double kPaintGridStep = 8.0;

int PaintGridIndex(double world_coord)
{
	return (int)round(world_coord / kPaintGridStep);
}

String PaintHeightKey(int ix, int iz)
{
	return "th@" + AsString(ix) + "," + AsString(iz);
}

namespace {

bool ParsePaintHeightKey(const String& key, int& ix, int& iz)
{
	if(!key.StartsWith("th@"))
		return false;
	int comma = key.Find(',');
	if(comma < 0)
		return false;
	ix = StrInt(key.Mid(3, comma - 3));
	iz = StrInt(key.Mid(comma + 1));
	return true;
}

} // anonymous namespace

Value TerrainPaintBrush::ToValue() const
{
	ValueMap m;
	m.Add("cx", cx);
	m.Add("cz", cz);
	m.Add("radius", radius);
	m.Add("delta", delta);
	m.Add("slope_limit", slope_limit);
	m.Add("flatten", flatten);
	return m;
}

TerrainPaintBrush TerrainPaintBrush::FromValue(const Value& v)
{
	TerrainPaintBrush b;
	b.cx = v["cx"];
	b.cz = v["cz"];
	b.radius = v["radius"];
	b.delta = v["delta"];
	b.slope_limit = v["slope_limit"];
	b.flatten = v["flatten"]; // IsNull -> false for pre-Task17 stored Values, same default as the struct itself
	return b;
}

Value TerrainErosionBrush::ToValue() const
{
	ValueMap m;
	m.Add("cx", cx);
	m.Add("cz", cz);
	m.Add("radius", radius);
	m.Add("droplets", droplets);
	m.Add("seed", (int)seed);
	return m;
}

TerrainErosionBrush TerrainErosionBrush::FromValue(const Value& v)
{
	TerrainErosionBrush b;
	b.cx = v["cx"];
	b.cz = v["cz"];
	b.radius = v["radius"];
	b.droplets = v["droplets"];
	b.seed = (dword)(int)v["seed"];
	return b;
}

namespace {

// Shared by execute() and fit_check(): the height at one grid node -
// whatever's already in the store there (painted by this stroke or an
// earlier one - this is what makes overlapping strokes both dependency-
// capture AND read-modify-write correctly), or the cached *pristine*
// base height if this exact node has never been painted by anything
// currently in the graph (see PaintBaseHeightCache's own comment for
// why this must NOT fall back to a live Terrain::HeightAt call - once
// CompileTerrainPaintToTerrain has run, that call would read back this
// same stroke's own previously-compiled output instead of the true
// original terrain, compounding on every edit+recompile cycle).
double ReadGridHeight(ActionContext& ctx, Terrain& terrain, PaintBaseHeightCache& base_cache, int ix, int iz)
{
	String key = PaintHeightKey(ix, iz);
	Value v = ctx.Read(key);
	if(!IsNull(v))
		return v;
	int i = base_cache.Find(key);
	if(i >= 0)
		return base_cache[i];
	double h = terrain.HeightAt(ix * kPaintGridStep, iz * kPaintGridStep);
	base_cache.Add(key, h);
	return h;
}

// Shared by AppendTerrainPaint's and AppendTerrainErosion's own
// fit_check closures - identical "does the center's own already-
// written height create too steep a step against its 4 grid
// neighbors" guard, extracted once a second caller (Task 17's erosion
// touch-up) needed the exact same check.
bool CheckPaintSlope(ActionContext& ctx, Terrain& terrain, PaintBaseHeightCache& base_cache,
                      int cix, int ciz, double slope_limit)
{
	double center = ReadGridHeight(ctx, terrain, base_cache, cix, ciz);
	static const int dix[4] = { 1, -1, 0, 0 };
	static const int diz[4] = { 0, 0, 1, -1 };
	for(int i = 0; i < 4; i++) {
		double neighbor = ReadGridHeight(ctx, terrain, base_cache, cix + dix[i], ciz + diz[i]);
		double slope = fabs(center - neighbor) / kPaintGridStep;
		if(slope > slope_limit)
			return false;
	}
	return true;
}

} // anonymous namespace

ActionId AppendTerrainPaint(ActionGraph& graph, Terrain& terrain, PaintBaseHeightCache& base_cache,
                             const TerrainPaintBrush& brush)
{
	return graph.Append(
		"terrain_paint", brush.ToValue(),
		[&terrain, &base_cache](ActionContext& ctx, const Value& param) {
			TerrainPaintBrush b = TerrainPaintBrush::FromValue(param);
			int ix0 = PaintGridIndex(b.cx - b.radius), ix1 = PaintGridIndex(b.cx + b.radius);
			int iz0 = PaintGridIndex(b.cz - b.radius), iz1 = PaintGridIndex(b.cz + b.radius);
			for(int iz = iz0; iz <= iz1; iz++) {
				for(int ix = ix0; ix <= ix1; ix++) {
					double wx = ix * kPaintGridStep, wz = iz * kPaintGridStep;
					double dx = wx - b.cx, dz = wz - b.cz;
					double dist = sqrt(dx * dx + dz * dz);
					if(dist > b.radius)
						continue;
					double base = ReadGridHeight(ctx, terrain, base_cache, ix, iz);
					double h;
					if(b.flatten) {
						// Task 17: flatten mode blends toward one absolute
						// target world-Y (b.delta, reinterpreted - see
						// TerrainPaintBrush::flatten's own comment)
						// instead of adding an offset. Unlike raise's own
						// pure cone falloff (1 at center, linearly 0 at
						// the edge - fine for a soft bump, useless for a
						// buildable site since it's only ever exactly
						// flat at one single point), this has a real
						// plateau: fully flat out to kPlateauFrac of the
						// radius, THEN blends smoothly back to the
						// untouched base height over the remaining outer
						// ring, so there's an actually-sizeable flat area
						// to place a city on, not just a single point.
						const double kPlateauFrac = 0.6;
						double inner = b.radius * kPlateauFrac;
						double blend = dist <= inner ? 1.0
						             : Clamp((b.radius - dist) / (b.radius - inner), 0.0, 1.0);
						h = base * (1.0 - blend) + b.delta * blend;
					}
					else {
						double falloff = 1.0 - dist / b.radius; // 1 at center, 0 at the radius edge
						h = base + b.delta * falloff;
					}
					ctx.Write(PaintHeightKey(ix, iz), h);
				}
			}
		},
		[&terrain, &base_cache](ActionContext& ctx, const Value& param) -> bool {
			TerrainPaintBrush b = TerrainPaintBrush::FromValue(param);
			int cix = PaintGridIndex(b.cx), ciz = PaintGridIndex(b.cz);
			return CheckPaintSlope(ctx, terrain, base_cache, cix, ciz, b.slope_limit);
		});
}

Vector<ActionId> SetTerrainPaintBrush(ActionGraph& graph, ActionId id, const TerrainPaintBrush& brush)
{
	return graph.SetParam(id, brush.ToValue());
}

ActionId AppendTerrainErosion(ActionGraph& graph, Terrain& terrain, PaintBaseHeightCache& base_cache,
                               const TerrainErosionBrush& brush)
{
	return graph.Append(
		"terrain_erosion", brush.ToValue(),
		[&terrain, &base_cache](ActionContext& ctx, const Value& param) {
			TerrainErosionBrush b = TerrainErosionBrush::FromValue(param);
			int ix0 = PaintGridIndex(b.cx - b.radius), ix1 = PaintGridIndex(b.cx + b.radius);
			int iz0 = PaintGridIndex(b.cz - b.radius), iz1 = PaintGridIndex(b.cz + b.radius);
			// ErodeHeightmap's own map is square (map_size x map_size) -
			// pad to the larger of the two spans so every node inside
			// the requested circle has a real place in the grid; a
			// floor of 7 leaves ErodeHeightmap's default erosion_radius
			// (3) a full neighborhood to sample even for a tiny brush.
			int n = max(ix1 - ix0 + 1, iz1 - iz0 + 1);
			if(n < 7)
				n = 7;
			Vector<float> map;
			map.SetCount(n * n, 0.0f);
			for(int iz = 0; iz < n; iz++)
				for(int ix = 0; ix < n; ix++)
					map[iz * n + ix] = (float)ReadGridHeight(ctx, terrain, base_cache, ix0 + ix, iz0 + iz);

			HydraulicErosionParams params; // defaults - "slight" is controlled via b.droplets, not by loosening these
			ErodeHeightmap(map, n, b.droplets, params, b.seed);

			for(int iz = 0; iz < n; iz++) {
				for(int ix = 0; ix < n; ix++) {
					double wx = (ix0 + ix) * kPaintGridStep, wz = (iz0 + iz) * kPaintGridStep;
					double dx = wx - b.cx, dz = wz - b.cz;
					double dist = sqrt(dx * dx + dz * dz);
					if(dist > b.radius)
						continue;
					double falloff = 1.0 - dist / b.radius;
					double base = ReadGridHeight(ctx, terrain, base_cache, ix0 + ix, iz0 + iz);
					double eroded = map[iz * n + ix];
					// Blend the erosion delta toward 0 at the radius edge
					// - same "meets the surroundings smoothly" reasoning
					// as the flatten brush above, and doubly important
					// here since ErodeHeightmap also perturbs nodes near
					// the window's own edge from droplets that wandered
					// in from outside the requested circle.
					ctx.Write(PaintHeightKey(ix0 + ix, iz0 + iz), base + (eroded - base) * falloff);
				}
			}
		},
		[&terrain, &base_cache](ActionContext& ctx, const Value& param) -> bool {
			TerrainErosionBrush b = TerrainErosionBrush::FromValue(param);
			int cix = PaintGridIndex(b.cx), ciz = PaintGridIndex(b.cz);
			// A generous fixed slope_limit, not a brush field - Task 17's
			// own erosion touch-up is meant to add gentle natural
			// texture, not to be rejected/tuned via a fit_check the way
			// a hand-placed raise/flatten brush's own slope_limit is;
			// still real protection against ErodeHeightmap ever
			// producing a pathological spike.
			return CheckPaintSlope(ctx, terrain, base_cache, cix, ciz, 50.0);
		});
}

void CompileTerrainPaintToTerrain(const ActionGraph& graph, Terrain& terrain)
{
	for(const String& key : graph.GetStoreKeys()) {
		int ix, iz;
		if(!ParsePaintHeightKey(key, ix, iz))
			continue;
		double h = graph.GetStoreValue(key);
		terrain.ApplyPaintedHeight(ix * kPaintGridStep, iz * kPaintGridStep, h);
	}
}

void InvalidateWorldChunksForPaint(const ActionGraph& graph, WorldBuilder& city)
{
	bool any = false;
	double minx = 0, maxx = 0, minz = 0, maxz = 0;
	for(const String& key : graph.GetStoreKeys()) {
		int ix, iz;
		if(!ParsePaintHeightKey(key, ix, iz))
			continue;
		double wx = ix * kPaintGridStep, wz = iz * kPaintGridStep;
		if(!any) {
			minx = maxx = wx;
			minz = maxz = wz;
			any = true;
		}
		else {
			minx = min(minx, wx);
			maxx = max(maxx, wx);
			minz = min(minz, wz);
			maxz = max(maxz, wz);
		}
	}
	if(!any)
		return;

	Vec2 center((minx + maxx) / 2, (minz + maxz) / 2);
	// Padded by one grid step so a painted node sitting right at a
	// chunk's own edge still counts as overlapping it.
	Vec2 half_extent((maxx - minx) / 2 + kPaintGridStep, (maxz - minz) / 2 + kPaintGridStep);
	city.InvalidateContentNear(center, half_extent);
}

END_UPP_NAMESPACE
