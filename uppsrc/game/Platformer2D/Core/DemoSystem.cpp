#include "DemoSystem.h"
#include <Core/Core.h>

NAMESPACE_UPP

// ============================================================================
// Demo Manager Implementation
// ============================================================================

DemoMode DemoManager::s_mode = DEMO_OFF;
int DemoManager::s_currentDemoNum = 0;
DemoFile DemoManager::s_demoFile;
int DemoManager::s_playbackIndex = 0;

void DemoManager::StartRecording(int demoNum)
{
	s_mode = DEMO_RECORD;
	s_currentDemoNum = demoNum;
	s_demoFile.ticks.Clear();
	
	// Initialize header
	memcpy(s_demoFile.header.signature, "DEMO", 4);
	s_demoFile.header.version = 1;
	s_demoFile.header.episode = (uint16_t)g_currentEpisodeEnum + 4; // 4, 5, 6
	s_demoFile.header.rngSeed = g_rng ? g_rng->GetSeed() : 0;
	s_demoFile.header.difficulty = (uint16_t)g_gameState.difficulty;
	s_demoFile.header.numLevels = 0;
	s_demoFile.header.totalTicks = 0;
	
	LOG("DemoManager: Started recording demo " << demoNum);
}

void DemoManager::StopRecording()
{
	if (s_mode != DEMO_RECORD) return;
	
	s_demoFile.header.totalTicks = s_demoFile.ticks.GetCount();
	
	// Save to file
	String filename = Format("demo%d.dem", s_currentDemoNum);
	if (s_demoFile.Save(filename))
	{
		LOG("DemoManager: Saved demo to " << filename);
	}
	else
	{
		LOG("DemoManager: Failed to save demo");
	}
	
	s_mode = DEMO_OFF;
	s_currentDemoNum = 0;
}

void DemoManager::StartPlayback(int demoNum)
{
	s_mode = DEMO_PLAYBACK;
	s_currentDemoNum = demoNum;
	s_playbackIndex = 0;
	
	String filename = Format("demo%d.dem", demoNum);
	if (!s_demoFile.Load(filename))
	{
		LOG("DemoManager: Failed to load demo " << filename);
		s_mode = DEMO_OFF;
		return;
	}
	
	// Set RNG seed for deterministic replay
	if (g_rng)
		g_rng->SetSeed(s_demoFile.header.rngSeed);
	
	LOG("DemoManager: Started playback demo " << demoNum << " (" << s_demoFile.ticks.GetCount() << " ticks)");
}

void DemoManager::StopPlayback()
{
	s_mode = DEMO_OFF;
	s_currentDemoNum = 0;
	s_playbackIndex = 0;
}

void DemoManager::RecordTick(const ControlFrame& controls)
{
	if (s_mode != DEMO_RECORD) return;
	
	DemoTick tick;
	tick.tickCount = g_simClock ? g_simClock->GetTickCount() : 0;
	tick.controls = controls;
	s_demoFile.ticks.Add(tick);
}

bool DemoManager::PlaybackTick(ControlFrame& controls)
{
	if (s_mode != DEMO_PLAYBACK) return false;
	
	if (s_playbackIndex >= s_demoFile.ticks.GetCount())
	{
		s_mode = DEMO_PLAYDONE;
		return false;
	}
	
	const DemoTick& tick = s_demoFile.ticks[s_playbackIndex];
	controls = tick.controls;
	s_playbackIndex++;
	return true;
}

bool DemoFile::Save(const char* filename)
{
	FileOut f(filename);
	if (!f.IsOpen()) return false;
	
	// Write header
	f.Put(&header, sizeof(header));
	
	// Write tick count
	uint32_t tickCount = ticks.GetCount();
	f.Put(&tickCount, sizeof(tickCount));
	
	// Write ticks
	for (const DemoTick& tick : ticks)
	{
		f.Put(&tick.tickCount, sizeof(tick.tickCount));
		// Write ControlFrame (simple binary write)
		f.Put(&tick.controls.jump, sizeof(tick.controls.jump));
		f.Put(&tick.controls.pogo, sizeof(tick.controls.pogo));
		f.Put(&tick.controls.fire, sizeof(tick.controls.fire));
		f.Put(&tick.controls.button2, sizeof(tick.controls.button2));
		f.Put(&tick.controls.button3, sizeof(tick.controls.button3));
		f.Put(&tick.controls.x, sizeof(tick.controls.x));
		f.Put(&tick.controls.y, sizeof(tick.controls.y));
		f.Put(&tick.controls.xDirection, sizeof(tick.controls.xDirection));
		f.Put(&tick.controls.yDirection, sizeof(tick.controls.yDirection));
		f.Put(&tick.controls.dir, sizeof(tick.controls.dir));
	}
	
	return f.IsOpen();
}

bool DemoFile::Load(const char* filename)
{
	FileIn f(filename);
	if (!f.IsOpen()) return false;
	
	// Read header
	f.Get(&header, sizeof(header));
	if (!IsValid()) return false;
	
	// Read tick count
	uint32_t tickCount = 0;
	f.Get(&tickCount, sizeof(tickCount));
	ticks.Clear();
	ticks.Reserve(tickCount);
	
	// Read ticks
	for (uint32_t i = 0; i < tickCount; ++i)
	{
		DemoTick tick;
		f.Get(&tick.tickCount, sizeof(tick.tickCount));
		f.Get(&tick.controls.jump, sizeof(tick.controls.jump));
		f.Get(&tick.controls.pogo, sizeof(tick.controls.pogo));
		f.Get(&tick.controls.fire, sizeof(tick.controls.fire));
		f.Get(&tick.controls.button2, sizeof(tick.controls.button2));
		f.Get(&tick.controls.button3, sizeof(tick.controls.button3));
		f.Get(&tick.controls.x, sizeof(tick.controls.x));
		f.Get(&tick.controls.y, sizeof(tick.controls.y));
		f.Get(&tick.controls.xDirection, sizeof(tick.controls.xDirection));
		f.Get(&tick.controls.yDirection, sizeof(tick.controls.yDirection));
		f.Get(&tick.controls.dir, sizeof(tick.controls.dir));
		ticks.Add(tick);
	}
	
	return f.IsOpen();
}

void PlayDemo(int demoNum)
{
	DemoManager::StartPlayback(demoNum);
}

void PlayDemoFile(const char* filename)
{
	// TODO: Load demo from specific file
}

void RecordDemo(int demoNum)
{
	DemoManager::StartRecording(demoNum);
}

void StopDemo()
{
	if (DemoManager::IsRecording())
		DemoManager::StopRecording();
	else if (DemoManager::IsPlaying())
		DemoManager::StopPlayback();
}

END_UPP_NAMESPACE