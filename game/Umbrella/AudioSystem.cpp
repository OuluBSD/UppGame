#include "Umbrella.h"
#include "AudioSystem.h"

using namespace Upp;

AudioSystem::AudioSystem() : initialized(false), masterVolume(1.0) {}

AudioSystem::~AudioSystem() {
	StopAllSounds();
}

bool AudioSystem::Initialize() {
	initialized = true;
	return true;
}

bool AudioSystem::LoadSound(const String& id, const String& filePath) {
	if(!initialized) return false;
	for(int i = 0; i < soundIds.GetCount(); i++)
		if(soundIds[i] == id) return false;  // already loaded
	soundIds.Add(id);
	soundPaths.Add(filePath);
	soundLooping.Add(false);
	soundVolumes.Add(1.0);
	return true;
}

int AudioSystem::GetSoundIndex(const String& id) const {
	for(int i = 0; i < soundIds.GetCount(); i++)
		if(soundIds[i] == id) return i;
	return -1;
}

bool AudioSystem::PlaySound(const String& id) {
	if(!initialized) return false;
	// Stub: log the event; replace with real audio when assets are available
	RLOG("SFX: " << id);
	return true;
}

bool AudioSystem::PlayMusic(const String& id) {
	if(!initialized) return false;
	RLOG("Music: " << id);
	return true;
}

void AudioSystem::StopAllSounds() {}
void AudioSystem::StopMusic()     {}

bool AudioSystem::SetVolume(const String& id, double volume) {
	int idx = GetSoundIndex(id);
	if(idx == -1) return false;
	soundVolumes[idx] = clamp(volume, 0.0, 1.0);
	return true;
}

void AudioSystem::SetMasterVolume(double volume) {
	masterVolume = clamp(volume, 0.0, 1.0);
}

void AudioSystem::Update() {}

AudioSystem& GetAudioSystem() {
	static AudioSystem s;
	if(!s.IsInitialized()) s.Initialize();
	return s;
}
