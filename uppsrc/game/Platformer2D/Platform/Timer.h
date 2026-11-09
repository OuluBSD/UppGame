#ifndef _Platformer2D_Platform_Timer_h_
#define _Platformer2D_Platform_Timer_h_

#include "Platform.h"
#include <game/Platformer2D/Core/Core.h>
#include <CtrlLib/CtrlLib.h>

NAMESPACE_UPP

// ============================================================================
// Fixed Step Timer (70 Hz)
// ============================================================================

class FixedStepTimer
{
public:
	static void Initialize();
	static void Shutdown();
	
	// For headless mode
	static void HeadlessTick();
	
	// Simulation clock interface implementation
	static uint32_t GetTickCount();
	static void SetTickCount(uint32_t ticks);
	static void WaitTick();
	
	// Called from CtrlLib timer callback (every ~14ms for 70 Hz)
	static void TimerCallback();
	
	// Start/stop the timer (requires window to exist)
	static void StartTimer();
	static void StopTimer();
	
private:
	static uint32_t s_tickCount;
	static int32_t s_lastTickCount;
	static uint16_t s_spriteSync;
	static bool s_initialized;
	static bool s_headless;
	static int s_timerId; // Timer ID from SetTimer
	
	// Accumulator for fixed step
	static double s_accumulator;
	static constexpr double MS_PER_TICK = 1000.0 / PLATFORMER2D_TICKS_PER_SECOND;
};

// Headless clock implementation
class HeadlessClock : public ISimulationClock
{
public:
	uint32_t GetTickCount() const override { return FixedStepTimer::GetTickCount(); }
	void SetTickCount(uint32_t ticks) override { FixedStepTimer::SetTickCount(ticks); }
	void WaitTick() override { FixedStepTimer::WaitTick(); }
};

END_UPP_NAMESPACE

#endif // _Platformer2D_Platform_Timer_h_