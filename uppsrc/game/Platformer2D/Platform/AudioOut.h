#ifndef _Platformer2D_Platform_AudioOut_h_
#define _Platformer2D_Platform_AudioOut_h_

#include "Platform.h"
#include <Sound/Sound.h>

NAMESPACE_UPP

// ============================================================================
// Audio Output Adapter (U++ Sound)
// ============================================================================

class AudioOutput
{
public:
	static void Initialize();
	static void Shutdown();
	static void SetEnabled(bool enabled);
	
private:
	static void AudioCallback(int16_t* buffer, int frames);
	static bool s_initialized;
	static bool s_enabled;
};

END_UPP_NAMESPACE

#endif // _Platformer2D_Platform_AudioOut_h_