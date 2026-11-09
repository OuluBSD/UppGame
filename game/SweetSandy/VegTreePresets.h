#ifndef _SweetSandy_VegTreePresets_h_
#define _SweetSandy_VegTreePresets_h_

// Vegetation phase, Task 4: this game's own small preset set - real,
// meter-scaled parameters (roadside tree, roughly 6-18m tall), NOT
// ported from ez-tree's own oak_medium.json/pine_medium.json (their
// trunk lengths alone are 37-50 "units" - meaningless as meters against
// this world's already-established ~4m road width and ~15m street
// spacing; see 001-planning.md's named scale-calibration risk). Just
// two species (one deciduous, one evergreen), not all 15 of ez-tree's
// own - "a small fixed set", per 004-task4-worldbuilder-integration.md.

enum class VegTreePreset { Oak, Pine };

// Fills every VegBranchOptions/VegLeafOptions field a real tree needs
// (nothing left at VegTreeOptions's own bare-bones defaults), with
// `seed` as the only thing the caller supplies - so two calls with the
// same preset+seed always build the exact same options, and different
// seeds/presets are the only source of variety.
VegTreeOptions MakeVegTreePreset(VegTreePreset preset, dword seed);

#endif
