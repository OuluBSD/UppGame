#include "SweetSandy.h"

NAMESPACE_UPP

static int FindCoord(const Vector<WorldChunkCoord>& v, const WorldChunkCoord& c)
{
	for(int i = 0; i < v.GetCount(); i++)
		if(v[i] == c)
			return i;
	return -1;
}

void WorldBuilder::Create(const String& seed_version)
{
	worker.Stop(); // must not touch content/octree while the thread might still be running (see Update()'s own lazy-Start)
	worker_started = false;
	pending.Clear();
	needs_resubmit.Clear();

	octree.Clear();
	content.Clear();

	cache.Open(seed_version);

	// Placeholder LOD levels (same shape as WorldOctree's own self-test
	// defaults) - real thresholds depend on real content cost/size, which
	// is unknown until a real generator exists (a later phase).
	Vector<WorldOctree::LodLevel> levels;
	{ WorldOctree::LodLevel l; l.cell_size = 20; l.load_distance = 60;  levels.Add(l); }
	{ WorldOctree::LodLevel l; l.cell_size = 40; l.load_distance = 150; levels.Add(l); }
	{ WorldOctree::LodLevel l; l.cell_size = 80; l.load_distance = 400; levels.Add(l); }
	octree.SetLodLevels(levels);
}

void WorldBuilder::Stop()
{
	worker.Stop();
	worker_started = false;
	pending.Clear();
	needs_resubmit.Clear();
}

Vector<WorldChunkCoord> WorldBuilder::Update(Vec3 viewer_pos)
{
	// Map Editor phase, Task 9: started lazily, here, rather than in
	// Create() - every caller (PlayScreen::StartRun, every self-test)
	// calls the Set*Generator setters *after* Create(), so Create()
	// itself is too early to hand `worker` real generator pointers.
	if(!worker_started) {
		worker.Start(&cache, erosion_gen.Get(), city_gen.Get(), road_gen.Get(),
		             veg_gen.Get(), building_gen.Get(), ground_gen.Get());
		worker_started = true;
	}

	octree.UpdateStreaming(viewer_pos);

	// Drop content for chunks the octree just unloaded. A chunk still
	// `pending` (its request already in flight) is deliberately left in
	// `pending` - DrainCompleted discards that result once it arrives
	// (FindChunk will miss by then) rather than trying to cancel an
	// in-flight request, which `worker` has no mechanism for and does
	// not need one for (a discarded result is cheap; a cancellation
	// protocol would not be).
	for(int i = content.GetCount() - 1; i >= 0; i--)
		if(octree.FindChunk(content.GetKey(i)) == NULL)
			content.Remove(i);

	// Submit a request for every chunk that's loaded but has neither
	// content nor an already-in-flight request.
	for(int i = 0; i < octree.GetLoadedChunkCount(); i++) {
		const WorldChunk& chunk = octree.GetLoadedChunk(i);
		if(content.Find(chunk.coord) >= 0)
			continue;
		if(FindCoord(pending, chunk.coord) >= 0)
			continue;
		worker.Submit(chunk);
		pending.Add(chunk.coord);
	}

	Vector<WorldChunkCoord> newly_installed;
	DrainCompleted(&newly_installed);
	return newly_installed;
}

void WorldBuilder::DrainCompleted(Vector<WorldChunkCoord>* newly_installed_out)
{
	WorldChunkGenWorker::Result r;
	while(worker.PollResult(r)) {
		int pi = FindCoord(pending, r.coord);
		if(pi < 0)
			continue; // not tracked as pending anymore - stale, nothing wants it
		pending.Remove(pi);

		int ri = FindCoord(needs_resubmit, r.coord);
		if(ri >= 0) {
			needs_resubmit.Remove(ri);
			// The chunk was invalidated again while this result was
			// still generating - discard it and resubmit fresh instead
			// of installing outdated content. Only resubmit if the
			// chunk is (still) actually loaded.
			const WorldChunk* chunk = octree.FindChunk(r.coord);
			if(chunk) {
				worker.Submit(*chunk);
				pending.Add(r.coord);
			}
			continue;
		}

		if(octree.FindChunk(r.coord) == NULL)
			continue; // unloaded while generating - discard, nothing wants it now

		content.Add(r.coord, pick(r.content));
		if(newly_installed_out)
			newly_installed_out->Add(r.coord);
	}
}

const WorldChunkContent* WorldBuilder::FindContent(const WorldChunkCoord& c) const
{
	int i = content.Find(c);
	return i >= 0 ? &content[i] : NULL;
}

void WorldBuilder::InvalidateContentNear(Vec2 center, Vec2 half_extent)
{
	for(int i = 0; i < octree.GetLoadedChunkCount(); i++) {
		const WorldChunk& chunk = octree.GetLoadedChunk(i);
		// AABB overlap test in x/z (Vec2.y = world z, same convention
		// ErosionTileOrigin's own comment already established).
		if(fabs(chunk.center.x - center.x) > chunk.half_extent.x + half_extent.x)
			continue;
		if(fabs(chunk.center.z - center.y) > chunk.half_extent.z + half_extent.y)
			continue;

		cache.Invalidate(chunk.coord);
		int ci = content.Find(chunk.coord);
		if(ci >= 0)
			content.Remove(ci);

		int pi = FindCoord(pending, chunk.coord);
		if(pi >= 0) {
			// Already in flight (either a first-time load or an earlier
			// paint's own invalidation) - mark it stale rather than
			// double-submitting; DrainCompleted resubmits once that
			// stale result arrives.
			if(FindCoord(needs_resubmit, chunk.coord) < 0)
				needs_resubmit.Add(chunk.coord);
			continue;
		}
		worker.Submit(chunk);
		pending.Add(chunk.coord);
	}
}

void WorldBuilder::WaitIdle()
{
	while(!pending.IsEmpty()) {
		worker.WaitIdle();
		DrainCompleted(NULL);
	}
}

END_UPP_NAMESPACE
