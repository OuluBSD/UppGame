#ifndef _Platformer2D_Platform_Platform_h_
#define _Platformer2D_Platform_Platform_h_

#include <game/Platformer2D/Core/Core.h>
#include <game/Platformer2D/Render/Render.h>
#include <game/Platformer2D/Audio/Audio.h>

NAMESPACE_UPP

// ============================================================================
// Configuration
// ============================================================================

struct Platformer2DConfig
{
	bool fullscreen = false;
	bool aspectCorrected = true;
	bool border = true;
	bool integerScaled = false;
	bool vsync = true;
	bool disableJoysticks = false;
	String episodeFile; // EPISODE.CK4/5/6
	int startingDifficulty = 1; // Normal
	String demoFile; // For /DEMOFILE
	int playDemoNum = -1; // For /PLAYDEMO
	bool tedLevel = false;
	String tedLevelFile;
	
	// Config file values
	String userPath;
	String gamePath;
	String omniPath;
};

extern Platformer2DConfig g_config;

// ============================================================================
// Platform Entry Points
// ============================================================================

// Main initialization (called from GUI_APP_MAIN or CONSOLE_APP_MAIN)
bool Platformer2D_PlatformStartup(int argc, char* argv[]);
void Platformer2D_PlatformShutdown();

// Main loop (called after startup)
int Platformer2D_PlatformRun();

// Headless mode (for testing)
void Platformer2D_HeadlessStartup(const Platformer2DConfig& cfg);
void Platformer2D_HeadlessShutdown();
void Platformer2D_HeadlessTick(); // Advance one simulation frame
bool Platformer2D_HeadlessIsRunning();

// ============================================================================
// Command Line Parsing
// ============================================================================

bool ParseCommandLine(int argc, char* argv[], Platformer2DConfig& cfg);
void LoadConfigFile(Platformer2DConfig& cfg);
void SaveConfigFile(const Platformer2DConfig& cfg);

// ============================================================================
// Episode Detection
// ============================================================================

bool DetectAndSelectEpisode(Platformer2DConfig& cfg);

END_UPP_NAMESPACE

#endif // _Platformer2D_Platform_Platform_h_