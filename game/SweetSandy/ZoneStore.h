#ifndef _SweetSandy_ZoneStore_h_
#define _SweetSandy_ZoneStore_h_

// Map Editor phase, Task 20: "fencing requires zoning first" - the
// user's own correction to Task 19's "not attempted" fencing gap.
// A zone is a hand-authored circular area (same brush-style shape as
// every other hand-authored thing in this phase - terrain paint,
// markers) with a type that says what kind of area it is, not what it
// looks like - FenceGenerator.h is the only real consumer of that
// type (which zones need protecting from a fast road, per
// ZoneNeedsProtection below), not a claim about zoning as an urban-
// planning system beyond that one real use.
const char* const kZoneStorePath = "share/sweetsandy/levels/zones.txt";

struct ZoneEntry : Moveable<ZoneEntry> {
	String name;
	String type = "residential"; // "residential"|"commercial"|"industrial"|"rural"|"park" - see ZoneNeedsProtection
	double cx = 0, cz = 0;
	double radius = 50.0;
};

// Returns an empty Vector (not an error) if the file doesn't exist yet
// - same convention as LoadLandmarks/LoadPaintStrokes/LoadRoadLinks.
Vector<ZoneEntry> LoadZones(const String& path);
void SaveZones(const String& path, const Vector<ZoneEntry>& entries);

// residential/park: people live or gather there, a fast road running
// past needs a fence (FenceGenerator.h's own real use of this).
// commercial/industrial/rural: no such expectation - false.
bool ZoneNeedsProtection(const String& type);

// Adds a thin, ground-hugging ring (RoadMeshBuilder::BuildRibbon
// around a closed circle centerline) so a zone's own boundary is
// directly visible in a render/export, tinted by type - otherwise a
// zone is invisible data with no way to check "is this really where I
// meant to put it" the way every other hand-authored thing in this
// phase can be.
void MaterializeZoneBoundary(GeomScene& scene, const Terrain& terrain, const ZoneEntry& zone);

#endif
