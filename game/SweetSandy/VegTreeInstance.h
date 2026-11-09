#ifndef _SweetSandy_VegTreeInstance_h_
#define _SweetSandy_VegTreeInstance_h_

// Vegetation phase, Task 4: one placed, already-built tree - see
// Manager/2-plan/.../005-vegetation/004-task4-worldbuilder-
// integration.md.
//
// bark_tint/leaf_tint were not in that doc's own sketch (which only
// carried position/yaw/trunk_radius/trunk_height/branches/leaves) -
// added here because PlayScreen's rendering needs a color at *draw*
// time and this instance is all it has: the VegTreeOptions used to
// build it (which does carry these tints, see VegTreeOptions.h) isn't
// retained anywhere past generation. Two Color fields are cheap to
// store per instance and keep the per-species look (oak vs. pine bark)
// distinguishable without threading options through PlayScreen.
struct VegTreeInstance : Moveable<VegTreeInstance> {
	Vec3 position;            // world-space; y is ground height at placement time (Terrain::HeightAt)
	double yaw = 0;           // random facing around Y, visual variety only
	double trunk_radius = 0;  // options.branch.radius[0] - needed for the ODE collider without re-deriving it from mesh triangles
	double trunk_height = 0;  // approx. options.branch.length[0] - "good enough to not drive through it" sizing, not a claim the real (gnarled) trunk is a perfect cylinder
	Color bark_tint = White();
	Color leaf_tint = White();
	VegTreePreset preset = VegTreePreset::Oak; // Unified Ground phase, Task 1 - which leaf texture PlayScreen should bind for this tree; not derivable from bark_tint/leaf_tint alone (those are randomized within a species, not a species identifier)
	VegMesh branches;
	VegMesh leaves;

	void Serialize(Stream& s)
	{
		s % position % yaw % trunk_radius % trunk_height % branches % leaves;
		int preset_raw = (int)preset;
		s % preset_raw;
		if(s.IsLoading())
			preset = (VegTreePreset)preset_raw;
		// Color has no Stream::operator% overload in this U++ tree (only
		// a legacy operator/ - see Core/Color.h - not used here to avoid
		// mixing two different serialization operators in one method);
		// round-tripped via its own raw dword instead, same idea as
		// every other non-container field above.
		dword bark_raw = bark_tint.GetRaw();
		dword leaf_raw = leaf_tint.GetRaw();
		s % bark_raw % leaf_raw;
		if(s.IsLoading()) {
			bark_tint = Color::FromRaw(bark_raw);
			leaf_tint = Color::FromRaw(leaf_raw);
		}
	}
};

#endif
