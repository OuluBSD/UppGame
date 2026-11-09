#include "AudioStreaming.h"
#include <plugin/wav/Wav.h>

NAMESPACE_UPP

AudioStreamSource::AudioStreamSource(const String& filepath, bool loop, double volume)
    : filepath(filepath), looping(loop), volume(volume), vfs(nullptr) {
}

AudioStreamSource::AudioStreamSource(const String& vfsPath, std::shared_ptr<VFS> vfs, bool loop, double volume)
    : filepath(vfsPath), vfs(vfs), looping(loop), volume(volume) {
}

AudioStreamSource::~AudioStreamSource() {
    Stop();

    // Wait for streaming thread to finish
    if (stream_thread_running) {
        stream_thread_running = false;
        if (stream_thread.IsOpen()) {
            stream_thread.Wait();
        }
    }
}

bool AudioStreamSource::Initialize() {
    // Initialize with file information
    return InitializeReader();
}

bool AudioStreamSource::InitializeFromVFS() {
    // Initialize with VFS file information
    return InitializeVFSReader();
}

bool AudioStreamSource::InitializeReader() {
    String ext = filepath.GetRight(4).ToLower();

    // For streaming, we'll just check the header to get format info
    FileIn fi(filepath);
    if (!fi.IsOpen()) {
        return false;
    }

    // Check file extension to determine decoder
    if (ext == ".wav") {
        // Read WAV header to get format info
        WAVDecoder decoder;
        SoundData temp_data;

        if (decoder.LoadFile(filepath, temp_data)) {
            sample_rate = temp_data.sample_rate;
            channels = temp_data.channels;
            total_samples = temp_data.samples.GetCount();
            valid = true;

            // Initialize streaming buffer
            streaming_buffer.SetCount(buffer_size * channels); // Account for channels
            return true;
        }
    }
    // Additional format support would go here
    /*
    else if (ext == ".ogg") {
        // Initialize OGG decoder
        // Get format info: sample_rate, channels, total_samples
        // Set valid = true;
        // return true;
    }
    else if (ext == ".mp3") {
        // Initialize MP3 decoder
        // Get format info: sample_rate, channels, total_samples
        // Set valid = true;
        // return true;
    }
    */

    return false;
}

bool AudioStreamSource::InitializeVFSReader() {
    if (!vfs) {
        return false;
    }

    String ext = GetFileExt(filepath).ToLower();

    // Check if file exists in VFS
    if (!vfs->FileExists(filepath)) {
        return false;
    }

    // Check file extension to determine decoder
    if (ext == ".wav") {
        // Load the sound data to get format info
        SoundData temp_data = vfs->LoadSound(filepath);

        if (temp_data.samples.GetCount() > 0) {
            sample_rate = temp_data.sample_rate;
            channels = temp_data.channels;
            total_samples = temp_data.samples.GetCount();
            valid = true;

            // Initialize streaming buffer
            streaming_buffer.SetCount(buffer_size * channels); // Account for channels
            return true;
        }
    }
    // Additional format support would go here
    /*
    else if (ext == ".ogg") {
        // Load OGG data from VFS
        // Get format info: sample_rate, channels, total_samples
        // Set valid = true;
        // return true;
    }
    else if (ext == ".mp3") {
        // Load MP3 data from VFS
        // Get format info: sample_rate, channels, total_samples
        // Set valid = true;
        // return true;
    }
    */

    return false;
}

void AudioStreamSource::Play() {
    if (!valid) return;

    {
        Mutex::Lock lock(stream_cs);
        if (state == State::PAUSED) {
            state = State::PLAYING;
            return;
        }
    }

    // Start from beginning if stopped
    if (state == State::STOPPED) {
        Rewind();
        state = State::PLAYING;

        // Start streaming thread if not already running
        if (!stream_thread_running) {
            stream_thread_running = true;
            stream_thread.Run([this] { StreamThread(); });
        }
    }

    // In a real implementation, we'd start audio playback through the audio system
}

void AudioStreamSource::Pause() {
    if (state == State::PLAYING) {
        state = State::PAUSED;
    }
}

void AudioStreamSource::Stop() {
    state = State::STOPPED;
    playback_position = 0;
    buffer_read_pos = 0;
    buffer_write_pos = 0;
    buffer_full = false;
    stream_eof = false;
}

void AudioStreamSource::Rewind() {
    playback_position = 0;
    buffer_read_pos = 0;
    buffer_write_pos = 0;
    buffer_full = false;
    stream_eof = false;
}

void AudioStreamSource::SetVolume(double volume) {
    this->volume = Clamp(volume, 0.0, 1.0);
}

void AudioStreamSource::SetLooping(bool loop) {
    this->looping = loop;
}

void AudioStreamSource::SetPitch(double pitch) {
    this->pitch = pitch;
}

void AudioStreamSource::SetPosition(const Point3& pos) {
    position = pos;
}

void AudioStreamSource::SetVelocity(const Vector3& vel) {
    velocity = vel;
}

bool AudioStreamSource::IsPlaying() const {
    return state == State::PLAYING;
}

bool AudioStreamSource::IsPaused() const {
    return state == State::PAUSED;
}

bool AudioStreamSource::IsStopped() const {
    return state == State::STOPPED;
}

double AudioStreamSource::GetPlaybackPosition() const {
    return playback_position / (double)sample_rate;  // Convert samples to seconds
}

void AudioStreamSource::SetPlaybackPosition(double seconds) {
    int new_position = (int)(seconds * sample_rate);
    if (new_position >= 0 && new_position <= total_samples) {
        playback_position = new_position;
    }
}

void AudioStreamSource::SetBufferSize(int sample_count) {
    if (sample_count > 0) {
        buffer_size = sample_count;
        // Resize buffer (accounting for channels)
        streaming_buffer.SetCount(buffer_size * channels);
    }
}

void AudioStreamSource::FillBuffer() {
    if (stream_eof) return;

    Mutex::Lock lock(stream_cs);

    // Calculate how much space is available in buffer
    int available_space;
    if (buffer_full) {
        available_space = 0;
    } else if (buffer_write_pos >= buffer_read_pos) {
        available_space = buffer_size - (buffer_write_pos - buffer_read_pos);
    } else {
        available_space = buffer_read_pos - buffer_write_pos;
    }

    if (available_space == 0) {
        return; // Buffer is full
    }

    // Try to fill as much as possible
    SoundData temp_data;

    // Determine if we're using VFS or regular files
    if (vfs) {
        // Load from VFS
        temp_data = vfs->LoadSound(filepath);
    } else {
        // Load from regular file
        WAVDecoder decoder;
        if (!decoder.LoadFile(filepath, temp_data)) {
            return; // Failed to load
        }
    }

    // Copy samples from the current playback position
    int samples_to_copy = min(available_space, (int)temp_data.samples.GetCount() - (int)playback_position);

    for (int i = 0; i < samples_to_copy; i++) {
        if (playback_position + i < temp_data.samples.GetCount()) {
            // Convert from int16 to float (-1.0 to 1.0)
            float sample = temp_data.samples[playback_position + i] / 32768.0f;
            if (channels == 2) {
                // For stereo, we need to handle both channels
                streaming_buffer[buffer_write_pos * channels + (i % 2)] = sample;
            } else {
                // For mono or single channel processing
                streaming_buffer[buffer_write_pos * channels + i % channels] = sample;
            }
        }
    }

    playback_position += samples_to_copy;
    buffer_write_pos = (buffer_write_pos + samples_to_copy) % buffer_size;

    // Check if buffer is now full
    if ((buffer_write_pos + 1) % buffer_size == buffer_read_pos) {
        buffer_full = true;
    }

    // Check if we've reached the end
    if (playback_position >= total_samples) {
        if (looping) {
            playback_position = 0; // Loop back to beginning
        } else {
            stream_eof = true; // Mark end of file
        }
    }
}

void AudioStreamSource::StreamThread() {
    while (stream_thread_running) {
        if (state == State::PLAYING) {
            FillBuffer();
        }

        // Small sleep to prevent busy waiting
        Sleep(10);
    }
}

// AudioStreamingSystem implementation
AudioStreamingSystem::AudioStreamingSystem() {
}

AudioStreamingSystem::~AudioStreamingSystem() {
    ClearAllStreams();
}

bool AudioStreamingSystem::Initialize(Engine& engine) {
    this->engine = &engine;
    initialized = true;
    return true;
}

std::shared_ptr<AudioStreamSource> AudioStreamingSystem::CreateStream(const String& filepath,
                                                                     bool loop,
                                                                     double volume) {
    if (!initialized) return nullptr;

    auto stream = std::make_shared<AudioStreamSource>(filepath, loop, volume);
    if (stream->Initialize()) {
        active_streams.Add(stream);
        stream->SetBufferSize(default_buffer_size);
        return stream;
    }

    return nullptr;
}

std::shared_ptr<AudioStreamSource> AudioStreamingSystem::CreateStreamFromVFS(const String& vfsPath,
                                                                           std::shared_ptr<VFS> vfs,
                                                                           bool loop,
                                                                           double volume) {
    if (!initialized) return nullptr;

    auto stream = std::make_shared<AudioStreamSource>(vfsPath, vfs, loop, volume);
    if (stream->InitializeFromVFS()) {
        active_streams.Add(stream);
        stream->SetBufferSize(default_buffer_size);
        return stream;
    }

    return nullptr;
}

void AudioStreamingSystem::Update(double dt) {
    if (!initialized) return;

    // Clean up any streams that are no longer needed
    for (int i = active_streams.GetCount() - 1; i >= 0; i--) {
        if (!active_streams[i] ||
            (active_streams[i]->IsStopped() && !active_streams[i]->IsPlaying() && !active_streams[i]->IsPaused())) {
            active_streams.Remove(i);
        }
    }
}

void AudioStreamingSystem::ClearAllStreams() {
    for (auto& stream : active_streams) {
        if (stream) {
            stream->Stop();
        }
    }
    active_streams.Clear();
}

END_UPP_NAMESPACE