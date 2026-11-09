#ifndef UPP_AUDIO_STREAMING_H
#define UPP_AUDIO_STREAMING_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <GameLib/GameLib.h>
#include <api/Audio/Audio.h>
#include <GameEngine/AudioSystem.h>
#include <GameEngine/VFS.h>
#include <Thread/Thread.h>

NAMESPACE_UPP

// Forward declaration
class AudioStreamBuffer;

// Audio stream source for playing large audio files without loading them entirely into memory
// Supports multiple formats including WAV, OGG, MP3 (additional formats implemented as needed)
class AudioStreamSource {
public:
    AudioStreamSource(const String& filepath, bool loop = false, double volume = 1.0);
    // Constructor for VFS-based streaming
    AudioStreamSource(const String& vfsPath, std::shared_ptr<VFS> vfs, bool loop = false, double volume = 1.0);
    virtual ~AudioStreamSource();

    // Initialize the stream
    bool Initialize();
    // Initialize the stream from VFS
    bool InitializeFromVFS();

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

    // Get file info
    int GetSampleRate() const { return sample_rate; }
    int GetChannels() const { return channels; }
    int GetTotalSamples() const { return total_samples; }
    double GetDuration() const { return total_samples / (double)sample_rate; } // in seconds
    double GetPlaybackPosition() const; // in seconds
    void SetPlaybackPosition(double seconds); // in seconds

    // Streaming buffer management
    void SetBufferSize(int sample_count); // Set streaming buffer size
    int GetBufferSize() const { return buffer_size; }

    // Check if stream is valid
    bool IsValid() const { return valid; }

    // Get supported file extensions
    static Vector<String> GetSupportedFormats() {
        return { ".wav", ".ogg", ".mp3", ".flac" }; // Formats that can be implemented
    }

private:
    String filepath;
    std::shared_ptr<VFS> vfs; // VFS for file access (null if using system files)
    bool looping = false;
    double volume = 1.0;
    double pitch = 1.0;
    Point3 position = Point3(0, 0, 0);
    Vector3 velocity = Vector3(0, 0, 0);

    // File properties
    int sample_rate = 44100;
    int channels = 2;
    int total_samples = 0;
    int buffer_size = 8192;  // Default buffer size (samples)

    // Playback position (in samples)
    int64 playback_position = 0;

    // Streaming state
    bool valid = false;
    enum class State { STOPPED, PLAYING, PAUSED };
    State state = State::STOPPED;

    // Streaming thread and buffer
    Thread stream_thread;
    CriticalSection stream_cs;
    bool stream_thread_running = false;
    bool stream_eof = false;
    Vector<float> streaming_buffer;
    int buffer_read_pos = 0;
    int buffer_write_pos = 0;
    bool buffer_full = false;

    // Fill the streaming buffer
    void FillBuffer();

    // Streaming thread function
    void StreamThread();

    // Initialize audio file reader
    bool InitializeReader();
    
    // Initialize audio file reader for VFS
    bool InitializeVFSReader();
};

// Audio streaming system to manage multiple audio streams
class AudioStreamingSystem {
public:
    AudioStreamingSystem();
    virtual ~AudioStreamingSystem();

    // Initialize the streaming system
    bool Initialize(Engine& engine);

    // Create a stream source
    std::shared_ptr<AudioStreamSource> CreateStream(const String& filepath,
                                                   bool loop = false,
                                                   double volume = 1.0);

    // Create a stream source from VFS
    std::shared_ptr<AudioStreamSource> CreateStreamFromVFS(const String& vfsPath,
                                                          std::shared_ptr<VFS> vfs,
                                                          bool loop = false,
                                                          double volume = 1.0);

    // Update the streaming system (call this regularly)
    void Update(double dt);

    // Get currently active streams
    int GetActiveStreamCount() const { return active_streams.GetCount(); }
    std::shared_ptr<AudioStreamSource> GetStream(int index) const { return active_streams[index]; }

    // Stop and remove all streams
    void ClearAllStreams();

    // Get/set default buffer size for new streams
    void SetDefaultBufferSize(int size) { default_buffer_size = size; }
    int GetDefaultBufferSize() const { return default_buffer_size; }

private:
    // Engine and system state
    Engine* engine = nullptr;
    bool initialized = false;
    int default_buffer_size = 8192;

    // Active streams
    Vector<std::shared_ptr<AudioStreamSource>> active_streams;
};

END_UPP_NAMESPACE

#endif