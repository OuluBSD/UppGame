#include "Platform.h"
#include "Window.h"
#include "Input.h"
#include "Timer.h"
#include "AudioOut.h"
#include <CtrlLib/CtrlLib.h>
#include <Sound/Sound.h>
#include <game/Platformer2D/Resource/Episode.h>

NAMESPACE_UPP

Platformer2DConfig g_config;
static bool g_headless = false;
static bool g_running = false;

// ============================================================================
// Platform Startup/Shutdown
// ============================================================================

bool Platformer2D_PlatformStartup(int argc, char* argv[])
{
	LOG("Platformer2D_PlatformStartup");
	
	// Parse command line
	if (!ParseCommandLine(argc, argv, g_config))
		return false;
	
	// Load config file
	LoadConfigFile(g_config);
	
	// Detect episode
	if (!DetectAndSelectEpisode(g_config))
	{
		LOG("No valid episode found");
		return false;
	}
	
	// Initialize subsystems in order
	FS_PathManager::SetGamePath(g_config.gamePath);
	FS_PathManager::SetUserPath(g_config.userPath);
	FS_PathManager::SetOmniPath(g_config.omniPath);
	
	ResourceManager::Startup();
	
	// Load episode resource files
	if (!ResourceManager::LoadGAMEMAPS())
	{
		LOG("Warning: GAMEMAPS load failed");
	}
	if (!ResourceManager::LoadEGAGRAPH())
	{
		LOG("Warning: EGAGRAPH load failed");
	}
	if (!ResourceManager::LoadAUDIO())
	{
		LOG("Warning: AUDIO load failed");
	}
	
	InitGame();
	Renderer::Startup();
	AudioSystem::Startup();
	
	// Create window
	if (!Platformer2DWindow::Create(g_config))
		return false;
	
	// Setup input
	InputMapper::Initialize();
	
	// Setup timer
	FixedStepTimer::Initialize();
	
	// Setup audio output
	AudioOutput::Initialize();
	
	g_running = true;
	return true;
}

void Platformer2D_PlatformShutdown()
{
	LOG("Platformer2D_PlatformShutdown");
	
	if (!g_running) return;
	
	AudioOutput::Shutdown();
	FixedStepTimer::Shutdown();
	InputMapper::Shutdown();
	Platformer2DWindow::Destroy();
	
	AudioSystem::Shutdown();
	Renderer::Shutdown();
	ShutdownGame();
	ResourceManager::Shutdown();
	
	g_running = false;
}

int Platformer2D_PlatformRun()
{
	LOG("Platformer2D_PlatformRun: Starting main loop");
	
	// Main loop handled by CtrlLib event loop
	// FixedStepTimer drives simulation
	// Window handles rendering
	
	return 0; // CtrlLib runs its own loop
}

// ============================================================================
// Headless Mode
// ============================================================================

void Platformer2D_HeadlessStartup(const Platformer2DConfig& cfg)
{
	LOG("Platformer2D_HeadlessStartup");
	g_headless = true;
	g_config = cfg;

	FS_PathManager::SetGamePath(cfg.gamePath);
	FS_PathManager::SetUserPath(cfg.userPath);
	FS_PathManager::SetOmniPath(cfg.omniPath);

	ResourceManager::Startup();
	InitGame();
	Renderer::Startup();
	AudioSystem::Startup(); // Uses null callback

	// Headless mode: episode is optional. Tests may set the episode explicitly
	// via CK_SetEpisode() / g_currentEpisodeEnum before running simulation.
	if (!cfg.episodeFile.IsEmpty())
	{
		if (EP_ConfigureEpisodeFile(cfg.episodeFile))
			ResourceManager::LoadGAMEMAPS();
	}

	// Set simulation clock to headless implementation
	// g_simClock = &HeadlessClock;

	g_running = true;
}

void Platformer2D_HeadlessShutdown()
{
	LOG("Platformer2D_HeadlessShutdown");
	if (!g_running) return;
	
	AudioSystem::Shutdown();
	Renderer::Shutdown();
	ShutdownGame();
	ResourceManager::Shutdown();
	
	g_running = false;
	g_headless = false;
}

void Platformer2D_HeadlessTick()
{
	if (!g_running) return;
	
	// Advance simulation by one tick
	FixedStepTimer::HeadlessTick();
	
	// Run one game tick
	// CK_GameTick(); // Would call game logic
}

bool Platformer2D_HeadlessIsRunning()
{
	return g_running;
}

// ============================================================================
// Command Line Parsing
// ============================================================================

bool ParseCommandLine(int argc, char* argv[], Platformer2DConfig& cfg)
{
	for (int i = 1; i < argc; ++i)
	{
		String arg = argv[i];
		
		if (arg == "/EPISODE" && i + 1 < argc)
		{
			cfg.episodeFile = argv[++i];
		}
		else if (arg == "/FULLSCREEN")
		{
			cfg.fullscreen = true;
		}
		else if (arg == "/FILLED")
		{
			cfg.aspectCorrected = false;
		}
		else if (arg == "/NOBORDER")
		{
			cfg.border = false;
		}
		else if (arg == "/INTEGER")
		{
			cfg.integerScaled = true;
		}
		else if (arg == "/VSYNC")
		{
			cfg.vsync = true;
		}
		else if (arg == "/NOVSYNC")
		{
			cfg.vsync = false;
		}
		else if (arg == "/NOJOYS")
		{
			cfg.disableJoysticks = true;
		}
		else if (arg == "/NOCOPY")
		{
			// Keen 6 copy protection bypass
			// ck6_creatureQuestionDone = true;
		}
		else if (arg == "/DEMOFILE" && i + 1 < argc)
		{
			cfg.demoFile = argv[++i];
		}
		else if (arg == "/PLAYDEMO" && i + 1 < argc)
		{
			cfg.playDemoNum = atoi(argv[++i]);
		}
		else if (arg == "/ASLEV" && i + 1 < argc)
		{
			cfg.tedLevel = true;
			cfg.tedLevelFile = argv[++i];
		}
		else if (arg == "/GAMEPATH" && i + 1 < argc)
		{
			cfg.gamePath = argv[++i];
		}
		else if (arg == "/USERPATH" && i + 1 < argc)
		{
			cfg.userPath = argv[++i];
		}
		else if (arg == "/OMNIPATH" && i + 1 < argc)
		{
			cfg.omniPath = argv[++i];
		}
		else if (arg[0] == '/')
		{
			LOG("Unknown option: " << arg);
		}
	}
	
	return true;
}

void LoadConfigFile(Platformer2DConfig& cfg)
{
	// TODO: Load OMNISPK.CFG from user path
	// Format: key = value
	LOG("LoadConfigFile: Not implemented");
}

void SaveConfigFile(const Platformer2DConfig& cfg)
{
	LOG("SaveConfigFile: Not implemented");
}

// ============================================================================
// Episode Detection
// ============================================================================

bool DetectAndSelectEpisode(Platformer2DConfig& cfg)
{
	// If episode specified on command line, use it
	if (!cfg.episodeFile.IsEmpty())
	{
		if (EP_ConfigureEpisodeFile(cfg.episodeFile))
			return true;
	}
	
	// Auto-detect: check for EPISODE.CK4/5/6 in game path
	Vector<String> episodes = { "EPISODE.CK4", "EPISODE.CK5", "EPISODE.CK6" };
	for (const String& ep : episodes)
	{
		if (FS_PathManager::FileExistsInPaths(ep))
		{
			if (EP_ConfigureEpisodeFile(ep))
				return true;
		}
	}
	
	return false;
}

END_UPP_NAMESPACE