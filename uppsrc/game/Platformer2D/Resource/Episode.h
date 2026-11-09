#ifndef _Platformer2D_Resource_Episode_h_
#define _Platformer2D_Resource_Episode_h_

#include <game/Platformer2D/Platformer2D.h>
#include <game/Platformer2D/Resource/Resource.h>

NAMESPACE_UPP

// ============================================================================
// Episode Variable Access (from EPISODE.CKx)
//
// EPISODE.CKx is a text file parsed by VAR_Engine. This class is a thin typed
// facade over the variable table so episode code reads naturally.
// ============================================================================

class EpisodeVars
{
public:
	EpisodeVars() {}
	~EpisodeVars() {}

	// Parse EPISODE.CKx (and its %include files) into the variable table.
	bool LoadFromFile(const char* filename);
	void Clear();

	// Typed access
	bool GetBool(const char* name, bool def = false) const;
	int GetInt(const char* name, int def = 0) const;
	const String GetString(const char* name, const String& def = String()) const;
	const Vector<int>* GetIntArray(const char* name) const;
	const Vector<String>* GetStringArray(const char* name) const;
	Action* GetAction(const char* name) const;

	// Episode metadata
	int GetEpisodeNumber() const { return episodeNumber; }
	const String& GetEpisodeName() const { return episodeName; }

private:
	int episodeNumber = -1;
	String episodeName;
};

// ============================================================================
// Global Episode Variables
// ============================================================================

extern EpisodeVars g_episodeVars;

// Parse an EPISODE.CKx file into the shared variable table. The episode number
// is taken from the parsed ck_episodeNumber variable.
bool EP_LoadEpisode(const char* filename);

// Configure the active episode: parse EPISODE.CKx via the VAR engine, populate
// the shared EpisodeDef (g_currentEpisode) from the ca_* resource variables and
// register it in g_episodes. Returns false if the episode file is missing.
bool EP_ConfigureEpisode(Episode ep);

// Configure the active episode from an explicit EPISODE.CKx path. The episode
// number is read from the parsed ck_episodeNumber variable.
bool EP_ConfigureEpisodeFile(const char* episodeFile);

const EpisodeVars* EP_GetCurrentEpisode();
int EP_GetEpisodeNumber();
const char* EP_GetEpisodeName();

END_UPP_NAMESPACE

#endif // _Platformer2D_Resource_Episode_h_
