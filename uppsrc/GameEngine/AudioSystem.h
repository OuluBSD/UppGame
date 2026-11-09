#ifndef UPP_AUDIOSYSTEM_H
#define UPP_AUDIOSYSTEM_H

#include <Core/Core.h>
#include <GameLib/GameLib.h>
#include <api/Audio/Audio.h>  // Audio API includes
#include <Eon/Eon.h>          // For Eon engine integration

NAMESPACE_UPP

// Forward declarations
class SoundBuffer;
class SoundSource;

// Audio system class that integrates with the Audio API
class AudioSystem {
public:
	AudioSystem();
	virtual ~AudioSystem();
	
	// Initialize the audio system
	bool Initialize(Engine& engine);
	void Uninitialize();
	
	// Update audio system (for 3D audio, etc.)
	void Update(double dt);
	
	// Load a sound buffer from file
	std::shared_ptr<SoundBuffer> LoadSound(const String& filepath);
	
	// Play a sound - returns a SoundSource that can be controlled
	std::shared_ptr<SoundSource> PlaySound(std::shared_ptr<SoundBuffer> buffer, 
	                                      bool loop = false, 
	                                      double volume = 1.0);
	
	// Set master volume
	void SetMasterVolume(double volume);
	double GetMasterVolume() const { return master_volume; }
	
	// 3D audio functions
	void SetListenerPosition(const Point3& position);
	void SetListenerOrientation(const Vector3& forward, const Vector3& up);
	Point3 GetListenerPosition() const { return listener_position; }
	
	// Check if audio system is initialized
	bool IsInitialized() const { return initialized; }
	
	// Direct access to audio atoms
	AtomBase* GetAudioSink() const { return audio_sink; }
	AtomBase* GetAudioSource() const { return audio_source; }

private:
	// Audio system state
	bool initialized = false;
	double master_volume = 1.0;
	Point3 listener_position = Point3(0, 0, 0);
	Vector3 listener_forward = Vector3(0, 0, 1);
	Vector3 listener_up = Vector3(0, 1, 0);
	
	// Audio atoms
	AtomBase* audio_sink = nullptr;
	AtomBase* audio_source = nullptr;
	
	// Engine reference
	Engine* engine = nullptr;
	
	// Initialize audio atoms
	bool InitializeAudioAtoms();
	
	// Internal update
	void ProcessAudioEvents();
};

// Sound buffer class to hold audio data
class SoundBuffer : public Moveable<SoundBuffer> {
public:
	SoundBuffer();
	virtual ~SoundBuffer();
	
	// Load from file
	bool LoadFromFile(const String& filepath);
	
	// Getters
	int GetSampleRate() const { return sample_rate; }
	int GetChannels() const { return channels; }
	int GetSampleCount() const { return sample_count; }
	
	// Get audio data
	const Vector<float>& GetAudioData() const { return audio_data; }
	
private:
	Vector<float> audio_data;
	int sample_rate = 44100;
	int channels = 2;
	int sample_count = 0;
	String filepath;
};

// Sound source for controlling playback
class SoundSource {
public:
	SoundSource(std::shared_ptr<SoundBuffer> buffer, bool loop = false, double volume = 1.0);
	virtual ~SoundSource();
	
	// Playback control
	void Play();
	void Pause();
	void Stop();
	void Rewind();
	
	// Properties
	void SetVolume(double volume);
	double GetVolume() const { return volume; }
	
	void SetLooping(bool loop);
	bool IsLooping() const { return looping; }
	
	void SetPitch(double pitch);
	double GetPitch() const { return pitch; }
	
	// 3D properties
	void SetPosition(const Point3& pos);
	Point3 GetPosition() const { return position; }
	
	void SetVelocity(const Vector3& vel);
	Vector3 GetVelocity() const { return velocity; }
	
	// State
	bool IsPlaying() const;
	bool IsPaused() const;
	bool IsStopped() const;
	
	// Get the associated buffer
	std::shared_ptr<SoundBuffer> GetBuffer() const { return buffer; }

private:
	std::shared_ptr<SoundBuffer> buffer;
	bool looping = false;
	double volume = 1.0;
	double pitch = 1.0;
	Point3 position = Point3(0, 0, 0);
	Vector3 velocity = Vector3(0, 0, 0);
	
	// Playback state
	enum class State { STOPPED, PLAYING, PAUSED };
	State state = State::STOPPED;
};

END_UPP_NAMESPACE

#endif