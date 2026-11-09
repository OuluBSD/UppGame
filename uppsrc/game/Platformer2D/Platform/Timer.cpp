#include "Timer.h"
#include "Window.h"

NAMESPACE_UPP

uint32_t FixedStepTimer::s_tickCount = 0;
int32_t FixedStepTimer::s_lastTickCount = 0;
uint16_t FixedStepTimer::s_spriteSync = 0;
bool FixedStepTimer::s_initialized = false;
bool FixedStepTimer::s_headless = false;
int FixedStepTimer::s_timerId = -1;
double FixedStepTimer::s_accumulator = 0.0;

void FixedStepTimer::Initialize()
{
	if (s_initialized) return;
	
	// Timer will be started when window is created
	s_initialized = true;
	LOG("FixedStepTimer::Initialize");
}

void FixedStepTimer::Shutdown()
{
	if (!s_initialized) return;
	
	// Stop timer if running
	StopTimer();
	
	s_initialized = false;
}

void FixedStepTimer::StartTimer()
{
	if (s_timerId >= 0) return; // Already started
	
	if (Platformer2DWindow::GetInstance())
	{
		// 70 Hz = ~14.28ms per tick
		// Use negative delay for periodic timer (U++ convention)
		s_timerId = 0; // Use ID 0 (offset into Ctrl object)
		Platformer2DWindow::GetInstance()->SetTimeCallback(-int(MS_PER_TICK), callback(&FixedStepTimer::TimerCallback), s_timerId);
		LOG("FixedStepTimer::StartTimer: timerId=" << s_timerId);
	}
}

void FixedStepTimer::StopTimer()
{
	if (s_timerId >= 0 && Platformer2DWindow::GetInstance())
	{
		Platformer2DWindow::GetInstance()->KillTimeCallback(s_timerId);
		s_timerId = -1;
	}
}

void FixedStepTimer::HeadlessTick()
{
	s_headless = true;
	TimerCallback();
	s_headless = false;
}

uint32_t FixedStepTimer::GetTickCount()
{
	return s_tickCount;
}

void FixedStepTimer::SetTickCount(uint32_t ticks)
{
	s_tickCount = ticks;
}

void FixedStepTimer::WaitTick()
{
	// In headless mode, this is a no-op (tick advanced manually)
	// In normal mode, would wait for next timer callback
	if (!s_headless)
	{
		// Could use a condition variable here
		Sleep(1);
	}
}

void FixedStepTimer::TimerCallback()
{
	s_lastTickCount = s_tickCount;
	s_tickCount++;
	s_spriteSync++;
	
	// Update audio system timing
	AudioSystem::WaitTick();
	
	// Run one simulation tick
	// CK_GameTick(); // Would be called here
	
	// Request repaint
	if (Platformer2DWindow::GetInstance())
		Platformer2DWindow::GetInstance()->Refresh();
}

END_UPP_NAMESPACE