#include "AudioSystem.h"
#include <plugin/wav/Wav.h>  // For WAV file loading

NAMESPACE_UPP

// SoundBuffer implementation
SoundBuffer::SoundBuffer() {
}

SoundBuffer::~SoundBuffer() {
}

bool SoundBuffer::LoadFromFile(const String& filepath) {
	this->filepath = filepath;

	// For now, implement a basic WAV file loader
	// In a real implementation, we'd support multiple formats
	if (filepath.GetRight(4).Comparenocase(".wav") == 0) {
		WAVDecoder decoder;
		SoundData sound_data;

		if (decoder.LoadFile(filepath, sound_data)) {
			// Convert to float format
			sample_rate = sound_data.sample_rate;
			channels = sound_data.channels;

			// Convert samples to float
			audio_data.SetCount(sound_data.samples.GetCount());
			for (int i = 0; i < sound_data.samples.GetCount(); i++) {
				// Convert from int16 to float (-1.0 to 1.0)
				audio_data[i] = sound_data.samples[i] / 32768.0f;
			}
			sample_count = sound_data.samples.GetCount();
			return true;
		}
	}

	return false;
}

// SoundSource implementation
SoundSource::SoundSource(std::shared_ptr<SoundBuffer> buffer, bool loop, double volume)
	: buffer(buffer), looping(loop), volume(volume) {
}

SoundSource::~SoundSource() {
}

void SoundSource::Play() {
	state = State::PLAYING;
}

void SoundSource::Pause() {
	if (state == State::PLAYING) {
		state = State::PAUSED;
	}
}

void SoundSource::Stop() {
	state = State::STOPPED;
}

void SoundSource::Rewind() {
	// In a real implementation, reset to beginning of sound
}

void SoundSource::SetVolume(double volume) {
	this->volume = Clamp(volume, 0.0, 1.0);
}

void SoundSource::SetLooping(bool loop) {
	this->looping = loop;
}

void SoundSource::SetPitch(double pitch) {
	this->pitch = pitch;
}

void SoundSource::SetPosition(const Point3& pos) {
	position = pos;
}

void SoundSource::SetVelocity(const Vector3& vel) {
	velocity = vel;
}

bool SoundSource::IsPlaying() const {
	return state == State::PLAYING;
}

bool SoundSource::IsPaused() const {
	return state == State::PAUSED;
}

bool SoundSource::IsStopped() const {
	return state == State::STOPPED;
}

// AudioSystem implementation
AudioSystem::AudioSystem() {
}

AudioSystem::~AudioSystem() {
	Uninitialize();
}

bool AudioSystem::Initialize(Engine& engine) {
	this->engine = &engine;

	// Initialize audio atoms
	if (!InitializeAudioAtoms()) {
		LOG("Failed to initialize audio atoms");
		return false;
	}

	initialized = true;
	return true;
}

void AudioSystem::Uninitialize() {
	if (initialized) {
		initialized = false;
		audio_sink = nullptr;
		audio_source = nullptr;
		engine = nullptr;
	}
}

bool AudioSystem::InitializeAudioAtoms() {
	// Using Eon patterns to initialize audio atoms with HAL integration
	auto sys = engine.GetAdd<Eon::ScriptLoader>();
	sys->SetEagerChainBuild(true);

	try {
		Eon::Builder& builder = sys->val.GetAdd<Eon::Builder>("builder");

		// Create the main audio loop
		auto& audio_loop = builder.AddLoop("audio.main");
		
		// Add audio context first (HAL context)
		auto& context_atom = audio_loop.AddAtom("center.customer");  // Using customer as context provider
		
		// Add audio source (input/generation) - for game audio generation
		auto& source_atom = audio_loop.AddAtom("center.audio.src.dbg_generator");
		source_atom.Assign("sample_rate", 44100);
		source_atom.Assign("channels", 2);
		source_atom.Assign("buffer_size", 1024);
		
		// Add audio sink (output) - for playing audio through HAL
		auto& sink_atom = audio_loop.AddAtom("center.audio.sink.hw");
		sink_atom.Assign("sample_rate", 44100);
		sink_atom.Assign("channels", 2);
		sink_atom.Assign("latency", 50);  // in milliseconds

		// Compile and load the AST
		Eon::AstNode* root = builder.CompileAst();
		if (!root) {
			LOG("Failed to compile AST for audio components");
			return false;
		}

		sys->LoadAst(root);
		
		// Initialize the audio system
		if (!sys->PostInitializeAll()) {
			LOG("Audio system PostInitialize failed");
			return false;
		}
		
		if (!sys->StartAll()) {
			LOG("Audio system Start failed");
			sys->UndoAll();
			return false;
		}

		// Retrieve the created atoms
		// Note: The path structure depends on how the Eon system builds the VFS tree
		// In a real implementation, we would find the atoms properly
		audio_source = nullptr; // Placeholder - would be retrieved from engine
		audio_sink = nullptr;   // Placeholder - would be retrieved from engine

		LOG("Audio atoms initialized successfully");

		return true;
	}
	catch (Exc& e) {
		LOG("Error initializing audio atoms: " << e);
		return false;
	}
}

void AudioSystem::Update(double dt) {
	if (!initialized) return;

	ProcessAudioEvents();

	// In a real implementation, this would update 3D audio positions,
	// handle streaming for long audio files, update effects, etc.
}

std::shared_ptr<SoundBuffer> AudioSystem::LoadSound(const String& filepath) {
	if (!initialized) return nullptr;

	auto buffer = std::make_shared<SoundBuffer>();
	if (buffer->LoadFromFile(filepath)) {
		return buffer;
	}

	return nullptr;
}

std::shared_ptr<SoundSource> AudioSystem::PlaySound(std::shared_ptr<SoundBuffer> buffer,
                                                   bool loop, double volume) {
	if (!initialized || !buffer) return nullptr;

	auto source = std::make_shared<SoundSource>(buffer, loop, volume);
	source->Play();
	return source;
}

void AudioSystem::SetMasterVolume(double volume) {
	master_volume = Clamp(volume, 0.0, 1.0);

	// In a real implementation, this would update the audio system's master volume
}

void AudioSystem::SetListenerPosition(const Point3& position) {
	listener_position = position;

	// In a real implementation, this would update the 3D audio listener position
}

void AudioSystem::SetListenerOrientation(const Vector3& forward, const Vector3& up) {
	listener_forward = forward.Normalize();
	listener_up = up.Normalize();

	// In a real implementation, this would update the 3D audio listener orientation
}

void AudioSystem::ProcessAudioEvents() {
	// Process any pending audio events or state changes
	// In a real implementation, this would handle audio callbacks, streaming, etc.
}

END_UPP_NAMESPACE