#include "SweetSandy.h"

NAMESPACE_UPP

void WorldOctree::SetLodLevels(const Vector<LodLevel>& new_levels)
{
	ASSERT_(!new_levels.IsEmpty(), "WorldOctree::SetLodLevels: need at least one LOD level");
	for(int i = 1; i < new_levels.GetCount(); i++)
		ASSERT_(new_levels[i].load_distance > new_levels[i - 1].load_distance,
		        "WorldOctree::SetLodLevels: load_distance must be strictly ascending by level");
	levels = clone(new_levels);
}

const WorldChunk* WorldOctree::FindChunk(const WorldChunkCoord& c) const
{
	int i = loaded.Find(c);
	return i >= 0 ? &loaded[i] : NULL;
}

void WorldOctree::UpdateStreaming(Vec3 viewer_pos)
{
	ASSERT_(!levels.IsEmpty(), "WorldOctree::UpdateStreaming: call SetLodLevels first");

	double finest_cell = levels[0].cell_size;
	double max_distance = levels.Top().load_distance;

	// Desired set: for every finest-grid cell within range, assign it to
	// the finest LOD level whose load_distance covers its distance to the
	// viewer, then record the (coarser) chunk coordinate at that level -
	// VectorMap key dedups automatically when several finest cells map to
	// the same coarser chunk.
	VectorMap<WorldChunkCoord, int> want;

	int half_range = (int)ceil(max_distance / finest_cell) + 1;
	int base_gx = (int)floor(viewer_pos.x / finest_cell);
	int base_gz = (int)floor(viewer_pos.z / finest_cell);

	for(int gz = base_gz - half_range; gz <= base_gz + half_range; gz++) {
		for(int gx = base_gx - half_range; gx <= base_gx + half_range; gx++) {
			double wx = (gx + 0.5) * finest_cell;
			double wz = (gz + 0.5) * finest_cell;
			double dx = wx - viewer_pos.x, dz = wz - viewer_pos.z;
			double dist = sqrt(dx * dx + dz * dz);

			int lod = -1;
			for(int i = 0; i < levels.GetCount(); i++)
				if(dist < levels[i].load_distance) { lod = i; break; }
			if(lod < 0)
				continue; // beyond every level's range - not streamed

			WorldChunkCoord c = WorldChunkCoordAt(wx, wz, levels[lod].cell_size, lod);
			want.GetAdd(c, lod);
		}
	}

	// Unload chunks that fell out of `want`, with hysteresis so a chunk
	// right at its LOD's boundary doesn't load/unload every call.
	for(int i = loaded.GetCount() - 1; i >= 0; i--) {
		const WorldChunkCoord& c = loaded.GetKey(i);
		if(want.Find(c) >= 0)
			continue;
		double dx = loaded[i].center.x - viewer_pos.x, dz = loaded[i].center.z - viewer_pos.z;
		double dist = sqrt(dx * dx + dz * dz);
		if(dist > levels[c.lod].load_distance * unload_hysteresis) {
			loaded.Remove(i);
			unload_events++;
		}
	}

	// Load newly-wanted chunks.
	for(int i = 0; i < want.GetCount(); i++) {
		const WorldChunkCoord& c = want.GetKey(i);
		if(loaded.Find(c) >= 0)
			continue;
		double cell = levels[c.lod].cell_size;
		WorldChunk& wc = loaded.Add(c);
		wc.coord = c;
		wc.center = WorldChunkCenterAt(c, cell);
		wc.half_extent = Vec3(cell * 0.5, 0, cell * 0.5);
		wc.state = CHUNK_LOADED;
		load_events++;
	}
}

END_UPP_NAMESPACE
