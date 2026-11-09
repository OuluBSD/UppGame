#include "Episode.h"
#include <game/Platformer2D/Resource/VAR.h>
#include <game/Platformer2D/Core/Core.h>

NAMESPACE_UPP

EpisodeVars g_episodeVars;

// ============================================================================
// EpisodeVars - typed facade over VAR_Engine
// ============================================================================

void EpisodeVars::Clear()
{
	episodeNumber = -1;
	episodeName.Clear();
}

bool EpisodeVars::LoadFromFile(const char* filename)
{
	Clear();

	if (!filename)
		return false;

	if (!VAR_Engine::LoadVars(filename))
	{
		LOG("EpisodeVars::LoadFromFile: Failed to parse " << filename);
		return false;
	}

	episodeNumber = VAR_Engine::GetInt("ck_episodeNumber", -1);
	episodeName = filename;

	LOG("EpisodeVars::LoadFromFile: Loaded " << filename << " (episode " << episodeNumber << ")");
	return true;
}

bool EpisodeVars::GetBool(const char* name, bool def) const
{
	return VAR_Engine::GetInt(name, def ? 1 : 0) != 0;
}

int EpisodeVars::GetInt(const char* name, int def) const
{
	return VAR_Engine::GetInt(name, def);
}

const String EpisodeVars::GetString(const char* name, const String& def) const
{
	const char* val = VAR_Engine::GetString(name, nullptr);
	return val ? String(val) : def;
}

const Vector<int>* EpisodeVars::GetIntArray(const char* name) const
{
	return VAR_Engine::GetIntArray(name);
}

const Vector<String>* EpisodeVars::GetStringArray(const char* name) const
{
	return VAR_Engine::GetStringArray(name);
}

Action* EpisodeVars::GetAction(const char* name) const
{
	return VAR_Engine::GetAction(name);
}

// ============================================================================
// Episode configuration
// ============================================================================

bool EP_LoadEpisode(const char* filename)
{
	return g_episodeVars.LoadFromFile(filename);
}

// One definition per supported episode (Keen 4/5/6), stored so that the
// const char* fields stay valid for the lifetime of the process.
static EpisodeDef s_episodeDefs[3];
static String s_episodeFileNames[3][11];
static String s_episodeExts[3];

enum EpisodeFileNameSlot
{
	SLOT_MAPHEAD = 0,
	SLOT_MAPFILE,
	SLOT_GRAPHDICT,
	SLOT_GRAPHHEAD,
	SLOT_GRAPHFILE,
	SLOT_GFXINFO,
	SLOT_AUDIODICT,
	SLOT_AUDIOHEAD,
	SLOT_AUDIOFILE,
	SLOT_AUDIOINFO,
	SLOT_TILEINFO,
	SLOT_COUNT
};

static const char* s_episodeDisplayNames[3] = { "Keen 4", "Keen 5", "Keen 6" };

static bool EP_IsKeen4Present() { return FS_PathManager::FileExistsInPaths("EPISODE.CK4"); }
static bool EP_IsKeen5Present() { return FS_PathManager::FileExistsInPaths("EPISODE.CK5"); }
static bool EP_IsKeen6Present() { return FS_PathManager::FileExistsInPaths("EPISODE.CK6"); }

static bool (*s_episodePresent[3])() = { EP_IsKeen4Present, EP_IsKeen5Present, EP_IsKeen6Present };
static void (*s_episodeSetup[3])() = { CK4_SetupFunctions, CK5_SetupFunctions, CK6_SetupFunctions };

// Populate the shared EpisodeDef at the given slot from the already-loaded
// variable table. The const char* fields point at s_episodeFileNames storage,
// so they stay valid for the lifetime of the process.
static bool EP_FinalizeEpisode(int index, const char* episodeFile)
{
	g_currentEpisodeEnum = (Episode)(4 + index);

	s_episodeExts[index] = "CK" + FormatInt(4 + index);
	const String& ext = s_episodeExts[index];

	struct VarSlot { const char* varName; const char* baseName; int slot; };
	static const VarSlot varMap[] = {
		{ "ca_mapHead",       "MAPHEAD",   SLOT_MAPHEAD   },
		{ "ca_mapFile",       "GAMEMAPS",  SLOT_MAPFILE   },
		{ "ca_graphDict",     "EGADICT",   SLOT_GRAPHDICT },
		{ "ca_graphHead",     "EGAHEAD",   SLOT_GRAPHHEAD },
		{ "ca_graphFile",     "EGAGRAPH",  SLOT_GRAPHFILE },
		{ "ca_gfxInfoFile",   "GFXINFOE",  SLOT_GFXINFO   },
		{ "ca_audioDict",     "AUDIODCT",  SLOT_AUDIODICT },
		{ "ca_audioHead",     "AUDIOHHD",  SLOT_AUDIOHEAD },
		{ "ca_audioFile",     "AUDIO",     SLOT_AUDIOFILE },
		{ "ca_audioInfoFile", "AUDINFOE",  SLOT_AUDIOINFO },
		{ "ca_tileInfo",      "TILEINFO",  SLOT_TILEINFO  },
	};

	for (const VarSlot& entry : varMap)
	{
		String assigned = g_episodeVars.GetString(entry.varName);
		if (assigned.IsEmpty())
			assigned = String(entry.baseName) + "." + ext;
		s_episodeFileNames[index][entry.slot] = assigned;
	}

	EpisodeDef& def = s_episodeDefs[index];
	def.episodeFile    = episodeFile;
	def.name           = s_episodeDisplayNames[index];
	def.setupFunctions = s_episodeSetup[index];
	def.isPresent      = s_episodePresent[index];
	def.mapHeadFile    = s_episodeFileNames[index][SLOT_MAPHEAD];
	def.mapFile        = s_episodeFileNames[index][SLOT_MAPFILE];
	def.graphDictFile  = s_episodeFileNames[index][SLOT_GRAPHDICT];
	def.graphHeadFile  = s_episodeFileNames[index][SLOT_GRAPHHEAD];
	def.graphFile      = s_episodeFileNames[index][SLOT_GRAPHFILE];
	def.gfxInfoFile    = s_episodeFileNames[index][SLOT_GFXINFO];
	def.audioDictFile  = s_episodeFileNames[index][SLOT_AUDIODICT];
	def.audioHeadFile  = s_episodeFileNames[index][SLOT_AUDIOHEAD];
	def.audioFile      = s_episodeFileNames[index][SLOT_AUDIOFILE];
	def.audioInfoFile  = s_episodeFileNames[index][SLOT_AUDIOINFO];
	def.tileInfoFile   = s_episodeFileNames[index][SLOT_TILEINFO];
	def.mapHeadExt     = ext.Begin();
	def.graphHeadExt   = ext.Begin();
	def.audioHeadExt   = ext.Begin();
	def.tileInfoExt    = ext.Begin();

	RegisterEpisode(index, &def);
	g_currentEpisode = &def;

	LOG("EP_ConfigureEpisode: Episode " << (4 + index) << " configured ("
		<< def.graphFile << ", " << def.mapFile << ")");
	return true;
}

bool EP_ConfigureEpisode(Episode ep)
{
	const int index = (int)ep - 4;
	if (index < 0 || index >= 3)
	{
		LOG("EP_ConfigureEpisode: Unsupported episode " << (int)ep);
		return false;
	}

	const char* episodeFile = GetEpisodeFile(ep);
	if (!FS_PathManager::FileExistsInPaths(episodeFile))
	{
		LOG("EP_ConfigureEpisode: No episode file " << episodeFile);
		return false;
	}

	if (!g_episodeVars.LoadFromFile(episodeFile))
		return false;

	int parsedEpisode = g_episodeVars.GetEpisodeNumber();
	if (parsedEpisode > 0 && parsedEpisode != (int)ep)
		LOG("EP_ConfigureEpisode: " << episodeFile << " declares episode "
			<< parsedEpisode << ", expected " << (int)ep);

	return EP_FinalizeEpisode(index, episodeFile);
}

bool EP_ConfigureEpisodeFile(const char* episodeFile)
{
	if (!episodeFile)
		return false;

	if (!g_episodeVars.LoadFromFile(episodeFile))
		return false;

	const int index = g_episodeVars.GetEpisodeNumber() - 4;
	if (index < 0 || index >= 3)
	{
		LOG("EP_ConfigureEpisodeFile: " << episodeFile
			<< " declares unsupported episode " << g_episodeVars.GetEpisodeNumber());
		return false;
	}

	return EP_FinalizeEpisode(index, episodeFile);
}

const EpisodeVars* EP_GetCurrentEpisode()
{
	return &g_episodeVars;
}

int EP_GetEpisodeNumber()
{
	return g_episodeVars.GetEpisodeNumber();
}

const char* EP_GetEpisodeName()
{
	return g_episodeVars.GetEpisodeName().Begin();
}

END_UPP_NAMESPACE
