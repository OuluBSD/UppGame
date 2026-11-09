#ifndef _SweetSandy_GroundMeshBuilder_h_
#define _SweetSandy_GroundMeshBuilder_h_

// Unified Ground phase, Task 3: a grass mesh that shares vertices with
// the real asphalt edge (no overlap, no y_offset hack) and still shows
// Terrain's own rolling-hill height variation - see Manager/2-plan/
// .../009-unified-ground/003-task3-ground-fill-mesh.md for the full
// scope reasoning (per-block inset, not a full road-network boundary
// trace/union - a deliberate, documented first-cut scope call).
class GroundMeshBuilder {
public:
	// Shrinks a city block polygon (CityBlockFinder's own output -
	// implicitly closed, same convention as PointInPolygon/
	// CityShoelaceArea) inward by inset_distance, using the identical
	// perpendicular-offset formula RoadMeshBuilder::BuildRibbon already
	// uses for its own ribbon edges - so the result sits right at the
	// real asphalt edge without needing to trace or union the actual
	// road mesh. Returns an empty Vector if the block is degenerate (<3
	// vertices) or the inset would self-intersect/collapse (checked via
	// the result's own shoelace area against a small positive floor) -
	// callers should treat an empty result as "skip this block",
	// falling back to BuildFlatGroundGrid for that area instead of
	// feeding a broken polygon to poly2tri.
	static Vector<Vec2> InsetBlockPolygon(const Vector<Vec2>& block, double inset_distance);

	// Triangulates the (already inset) polygon's own interior via
	// poly2tri, with interior Steiner points sampling terrain.HeightAt
	// so a large open area still shows real hill shape instead of
	// collapsing into one or two huge flat triangles (see the task
	// doc's own note on why boundary-only triangulation isn't enough).
	// Per-triangle normals from the actual (possibly sloped) geometry,
	// not a blanket straight-up normal. Returns an empty VegMesh if
	// inset_boundary has fewer than 3 points.
	static VegMesh Build(const Vector<Vec2>& inset_boundary, const Terrain& terrain);

	// A plain regular grid of flat quads (2 triangles each, y =
	// terrain.HeightAt per vertex) covering [min_pt,max_pt] - no
	// poly2tri involved, nothing to conform to. Used for whatever area
	// isn't covered by any (successfully inset) block - a chunk with no
	// road content at all just gets 100% of this. is_clear, when set, is
	// called once per cell (its own center point) - a cell it rejects is
	// simply left out of the one returned mesh, not built at all; pass an
	// empty Function to fill the whole grid unconditionally.
	static VegMesh BuildFlatGroundGrid(Vec2 min_pt, Vec2 max_pt, double cell_size, const Terrain& terrain,
	                                    const Function<bool(Vec2)>& is_clear = Function<bool(Vec2)>());
};

#endif
