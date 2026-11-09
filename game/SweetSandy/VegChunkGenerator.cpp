#include "SweetSandy.h"

NAMESPACE_UPP

namespace {

// Meters between candidate placement points (jittered grid, see
// Generate() below) - a modest budget, not "as many trees as would
// look photorealistic", per 001-planning.md's own named draw-cost risk.
// Not exposed as a tunable yet; revisit if the real measured triangle/
// tree counts (see --vegtest) say this needs adjusting.
// Tuned against a real measurement (--vegtest), not guessed: the first
// values tried (spacing=20, avoid=6) placed only 1 tree in a 90x90m
// chunk - City Layout's own street spacing (dsep=15m) means a 6m
// avoidance radius on every road excludes most of a chunk's area, and a
// 20m candidate grid left too few surviving points to begin with. These
// values were re-measured after adjusting and give a visibly denser,
// still road-clear result - see 004-task4-worldbuilder-integration.md's
// own verification section.
const double kVegGridSpacing = 14.0;

// Meters from a road centerline segment a tree must clear - keeps a
// tree's canopy clear of the ~4m-wide road ribbon (Road Network's own
// kRoadWidth) plus a real margin, not just "not literally on the
// centerline".
const double kRoadAvoidRadius = 4.5;

// Meters a tree's own trunk must clear a building's own footprint -
// Unified Ground phase, Task 2 (fixes trees rendering inside
// buildings). Deliberately its own constant, not reused from
// kRoadAvoidRadius or Buildings phase's own kBuildingSetback - a
// similar-looking value here would be a coincidence worth being
// suspicious of, not a simplification: this clears a trunk (checked
// against the tree's own placement *point*, not its full canopy -
// grid spacing/jitter already keeps canopies reasonably apart the same
// way it always has), a different physical thing than a building
// footprint corner clearing a road's own paved width.
const double kBuildingAvoidClearance = 1.5;

// Deterministic per (world_seed, chunk coord), same idea as
// MapGeneratorCityLayoutGenerator's own DeterministicChunkSeed - but
// salted distinctly ("veg" folded into the hash) so this generator's
// own RNG stream doesn't start at the exact same point that OTHER
// generator already reseeded the shared RNG to for this chunk, which
// would otherwise correlate vegetation placement with city layout's own
// first few random draws for no reason.
dword DeterministicVegSeed(const String& world_seed, const WorldChunkCoord& coord)
{
	hash_t h = CombineHash(world_seed.GetHashValue(), coord.lod, coord.x, coord.y);
	h = CombineHash(h, coord.z);
	h = CombineHash(h, 'v', 'e', 'g');
	return (dword)h;
}

// Chunk LOD -> mesh detail: coarser chunk LOD (farther-away, less-
// detailed content) gets a more reduced tree mesh. A whole-chunk-
// granularity proxy for "how far is this from the viewer", not true
// per-instance distance LOD (see 001-planning.md's own "per-instance
// LOD deferred" call) - a reasonable first cut, revisit if it looks
// wrong in practice. Roughly matches ez-tree's own defaultLODLevels
// proportions (LOD1 ~40% of full triangle count, LOD2 ~20%).
VegMeshDetail DetailForLod(int lod)
{
	VegMeshDetail d;
	if(lod <= 0)
		return d; // full detail
	if(lod == 1) {
		d.section_stride = 2;
		d.segment_factor = 0.6;
		d.leaf_stride = 2;
		d.leaf_scale = 1.3;
		return d;
	}
	d.section_stride = 3;
	d.segment_factor = 0.4;
	d.leaf_stride = 3;
	d.leaf_scale = 1.5;
	d.force_single_billboard = true;
	return d;
}

struct PlannedTree : Moveable<PlannedTree> {
	Vec2 pos;
	double yaw = 0;
	VegTreePreset preset = VegTreePreset::Oak;
	dword seed = 0;
};

} // anonymous namespace

void VegChunkGenerator::Generate(const WorldChunk& chunk, WorldChunkContent& content)
{
	if(chunk.half_extent.x <= 0 || chunk.half_extent.z <= 0)
		return; // degenerate chunk bounds - nothing to place

	SeedRandom(DeterministicVegSeed(world_seed, chunk.coord));

	double min_x = chunk.center.x - chunk.half_extent.x;
	double max_x = chunk.center.x + chunk.half_extent.x;
	double min_z = chunk.center.z - chunk.half_extent.z;
	double max_z = chunk.center.z + chunk.half_extent.z;

	// Pass 1: decide placement (position, yaw, species, per-tree seed)
	// for every accepted candidate, consuming one uninterrupted chunk-
	// level RNG stream. Kept separate from pass 2 (actually growing each
	// tree) on purpose: VegTreeSkeletonGenerator::Generate reseeds the
	// shared RNG independently per tree (Task 2's own self-contained
	// convention), so if placement decisions were interleaved with tree
	// growth, a later candidate's position/species would end up
	// depending on exactly how many RNG draws an earlier tree's own
	// skeleton happened to consume - still technically deterministic,
	// but needlessly entangled. Separating the passes avoids that.
	Vector<PlannedTree> planned;
	int candidate_index = 0;
	for(double gx = min_x + kVegGridSpacing * 0.5; gx < max_x; gx += kVegGridSpacing) {
		for(double gz = min_z + kVegGridSpacing * 0.5; gz < max_z; gz += kVegGridSpacing) {
			double x = gx + RandomRange(-kVegGridSpacing * 0.5, kVegGridSpacing * 0.5);
			double z = gz + RandomRange(-kVegGridSpacing * 0.5, kVegGridSpacing * 0.5);
			Vec2 p(x, z);

			// candidate_index advances regardless of accept/reject, so
			// the seed sequence for LATER candidates doesn't shift just
			// because road layout rejected an EARLIER one.
			int this_index = candidate_index++;

			if(DistanceToNearestRoad(p, content.road_centerlines) < kRoadAvoidRadius)
				continue;

			// Unified Ground phase, Task 2: reject a candidate whose own
			// trunk would land inside (or flush against) a building's own
			// footprint - a conservative footprint-bounding-circle test,
			// the same shape PlanBuildingsForBlock's own building-to-
			// building overlap check already uses (Buildings phase, Task
			// 1). Only possible now that building_gen runs before
			// veg_gen (WorldBuilder::Update's own reordered call sequence
			// - see that file's own comment on why the reorder is safe).
			bool inside_building = false;
			for(const BuildingInstance& b : content.buildings) {
				double building_radius = 0.5 * sqrt(b.width * b.width + b.depth * b.depth);
				if((p - Vec2(b.position.x, b.position.z)).Length() < building_radius + kBuildingAvoidClearance) {
					inside_building = true;
					break;
				}
			}
			if(inside_building)
				continue;

			PlannedTree t;
			t.pos = p;
			t.yaw = Randomf() * 2 * M_PI;
			t.preset = (Randomf() < 0.5) ? VegTreePreset::Oak : VegTreePreset::Pine;
			t.seed = (dword)CombineHash(DeterministicVegSeed(world_seed, chunk.coord), this_index);
			planned.Add(t);
		}
	}

	// Pass 2: grow and mesh each planned tree (Tasks 2-3). Each
	// VegTreeSkeletonGenerator::Generate call reseeds the shared RNG
	// from its own per-tree seed, so running this after pass 1 has
	// already fully consumed the chunk-level stream doesn't affect
	// determinism - by design, per the comment above.
	VegMeshDetail detail = DetailForLod(chunk.coord.lod);
	for(const PlannedTree& t : planned) {
		VegTreeOptions opt = MakeVegTreePreset(t.preset, t.seed);
		VegSkeleton skeleton = VegTreeSkeletonGenerator::Generate(opt);
		VegMesh branches, leaves;
		VegMeshBuilder::Build(skeleton, opt, detail, branches, leaves);

		VegTreeInstance tree;
		tree.position = Vec3(t.pos.x, terrain.HeightAt(t.pos.x, t.pos.y), t.pos.y);
		tree.yaw = t.yaw;
		tree.trunk_radius = opt.branch.radius[0];
		tree.trunk_height = opt.branch.length[0];
		tree.bark_tint = opt.bark_tint;
		tree.leaf_tint = opt.leaf_tint;
		tree.preset = t.preset;
		tree.branches = pick(branches);
		tree.leaves = pick(leaves);
		content.vegetation.Add(pick(tree));
	}
}

END_UPP_NAMESPACE
