#ifndef _SweetSandy_TrafficManager_h_
#define _SweetSandy_TrafficManager_h_

// AI traffic and pedestrians phase, Task 2: spawns kinematically-moving
// AI cars onto a newly-loaded chunk's own roads, routes them by random-
// walking a CityGraph rebuilt from that chunk's own road_centerlines,
// moves them with Task 1's AIPathFollower, gives each a static ODE
// collider so the player's real vehicle can actually hit one, and draws
// a simplified flat-colored car. See Manager/2-plan/.../
// 006-ai-traffic-and-pedestrians/002-task2-ai-traffic.md.
//
// This project's first genuinely dynamic runtime system (see that
// phase's own 001-planning.md) - not routed through WorldBuilder/
// WorldChunkCache at all. Owned directly by PlayScreen, the same way
// Vehicle/PhysicsWorld already are (wired in Task 4).
struct TrafficCar : Moveable<TrafficCar> {
	AIPathFollower follower;
	PhysGeomId geom = 0;         // static box collider, repositioned every tick - no body, see 001-planning.md
	double speed = 0;            // meters/second
	WorldChunkCoord owner_chunk; // which chunk's own load/unload this car is attributed to
	int current_node = -1;       // index into owner_chunk's own CityGraph - where the follower's current path ends
};

class TrafficManager {
	PhysSpaceId space = 0;  // not owned - PhysicsWorld's own space
	Terrain *terrain = NULL; // not owned - PlayScreen's own Terrain, same threading convention RoadCreatorMeshGenerator/VegChunkGenerator already use
	String world_seed;
	Vector<TrafficCar> cars;
	VectorMap<WorldChunkCoord, CityGraph> chunk_graphs; // one CityGraph per chunk that currently has live cars

public:
	void Create(PhysSpaceId space, Terrain& terrain, const String& world_seed);

	// Rebuilds a CityGraph from content.road_centerlines and spawns some
	// number of cars onto it, each starting at a random node with a
	// short random-walk route already laid out. No-ops if the chunk has
	// no roads (or its graph ends up with no nodes at all).
	void SpawnForChunk(const WorldChunkCoord& coord, const WorldChunkContent& content);

	// Destroys every car whose owner_chunk == coord (PhysDestroyGeom
	// each one's collider) and drops that chunk's own CityGraph entry,
	// if any.
	void DespawnForChunk(const WorldChunkCoord& coord);

	void Tick(double dt);
	void DrawGL() const;

	int GetCarCount() const { return cars.GetCount(); }
	const TrafficCar& GetCar(int i) const { return cars[i]; }

	// The actual random-walk routing decision, pulled out and exposed
	// publicly for direct testing (see RunTrafficManagerSelfTest) - the
	// same "expose the core decision, don't only test it through the
	// whole pipeline's emergent behavior" call City Layout's own
	// CityStreamlineIntegrator::Integrate was already made for. Picks a
	// random neighbor of `at` other than `came_from` (so a route doesn't
	// immediately reverse); if `at` has no other neighbor (a genuine
	// dead end), the only option *is* to go back.
	static int PickNextNode(const CityGraph& graph, int at, int came_from);
};

#endif
