#include "SweetSandy.h"

NAMESPACE_UPP

namespace {

int FindInt(const Vector<int>& v, int x)
{
	for(int i = 0; i < v.GetCount(); i++)
		if(v[i] == x)
			return i;
	return -1;
}

// Given the edge just arrived along (from -> to), picks the neighbor of
// `to` that turns most sharply right - the smallest positive angle from
// the reversed arrival direction, wrapped into [0, 2*pi). Faithful port
// of polygon_finder.ts's getRightmostNode. Returns -1 if `to` has no
// candidate (a dead end, or its only neighbor is `from` itself).
int GetRightmostNode(const CityGraph& graph, const Vector<Vector<int>>& adj, int from, int to)
{
	if(adj[to].IsEmpty())
		return -1;

	Vec2 backwards_dir = graph.GetNode(from).pos - graph.GetNode(to).pos;
	double transform_angle = atan2(backwards_dir.y, backwards_dir.x);

	int rightmost = -1;
	double smallest_theta = 2 * M_PI;

	for(int next : adj[to]) {
		if(next == from)
			continue;
		Vec2 next_dir = graph.GetNode(next).pos - graph.GetNode(to).pos;
		double next_angle = atan2(next_dir.y, next_dir.x) - transform_angle;
		if(next_angle < 0)
			next_angle += 2 * M_PI;
		if(next_angle < smallest_theta) {
			smallest_theta = next_angle;
			rightmost = next;
		}
	}
	return rightmost;
}

// Walks rightmost-turn-at-every-junction starting from the last edge in
// `visited` (which must already hold >= 2 node indices) until it
// revisits a node already in `visited`, or fails (dead end / exceeds
// max_walk_length). On success, trims `visited` down to the closed
// cycle (from the revisited node onward) and returns true.
bool RecursiveWalk(const CityGraph& graph, const Vector<Vector<int>>& adj, Vector<int>& visited, int max_walk_length)
{
	while(true) {
		if(visited.GetCount() >= max_walk_length)
			return false;

		int from = visited[visited.GetCount() - 2];
		int to = visited[visited.GetCount() - 1];
		int next = GetRightmostNode(graph, adj, from, to);
		if(next < 0)
			return false;

		int visited_index = FindInt(visited, next);
		if(visited_index >= 0) {
			Vector<int> cycle;
			for(int i = visited_index; i < visited.GetCount(); i++)
				cycle.Add(visited[i]);
			visited = pick(cycle);
			return true;
		}
		visited.Add(next);
	}
}

} // anonymous namespace

Vector<Vector<Vec2>> CityBlockFinder::FindBlocks(const CityGraph& graph, int max_walk_length)
{
	int n = graph.GetNodeCount();

	// Own mutable working copy of the adjacency lists - directed edges
	// get consumed (removed) as faces are found, same as the source's
	// per-node `adj` arrays; CityGraph's own (undirected) neighbor lists
	// are never touched.
	Vector<Vector<int>> adj;
	adj.SetCount(n);
	for(int i = 0; i < n; i++)
		adj[i] = clone(graph.GetNode(i).neighbors);

	Vector<Vector<Vec2>> blocks;

	for(int i = 0; i < n; i++) {
		if(adj[i].GetCount() < 2)
			continue;

		// Snapshot i's *current* neighbor list before walking: a walk
		// started from an earlier outer-loop node can consume one of
		// i's edges (if the found face passes through i), and the
		// source's own live-array iteration is a JS-iterator quirk, not
		// algorithm intent - not worth chasing into undefined behavior
		// by mutating a Vector while range-for iterating it in C++.
		Vector<int> start_neighbors = clone(adj[i]);
		for(int next : start_neighbors) {
			Vector<int> visited;
			visited.Add(i);
			visited.Add(next);

			if(!RecursiveWalk(graph, adj, visited, max_walk_length))
				continue;

			for(int k = 0; k < visited.GetCount(); k++) {
				int cur = visited[k];
				int nxt = visited[(k + 1) % visited.GetCount()];
				int idx = FindInt(adj[cur], nxt);
				if(idx >= 0)
					adj[cur].Remove(idx);
			}

			Vector<Vec2>& poly = blocks.Add();
			for(int idx : visited)
				poly.Add(graph.GetNode(idx).pos);
		}
	}

	return blocks;
}

END_UPP_NAMESPACE
