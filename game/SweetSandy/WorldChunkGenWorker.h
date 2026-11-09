#ifndef _SweetSandy_WorldChunkGenWorker_h_
#define _SweetSandy_WorldChunkGenWorker_h_

// Map Editor phase, Task 9: the single background worker thread all
// per-chunk generation (streaming *and* paint-triggered regeneration)
// now runs on - see 012-map-editor/009-task9-async-chunk-regeneration.md.
// Deliberately ONE thread, never a pool: MapGeneratorCityLayoutGenerator
// and VegChunkGenerator both reseed and consume U++'s own global
// Randomf()/SeedRandom() RNG - safe only as long as generation never runs
// on more than one thread at a time (see that planning doc's own "one
// worker thread, not a pool" section) - confining every chunk generation,
// ever, for the whole process, to this one thread satisfies that by
// construction, with no lock needed around Randomf() itself.
//
// Owns only the Thread + two small queues; the generator stages and the
// disk cache are borrowed (non-owning) pointers into WorldBuilder's own
// members, which must outlive this worker - WorldBuilder::Stop()s it
// before those members can be destroyed (see WorldBuilder.cpp).
class WorldChunkGenWorker {
public:
	struct Result : Moveable<Result> {
		WorldChunkCoord coord;
		WorldChunkContent content;
	};

	WorldChunkGenWorker() {}
	~WorldChunkGenWorker() { Stop(); }
	WorldChunkGenWorker(const WorldChunkGenWorker&) = delete;
	void operator=(const WorldChunkGenWorker&) = delete;

	// (Re)starts the worker thread against a fresh set of generators/
	// cache - safe to call on an already-started instance (stops the old
	// thread first, matching AudioStreamSource's own restart-safe
	// pattern in uppsrc/GameEngine/AudioStreaming.cpp), which
	// WorldBuilder::Create() relies on when a run picks a different
	// track/seed. Any generator pointer may be NULL (matches the
	// optional-stage convention WorldBuilder::RegenerateChunk already
	// established) - GenerateOne simply skips a NULL stage.
	void Start(WorldChunkCache* cache,
	           ITerrainErosionGenerator* erosion_gen,
	           ICityLayoutGenerator* city_gen,
	           IRoadMeshGenerator* road_gen,
	           IVegetationGenerator* veg_gen,
	           IBuildingGenerator* building_gen,
	           IGroundGenerator* ground_gen);
	// Joins the thread. Safe to call when not started (a no-op) and more
	// than once.
	void Stop();

	// Enqueues one chunk for generation - returns immediately, the real
	// work happens on the worker thread. WorldBuilder is responsible for
	// never submitting the same coord twice while a request/result for
	// it is still outstanding (see its own `pending`/`needs_resubmit`).
	void Submit(const WorldChunk& chunk);
	// Pops one completed result if any is ready (FIFO, not necessarily
	// submission order once more than one chunk is in flight at a time -
	// WorldBuilder does not depend on ordering). Returns false, `out`
	// untouched, if nothing is ready yet.
	bool PollResult(Result& out);

	// Test-only: blocks (busy-polling with a short sleep, matching this
	// class's own worker-loop convention below) until every request
	// submitted so far has a result sitting in the (unconsumed) result
	// queue - i.e. the worker is genuinely caught up. Never called from
	// the real gameplay path - see this task's own planning doc.
	void WaitIdle();

private:
	void ThreadLoop();
	// Folds the old WorldBuilder::Update's inline cache-check-then-
	// generate-then-save sequence, plus the previously-separate
	// erosion_gen step, into the one place every per-chunk generation
	// path (streaming and forced regeneration after invalidation alike)
	// now shares. A forced regeneration needs no special-case here: the
	// caller (WorldBuilder::InvalidateContentNear) already deleted the
	// cache entry synchronously before submitting, so cache->Load simply
	// misses and this falls through to the same real-generation path a
	// first-ever visit would take.
	void GenerateOne(const WorldChunk& chunk, WorldChunkContent& c);

	WorldChunkCache*           cache = NULL;
	ITerrainErosionGenerator*  erosion_gen = NULL;
	ICityLayoutGenerator*      city_gen = NULL;
	IRoadMeshGenerator*        road_gen = NULL;
	IVegetationGenerator*      veg_gen = NULL;
	IBuildingGenerator*        building_gen = NULL;
	IGroundGenerator*          ground_gen = NULL;

	// Each queue behind its own plain Mutex, matching uppsrc/GameEngine/
	// AudioStreaming.cpp's own already-established polling-thread
	// convention in this codebase - a short Sleep between empty-queue
	// checks, not a condition variable, kept deliberately simple per
	// that same file's own precedent.
	Mutex             req_lock;
	Vector<WorldChunk> requests;
	// Guarded by req_lock alongside `requests` (not its own lock) - both
	// together define "is the worker idle", which WaitIdle needs to
	// observe as one consistent snapshot: true from the moment a request
	// is popped until its result has been pushed to `results`.
	bool               busy = false;

	Mutex          res_lock;
	Vector<Result> results;

	Thread thread;
	bool   thread_running = false;
};

#endif
