#include "SweetSandy.h"
#include <Core/MathNumeric/MathNumeric.h>

NAMESPACE_UPP

namespace {

struct HeightAndGradient {
	double height, gradient_x, gradient_y;
};

// Precomputes, for every cell, the list of neighbor-cell indices and
// normalized weights (sum to 1) within a circular brush of the given
// radius - used to *spread* erosion over an area (deposition is not
// brush-spread; see ErodeHeightmap's own comment on why).
//
// Deliberate simplification vs. the reference's own InitializeBrushIndices:
// that one only recomputes the brush near a map edge and *reuses* the
// last-computed shape for interior cells (valid only because every
// unclipped-by-edges brush has the identical shape/weights regardless
// of which interior cell it's centered on, and the reference's own
// row-major iteration order happens to guarantee the reused shape is
// always an unclipped one by the time an interior cell reaches it) -
// a real but subtle invariant, not worth the risk of a fragile port
// for a one-time-per-tile bake. This version computes every cell's own
// brush directly instead - same output for every cell, no reliance on
// iteration-order invariants.
void ComputeErosionBrushForCellImpl(int map_size, int radius, int centre_x, int centre_y,
                                     Vector<int>& idx, Vector<double>& wt)
{
	double weight_sum = 0;
	for(int y = -radius; y <= radius; y++)
		for(int x = -radius; x <= radius; x++) {
			double sqr_dst = (double)(x * x + y * y);
			if(sqr_dst >= (double)(radius * radius))
				continue;
			int coord_x = centre_x + x;
			int coord_y = centre_y + y;
			if(coord_x < 0 || coord_x >= map_size || coord_y < 0 || coord_y >= map_size)
				continue;
			double weight = 1.0 - sqrt(sqr_dst) / radius;
			weight_sum += weight;
			idx.Add(coord_y * map_size + coord_x);
			wt.Add(weight);
		}

	for(int j = 0; j < wt.GetCount(); j++)
		wt[j] /= weight_sum;
}

void InitializeBrush(int map_size, int radius, Vector<Vector<int>>& brush_indices, Vector<Vector<double>>& brush_weights)
{
	brush_indices.SetCount(map_size * map_size);
	brush_weights.SetCount(map_size * map_size);

	for(int i = 0; i < map_size * map_size; i++)
		ComputeErosionBrushForCellImpl(map_size, radius, i % map_size, i / map_size, brush_indices[i], brush_weights[i]);
}

HeightAndGradient CalculateHeightAndGradient(const Vector<float>& map, int map_size, double pos_x, double pos_y)
{
	int coord_x = (int)pos_x;
	int coord_y = (int)pos_y;

	double x = pos_x - coord_x;
	double y = pos_y - coord_y;

	int node_index_nw = coord_y * map_size + coord_x;
	double height_nw = map[node_index_nw];
	double height_ne = map[node_index_nw + 1];
	double height_sw = map[node_index_nw + map_size];
	double height_se = map[node_index_nw + map_size + 1];

	double gradient_x = (height_ne - height_nw) * (1 - y) + (height_se - height_sw) * y;
	double gradient_y = (height_sw - height_nw) * (1 - x) + (height_se - height_ne) * x;

	double height = height_nw * (1 - x) * (1 - y) + height_ne * x * (1 - y)
	              + height_sw * (1 - x) * y       + height_se * x * y;

	return { height, gradient_x, gradient_y };
}

// The real per-droplet simulation, given already-computed brush data -
// ErodeHeightmap computes the brush once and calls this per droplet;
// SimulateOneDroplet (below, exposed for direct single-droplet testing)
// computes its own brush and calls it once.
void SimulateDropletWithBrush(Vector<float>& map, int map_size, double pos_x, double pos_y,
                               const HydraulicErosionParams& params,
                               const Vector<Vector<int>>& brush_indices, const Vector<Vector<double>>& brush_weights)
{
	double dir_x = 0, dir_y = 0;
	double speed = params.initial_speed;
	double water = params.initial_water_volume;
	double sediment = 0;

	for(int lifetime = 0; lifetime < params.max_droplet_lifetime; lifetime++) {
		int node_x = (int)pos_x;
		int node_y = (int)pos_y;
		int droplet_index = node_y * map_size + node_x;
		double cell_offset_x = pos_x - node_x;
		double cell_offset_y = pos_y - node_y;

		HeightAndGradient hg = CalculateHeightAndGradient(map, map_size, pos_x, pos_y);

		dir_x = dir_x * params.inertia - hg.gradient_x * (1 - params.inertia);
		dir_y = dir_y * params.inertia - hg.gradient_y * (1 - params.inertia);
		double len = sqrt(dir_x * dir_x + dir_y * dir_y);
		if(len != 0) { dir_x /= len; dir_y /= len; }
		pos_x += dir_x;
		pos_y += dir_y;

		if((dir_x == 0 && dir_y == 0) || pos_x < 0 || pos_x >= map_size - 1 || pos_y < 0 || pos_y >= map_size - 1)
			break;

		double new_height = CalculateHeightAndGradient(map, map_size, pos_x, pos_y).height;
		double delta_height = new_height - hg.height;

		double sediment_capacity = max(-delta_height * speed * water * params.sediment_capacity_factor,
		                                params.min_sediment_capacity);

		if(sediment > sediment_capacity || delta_height > 0) {
			double amount_to_deposit = (delta_height > 0) ? min(delta_height, sediment)
			                                               : (sediment - sediment_capacity) * params.deposit_speed;
			sediment -= amount_to_deposit;

			map[droplet_index]                 += (float)(amount_to_deposit * (1 - cell_offset_x) * (1 - cell_offset_y));
			map[droplet_index + 1]             += (float)(amount_to_deposit * cell_offset_x       * (1 - cell_offset_y));
			map[droplet_index + map_size]      += (float)(amount_to_deposit * (1 - cell_offset_x) * cell_offset_y);
			map[droplet_index + map_size + 1]  += (float)(amount_to_deposit * cell_offset_x       * cell_offset_y);
		}
		else {
			double amount_to_erode = min((sediment_capacity - sediment) * params.erode_speed, -delta_height);

			const Vector<int>& idx = brush_indices[droplet_index];
			const Vector<double>& wt = brush_weights[droplet_index];
			for(int b = 0; b < idx.GetCount(); b++) {
				int node_index = idx[b];
				double weighed_erode_amount = amount_to_erode * wt[b];
				double delta_sediment = (map[node_index] < weighed_erode_amount) ? (double)map[node_index] : weighed_erode_amount;
				map[node_index] -= (float)delta_sediment;
				sediment += delta_sediment;
			}
		}

		// Energy-conservation speed update, straight from the
		// reference - deliberately guarded against a negative sqrt
		// argument (clamped to 0) which the reference's own formula
		// doesn't guard against. The reference implicitly assumes a
		// heightmap with small per-step delta_height (a normalized
		// [0,1]-ish range over a large grid); this project's own tiles
		// carry real absolute meters (IslandTerrain's own height_scale),
		// where a single grid step can plausibly change height by
		// several meters on a steep slope - a case the reference was
		// very likely never exercised against. Without this guard, a
		// big enough downhill single step makes speed*speed +
		// delta_height*gravity go negative, sqrt() returns NaN, and
		// every later step of that droplet (and everywhere it deposits/
		// erodes) permanently corrupts the map with NaN - see this
		// task's own report for whether real IslandTerrain-baked data
		// actually triggers this in practice.
		speed = sqrt(max(0.0, speed * speed + delta_height * params.gravity));
		water *= (1 - params.evaporate_speed);
	}
}

} // anonymous namespace

void ErodeHeightmap(Vector<float>& map, int map_size, int num_droplets,
                     const HydraulicErosionParams& params, dword seed)
{
	if(map_size < 2 * params.erosion_radius + 3)
		return; // too small for even one droplet to move safely - nothing to do

	Vector<Vector<int>> brush_indices;
	Vector<Vector<double>> brush_weights;
	InitializeBrush(map_size, params.erosion_radius, brush_indices, brush_weights);

	RandomNumberGenerator rng;
	rng.Seed(seed);

	for(int iter = 0; iter < num_droplets; iter++) {
		double pos_x = (double)rng.Random(map_size - 1);
		double pos_y = (double)rng.Random(map_size - 1);
		SimulateDropletWithBrush(map, map_size, pos_x, pos_y, params, brush_indices, brush_weights);
	}
}

void SimulateOneDroplet(Vector<float>& map, int map_size, double start_x, double start_y,
                         const HydraulicErosionParams& params)
{
	Vector<Vector<int>> brush_indices;
	Vector<Vector<double>> brush_weights;
	InitializeBrush(map_size, params.erosion_radius, brush_indices, brush_weights);
	SimulateDropletWithBrush(map, map_size, start_x, start_y, params, brush_indices, brush_weights);
}

void ComputeErosionBrushForCell(int map_size, int radius, int centre_x, int centre_y,
                                 Vector<int>& indices_out, Vector<double>& weights_out)
{
	indices_out.Clear();
	weights_out.Clear();
	ComputeErosionBrushForCellImpl(map_size, radius, centre_x, centre_y, indices_out, weights_out);
}

END_UPP_NAMESPACE
