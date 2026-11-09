#include "SweetSandy.h"

NAMESPACE_UPP

namespace {

// Fallback grid cell size for whatever a chunk's own area isn't already
// covered by a block's own precise grass mesh or (via road proximity) by
// asphalt - this is a "don't leave a total void" safety net, not meant
// to look as detailed as real block-fitted grass.
//
// Two earlier, more lossy versions both got caught by
// RunGroundChunkGeneratorSelfTest's own real-block coverage check and by
// --groundpic's own visual capture (sky-blue voids right where a road
// merely grazed a cell): a 10m version that required ALL 4 corners of
// each cell clear (only 37.7% of a real 100x100m chunk covered - a road
// grazing just one corner voided the *whole* cell), and a 5m version
// classified by center point but still one separate VegMesh per cell
// (only 54.7% covered, same underlying problem at a smaller scale).
// GroundMeshBuilder::BuildFlatGroundGrid now takes the clearance
// predicate itself and classifies per-cell at 1m resolution *inside* one
// shared mesh for the whole chunk (no more "reject the whole cell's
// VegMesh" cost to shrinking the cell further) - a 1m cell's own
// rejection wastes at most 1m^2, small enough that any remaining edge
// isn't visible while actually driving.
const double kFallbackCellSize = 1.0;

bool ClearOfBlocksAndRoads(Vec2 p, const Vector<Vector<Vec2>>& inset_blocks,
                            const Vector<Vector<Vec2>>& road_centerlines)
{
	if(DistanceToNearestRoad(p, road_centerlines) < kRoadWidth * 0.5)
		return false; // would overlap the real asphalt
	for(const Vector<Vec2>& block : inset_blocks)
		if(PointInPolygon(block, p))
			return false; // would overlap a block's own precise grass mesh
	return true;
}

} // anonymous namespace

void GroundChunkGenerator::Generate(const WorldChunk& chunk, WorldChunkContent& content)
{
	// One precise grass mesh per city block, inset to the real road
	// edge (same inset distance RunGroundMeshBuilderSelfTest's own
	// "real generated blocks" check already uses) - Task 3's own
	// builder, no generator-level logic beyond calling it per block.
	Vector<Vector<Vec2>> inset_blocks; // kept to test the fallback grid below against
	for(const Vector<Vec2>& block : content.blocks) {
		Vector<Vec2> inset = GroundMeshBuilder::InsetBlockPolygon(block, kRoadWidth * 0.5);
		if(inset.IsEmpty())
			continue;
		VegMesh mesh = GroundMeshBuilder::Build(inset, terrain);
		if(mesh.triangles.IsEmpty())
			continue;
		content.ground.Add(pick(mesh));
		inset_blocks.Add(pick(inset));
	}

	// Fallback fill for the rest of this chunk's own area - a chunk with
	// zero road content at all just gets 100% of this, as one single
	// mesh (not one per cell - see kFallbackCellSize's own comment).
	double min_x = chunk.center.x - chunk.half_extent.x;
	double max_x = chunk.center.x + chunk.half_extent.x;
	double min_z = chunk.center.z - chunk.half_extent.z;
	double max_z = chunk.center.z + chunk.half_extent.z;
	if(max_x <= min_x || max_z <= min_z)
		return; // degenerate chunk bounds - nothing to fill

	VegMesh fallback = GroundMeshBuilder::BuildFlatGroundGrid(
		Vec2(min_x, min_z), Vec2(max_x, max_z), kFallbackCellSize, terrain,
		[&inset_blocks, &content](Vec2 p) { return ClearOfBlocksAndRoads(p, inset_blocks, content.road_centerlines); });
	if(!fallback.triangles.IsEmpty())
		content.ground.Add(pick(fallback));
}

END_UPP_NAMESPACE
