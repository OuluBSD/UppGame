#ifndef _Platformer2D_Audio_Audio_h_
#define _Platformer2D_Audio_Audio_h_

#include <game/Platformer2D/Platformer2D.h>
#include <game/Platformer2D/Core/Core.h>
#include <game/Platformer2D/Resource/Resource.h>
#include <Sound/Sound.h>

NAMESPACE_UPP

// ============================================================================
// Audio Constants
// ============================================================================

inline constexpr int AUDIO_SAMPLE_RATE = 44100;
inline constexpr int AUDIO_BUFFER_SIZE = 2048;
inline constexpr int OPL_CHANNELS = 9;
inline constexpr int OPL3_CHANNELS = 18;

// ============================================================================
// OPL Emulator Interface
// ============================================================================

class OPLEmulator
{
public:
	virtual ~OPLEmulator() {}
	
	virtual void Reset() = 0;
	virtual void WriteReg(uint8_t reg, uint8_t val) = 0;
	virtual void Generate(int16_t* buffer, int frames) = 0; // Mono output
	virtual void SetSampleRate(int rate) = 0;
	virtual const char* GetName() const = 0;
};

// Factory
OPLEmulator* CreateDBOPL();
OPLEmulator* CreateNukedOPL3();

// Forward declaration
class IMFPlayer;

// ============================================================================
// PC Speaker
// ============================================================================

class PCSpeaker
{
public:
	PCSpeaker();
	
	void Reset();
	void SetFrequency(int freq); // 0 = off
	void SetEnabled(bool on);
	void Generate(int16_t* buffer, int frames); // Mono 1-bit PWM -> 16-bit
	uint32_t GetPhaseInc() const { return phaseInc; }
	int GetFrequency() const { return frequency; }
	bool IsEnabled() const { return enabled; }
	
private:
	int frequency = 0;
	bool enabled = false;
	uint32_t phase = 0;
	uint32_t phaseInc = 0;
};

// ============================================================================
// Audio Mixer
// ============================================================================

class AudioMixer
{
public:
	AudioMixer();
	~AudioMixer();
	
	void Initialize(int sampleRate = AUDIO_SAMPLE_RATE);
	void Shutdown();
	
	// Main mixing function (called from audio callback)
	void Mix(int16_t* output, int frames); // Stereo interleaved
	
	// OPL control
	void OPLWriteReg(uint8_t reg, uint8_t val);
	void OPLWriteReg16(uint16_t reg, uint8_t val); // For OPL3
	
	// PC Speaker
	void PCSpeakerSetFreq(int freq);
	void PCSpeakerSetEnabled(bool on);
	
	// Sound Effects
	int PlaySound(const uint8_t* data, int length, int priority = 0, bool loop = false);
	void StopSound(int handle);
	void StopAllSounds();
	
	// Music (IMF)
	void PlayMusic(const uint8_t* imfData, int length, bool loop = true);
	void StopMusic();
	void PauseMusic(bool pause);
	
	// Volume
	void SetMasterVolume(float vol); // 0.0 - 1.0
	void SetMusicVolume(float vol);
	void SetSfxVolume(float vol);
	float GetMasterVolume() const { return masterVolume; }
	float GetMusicVolume() const { return musicVolume; }
	float GetSfxVolume() const { return sfxVolume; }
	
	// Timing (called at 70 Hz)
	void Tick();
	
private:
	struct SoundChannel
	{
		const uint8_t* data = nullptr;
		int length = 0;
		int pos = 0;
		int priority = 0;
		bool loop = false;
		bool active = false;
		float volume = 1.0f;
	};
	
	static constexpr int MAX_SFX_CHANNELS = 8;
	SoundChannel sfxChannels[MAX_SFX_CHANNELS];
	
	OPLEmulator* opl = nullptr;
	PCSpeaker pcSpeaker;
	
	// Music
	IMFPlayer* musicPlayer = nullptr;
	
public:
	bool IsMusicPlaying() const;
	
private:
	// Mixing buffers
	Vector<int16_t> oplBuffer;
	Vector<int16_t> pcBuffer;
	Vector<int16_t> sfxBuffer;
	
	float masterVolume = 1.0f;
	float musicVolume = 1.0f;
	float sfxVolume = 1.0f;
	
	int sampleRate = AUDIO_SAMPLE_RATE;
};

// ============================================================================
// IMF Music Player
// ============================================================================

class IMFPlayer
{
public:
	IMFPlayer(OPLEmulator* opl);
	~IMFPlayer();
	
	bool Load(const uint8_t* data, int length);
	void Play(bool loop = true);
	void Stop();
	void Pause(bool pause);
	void Tick(); // Called at 70 Hz
	bool IsPlaying() const { return playing && !paused; }
	
private:
	struct IMFCommand
	{
		uint16_t delay; // In 70 Hz ticks
		uint8_t reg;
		uint8_t val;
	};
	
	Vector<IMFCommand> commands;
	int cmdIndex = 0;
	uint16_t nextDelay = 0;
	bool playing = false;
	bool paused = false;
	bool loop = false;
	OPLEmulator* opl = nullptr;
};

// ============================================================================
// Audio System (Global)
// ============================================================================

class AudioSystem
{
public:
	static void Startup();
	static void Shutdown();
	
	static void SetOPLEmulator(const char* name); // "dbopl" or "nukedopl3"
	
	// Sound/Music
	static void PlaySound(int soundNum);
	static void StopSound();
	static void WaitSoundDone();
	static void MusicOn();
	static void MusicOff();
	static void StartMusic(const uint8_t* data, int length);
	static void FadeOutMusic();
	static bool MusicPlaying();
	
	// Modes
	static bool SetSoundMode(int mode); // SD_SoundMode
	static bool SetMusicMode(int mode); // ID_MusicMode
	static int GetSoundMode();
	static int GetMusicMode();
	
	// Volume
	static void SetQuietSfx(bool quiet);
	static bool GetQuietSfx();
	
	// Timing (matches OmniSpeak SD_*)
	static uint32_t GetTimeCount();
	static void SetTimeCount(uint32_t val);
	static int32_t GetLastTimeCount();
	static void SetLastTimeCount(int32_t val);
	static uint16_t GetSpriteSync();
	static void SetSpriteSync(uint16_t val);
	static void WaitTick();
	
	// Hardware detection
	static unsigned int DetectHardware();
	static void SetOPL3(bool enable);
	
	// U++ Sound callback
	static void AudioCallback(void* userdata, int16_t* stream, int frames);
	
private:
	static AudioMixer* s_mixer;
	static bool s_initialized;
	static uint32_t s_timeCount;
	static int32_t s_lastTimeCount;
	static uint16_t s_spriteSync;
	static int s_soundMode;
	static int s_musicMode;
	static bool s_quietSfx;
};

// ============================================================================
// Sound Mode Enums (matching OmniSpeak)
// ============================================================================

enum SD_SoundMode : int16_t
{
	sdm_Off = 0,
	sdm_PC = 1,
	sdm_AdLib = 2,
};

enum ID_MusicMode : int16_t
{
	smm_Off = 0,
	smm_AdLib = 1,
};

// Soundcard detection bits
inline constexpr int SD_CARD_PC_SPEAKER = 1;
inline constexpr int SD_CARD_OPL2 = 2;
inline constexpr int SD_CARD_OPL3 = 4;
inline constexpr int SD_CARD_BLASTER = 8;

// Implementation headers (included after class definitions)
#include <game/Platformer2D/Audio/PCSpeaker.h>
#include <game/Platformer2D/Audio/OPL.h>
#include <game/Platformer2D/Audio/Mixer.h>
#include <game/Platformer2D/Audio/IMF.h>

END_UPP_NAMESPACE

#endif // _Platformer2D_Audio_Audio_h_