#include "SweetSandy.h"

NAMESPACE_UPP

void WorldChunkGenWorker::Start(WorldChunkCache* cache_,
                                 ITerrainErosionGenerator* erosion_gen_,
                                 ICityLayoutGenerator* city_gen_,
                                 IRoadMeshGenerator* road_gen_,
                                 IVegetationGenerator* veg_gen_,
                                 IBuildingGenerator* building_gen_,
                                 IGroundGenerator* ground_gen_)
{
	Stop();

	cache = cache_;
	erosion_gen = erosion_gen_;
	city_gen = city_gen_;
	road_gen = road_gen_;
	veg_gen = veg_gen_;
	building_gen = building_gen_;
	ground_gen = ground_gen_;

	{
		Mutex::Lock __(req_lock);
		requests.Clear();
		busy = false;
	}
	{
		Mutex::Lock __(res_lock);
		results.Clear();
	}

	thread_running = true;
	thread.Run([this] { ThreadLoop(); });
}

void WorldChunkGenWorker::Stop()
{
	if(thread_running) {
		thread_running = false;
		if(thread.IsOpen())
			thread.Wait();
	}
}

void WorldChunkGenWorker::Submit(const WorldChunk& chunk)
{
	Mutex::Lock __(req_lock);
	requests.Add(chunk);
}

bool WorldChunkGenWorker::PollResult(Result& out)
{
	Mutex::Lock __(res_lock);
	if(results.IsEmpty())
		return false;
	out = pick(results[0]);
	results.Remove(0);
	return true;
}

void WorldChunkGenWorker::WaitIdle()
{
	for(;;) {
		bool idle;
		{
			Mutex::Lock __(req_lock);
			idle = requests.IsEmpty() && !busy;
		}
		if(idle)
			return;
		Sleep(2);
	}
}

void WorldChunkGenWorker::ThreadLoop()
{
	while(thread_running) {
		WorldChunk chunk;
		bool have = false;
		{
			Mutex::Lock __(req_lock);
			if(!requests.IsEmpty()) {
				chunk = requests[0];
				requests.Remove(0);
				busy = true;
				have = true;
			}
		}
		if(!have) {
			Sleep(5);
			continue;
		}

		Result r;
		r.coord = chunk.coord;
		GenerateOne(chunk, r.content);

		{
			Mutex::Lock __(res_lock);
			results.Add(pick(r));
		}
		{
			Mutex::Lock __(req_lock);
			busy = false;
		}
	}
}

void WorldChunkGenWorker::GenerateOne(const WorldChunk& chunk, WorldChunkContent& c)
{
	// Terrain Erosion phase, Task 3 (moved to this worker thread in Task
	// 9): runs unconditionally, *before* the cache-hit/miss branch below
	// - this is about populating Terrain's own in-memory eroded-tile
	// cache (session-local state HeightAt reads live - see Terrain.h's
	// own height_lock comment), not about producing WorldChunkContent
	// itself. A chunk whose *content* was already cached on disk from an
	// earlier session still needs its own erosion tile(s) loaded now, in
	// this session, before anything calls HeightAt for it.
	if(erosion_gen) {
		WorldChunkContent unused; // ErosionChunkGenerator never reads/writes its own content parameter
		erosion_gen->Generate(chunk, unused);
	}

	String cached;
	if(cache && cache->Load(chunk.coord, cached)) {
		c = WorldChunkContent::Deserialize(cached);
		return;
	}

	c = WorldChunkContent();
	if(city_gen)
		city_gen->Generate(chunk, c);
	if(road_gen)
		road_gen->Generate(chunk, c);
	// Buildings phase, Task 3: after city_gen so it can read
	// blocks/road_centerlines - order relative to road_gen doesn't
	// matter, it reads neither road_mesh nor vegetation. Unified Ground
	// phase, Task 2: before veg_gen (vegetation needs to avoid
	// buildings' footprints).
	if(building_gen)
		building_gen->Generate(chunk, c);
	// Vegetation phase, Task 4: after city_gen (reads road_centerlines)
	// and building_gen (avoids building footprints).
	if(veg_gen)
		veg_gen->Generate(chunk, c);
	// Unified Ground phase, Task 4: after city_gen/building_gen (reads
	// blocks) - order relative to road_gen/veg_gen doesn't matter.
	if(ground_gen)
		ground_gen->Generate(chunk, c);
	if(cache)
		cache->Save(chunk.coord, c.Serialize());
}

END_UPP_NAMESPACE
