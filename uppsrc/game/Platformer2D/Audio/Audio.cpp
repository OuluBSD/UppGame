#include "Audio.h"
#include "OPL.h"
#include "PCSpeaker.h"
#include "Mixer.h"
#include "IMF.h"

NAMESPACE_UPP

// ============================================================================
// AudioSystem Static Members
// ============================================================================

AudioMixer* AudioSystem::s_mixer = nullptr;
bool AudioSystem::s_initialized = false;
uint32_t AudioSystem::s_timeCount = 0;
int32_t AudioSystem::s_lastTimeCount = 0;
uint16_t AudioSystem::s_spriteSync = 0;
int AudioSystem::s_soundMode = sdm_AdLib;
int AudioSystem::s_musicMode = smm_AdLib;
bool AudioSystem::s_quietSfx = false;

// ============================================================================
// AudioSystem Implementation
// ============================================================================

void AudioSystem::Startup()
{
	if (s_initialized) return;
	
	s_mixer = new AudioMixer();
	s_mixer->Initialize();
	
	// TODO: Initialize U++ Sound output
	// Sound::SetCallback(AudioCallback);
	
	s_initialized = true;
	LOG("AudioSystem::Startup");
}

void AudioSystem::Shutdown()
{
	if (!s_initialized) return;
	
	// Sound::SetCallback(nullptr);
	
	delete s_mixer;
	s_mixer = nullptr;
	s_initialized = false;
	LOG("AudioSystem::Shutdown");
}

void AudioSystem::SetOPLEmulator(const char* name)
{
	if (s_mixer)
	{
		// TODO: Recreate OPL emulator
		LOG("AudioSystem::SetOPLEmulator: " << name);
	}
}

void AudioSystem::PlaySound(int soundNum)
{
	if (s_mixer && !s_quietSfx)
	{
		// TODO: Load sound from resource and play
		LOG("AudioSystem::PlaySound: " << soundNum);
	}
}

void AudioSystem::StopSound()
{
	if (s_mixer) s_mixer->StopAllSounds();
}

void AudioSystem::WaitSoundDone()
{
	// TODO: Wait for all sound channels to finish
}

void AudioSystem::MusicOn()
{
	if (s_mixer && s_musicMode == smm_AdLib)
	{
		// TODO: Resume music
	}
}

void AudioSystem::MusicOff()
{
	if (s_mixer) s_mixer->StopMusic();
}

void AudioSystem::StartMusic(const uint8_t* data, int length)
{
	if (s_mixer && s_musicMode == smm_AdLib && data)
	{
		s_mixer->PlayMusic(data, length);
	}
}

void AudioSystem::FadeOutMusic()
{
	if (s_mixer) s_mixer->PauseMusic(true); // Simplified
}

bool AudioSystem::MusicPlaying()
{
	return s_mixer && s_mixer->IsMusicPlaying();
}

bool AudioSystem::SetSoundMode(int mode)
{
	if (mode >= sdm_Off && mode <= sdm_AdLib)
	{
		s_soundMode = mode;
		return true;
	}
	return false;
}

bool AudioSystem::SetMusicMode(int mode)
{
	if (mode >= smm_Off && mode <= smm_AdLib)
	{
		s_musicMode = mode;
		if (mode == smm_Off) MusicOff();
		return true;
	}
	return false;
}

int AudioSystem::GetSoundMode() { return s_soundMode; }
int AudioSystem::GetMusicMode() { return s_musicMode; }

void AudioSystem::SetQuietSfx(bool quiet) { s_quietSfx = quiet; }
bool AudioSystem::GetQuietSfx() { return s_quietSfx; }

uint32_t AudioSystem::GetTimeCount() { return s_timeCount; }
void AudioSystem::SetTimeCount(uint32_t val) { s_timeCount = val; }
int32_t AudioSystem::GetLastTimeCount() { return s_lastTimeCount; }
void AudioSystem::SetLastTimeCount(int32_t val) { s_lastTimeCount = val; }
uint16_t AudioSystem::GetSpriteSync() { return s_spriteSync; }
void AudioSystem::SetSpriteSync(uint16_t val) { s_spriteSync = val; }

void AudioSystem::WaitTick()
{
	// Called at 70 Hz from simulation
	s_lastTimeCount = s_timeCount;
	s_timeCount++;
	s_spriteSync++;
	
	if (s_mixer) s_mixer->Tick();
}

unsigned int AudioSystem::DetectHardware()
{
	// Return detected capabilities
	return SD_CARD_PC_SPEAKER | SD_CARD_OPL2 | SD_CARD_OPL3;
}

void AudioSystem::SetOPL3(bool enable)
{
	// TODO: Switch OPL emulator
}

void AudioSystem::AudioCallback(void* userdata, int16_t* stream, int frames)
{
	if (s_mixer && stream)
	{
		s_mixer->Mix(stream, frames);
	}
	else if (stream)
	{
		memset(stream, 0, frames * 2 * sizeof(int16_t)); // Stereo silence
	}
}

END_UPP_NAMESPACE