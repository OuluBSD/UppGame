#include "Core.h"
#include <unistd.h>

NAMESPACE_UPP

// Global instances
EpisodeDef* g_episodes[4] = { nullptr, nullptr, nullptr, nullptr };
EpisodeDef* g_currentEpisode = nullptr;
Episode g_currentEpisodeEnum = Episode::KEEN4;

GameState g_gameState;
LevelState g_levelState = LevelState::Playing;

// ============================================================================
// Simulation Clock Implementation (70 Hz fixed step)
// ============================================================================

class Platformer2D_SimulationClock : public ISimulationClock
{
public:
	uint32_t GetTickCount() const override { return s_tickCount; }
	void SetTickCount(uint32_t ticks) override { s_tickCount = ticks; }
	void WaitTick() override 
	{ 
		s_tickCount++; 
		usleep(1000000 / PLATFORMER2D_TICKS_PER_SECOND); // ~14ms for 70 Hz
	}
	int GetTicksPerSecond() const override { return PLATFORMER2D_TICKS_PER_SECOND; }
	
	static void SetTimeCount(uint32_t count) { s_tickCount = count; }
	static uint32_t GetTimeCount() { return s_tickCount; }
	static void IncrementTick() { s_tickCount++; }

private:
	static uint32_t s_tickCount;
};

uint32_t Platformer2D_SimulationClock::s_tickCount = 0;

// Global clock instance (set by platform layer)
static Platformer2D_SimulationClock g_platformClock;
ISimulationClock* g_simClock = &g_platformClock;

// ============================================================================
// RNG Implementation (deterministic for replay)
// ============================================================================

class Platformer2D_RandomGenerator : public IRandomGenerator
{
public:
	Platformer2D_RandomGenerator() : m_seed(0) {}
	
	uint16_t Random() override 
	{
		// Original ID US_RndT algorithm (from id_us.c)
		m_seed = m_seed * 0x41C64E6D + 0x3039;
		return (uint16_t)(m_seed >> 16);
	}
	
	void SetSeed(uint32_t seed) override { m_seed = seed; }
	uint32_t GetSeed() const override { return m_seed; }

private:
	uint32_t m_seed;
};

static Platformer2D_RandomGenerator g_platformRng;
IRandomGenerator* g_rng = &g_platformRng;

// ============================================================================
// Episode Registration
// ============================================================================

void RegisterEpisode(int index, EpisodeDef* ep)
{
	if (index >= 0 && index < 4)
		g_episodes[index] = ep;
}

// ============================================================================
// Game State Serialization
// ============================================================================

void GameState::Serialize(Stream& s)
{
	s % mapPosX % mapPosY;
	for (int i = 0; i < 25; ++i) s % levelsDone[i];
	s % keenScore % nextKeenAt % numShots % numCentilife;
	
	switch (g_currentEpisodeEnum)
	{
	case Episode::KEEN4:
		s % ep.ck4.wetsuit % ep.ck4.membersRescued;
		break;
	case Episode::KEEN5:
		s % ep.ck5.securityCard % ep.ck5.word_4729C % ep.ck5.fusesRemaining;
		break;
	case Episode::KEEN6:
		s % ep.ck6.sandwich % ep.ck6.rope % ep.ck6.passcard % ep.ck6.inRocket;
		break;
	}
	
	for (int i = 0; i < 4; ++i) s % keyGems[i];
	s % currentLevel % numLives % difficulty % platformOffset;
}

// ============================================================================
// Core Functions
// ============================================================================

void InitGame()
{
	// Initialize game state
	g_gameState.Clear();
	g_levelState = LevelState::Playing;
	g_numObjects = 0;
	for (int i = 0; i < 100; ++i) g_objects[i] = Object();
	
	// Setup episode functions
	ACT_SetupFunctions();
	KeenSetupFunctions();
	OBJ_SetupFunctions();
	Map_SetupFunctions();
	Misc_SetupFunctions();
	
	if (g_currentEpisode && g_currentEpisode->setupFunctions)
		g_currentEpisode->setupFunctions();
}

void ShutdownGame()
{
	// Cleanup
}

void DemoLoop()
{
	// Main demo/title loop - runs at 70 Hz
	
	g_simClock->SetTickCount(0);
	g_rng->SetSeed(0); // Deterministic for demo
	
	// Initialize demo state
	g_levelState = LevelState::Playing;
	
	while (g_levelState == LevelState::Playing)
	{
		// Pump input events (for demo playback)
		// IN_PumpEvents();
		
		// Update game logic at 70 Hz
		if (g_currentEpisode && g_currentEpisode->episodeLogic)
			g_currentEpisode->episodeLogic();
		
		// Wait for next tick
		g_simClock->WaitTick();
	}
	
}

void GameLoop()
{
	// Main gameplay loop - runs at 70 Hz
	
	g_simClock->SetTickCount(0);
	
	// Initialize game state for new game/level
	g_levelState = LevelState::Playing;
	
	while (g_levelState == LevelState::Playing)
	{
		// Process input
		ControlFrame controls;
		ReadControls(0, &controls);
		
		// Update game logic at 70 Hz
		if (g_currentEpisode && g_currentEpisode->episodeLogic)
			g_currentEpisode->episodeLogic();
		
		// Wait for next tick
		g_simClock->WaitTick();
	}
	
}

void NewGame()
{
	g_gameState.Clear();
	g_gameState.numLives = 3;
	g_gameState.difficulty = (int16_t)Difficulty::Normal;
	g_levelState = LevelState::ResetGame;
}

void LoadGame(int slot)
{
	// Not yet implemented
}

void SaveGame(int slot)
{
	// Not yet implemented
}

void ReadControls(int player, ControlFrame* controls)
{
	// Default: no input
	if (controls) Zero(*controls);
}

bool DetectEpisodes()
{
	// Probe for EPISODE.CK4/.CK5/.CK6 in the current directory. Full search-path
	// aware selection lives in the Platform layer (DetectAndSelectEpisode).
	for (int i = 0; i < 3; ++i)
	{
		const char* file = GetEpisodeFile((Episode)(4 + i));
		if (file && FileExists(file))
			return true;
	}
	return false;
}

const char* GetEpisodeFile(Episode ep)
{
	switch (ep)
	{
	case Episode::KEEN4: return "EPISODE.CK4";
	case Episode::KEEN5: return "EPISODE.CK5";
	case Episode::KEEN6: return "EPISODE.CK6";
	}
	return nullptr;
}

END_UPP_NAMESPACE