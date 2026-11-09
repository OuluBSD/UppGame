#ifndef _Platformer2D_Audio_PCSpeaker_h_
#define _Platformer2D_Audio_PCSpeaker_h_

#include "Audio.h"


// ============================================================================
// PCSpeaker Implementation
// ============================================================================

inline PCSpeaker::PCSpeaker()
{
	Reset();
}

inline void PCSpeaker::Reset()
{
	frequency = 0;
	enabled = false;
	phase = 0;
	phaseInc = 0;
}

inline void PCSpeaker::SetFrequency(int freq)
{
	frequency = freq;
	if (freq > 0)
	{
		// Phase increment for 1-bit square wave at sample rate
		phaseInc = (uint32_t)((uint64_t)freq * (1ull << 32) / AUDIO_SAMPLE_RATE);
	}
	else
	{
		phaseInc = 0;
	}
}

inline void PCSpeaker::SetEnabled(bool on)
{
	enabled = on;
	if (!on) phase = 0;
}

inline void PCSpeaker::Generate(int16_t* buffer, int frames)
{
	if (!enabled || frequency == 0)
	{
		memset(buffer, 0, frames * sizeof(int16_t));
		return;
	}
	
	// Generate 1-bit square wave, output as 16-bit PCM
	for (int i = 0; i < frames; ++i)
	{
		// Square wave: high for half period, low for half
		uint32_t halfPeriod = (1u << 31) / phaseInc; // Not exactly right
		// Simplified: use phase bit 31 as square wave
		int16_t sample = (phase & 0x80000000) ? 32767 : -32768;
		buffer[i] = sample;
		phase += phaseInc;
	}
}


#endif // _Platformer2D_Audio_PCSpeaker_h_