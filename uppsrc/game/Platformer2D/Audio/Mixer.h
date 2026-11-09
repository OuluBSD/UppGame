#ifndef _Platformer2D_Audio_Mixer_h_
#define _Platformer2D_Audio_Mixer_h_

#include "Audio.h"


// ============================================================================
// AudioMixer Implementation
// ============================================================================

inline AudioMixer::AudioMixer()
{
	oplBuffer.SetCount(AUDIO_BUFFER_SIZE);
	pcBuffer.SetCount(AUDIO_BUFFER_SIZE);
	sfxBuffer.SetCount(AUDIO_BUFFER_SIZE);
}

inline AudioMixer::~AudioMixer()
{
	Shutdown();
}

inline void AudioMixer::Initialize(int sampleRate)
{
	this->sampleRate = sampleRate;
	
	// Create OPL emulator (default DBOPL)
	opl = CreateDBOPL();
	if (opl) opl->SetSampleRate(sampleRate);
	
	pcSpeaker.Reset();
	
	oplBuffer.SetCount(sampleRate / 70); // Buffer for one tick at 70 Hz
	pcBuffer.SetCount(sampleRate / 70);
	sfxBuffer.SetCount(sampleRate / 70);
}

inline void AudioMixer::Shutdown()
{
	delete opl; opl = nullptr;
	delete musicPlayer; musicPlayer = nullptr;
}

inline void AudioMixer::Mix(int16_t* output, int frames)
{
	// Generate OPL (mono)
	if (opl)
		opl->Generate(oplBuffer.Begin(), frames);
	else
		memset(oplBuffer.Begin(), 0, frames * sizeof(int16_t));
	
	// Generate PC Speaker (mono)
	pcSpeaker.Generate(pcBuffer.Begin(), frames);
	
	// Mix SFX
	memset(sfxBuffer.Begin(), 0, frames * sizeof(int16_t));
	for (int i = 0; i < MAX_SFX_CHANNELS; ++i)
	{
		SoundChannel& ch = sfxChannels[i];
		if (ch.active && ch.data)
		{
			int remain = ch.length - ch.pos;
			int todo = min(frames, remain);
			
			// Simple 8-bit unsigned -> 16-bit signed conversion
			for (int j = 0; j < todo; ++j)
			{
				int16_t sample = (int16_t)((ch.data[ch.pos + j] - 128) * 256 * ch.volume);
				sfxBuffer[j] = (int16_t)((sfxBuffer[j] + sample) / 2);
			}
			
			ch.pos += todo;
			if (ch.pos >= ch.length)
			{
				if (ch.loop) ch.pos = 0;
				else { ch.active = false; ch.data = nullptr; }
			}
		}
	}
	
	// Music tick (handled separately in Tick())
	
	// Final mix to stereo output
	float oplVol = musicVolume * masterVolume;
	float pcVol = sfxVolume * masterVolume * 0.5f; // PC speaker quieter
	float sfxVol = sfxVolume * masterVolume;
	
	for (int i = 0; i < frames; ++i)
	{
		int32_t left = 0, right = 0;
		
		// OPL (music) - center panned
		left += (int32_t)(oplBuffer[i] * oplVol);
		right += (int32_t)(oplBuffer[i] * oplVol);
		
		// PC Speaker - center
		left += (int32_t)(pcBuffer[i] * pcVol);
		right += (int32_t)(pcBuffer[i] * pcVol);
		
		// SFX - center
		left += (int32_t)(sfxBuffer[i] * sfxVol);
		right += (int32_t)(sfxBuffer[i] * sfxVol);
		
		// Clamp
		if (left > 32767) left = 32767;
		if (left < -32768) left = -32768;
		if (right > 32767) right = 32767;
		if (right < -32768) right = -32768;
		
		output[i * 2] = (int16_t)left;
		output[i * 2 + 1] = (int16_t)right;
	}
}

inline void AudioMixer::OPLWriteReg(uint8_t reg, uint8_t val)
{
	if (opl) opl->WriteReg(reg, val);
}

inline void AudioMixer::OPLWriteReg16(uint16_t reg, uint8_t val)
{
	// For OPL3
	if (auto nuked = dynamic_cast<NukedOPL3*>(opl))
		nuked->WriteReg16(reg, val);
}

inline void AudioMixer::PCSpeakerSetFreq(int freq)
{
	pcSpeaker.SetFrequency(freq);
}

inline void AudioMixer::PCSpeakerSetEnabled(bool on)
{
	pcSpeaker.SetEnabled(on);
}

inline int AudioMixer::PlaySound(const uint8_t* data, int length, int priority, bool loop)
{
	// Find free or lowest priority channel
	int bestIdx = -1;
	int bestPriority = 255;
	
	for (int i = 0; i < MAX_SFX_CHANNELS; ++i)
	{
		if (!sfxChannels[i].active)
		{
			bestIdx = i;
			break;
		}
		if (sfxChannels[i].priority < bestPriority)
		{
			bestPriority = sfxChannels[i].priority;
			bestIdx = i;
		}
	}
	
	if (bestIdx >= 0)
	{
		SoundChannel& ch = sfxChannels[bestIdx];
		ch.data = data;
		ch.length = length;
		ch.pos = 0;
		ch.priority = priority;
		ch.loop = loop;
		ch.active = true;
		ch.volume = 1.0f;
		return bestIdx;
	}
	return -1;
}

inline void AudioMixer::StopSound(int handle)
{
	if (handle >= 0 && handle < MAX_SFX_CHANNELS)
	{
		sfxChannels[handle].active = false;
		sfxChannels[handle].data = nullptr;
	}
}

inline void AudioMixer::StopAllSounds()
{
	for (int i = 0; i < MAX_SFX_CHANNELS; ++i)
	{
		sfxChannels[i].active = false;
		sfxChannels[i].data = nullptr;
	}
}

inline void AudioMixer::PlayMusic(const uint8_t* imfData, int length, bool loop)
{
	if (!musicPlayer)
		musicPlayer = new IMFPlayer(opl);
	
	if (musicPlayer->Load(imfData, length))
		musicPlayer->Play(loop);
}

inline void AudioMixer::StopMusic()
{
	if (musicPlayer) musicPlayer->Stop();
}

inline void AudioMixer::PauseMusic(bool pause)
{
	if (musicPlayer) musicPlayer->Pause(pause);
}

inline void AudioMixer::SetMasterVolume(float vol)
{
	masterVolume = clamp(vol, 0.0f, 1.0f);
}

inline void AudioMixer::SetMusicVolume(float vol)
{
	musicVolume = clamp(vol, 0.0f, 1.0f);
}

inline void AudioMixer::SetSfxVolume(float vol)
{
	sfxVolume = clamp(vol, 0.0f, 1.0f);
}

inline void AudioMixer::Tick()
{
	// Called at 70 Hz
	if (musicPlayer) musicPlayer->Tick();
}

inline bool AudioMixer::IsMusicPlaying() const
{
	return musicPlayer && musicPlayer->IsPlaying();
}


#endif // _Platformer2D_Audio_Mixer_h_