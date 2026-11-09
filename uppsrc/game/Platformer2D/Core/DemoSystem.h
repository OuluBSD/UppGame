#ifndef _Platformer2D_Core_DemoSystem_h_
#define _Platformer2D_Core_DemoSystem_h_

#include "Core.h"
#include "ObjectSystem.h"

NAMESPACE_UPP

// ============================================================================
// Demo System (matching OmniSpeak demo format)
// ============================================================================

enum DemoMode : uint8_t
{
	DEMO_OFF = 0,
	DEMO_RECORD = 1,
	DEMO_PLAYBACK = 2,
	DEMO_PLAYDONE = 3
};

// Demo header (matches OmniSpeak)
struct DemoHeader
{
	char signature[4];        // "DEMO"
	uint16_t version;         // Demo format version
	uint16_t episode;         // Episode number (4, 5, 6)
	uint32_t rngSeed;         // RNG seed for deterministic replay
	uint16_t difficulty;      // Difficulty level
	uint16_t numLevels;       // Number of levels in demo
	uint32_t totalTicks;      // Total simulation ticks
};

// Demo tick event
struct DemoTick
{
	uint32_t tickCount;       // Absolute tick count
	ControlFrame controls;    // Input state for this tick
};

// Demo file structure
class DemoFile
{
public:
	DemoHeader header;
	Vector<DemoTick> ticks;
	
	bool IsValid() const { return memcmp(header.signature, "DEMO", 4) == 0; }
	
	bool Save(const char* filename);
	bool Load(const char* filename);
};

// Demo manager
class DemoManager
{
public:
	static void StartRecording(int demoNum);
	static void StopRecording();
	static void StartPlayback(int demoNum);
	static void StopPlayback();
	static void RecordTick(const ControlFrame& controls);
	static bool PlaybackTick(ControlFrame& controls);
	static bool IsRecording() { return s_mode == DEMO_RECORD; }
	static bool IsPlaying() { return s_mode == DEMO_PLAYBACK; }
	static DemoMode GetMode() { return s_mode; }
	
private:
	static DemoMode s_mode;
	static int s_currentDemoNum;
	static DemoFile s_demoFile;
	static int s_playbackIndex;
};

// High-level demo functions
void PlayDemo(int demoNum);
void PlayDemoFile(const char* filename);
void RecordDemo(int demoNum);
void StopDemo();

END_UPP_NAMESPACE

#endif // _Platformer2D_Core_DemoSystem_h_