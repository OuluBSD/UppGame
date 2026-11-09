#include "AudioOut.h"
#include <game/Platformer2D/Audio/Audio.h>

NAMESPACE_UPP

bool AudioOutput::s_initialized = false;
bool AudioOutput::s_enabled = true;

void AudioOutput::Initialize()
{
	if (s_initialized) return;
	
	// Initialize U++ Sound system
	// Sound::SetCallback(AudioCallback);
	// Sound::Start();
	
	s_initialized = true;
	LOG("AudioOutput::Initialize");
}

void AudioOutput::Shutdown()
{
	if (!s_initialized) return;
	
	// Sound::Stop();
	// Sound::SetCallback(nullptr);
	
	s_initialized = false;
}

void AudioOutput::SetEnabled(bool enabled)
{
	s_enabled = enabled;
}

void AudioOutput::AudioCallback(int16_t* buffer, int frames)
{
	if (s_enabled)
		AudioSystem::AudioCallback(nullptr, buffer, frames);
	else
		memset(buffer, 0, frames * 2 * sizeof(int16_t));
}

END_UPP_NAMESPACE