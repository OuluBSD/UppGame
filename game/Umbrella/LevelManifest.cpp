#include "Umbrella.h"
#include "LevelManifest.h"

using namespace Upp;

bool LevelManifest::Load(const String& modRoot, const String& manifestFile) {
	levels.Clear();

	String path = AppendFileName(modRoot, manifestFile);
	String text = LoadFile(path);
	if(text.IsEmpty()) {
		RLOG("LevelManifest: could not read " << path);
		return false;
	}

	Value json = ParseJSON(text);
	if(json.IsError()) {
		RLOG("LevelManifest: JSON parse error in " << path);
		return false;
	}

	Value worlds = json["worlds"];
	for(int w = 0; w < worlds.GetCount(); w++) {
		Value lvls = worlds[w]["levels"];
		for(int l = 0; l < lvls.GetCount(); l++) {
			String rel = lvls[l];
			String full = AppendFileName(modRoot, rel);
			levels.Add(full);
		}
	}

	RLOG("LevelManifest: loaded " << levels.GetCount() << " levels from " << path);
	return !levels.IsEmpty();
}

String LevelManifest::GetFirstLevel() const {
	return levels.IsEmpty() ? String() : levels[0];
}

String LevelManifest::GetNextLevel(const String& currentPath) const {
	int idx = GetLevelIndex(currentPath);
	if(idx < 0 || idx + 1 >= levels.GetCount())
		return String();
	return levels[idx + 1];
}

bool LevelManifest::IsLastLevel(const String& currentPath) const {
	if(levels.IsEmpty()) return true;
	return currentPath == levels.Top();
}

int LevelManifest::GetLevelIndex(const String& path) const {
	for(int i = 0; i < levels.GetCount(); i++)
		if(levels[i] == path)
			return i;
	return -1;
}

LevelManifest& GetLevelManifest() {
	static LevelManifest instance;
	return instance;
}
