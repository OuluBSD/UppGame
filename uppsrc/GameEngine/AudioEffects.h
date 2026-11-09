#ifndef UPP_AUDIO_EFFECTS_H
#define UPP_AUDIO_EFFECTS_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <GameLib/GameLib.h>
#include <GameEngine/GameEngine.h>
#include <GameEngine/AudioSystem.h>

NAMESPACE_UPP

// Audio effect types
enum class AudioEffectType {
    REVERB,
    ECHO,
    LOW_PASS_FILTER,
    HIGH_PASS_FILTER,
    BAND_PASS_FILTER,
    CHORUS,
    FLANGER,
    PHASER,
    COMPRESSOR,
    LIMITER,
    DISTORTION,
    PITCH_SHIFT,
    TIME_STRETCH
};

// Base class for audio effects
class AudioEffect {
public:
    AudioEffect();
    virtual ~AudioEffect() = default;

    // Process audio samples
    virtual Vector<float> Process(const Vector<float>& input) = 0;

    // Enable/disable the effect
    void SetEnabled(bool enabled) { this->enabled = enabled; }
    bool IsEnabled() const { return enabled; }

    // Get/set effect type
    void SetEffectType(AudioEffectType type) { this->effectType = type; }
    AudioEffectType GetEffectType() const { return effectType; }

    // Get/set effect name
    void SetName(const String& name) { this->name = name; }
    const String& GetName() const { return name; }

    // Set effect parameters (generic)
    virtual void SetParameter(const String& paramName, double value) = 0;
    virtual double GetParameter(const String& paramName) const = 0;

protected:
    bool enabled = true;
    AudioEffectType effectType;
    String name;
};

// Reverb effect
class ReverbEffect : public AudioEffect {
public:
    ReverbEffect();
    virtual ~ReverbEffect() = default;

    Vector<float> Process(const Vector<float>& input) override;

    void SetParameter(const String& paramName, double value) override;
    double GetParameter(const String& paramName) const override;

    // Reverb-specific parameters
    void SetDecayTime(double time) { decayTime = clamp(time, 0.1, 10.0); }
    double GetDecayTime() const { return decayTime; }

    void SetDamping(double damping) { this->damping = clamp(damping, 0.0, 1.0); }
    double GetDamping() const { return damping; }

    void SetRoomSize(double size) { roomSize = clamp(size, 0.0, 1.0); }
    double GetRoomSize() const { return roomSize; }

    void SetWetDryMix(double mix) { wetDryMix = clamp(mix, 0.0, 1.0); }
    double GetWetDryMix() const { return wetDryMix; }

private:
    double decayTime = 1.0;      // How long reverb lasts (seconds)
    double damping = 0.5;        // High frequency damping
    double roomSize = 0.5;       // Room size (affects delay times)
    double wetDryMix = 0.5;      // Mix between dry (original) and wet (processed) signals

    // Delay lines for reverb
    Vector<float> delayLines[4];
    int delayLengths[4];
    int readPositions[4];
    int writePositions[4];
};

// Echo/Delay effect
class EchoEffect : public AudioEffect {
public:
    EchoEffect();
    virtual ~EchoEffect() = default;

    Vector<float> Process(const Vector<float>& input) override;

    void SetParameter(const String& paramName, double value) override;
    double GetParameter(const String& paramName) const override;

    // Echo-specific parameters
    void SetDelayTime(double time) { delayTime = clamp(time, 0.01, 2.0); }
    double GetDelayTime() const { return delayTime; }

    void SetFeedback(double feedback) { this->feedback = clamp(feedback, 0.0, 0.99); }
    double GetFeedback() const { return feedback; }

    void SetWetDryMix(double mix) { wetDryMix = clamp(mix, 0.0, 1.0); }
    double GetWetDryMix() const { return wetDryMix; }

private:
    double delayTime = 0.5;      // Echo delay in seconds
    double feedback = 0.3;       // Amount of echo fed back
    double wetDryMix = 0.5;      // Mix between dry and wet signals

    Vector<float> delayBuffer;
    int readPosition = 0;
    int writePosition = 0;
};

// Low-pass filter
class LowPassFilter : public AudioEffect {
public:
    LowPassFilter();
    virtual ~LowPassFilter() = default;

    Vector<float> Process(const Vector<float>& input) override;

    void SetParameter(const String& paramName, double value) override;
    double GetParameter(const String& paramName) const override;

    // Filter parameters
    void SetCutoffFrequency(double freq) { cutoffFreq = clamp(freq, 20.0, 20000.0); }
    double GetCutoffFrequency() const { return cutoffFreq; }

    void SetResonance(double resonance) { this->resonance = clamp(resonance, 0.1, 20.0); }
    double GetResonance() const { return resonance; }

private:
    double cutoffFreq = 1000.0;  // Cutoff frequency in Hz
    double resonance = 0.707;    // Resonance/Q factor
    double sampleRate = 44100.0; // Sample rate

    // Filter state for each channel
    double xn1 = 0, xn2 = 0;     // Input delay line
    double yn1 = 0, yn2 = 0;     // Output delay line
    double a0, a1, a2, b1, b2;   // Filter coefficients
    void UpdateCoefficients();
};

// Audio mixer for combining multiple audio sources
class AudioMixer {
public:
    AudioMixer();
    virtual ~AudioMixer();

    // Initialize with sample rate and channel count
    bool Initialize(int sampleRate = 44100, int channels = 2);

    // Add/remove audio sources to mix
    int AddSource(std::shared_ptr<SoundSource> source);
    void RemoveSource(int index);
    void ClearSources();

    // Add/remove effects to the mixer
    void AddEffect(std::shared_ptr<AudioEffect> effect);
    void RemoveEffect(int index);
    void ClearEffects();

    // Mix all sources together
    Vector<float> Mix(int numSamples);

    // Set master volume
    void SetMasterVolume(double volume) { masterVolume = clamp(volume, 0.0, 1.0); }
    double GetMasterVolume() const { return masterVolume; }

    // Set volume for a specific source
    void SetSourceVolume(int index, double volume);
    double GetSourceVolume(int index) const;

    // Get the number of active sources
    int GetSourceCount() const { return sources.GetCount(); }

    // Apply effects to the mixed output
    void SetApplyEffectsToMix(bool apply) { applyEffectsToMix = apply; }
    bool IsApplyingEffectsToMix() const { return applyEffectsToMix; }

    // Get/set sample rate
    int GetSampleRate() const { return sampleRate; }

private:
    Vector<std::pair<std::shared_ptr<SoundSource>, double>> sources; // Source + volume
    Vector<std::shared_ptr<AudioEffect>> effects;
    int sampleRate;
    int channels;
    double masterVolume = 1.0;
    bool applyEffectsToMix = true;

    // Mix sources together
    Vector<float> MixSources(int numSamples);
    
    // Apply effects to a buffer
    Vector<float> ApplyEffects(const Vector<float>& input);
};

// Spatial audio system for 3D positioning and effects
class SpatialAudioSystem {
public:
    SpatialAudioSystem();
    virtual ~SpatialAudioSystem();

    // Initialize the system
    bool Initialize(int sampleRate = 44100);

    // Set the listener position and orientation
    void SetListenerPosition(const Point3& pos);
    void SetListenerOrientation(const Vector3& forward, const Vector3& up);
    Point3 GetListenerPosition() const { return listenerPosition; }

    // Add an audio source to the system
    int AddSource(std::shared_ptr<SoundSource> source);
    void RemoveSource(int index);
    void ClearSources();

    // Update a source's position
    void SetSourcePosition(int index, const Point3& pos);
    Point3 GetSourcePosition(int index) const;

    // Apply spatial effects to all sources
    Vector<float> Process(int numSamples);

    // Get mixing results
    AudioMixer& GetMixer() { return mixer; }

    // Attenuation settings
    void SetAttenuationModel(double refDistance = 1.0, double maxDistance = 100.0, 
                            double rolloffFactor = 1.0);
    double GetReferenceDistance() const { return refDistance; }
    double GetMaxDistance() const { return maxDistance; }
    double GetRolloffFactor() const { return rolloffFactor; }

private:
    Point3 listenerPosition = Point3(0, 0, 0);
    Vector3 listenerForward = Vector3(0, 0, 1);
    Vector3 listenerUp = Vector3(0, 1, 0);
    
    struct AudioSourceInfo {
        std::shared_ptr<SoundSource> source;
        Point3 position = Point3(0, 0, 0);
        double distanceAttenuation = 1.0;
        double dopplerShift = 1.0;
    };
    
    Vector<AudioSourceInfo> sources;
    AudioMixer mixer;
    
    // Attenuation parameters
    double refDistance = 1.0;
    double maxDistance = 100.0;
    double rolloffFactor = 1.0;
    
    // Calculate attenuation based on distance
    double CalculateAttenuation(const Point3& sourcePos, const Point3& listenerPos) const;
};

END_UPP_NAMESPACE

#endif