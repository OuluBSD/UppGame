#include "SweetSandy.h"

NAMESPACE_UPP

static dword DeterministicChunkSeed(const String& world_seed, const WorldChunkCoord& coord)
{
	hash_t h = CombineHash(world_seed.GetHashValue(), coord.lod, coord.x, coord.y);
	h = CombineHash(h, coord.z);
	return (dword)h;
}

// dstep is a fixed real-world-scale constant (not chunk-dependent - see
// GenerateCell's own comment on dsep/dstep), and Generate() needs it
// too (for the merged graph's own fuzzy-merge tolerance) - named once
// here so the two stay the same value by construction, not by
// duplicated literals.
static const double kDstep = 1.0;

// The actual tensor-field/streamline algorithm, run over exactly one
// cell's own bounds and seeded by that cell's own coordinate - this is
// everything Generate() used to do directly before this file started
// deriving coarser LOD chunks from lod-0 cells (see this file's own
// header comment). Appends into out_road_centerlines rather than
// returning/clearing it, so Generate() can call this once (lod 0) or
// several times in a row (lod > 0, one call per covered lod-0 cell) and
// end up with one merged set either way.
void MapGeneratorCityLayoutGenerator::GenerateCell(const WorldChunkCoord& cell_coord, Vec2 cell_centre,
                                                     Vec2 cell_world_dims, Vector<Vector<Vec2>>& out_road_centerlines) const
{
	if(cell_world_dims.x <= 0 || cell_world_dims.y <= 0)
		return; // degenerate cell bounds - nothing to generate

	Vec2 origin(cell_centre.x - cell_world_dims.x * 0.5, cell_centre.y - cell_world_dims.y * 0.5);

	// Deterministic per (world_seed, cell coord): reseeds the shared
	// global RNG (Randomf(), used throughout CityStreamlineGenerator) so
	// this cell always generates the same layout regardless of what
	// generation order other cells/chunks loaded in - required for a
	// streamed world where chunk load order depends on player movement.
	// Harmless while nothing else in the game consumes Randomf()
	// concurrently with world generation; revisit if that changes.
	SeedRandom(DeterministicChunkSeed(world_seed, cell_coord));
	double theta = Randomf() * M_PI;

	CityTensorField field;
	// field_size comfortably covers the cell's diagonal (half-extent's
	// worst case is *sqrt(2)) so CityBasisField::GetTensorWeight's
	// falloff never kicks in inside the cell - uniform weight, same
	// setup this phase's own tests use.
	double field_size = max(cell_world_dims.x, cell_world_dims.y);
	field.AddGrid(cell_centre, field_size, 0, theta);

	// Fixed real-world-scale values, in the same "meters" unit every other
	// number in this game already uses (VehicleDef::chassis_size_z etc.,
	// Terrain::world_size) - *not* derived from this cell's own
	// world_dims. An earlier version set dsep = min_dim / 6 (a fraction of
	// the chunk), which quietly made block size purely a function of
	// arbitrary WorldOctree cell sizes instead of anything resembling a
	// real city block - at a 20m LOD0 chunk that gave ~3.3m street
	// spacing, roughly the length of the car itself ("kaupungin koko
	// suhteessa autoon vaikutti väärältä" - the reported bug).
	//
	// A first fix used dsep=45 (a real, if largish, city block dimension)
	// - measured against VehicleDef::chassis_size_z=3.12, that's a ~14.4x
	// ratio, and still looked disproportionate in practice ("katu
	// [oikeasti: auto?] olisi yli 5x suhteessa auton kokoon" - reported
	// after actually looking via --citytest). 15m (~4.8x the chassis
	// length) reads as a tighter, denser, more human/car-scaled block
	// grid - closer to a dense old-town block than a modern arterial
	// one, and closer to what this game's own vehicle scale (a small
	// off-road buggy, not a highway car) suggests the city around it
	// should feel like. Cell sizes (WorldOctree lod-0 cell size, set by
	// whoever calls WorldBuilder - see PlayScreen::StartRun) need to
	// stay comfortably larger than this or most cells will contain at
	// most one street - re-check if this is tuned again.
	CityStreamlineParams params;
	params.dsep = 15;                 // meters between parallel streets
	params.dtest = params.dsep * 0.75;
	params.dstep = kDstep;             // meters per integration step - halved with dsep so streamlines still sample ~15 points per block, not ~7
	params.dcirclejoin = params.dstep * 5;
	params.dlookahead = params.dsep * 2;
	params.joinangle = 0.2;
	params.path_iterations = 3000;    // finer dstep needs proportionally more steps to cover the same cell
	params.seed_tries = 200;
	params.simplify_tolerance = params.dstep * 0.5;
	params.collide_early = 0;

	CityStreamlineGenerator stream_gen(field, origin, cell_world_dims, params);
	stream_gen.CreateAllStreamlines();

	out_road_centerlines.Append(stream_gen.GetStreamlines(true));
	out_road_centerlines.Append(stream_gen.GetStreamlines(false));
}

void MapGeneratorCityLayoutGenerator::Generate(const WorldChunk& chunk, WorldChunkContent& content)
{
	if(chunk.half_extent.x <= 0 || chunk.half_extent.z <= 0)
		return; // degenerate chunk bounds - nothing to generate

	content.road_centerlines.Clear();

	if(chunk.coord.lod == 0) {
		// A real lod-0 chunk generates directly over its own bounds,
		// exactly as this method always has - no cell-subdivision
		// involved (this *is* a cell). Kept as its own path rather than
		// unconditionally going through the lod>0 loop below so every
		// existing lod-0 caller (most of this project's own self-tests
		// use arbitrary, not-necessarily-finest_cell_size-aligned
		// bounds) keeps behaving byte-identically to before this change.
		Vec2 centre(chunk.center.x, chunk.center.z);
		Vec2 world_dims(chunk.half_extent.x * 2, chunk.half_extent.z * 2);
		GenerateCell(chunk.coord, centre, world_dims, content.road_centerlines);
	}
	else {
		// A coarser chunk does not get its own independent run - see
		// this file's own header comment on why. Instead, find every
		// lod-0 cell (at finest_cell_size) this chunk's own bounds
		// cover, and generate+merge each of them exactly as if it were
		// really being requested as its own lod-0 chunk (same
		// DeterministicChunkSeed input: cell_coord.lod is always 0 here,
		// matching what WorldOctree itself would assign a real lod-0
		// chunk at that position) - so the result is not an
		// *approximation* of the finest layout, it is literally built
		// from the same per-cell generation.
		double min_x = chunk.center.x - chunk.half_extent.x, max_x = chunk.center.x + chunk.half_extent.x;
		double min_z = chunk.center.z - chunk.half_extent.z, max_z = chunk.center.z + chunk.half_extent.z;
		// Tiny epsilon so a bound landing exactly on a multiple of
		// finest_cell_size doesn't spuriously pull in one extra,
		// otherwise-untouched cell beyond the edge.
		const double eps = 1e-6;
		int gx0 = (int)floor(min_x / finest_cell_size);
		int gx1 = (int)floor((max_x - eps) / finest_cell_size);
		int gz0 = (int)floor(min_z / finest_cell_size);
		int gz1 = (int)floor((max_z - eps) / finest_cell_size);

		for(int gz = gz0; gz <= gz1; gz++) {
			for(int gx = gx0; gx <= gx1; gx++) {
				WorldChunkCoord cell_coord;
				cell_coord.lod = 0;
				cell_coord.x = gx;
				cell_coord.y = 0;
				cell_coord.z = gz;
				Vec3 cell_centre3 = WorldChunkCenterAt(cell_coord, finest_cell_size);
				Vec2 cell_centre(cell_centre3.x, cell_centre3.z);
				Vec2 cell_world_dims(finest_cell_size, finest_cell_size);
				GenerateCell(cell_coord, cell_centre, cell_world_dims, content.road_centerlines);
			}
		}
	}

	// Built once over the merged road network (not per-cell and then
	// combined) - a coarse chunk's own constituent cells only connect to
	// each other through this shared graph build, same fuzzy-merge
	// tolerance (dstep*2) City Layout Task 3 already established for
	// lod-0's own internal continuity.
	CityGraph graph;
	graph.Build(content.road_centerlines, kDstep * 2);

	content.blocks = CityBlockFinder::FindBlocks(graph, 100);
}

END_UPP_NAMESPACE
