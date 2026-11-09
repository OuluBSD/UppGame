#ifndef _Umbrella_LevelManifest_h_
#define _Umbrella_LevelManifest_h_

#include <Core/Core.h>

using namespace Upp;

// Parses share/mods/umbrella/levels/manifest.json and provides
// ordered level progression as a flat list of resolved file paths.
//
// Usage:
//   GetLevelManifest().Load("share/mods/umbrella");
//   String first = GetLevelManifest().GetFirstLevel();
//   String next  = GetLevelManifest().GetNextLevel(current);
class LevelManifest {
	Vector<String> levels;  // Ordered, fully-resolved paths

public:
	// Load from modRoot/manifestFile.  Returns false on parse error.
	bool Load(const String& modRoot,
	          const String& manifestFile = "levels/manifest.json");

	// Path of the first level, or "" if manifest not loaded.
	String GetFirstLevel() const;

	// Path after currentPath in the sequence, or "" if it is the last.
	String GetNextLevel(const String& currentPath) const;

	// True when currentPath is the final level.
	bool IsLastLevel(const String& currentPath) const;

	// 0-based index in the flat sequence, -1 if not found.
	int GetLevelIndex(const String& path) const;

	int GetLevelCount() const { return levels.GetCount(); }

	bool IsLoaded() const { return !levels.IsEmpty(); }
};

LevelManifest& GetLevelManifest();

#endif
