#ifndef _Umbrella_AudioSystem_h_
#define _Umbrella_AudioSystem_h_

#include <Core/Core.h>

using namespace Upp;

class AudioSystem {
	Vector<String> soundIds;
	Vector<String> soundPaths;
	Vector<bool>   soundLooping;
	Vector<double> soundVolumes;

	bool   initialized;
	double masterVolume;

	int GetSoundIndex(const String& id) const;

public:
	AudioSystem();
	~AudioSystem();

	bool Initialize();
	bool LoadSound(const String& id, const String& filePath);
	bool PlaySound(const String& id);
	bool PlayMusic(const String& id);
	void StopAllSounds();
	void StopMusic();
	bool SetVolume(const String& id, double volume);
	void SetMasterVolume(double volume);
	void Update();

	// Convenience wrapper used by gameplay code
	void Play(const char* id) { PlaySound(id); }

	bool IsInitialized() const { return initialized; }
};

AudioSystem& GetAudioSystem();

#endif
