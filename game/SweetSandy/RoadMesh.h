#ifndef _SweetSandy_RoadMesh_h_
#define _SweetSandy_RoadMesh_h_

// Meters - matches City Layout's own established scale (dsep=15,
// MapGeneratorCityLayoutGenerator.cpp) without being tied to it; a
// small off-road-buggy-scale road, not a highway lane. Promoted from
// RoadCreatorMeshGenerator.cpp's own anonymous-namespace constant once
// GroundMeshBuilder (Unified Ground phase, Task 3) became a second
// real consumer - needs the identical value to inset a block's own
// grass boundary right up to the real ribbon edge, not its own
// independent guess.
const double kRoadWidth = 4.0;

// Road Network phase, Task 1: a real triangulated ribbon mesh for one
// road centerline - port of RoadCreator's RoadSegment.GenerateMesh, see
// Manager/2-plan/.../004-road-network/001-task1-ribbon-mesh.md.
//
// Deliberately 2D (no height): every other generated-content drawing in
// this game (PlayScreen's City Layout wireframe) resolves ground height
// at *use* time via Terrain::HeightAt rather than storing it - Terrain
// is the single source of truth for height (see Terrain.h). Road mesh
// vertices follow the same convention; Task 2 flattens Terrain's own
// heightmap under the road, so Terrain::HeightAt along it already
// returns the correct, flattened height with no separate per-vertex
// height storage needed here.
struct RoadMeshVertex : Moveable<RoadMeshVertex> {
	Vec2 pos;
	Vec2 uv; // u: 0 (left edge) or 1 (right edge); v: cumulative arc-length
	         // in meters along that edge - untiled, no texture/material
	         // system exists yet to consume this (see the phase's
	         // planning doc), kept simple until something does.

	// Task 4: plugs into U++'s generic Stream serialization the same way
	// Vec2 does (SandyMath.h) - lets WorldChunkContent::Serialize round-
	// trip road_mesh for free through Vector<T>'s own container support.
	void Serialize(Stream& s) { s % pos % uv; }
};

struct RoadMesh : Moveable<RoadMesh> {
	Vector<RoadMeshVertex> vertices; // pairs: [2*i]=left edge, [2*i+1]=right edge, per centerline point i
	Vector<int> triangles;           // indices into vertices, 3 per triangle

	void Serialize(Stream& s) { s % vertices % triangles; }
};

#endif
