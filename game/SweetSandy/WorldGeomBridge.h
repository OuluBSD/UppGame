#ifndef _SweetSandy_WorldGeomBridge_h_
#define _SweetSandy_WorldGeomBridge_h_

// Map Editor phase, Task 10: converts WorldBuilder's own generated,
// currently-loaded chunk content into real GeomObject/Model geometry
// inside a GeomScene, so Edit3D's own viewports (in particular its
// already-existing free-fly VIEWMODE_PERSPECTIVE viewport) can actually
// show something - see 012-map-editor/010-task10-terrain-content-bridge.md.
// One GeomObject per loaded chunk (not per tree/building instance - see
// that doc's own "why merged" note); ground/road/building/vegetation
// content each merged into one Mesh per chunk (buildings/vegetation
// further split per distinct tint, since a Mesh has only one Material),
// flat-tinted (no texture system, matching every other mesh in this
// project). Building/vegetation source meshes are local-space
// (BuildingMeshBuilder/tree meshing both bake at the origin) and are
// transformed into world space here exactly like PlayScreen's own
// DrawBuildingsGL/DrawVegetationGL do (translate(position) then
// rotateY(yaw), yaw in radians) - not a second, independent transform
// convention.
struct WorldGeomBridgeStats {
	int chunks = 0, objects = 0, meshes = 0, triangles = 0;
};

// Adds one GeomObject per currently-loaded chunk in `city` (skips a
// chunk with no content - e.g. one whose generation is still in flight;
// call after WorldBuilder::WaitIdle() for a complete bake). `terrain` is
// needed to resolve road_mesh's own 2D vertices to world height (see
// RoadMesh.h's own convention). Never clears `scene` first - the caller
// (MapEditorWindow's "Bake" action) owns that decision, e.g. to remove
// only its own previous bake's objects before calling this again.
WorldGeomBridgeStats BuildWorldGeomObjects(GeomScene& scene, const WorldBuilder& city, const Terrain& terrain);

// Map Editor phase, Task 11: the first real hand-authored (not
// procedurally generated) piece of level content - a simple two-
// pillar-plus-crossbeam start gate, straddling world X at `center`
// (gap along X, so a vehicle drives through it along Z), sized to
// clear a vehicle. Bright orange - deliberately distinct from every
// procedural tint this bridge otherwise produces, so hand-authored
// content is visually obvious against the generated world in every
// view. A named GeomObject ("landmark_start_gate") - calling this
// again replaces it (via GeomDirectory::GetAddModel's own find-or-add
// behavior), not accumulate duplicates.
void AddStartGateLandmark(GeomScene& scene, const Terrain& terrain, Vec2 center);

// Map Editor phase, Task 12: a single small marker box - what
// MapEditorWindow::SpawnMarker adds so there is something new to
// select and drag with Edit3D's own already-existing TOOL_OBJ_MOVE
// (see that method's own comment for why no new move mechanism was
// needed - GeomObject::GetTransform, already wired to both the
// renderer via GeomWorldState::UpdateObjects and to the real mouse-
// drag gizmo code in uppsrc/ModelerApp/Editor.cpp, does the rest).
// `name` must be unique per call (MapEditorWindow tracks a counter) -
// GetAddModel finds-or-replaces by name, so a reused name would
// silently overwrite a previous marker instead of adding a new one.
void AddMarkerObject(GeomScene& scene, const String& name, Vec3 position, Color tint);

// Map Editor phase, Task 18/19: "join roads from city to rural places
// and to other cities" - a hand-authored road link, built the same way
// a procedural road is (RoadMeshBuilder::BuildRibbon), just from
// RoadPathfinder's own found path instead of a traced streamline. Each
// point's own world-Y is resolved individually (Task 19): a normal
// (non-bridge) point follows Terrain::HeightAt like every other road
// here; a bridge point (RoadPathPoint::bridge) instead gets a level
// deck height - the higher of its bridge run's two land ends plus a
// fixed clearance, held constant across the whole span - since
// Terrain::HeightAt under a bridge is the (often underwater) ground
// the bridge exists specifically to clear, not something the deck
// should literally follow. A named GeomObject (same find-or-replace-
// by-name behavior as AddMarkerObject).
void AddRoadLinkObject(GeomScene& scene, const String& name, const Vector<RoadPathPoint>& path, double width,
                        const Terrain& terrain, Color tint);

// Map Editor phase, Task 20: a real extruded wall/berm following
// `centerline` - box cross-section (outer face, inner face, top cap),
// `width` thick, `height` tall, base resting on Terrain::HeightAt at
// each point (its own base_offset, like every other road-ish mesh
// here, to avoid z-fighting with the ground). Used for both fence
// kinds FenceGenerator.h produces ("wall": thin+tall, "dirtpile":
// wide+short) - the two are distinguished by their own width/height/
// tint, not a different mesh algorithm (a dirt-pile's real triangular
// mound cross-section is approximated as a box here - stated
// simplification, not an attempt at an accurate berm profile).
void AddFenceObject(GeomScene& scene, const String& name, const Vector<Vec2>& centerline, double width, double height,
                     const Terrain& terrain, Color tint);

#endif
