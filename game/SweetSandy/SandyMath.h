#ifndef _SweetSandy_SandyMath_h_
#define _SweetSandy_SandyMath_h_

// inline (not static): this header is included from several .cpp files,
// and BLITZ sometimes merges them into one translation unit for build
// speed - a `static` helper of the same name+signature in two of those
// files then collides ("redefinition"). inline is the correct fix, not
// per-file `static` copies (that was the actual bug that triggered this).
inline double Clampd(double v, double lo, double hi) { return v < lo ? lo : v > hi ? hi : v; }
inline int    Clampi(int v, int lo, int hi)           { return v < lo ? lo : v > hi ? hi : v; }
inline double Smooth01(double t) { t = Clampd(t, 0.0, 1.0); return t * t * (3 - 2 * t); }

// A value in [lo, hi) from this project's own Randomf() (Upp::Randomf(),
// seeded via SeedRandom() - see any generator's own "deterministic per
// chunk/instance" comment for why this project uses that instead of a
// ported RNG). Promoted here from VegTreeSkeletonGenerator.cpp once a
// second file (VegChunkGenerator.cpp) needed the exact same one-liner -
// not written speculatively for a hypothetical future consumer.
inline double RandomRange(double lo, double hi) { return lo + Randomf() * (hi - lo); }

// Minimal 3D vector math for this demo - deliberately not a general-purpose
// library (see AGENTS.md: uppsrc/Geometry pulls in Eigen/assimp/AI/ByteVM,
// which this phase avoids). Extend only when the demo actually needs more.
struct Vec3 : Moveable<Vec3> {
	double x = 0, y = 0, z = 0;

	Vec3() {}
	Vec3(double x, double y, double z) : x(x), y(y), z(z) {}

	Vec3 operator+(const Vec3& b) const { return Vec3(x + b.x, y + b.y, z + b.z); }
	Vec3 operator-(const Vec3& b) const { return Vec3(x - b.x, y - b.y, z - b.z); }
	Vec3 operator-() const              { return Vec3(-x, -y, -z); }
	Vec3 operator*(double s) const      { return Vec3(x * s, y * s, z * s); }

	double Dot(const Vec3& b) const { return x * b.x + y * b.y + z * b.z; }
	Vec3   Cross(const Vec3& b) const {
		return Vec3(y * b.z - z * b.y, z * b.x - x * b.z, x * b.y - y * b.x);
	}
	double Length() const { return sqrt(Dot(*this)); }
	Vec3   Normalized() const {
		double l = Length();
		return l > 1e-12 ? *this * (1.0 / l) : Vec3(0, 1, 0);
	}

	// Vegetation phase, Task 1: plugs Vec3 into U++'s generic Stream
	// serialization the same way Vec2 already does below - needed for
	// VegMesh's Vec3-valued vertices/normals (see VegMesh.h) and
	// VegTreeInstance's placement (Vegetation Task 4) to round-trip
	// through WorldChunkContent::Serialize.
	void Serialize(Stream& s) { s % x % y % z; }
};

inline Vec3 operator*(double s, const Vec3& v) { return v * s; }

// Ground-plane (x/z world axes) 2D vector - City Layout phase (see
// Manager/2-plan/.../003-city-layout/001-planning.md): the tensor-field/
// streamline/graph/polygon algorithm is inherently 2D, so it works in
// its own Vec2 rather than an under-used Vec3 with a dummy 3rd component.
// Same "deliberately not a general-purpose library" scope as Vec3 above.
// Tagged Moveable (like Vec3, retroactively) since Task 2 stores these in
// Vector<Vec2> - see AGENTS.md/other memory notes on why untagged value
// types in Vector/VectorMap are a footgun in this codebase.
struct Vec2 : Moveable<Vec2> {
	double x = 0, y = 0;

	Vec2() {}
	Vec2(double x, double y) : x(x), y(y) {}

	Vec2 operator+(const Vec2& b) const { return Vec2(x + b.x, y + b.y); }
	Vec2 operator-(const Vec2& b) const { return Vec2(x - b.x, y - b.y); }
	Vec2 operator-() const              { return Vec2(-x, -y); }
	Vec2 operator*(double s) const      { return Vec2(x * s, y * s); }

	bool operator==(const Vec2& b) const { return x == b.x && y == b.y; } // exact, matches MapGenerator's Vector.equals

	double Dot(const Vec2& b) const   { return x * b.x + y * b.y; }
	double Cross(const Vec2& b) const { return x * b.y - y * b.x; } // z of the 3D cross product
	double LengthSq() const { return Dot(*this); }
	double Length() const   { return sqrt(LengthSq()); }
	Vec2   Normalized() const {
		double l = Length();
		return l > 1e-12 ? *this * (1.0 / l) : Vec2(1, 0);
	}

	// Plugs Vec2 into U++'s generic Stream serialization (Stream::
	// operator%) - makes Vector<Vec2>/Vector<Vector<Vec2>> serializable
	// for free via Vector<T>::Serialize's existing container support,
	// used by WorldChunkContent::Serialize/Deserialize (WorldGenStages.cpp).
	void Serialize(Stream& s) { s % x % y; }
};

inline Vec2 operator*(double s, const Vec2& v) { return v * s; }

// Signed area (shoelace formula) of a simple polygon - poly is
// implicitly closed (last point connects back to poly[0], not
// duplicated), matching CityBlockFinder's own polygon convention.
// Positive/negative sign indicates winding order; callers needing area
// alone use fabs(). Promoted from main.cpp's own CityShoelaceArea (was
// a test-only static helper) once Buildings phase Task 1 became a
// second, non-test consumer - see Manager/2-plan/.../008-buildings/
// 001-planning.md.
inline double CityShoelaceArea(const Vector<Vec2>& poly)
{
	double a = 0;
	for(int i = 0; i < poly.GetCount(); i++) {
		Vec2 p = poly[i], q = poly[(i + 1) % poly.GetCount()];
		a += p.x * q.y - q.x * p.y;
	}
	return a / 2;
}

// Ray-casting / crossing-number point-in-polygon test - works for any
// simple polygon, convex or not (Terrain.cpp's own PointInTriangle/
// PointInQuad are fixed-arity and road-ribbon-specific, not reusable
// for an arbitrary N-gon city block). poly is implicitly closed, same
// convention as CityShoelaceArea above. Added for Buildings phase
// Task 1 (block-interior placement) - grepped uppsrc/ first per this
// project's own ODR-collision rule: one hit, a `static` (internal-
// linkage, so no real collision risk) function of the same name in
// the unrelated ModelerApp package.
inline bool PointInPolygon(const Vector<Vec2>& poly, Vec2 p)
{
	bool inside = false;
	for(int i = 0, j = poly.GetCount() - 1; i < poly.GetCount(); j = i++) {
		Vec2 a = poly[i], b = poly[j];
		if(((a.y > p.y) != (b.y > p.y)) &&
		   (p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x))
			inside = !inside;
	}
	return inside;
}

// Promoted from VegChunkGenerator.cpp's own anonymous-namespace
// helpers once Buildings phase Task 1 became a second consumer of the
// identical "how far is this point from the nearest road" computation
// (its own road-avoidance check, alongside Vegetation's tree-placement
// one) - see Manager/2-plan/.../008-buildings/001-planning.md.
inline double DistancePointToSegment(Vec2 p, Vec2 a, Vec2 b)
{
	Vec2 ab = b - a;
	double len_sq = ab.LengthSq();
	double t = len_sq > 1e-12 ? Clampd((p - a).Dot(ab) / len_sq, 0.0, 1.0) : 0.0;
	Vec2 closest = a + ab * t;
	return (p - closest).Length();
}

inline double DistanceToNearestRoad(Vec2 p, const Vector<Vector<Vec2>>& road_centerlines)
{
	double best = 1e300;
	for(const Vector<Vec2>& line : road_centerlines)
		for(int i = 0; i + 1 < line.GetCount(); i++)
			best = min(best, DistancePointToSegment(p, line[i], line[i + 1]));
	return best;
}

// Average of a polygon's vertices - not a true area centroid, but cheap
// and good enough for both PlayScreen's own inward-nudge block-outline
// drawing and GroundMeshBuilder's own inset-direction determination
// (Unified Ground phase, Task 3) - promoted from a PlayScreen.cpp-local
// static on its second real consumer, same pattern as every other
// helper in this file.
inline Vec2 CityPolygonAverage(const Vector<Vec2>& poly)
{
	Vec2 sum(0, 0);
	for(Vec2 v : poly)
		sum = sum + v;
	return poly.IsEmpty() ? sum : sum * (1.0 / poly.GetCount());
}

#endif
