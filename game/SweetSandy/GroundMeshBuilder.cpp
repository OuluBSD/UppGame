#include <plugin/poly2tri/poly2tri.h>
#include <vector>

#include "SweetSandy.h"

NAMESPACE_UPP

namespace {

// A floor a successfully-inset polygon's own area must clear - catches
// a self-intersected/collapsed inset (possible for a very narrow or
// sharply-notched block, where insetting can push opposite edges past
// each other) before it's ever fed to poly2tri, which expects a valid
// simple polygon. Not the same concern as BuildingPlacement.cpp's own
// kMinBlockArea (that gates whether to even TRY placing a building;
// this only detects "the inset math itself went wrong").
const double kMinGroundPatchArea = 10.0;

// Interior Steiner point grid spacing (Unified Ground phase, Task 3's
// own explicit reason for existing: a boundary-only triangulation of a
// large open block collapses into one or two huge flat triangles,
// losing Terrain's own hill shape). Starting value - not yet re-tuned
// against real generated blocks the way this project's own placement
// constants elsewhere had to be; revisit if real output looks too
// coarse or too expensive.
const double kSteinerSpacing = 6.0;

Vec3 UpwardTriangleNormal(Vec3 a, Vec3 b, Vec3 c)
{
	Vec3 n = (b - a).Cross(c - a).Normalized();
	return n.y < 0 ? -n : n; // ground never faces down
}

// poly2tri (like most constrained-Delaunay implementations) is fragile
// against near-duplicate or self-intersecting input - found the hard
// way, via a real crash (not just a wrong-but-harmless result) feeding
// it a real 27-vertex inset block polygon during this task's own
// "against real generated blocks, not just hand-built ones" self-test.
// Both checks below run BEFORE any p2t::Point is ever allocated, so a
// bad polygon is rejected (falls back to BuildFlatGroundGrid for that
// area) instead of ever reaching the library at all.

// Merges consecutive points closer than kMinEdgeLength - a real
// possibility here even for a valid (non-self-intersecting) polygon,
// since CityGraph nodes can legitimately sit very close together
// (fuzzy-merge tolerance, near-parallel streamlines meeting almost but
// not quite at the same point).
const double kMinEdgeLength = 0.1;

Vector<Vec2> DedupClosePoints(const Vector<Vec2>& poly)
{
	Vector<Vec2> out;
	for(Vec2 v : poly)
		if(out.IsEmpty() || (v - out.Top()).Length() >= kMinEdgeLength)
			out.Add(v);
	// Closing edge (last point back to first) needs the same check -
	// poly is implicitly closed, same convention as PointInPolygon.
	if(out.GetCount() > 1 && (out[0] - out.Top()).Length() < kMinEdgeLength)
		out.Remove(out.GetCount() - 1);
	return out;
}

// Standard orientation-based segment intersection test (proper
// crossing only - touching at a shared endpoint, which adjacent edges
// of the same polygon always do, is not counted as an intersection).
double Orient(Vec2 a, Vec2 b, Vec2 c)
{
	return (b - a).Cross(c - a);
}

bool SegmentsProperlyIntersect(Vec2 a1, Vec2 a2, Vec2 b1, Vec2 b2)
{
	double d1 = Orient(b1, b2, a1);
	double d2 = Orient(b1, b2, a2);
	double d3 = Orient(a1, a2, b1);
	double d4 = Orient(a1, a2, b2);
	return ((d1 > 0) != (d2 > 0)) && (d1 != 0 && d2 != 0)
	    && ((d3 > 0) != (d4 > 0)) && (d3 != 0 && d4 != 0);
}

// O(n^2) over all non-adjacent edge pairs - fine at this project's own
// real block vertex counts (~20-45), not meant for a general-purpose
// large-polygon path.
bool IsSimplePolygon(const Vector<Vec2>& poly)
{
	int n = poly.GetCount();
	for(int i = 0; i < n; i++) {
		Vec2 a1 = poly[i], a2 = poly[(i + 1) % n];
		for(int j = i + 1; j < n; j++) {
			if(j == i + 1)             // shares vertex poly[i+1]
				continue;
			if(i == 0 && j == n - 1)   // wraparound: edge n-1 ends at poly[0] = edge 0's own start
				continue;
			Vec2 b1 = poly[j], b2 = poly[(j + 1) % n];
			if(SegmentsProperlyIntersect(a1, a2, b1, b2))
				return false;
		}
	}
	return true;
}

} // anonymous namespace

Vector<Vec2> GroundMeshBuilder::InsetBlockPolygon(const Vector<Vec2>& raw_block, double inset_distance)
{
	// Dedup first - a zero-length edge (two consecutive near-duplicate
	// points) would make edge_dir.Normalized() below divide by ~zero.
	Vector<Vec2> block = DedupClosePoints(raw_block);
	int n = block.GetCount();
	if(n < 3)
		return Vector<Vec2>();

	// Proper mitered polygon inset, not a naive "average tangent,
	// offset straight by inset_distance" - an earlier draft did exactly
	// that (the same centered-difference-tangent idea RoadMeshBuilder::
	// BuildRibbon uses for its own OPEN ribbon, which only ever needs
	// one perpendicular per point, never a real corner) and undershot
	// badly at every real corner: for a square's own 90-degree corner,
	// the averaged-tangent's own perpendicular points along the
	// diagonal, so offsetting by inset_distance along it only moves
	// each axis by inset_distance/sqrt(2), not the full inset_distance
	// - caught immediately by this task's own hand-computable self-test
	// (a 20x20 square inset by 2m must land at exactly (2,2)-(18,18),
	// not ~(1.41,1.41)-(18.59,18.59)).
	//
	// Correct approach: per EDGE, compute its own inward unit normal;
	// per VERTEX, the two adjacent edges' own normals sum to the
	// vertex's own bisector direction, and the offset MAGNITUDE along
	// that bisector is inset_distance/cos(half the angle between the
	// two edge normals) - not inset_distance itself - so that the
	// result, measured perpendicular to EITHER adjacent edge, is
	// exactly inset_distance.
	Vector<Vec2> edge_normal;
	edge_normal.SetCount(n);
	for(int i = 0; i < n; i++) {
		Vec2 edge_dir = (block[(i + 1) % n] - block[i]).Normalized();
		edge_normal[i] = Vec2(-edge_dir.y, edge_dir.x);
	}

	// Sign determined once (from edge 0's own two candidate normal
	// directions against the polygon centroid), applied to every edge -
	// assumes a simple (non-self-intersecting) polygon, which
	// CityBlockFinder's own output always is.
	Vec2 centroid = CityPolygonAverage(block);
	Vec2 edge0_mid = (block[0] + block[1]) * 0.5;
	double sign = ((edge0_mid + edge_normal[0]) - centroid).Length()
	            < ((edge0_mid - edge_normal[0]) - centroid).Length() ? 1.0 : -1.0;
	for(int i = 0; i < n; i++)
		edge_normal[i] = edge_normal[i] * sign;

	Vector<Vec2> result;
	result.SetCount(n);
	for(int i = 0; i < n; i++) {
		Vec2 n1 = edge_normal[(i - 1 + n) % n]; // edge ending at vertex i
		Vec2 n2 = edge_normal[i];               // edge starting at vertex i
		Vec2 bisector = (n1 + n2);
		double blen = bisector.Length();
		if(blen < 1e-9) {
			// n1 and n2 point opposite ways (a near-180-degree reflex
			// spike) - no well-defined miter direction; offset straight
			// along n2 as a fallback, the area-floor check below is
			// what actually catches this case turning out degenerate.
			result[i] = block[i] + n2 * inset_distance;
			continue;
		}
		Vec2 dir = bisector * (1.0 / blen);
		double cos_half_angle = max(0.1, dir.Dot(n2)); // clamp - a near-0 divisor would blow the offset up toward infinity for a near-180-degree turn
		result[i] = block[i] + dir * (inset_distance / cos_half_angle);
	}

	if(fabs(CityShoelaceArea(result)) < kMinGroundPatchArea)
		return Vector<Vec2>();

	// The area floor above only catches a *collapsed* inset (near-zero
	// net area) - a self-intersecting (bowtie-shaped) result can still
	// have a "reasonable" signed area while being invalid input for
	// poly2tri, which expects a genuinely simple polygon. This is the
	// real gate; the area check above is a cheap early-out kept for the
	// (common) fully-collapsed case, not relied on alone anymore - a
	// real crash (not just a wrong-but-harmless triangulation) feeding
	// poly2tri a self-intersecting real block's own inset is what
	// caught this gap.
	Vector<Vec2> deduped = DedupClosePoints(result);
	if(deduped.GetCount() < 3 || !IsSimplePolygon(deduped))
		return Vector<Vec2>();

	return deduped;
}

VegMesh GroundMeshBuilder::Build(const Vector<Vec2>& raw_boundary, const Terrain& terrain)
{
	VegMesh out;
	// Defensive re-validation at this function's own boundary too, not
	// just inside InsetBlockPolygon - Build is a public entry point on
	// its own, and feeding poly2tri anything less than a clean, simple,
	// duplicate-free polygon is what caused a real crash during this
	// task's own development (see IsSimplePolygon's own comment).
	Vector<Vec2> inset_boundary = DedupClosePoints(raw_boundary);
	if(inset_boundary.GetCount() < 3 || !IsSimplePolygon(inset_boundary))
		return out;

	// p2t::CDT does not own the Point*s passed to its constructor/
	// AddPoint/AddHole (confirmed by reading sweep_context.cc's own
	// destructor - it deletes its own internal artificial points and
	// the Triangle/Edge structures it allocated, never the caller-
	// supplied polyline/Steiner points), so every allocation here is
	// tracked and freed in this one function, not left to CDT's own
	// cleanup or scattered across early-return paths.
	Vector<p2t::Point*> owned_points;

	std::vector<p2t::Point*> boundary;
	for(Vec2 v : inset_boundary) {
		p2t::Point *pt = new p2t::Point(v.x, v.y);
		owned_points.Add(pt);
		boundary.push_back(pt);
	}

	p2t::CDT cdt(boundary);

	double min_x = 1e300, max_x = -1e300, min_z = 1e300, max_z = -1e300;
	for(Vec2 v : inset_boundary) {
		min_x = min(min_x, v.x); max_x = max(max_x, v.x);
		min_z = min(min_z, v.y); max_z = max(max_z, v.y);
	}
	for(double x = min_x + kSteinerSpacing * 0.5; x < max_x; x += kSteinerSpacing)
		for(double z = min_z + kSteinerSpacing * 0.5; z < max_z; z += kSteinerSpacing) {
			if(!PointInPolygon(inset_boundary, Vec2(x, z)))
				continue;
			p2t::Point *pt = new p2t::Point(x, z);
			owned_points.Add(pt);
			cdt.AddPoint(pt);
		}

	cdt.Triangulate();

	for(p2t::Triangle *t : cdt.GetTriangles()) {
		p2t::Point *p0 = t->GetPoint(0);
		p2t::Point *p1 = t->GetPoint(1);
		p2t::Point *p2 = t->GetPoint(2);

		Vec3 v0(p0->x, terrain.HeightAt(p0->x, p0->y), p0->y);
		Vec3 v1(p1->x, terrain.HeightAt(p1->x, p1->y), p1->y);
		Vec3 v2(p2->x, terrain.HeightAt(p2->x, p2->y), p2->y);
		Vec3 normal = UpwardTriangleNormal(v0, v1, v2);

		int base = out.vertices.GetCount();
		VegMeshVertex mv0; mv0.pos = v0; mv0.normal = normal; mv0.uv = Vec2(0, 0);
		VegMeshVertex mv1; mv1.pos = v1; mv1.normal = normal; mv1.uv = Vec2(1, 0);
		VegMeshVertex mv2; mv2.pos = v2; mv2.normal = normal; mv2.uv = Vec2(0, 1);
		out.vertices.Add(mv0);
		out.vertices.Add(mv1);
		out.vertices.Add(mv2);
		out.triangles.Add(base);
		out.triangles.Add(base + 1);
		out.triangles.Add(base + 2);
	}

	for(p2t::Point *pt : owned_points)
		delete pt;

	return out;
}

VegMesh GroundMeshBuilder::BuildFlatGroundGrid(Vec2 min_pt, Vec2 max_pt, double cell_size, const Terrain& terrain,
                                                const Function<bool(Vec2)>& is_clear)
{
	VegMesh out;
	for(double x = min_pt.x; x < max_pt.x; x += cell_size) {
		double x1 = min(x + cell_size, max_pt.x);
		for(double z = min_pt.y; z < max_pt.y; z += cell_size) {
			double z1 = min(z + cell_size, max_pt.y);

			// Per-cell clearance is decided at the cell's own center, one
			// point per cell rather than one VegMesh per cell (see
			// GroundChunkGenerator.h's own kFallbackCellSize comment for
			// why per-cell classification exists at all, and why small
			// cells beat "all 4 corners clear" at a coarser size) -
			// skipped cells just leave a hole in this one shared mesh,
			// not a separate rejected VegMesh, keeping the real per-chunk
			// mesh/draw-call count from scaling with 1/cell_size^2.
			if(is_clear && !is_clear(Vec2((x + x1) * 0.5, (z + z1) * 0.5)))
				continue;

			Vec3 p00(x, terrain.HeightAt(x, z), z);
			Vec3 p10(x1, terrain.HeightAt(x1, z), z);
			Vec3 p11(x1, terrain.HeightAt(x1, z1), z1);
			Vec3 p01(x, terrain.HeightAt(x, z1), z1);

			// The quad's own 4 corners are not necessarily coplanar
			// (each corner's own height comes independently from
			// Terrain::HeightAt) - unlike a flat wall/roof quad, the
			// two triangles here can have genuinely different normals,
			// so each gets its own 3 fresh vertices (6 total, not 4
			// shared) rather than reusing BuildingMeshBuilder's own
			// shared-4-vertex quad helper, which assumes coplanarity.
			Vec3 n0 = UpwardTriangleNormal(p00, p10, p11);
			Vec3 n1 = UpwardTriangleNormal(p00, p11, p01);

			int base = out.vertices.GetCount();
			VegMeshVertex a0; a0.pos = p00; a0.normal = n0; a0.uv = Vec2(0, 0);
			VegMeshVertex a1; a1.pos = p10; a1.normal = n0; a1.uv = Vec2(1, 0);
			VegMeshVertex a2; a2.pos = p11; a2.normal = n0; a2.uv = Vec2(1, 1);
			out.vertices.Add(a0);
			out.vertices.Add(a1);
			out.vertices.Add(a2);
			out.triangles.Add(base);
			out.triangles.Add(base + 1);
			out.triangles.Add(base + 2);

			base = out.vertices.GetCount();
			VegMeshVertex b0; b0.pos = p00; b0.normal = n1; b0.uv = Vec2(0, 0);
			VegMeshVertex b1; b1.pos = p11; b1.normal = n1; b1.uv = Vec2(1, 1);
			VegMeshVertex b2; b2.pos = p01; b2.normal = n1; b2.uv = Vec2(0, 1);
			out.vertices.Add(b0);
			out.vertices.Add(b1);
			out.vertices.Add(b2);
			out.triangles.Add(base);
			out.triangles.Add(base + 1);
			out.triangles.Add(base + 2);
		}
	}
	return out;
}

END_UPP_NAMESPACE
