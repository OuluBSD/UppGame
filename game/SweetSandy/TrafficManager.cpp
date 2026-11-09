#include "SweetSandy.h"

NAMESPACE_UPP

namespace {

// Meters - a "regular car" box, bigger than the player's own small
// off-road buggy (VehicleDef: 1.6m x 3.12m) since AI traffic represents
// ordinary city cars, not more buggies. DrawUnitBox's own sx maps to
// this box's local X, used as the "forward" axis below (see the
// heading-to-rotation note in Tick()/DrawGL()).
const double kCarLength = 4.2;
const double kCarWidth = 1.8;
const double kCarHeight = 1.4;

const double kMinSpeed = 4.0; // m/s - a leisurely city driving pace, not aggressive
const double kMaxSpeed = 8.0;

const int kRouteHops = 6; // graph nodes per laid-out route segment, see 002-task2-ai-traffic.md

// Deterministic per (world_seed, chunk coord), same idea as
// MapGeneratorCityLayoutGenerator's/VegChunkGenerator's own per-chunk
// seeds - salted distinctly ("traf") so this generator's own RNG stream
// doesn't start at the exact same point either of those already
// reseeded the shared RNG to for this chunk.
dword DeterministicTrafficSeed(const String& world_seed, const WorldChunkCoord& coord)
{
	hash_t h = CombineHash(world_seed.GetHashValue(), coord.lod, coord.x, coord.y);
	h = CombineHash(h, coord.z);
	h = CombineHash(h, 't', 'r', 'f'); // "traffic", trimmed to 3 chars - CombineHash tops out at 4 total args
	return (dword)h;
}

// Random-walks `hops` edges starting at `start_node`, collecting each
// visited node's own position into out_path (out_path[0] is
// start_node's own position, so the result always has at least 1
// point - a fully isolated node with no neighbors produces a 1-point
// "route", which the caller must treat as "nothing to follow"). Stops
// early if a node with no neighbors at all is reached.
void BuildRoute(const CityGraph& graph, int start_node, int hops, Vector<Vec2>& out_path, int& out_end_node)
{
	out_path.Clear();
	out_path.Add(graph.GetNode(start_node).pos);
	int at = start_node;
	int came_from = -1;
	for(int i = 0; i < hops; i++) {
		if(graph.GetNode(at).neighbors.IsEmpty())
			break;
		int next = TrafficManager::PickNextNode(graph, at, came_from);
		out_path.Add(graph.GetNode(next).pos);
		came_from = at;
		at = next;
	}
	out_end_node = at;
}

} // anonymous namespace

int TrafficManager::PickNextNode(const CityGraph& graph, int at, int came_from)
{
	const CityGraphNode& node = graph.GetNode(at);
	Vector<int> candidates;
	for(int n : node.neighbors)
		if(n != came_from)
			candidates.Add(n);
	if(candidates.IsEmpty())
		candidates = clone(node.neighbors);
	return candidates[(int)(Randomf() * candidates.GetCount())];
}

void TrafficManager::Create(PhysSpaceId new_space, Terrain& new_terrain, const String& new_world_seed)
{
	space = new_space;
	terrain = &new_terrain;
	world_seed = new_world_seed;
	cars.Clear();
	chunk_graphs.Clear();
}

void TrafficManager::SpawnForChunk(const WorldChunkCoord& coord, const WorldChunkContent& content)
{
	if(content.road_centerlines.IsEmpty())
		return;

	CityGraph graph;
	graph.Build(content.road_centerlines, kCityGraphRebuildFuzzyRadius);
	if(graph.GetNodeCount() == 0)
		return;

	SeedRandom(DeterministicTrafficSeed(world_seed, coord));

	int car_count = 1 + (int)(Randomf() * 2); // 1 or 2 cars per chunk with roads
	int spawned = 0;

	for(int i = 0; i < car_count; i++) {
		int start_node = (int)(Randomf() * graph.GetNodeCount());
		Vector<Vec2> route;
		int end_node;
		BuildRoute(graph, start_node, kRouteHops, route, end_node);
		if(route.GetCount() < 2)
			continue; // an isolated start node - nowhere to build a route, skip this car

		TrafficCar car;
		car.follower.SetPath(route, false);
		car.speed = kMinSpeed + Randomf() * (kMaxSpeed - kMinSpeed);
		car.owner_chunk = coord;
		car.current_node = end_node;

		car.geom = PhysCreateBox(space, kCarLength, kCarHeight, kCarWidth);
		double ground_y = terrain->HeightAt(car.follower.position.x, car.follower.position.y);
		PhysGeomSetPosition(car.geom, Vec3(car.follower.position.x, ground_y + kCarHeight * 0.5, car.follower.position.y));

		cars.Add(pick(car));
		spawned++;
	}

	// Only keep this chunk's own graph around if at least one car
	// actually ended up using it - SpawnForChunk only ever runs once per
	// chunk load, so an unused graph here would just be dead weight.
	if(spawned > 0)
		chunk_graphs.Add(coord, pick(graph));
}

void TrafficManager::DespawnForChunk(const WorldChunkCoord& coord)
{
	for(int i = cars.GetCount() - 1; i >= 0; i--)
		if(cars[i].owner_chunk == coord) {
			PhysDestroyGeom(cars[i].geom);
			cars.Remove(i);
		}
	int idx = chunk_graphs.Find(coord);
	if(idx >= 0)
		chunk_graphs.Remove(idx);
}

void TrafficManager::Tick(double dt)
{
	for(TrafficCar& car : cars) {
		double distance = car.speed * dt;
		if(!car.follower.Advance(distance)) {
			// Reached the end of its current laid-out route - extend it
			// from exactly where it is now (BuildRoute's own first point
			// is current_node's own position, matching follower.position
			// exactly, so there's no visible jump).
			int graph_idx = chunk_graphs.Find(car.owner_chunk);
			if(graph_idx >= 0) {
				const CityGraph& graph = chunk_graphs[graph_idx];
				Vector<Vec2> route;
				int end_node;
				BuildRoute(graph, car.current_node, kRouteHops, route, end_node);
				car.current_node = end_node;
				if(route.GetCount() >= 2)
					car.follower.SetPath(route, false);
				// A single-point route (car.current_node turned out
				// isolated, shouldn't happen for a node that already had
				// at least one edge, but stay defensive) just leaves the
				// car sitting at its current position/path instead of
				// crashing - it will simply not move further.
			}
		}

		double ground_y = terrain->HeightAt(car.follower.position.x, car.follower.position.y);
		PhysGeomSetPosition(car.geom, Vec3(car.follower.position.x, ground_y + kCarHeight * 0.5, car.follower.position.y));

		// Local X is this box's own "forward" axis (see kCarLength's own
		// comment) - rotating it by -heading around world Y lands it on
		// world direction (cos(heading), 0, sin(heading)), matching
		// AIPathFollower's own heading convention exactly (derived from
		// the standard right-handed Rotate-Y matrix, not guessed - a
		// plain symmetric box wouldn't show a sign error visually either
		// way, but the derivation is worth being exact about for when a
		// less-symmetric look is added later).
		PhysGeomSetRotationAxisAngle(car.geom, Vec3(0, 1, 0), -car.follower.heading);
	}
}

void TrafficManager::DrawGL() const
{
	glColor3ub(150, 40, 40); // a plain reddish "other traffic" color, distinct from the player's own yellow
	for(const TrafficCar& car : cars) {
		double ground_y = terrain->HeightAt(car.follower.position.x, car.follower.position.y);
		glPushMatrix();
		glTranslated(car.follower.position.x, ground_y + kCarHeight * 0.5, car.follower.position.y);
		glRotated(-car.follower.heading * 180.0 / M_PI, 0, 1, 0);
		DrawUnitBox(kCarLength, kCarHeight, kCarWidth);
		glPopMatrix();
	}
}

END_UPP_NAMESPACE
