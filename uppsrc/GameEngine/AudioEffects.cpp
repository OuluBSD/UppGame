#include "AudioEffects.h"

NAMESPACE_UPP

AudioEffect::AudioEffect() {
}

ReverbEffect::ReverbEffect() {
    effectType = AudioEffectType::REVERB;
    name = "Reverb";
    
    // Initialize delay lines with different sizes to create a spacious reverb
    int baseLength = 1000; // Base delay length at 44.1kHz
    for (int i = 0; i < 4; i++) {
        delayLengths[i] = baseLength * (i + 1) * 0.7; // Different delay lengths
        delayLines[i].SetCount(delayLengths[i]);
        readPositions[i] = 0;
        writePositions[i] = 0;
    }
}

Vector<float> ReverbEffect::Process(const Vector<float>& input) {
    if (!enabled) return input;
    
    Vector<float> output;
    output.SetCount(input.GetCount());
    
    for (int i = 0; i < input.GetCount(); i++) {
        float inputSample = input[i];
        float outputSample = 0.0f;
        
        // Process each delay line
        for (int dl = 0; dl < 4; dl++) {
            // Write input to delay line
            delayLines[dl][writePositions[dl]] = inputSample;
            
            // Read from delay line with feedback
            float delayedSample = delayLines[dl][readPositions[dl]];
            delayLines[dl][writePositions[dl]] += delayedSample * damping;
            
            outputSample += delayedSample;
            
            // Update positions with wraparound
            readPositions[dl] = (readPositions[dl] + 1) % delayLengths[dl];
            writePositions[dl] = (writePositions[dl] + 1) % delayLengths[dl];
        }
        
        // Mix wet and dry signals
        output[i] = inputSample * (1.0f - wetDryMix) + outputSample * wetDryMix;
    }
    
    return output;
}

void ReverbEffect::SetParameter(const String& paramName, double value) {
    if (paramName == "decay_time") SetDecayTime(value);
    else if (paramName == "damping") SetDamping(value);
    else if (paramName == "room_size") SetRoomSize(value);
    else if (paramName == "wet_dry_mix") SetWetDryMix(value);
}

double ReverbEffect::GetParameter(const String& paramName) const {
    if (paramName == "decay_time") return GetDecayTime();
    else if (paramName == "damping") return GetDamping();
    else if (paramName == "room_size") return GetRoomSize();
    else if (paramName == "wet_dry_mix") return GetWetDryMix();
    return 0.0;
}

EchoEffect::EchoEffect() {
    effectType = AudioEffectType::ECHO;
    name = "Echo";
    delayBuffer.SetCount(88200); // 2 seconds at 44.1kHz
}

Vector<float> EchoEffect::Process(const Vector<float>& input) {
    if (!enabled) return input;
    
    Vector<float> output;
    output.SetCount(input.GetCount());
    
    int delaySamples = (int)(delayTime * 44100); // Assuming 44.1kHz sample rate
    
    for (int i = 0; i < input.GetCount(); i++) {
        // Write current input to delay buffer
        delayBuffer[writePosition] = input[i];
        
        // Read delayed sample
        float delayedSample = delayBuffer[readPosition];
        
        // Mix original and delayed samples
        output[i] = input[i] * (1.0f - wetDryMix) + delayedSample * wetDryMix;
        
        // Update positions with wraparound
        readPosition = (readPosition + 1) % delayBuffer.GetCount();
        writePosition = (writePosition + 1) % delayBuffer.GetCount();
    }
    
    return output;
}

void EchoEffect::SetParameter(const String& paramName, double value) {
    if (paramName == "delay_time") SetDelayTime(value);
    else if (paramName == "feedback") SetFeedback(value);
    else if (paramName == "wet_dry_mix") SetWetDryMix(value);
}

double EchoEffect::GetParameter(const String& paramName) const {
    if (paramName == "delay_time") return GetDelayTime();
    else if (paramName == "feedback") return GetFeedback();
    else if (paramName == "wet_dry_mix") return GetWetDryMix();
    return 0.0;
}

LowPassFilter::LowPassFilter() {
    effectType = AudioEffectType::LOW_PASS_FILTER;
    name = "Low Pass Filter";
    UpdateCoefficients();
}

Vector<float> LowPassFilter::Process(const Vector<float>& input) {
    if (!enabled) return input;
    
    Vector<float> output;
    output.SetCount(input.GetCount());
    
    for (int i = 0; i < input.GetCount(); i++) {
        double xn = input[i];
        
        // Direct Form II implementation of biquad filter
        double yn = a0 * xn + a1 * xn1 + a2 * xn2 - b1 * yn1 - b2 * yn2;
        
        // Update delay line
        xn2 = xn1;
        xn1 = xn;
        yn2 = yn1;
        yn1 = yn;
        
        output[i] = (float)yn;
    }
    
    return output;
}

void LowPassFilter::UpdateCoefficients() {
    // Calculate biquad filter coefficients for low-pass filter
    double w0 = 2 * M_PI * cutoffFreq / sampleRate;
    double cos_w0 = cos(w0);
    double sin_w0 = sin(w0);
    double alpha = sin_w0 / (2 * resonance);
    
    double b0 = (1 - cos_w0) / 2;
    double b1 = 1 - cos_w0;
    double b2 = b0;  // (1 - cos_w0) / 2
    double a0 = 1 + alpha;
    double a1 = -2 * cos_w0;
    double a2 = 1 - alpha;
    
    // Normalize by a0
    this->b1 = b1 / a0;
    this->b2 = b2 / a0;
    this->a0 = b0 / a0;
    this->a1 = a1 / a0;
    this->a2 = a2 / a0;
}

void LowPassFilter::SetParameter(const String& paramName, double value) {
    if (paramName == "cutoff_frequency") SetCutoffFrequency(value);
    else if (paramName == "resonance") SetResonance(value);
    
    UpdateCoefficients();
}

double LowPassFilter::GetParameter(const String& paramName) const {
    if (paramName == "cutoff_frequency") return GetCutoffFrequency();
    else if (paramName == "resonance") return GetResonance();
    return 0.0;
}

AudioMixer::AudioMixer() : sampleRate(44100), channels(2) {
}

AudioMixer::~AudioMixer() {
    ClearSources();
    ClearEffects();
}

bool AudioMixer::Initialize(int sampleRate, int channels) {
    this->sampleRate = sampleRate;
    this->channels = channels;
    return true;
}

int AudioMixer::AddSource(std::shared_ptr<SoundSource> source) {
    sources.Add(std::make_pair(source, 1.0)); // Default volume of 1.0
    return sources.GetCount() - 1;
}

void AudioMixer::RemoveSource(int index) {
    if (index >= 0 && index < sources.GetCount()) {
        sources.Remove(index);
    }
}

void AudioMixer::ClearSources() {
    sources.Clear();
}

void AudioMixer::AddEffect(std::shared_ptr<AudioEffect> effect) {
    effects.Add(effect);
}

void AudioMixer::RemoveEffect(int index) {
    if (index >= 0 && index < effects.GetCount()) {
        effects.Remove(index);
    }
}

void AudioMixer::ClearEffects() {
    effects.Clear();
}

Vector<float> AudioMixer::Mix(int numSamples) {
    // First, mix all sources together
    Vector<float> mixed = MixSources(numSamples);
    
    // Then apply effects if enabled
    if (applyEffectsToMix) {
        mixed = ApplyEffects(mixed);
    }
    
    // Apply master volume
    for (auto& sample : mixed) {
        sample *= masterVolume;
    }
    
    return mixed;
}

Vector<float> AudioMixer::MixSources(int numSamples) {
    Vector<float> output;
    output.SetCount(numSamples * channels);
    
    // Mix all active sources
    for (const auto& sourcePair : sources) {
        auto source = sourcePair.first;
        double volume = sourcePair.second;
        
        if (source && source->IsPlaying()) {
            // For this implementation, we'll synthesize basic sounds
            // In a real implementation, we would get samples from the actual audio buffer
            
            // For demonstration, create a simple sine wave for each source
            for (int i = 0; i < numSamples; i++) {
                double t = i / (double)sampleRate;
                float sample = (float)(sin(2 * M_PI * 440.0 * t) * volume); // 440Hz sine wave
                
                if (i < output.GetCount()) {
                    output[i] += sample;
                }
            }
        }
    }
    
    return output;
}

Vector<float> AudioMixer::ApplyEffects(const Vector<float>& input) {
    Vector<float> processed = input;
    
    // Apply each effect in sequence
    for (auto& effect : effects) {
        if (effect && effect->IsEnabled()) {
            processed = effect->Process(processed);
        }
    }
    
    return processed;
}

void AudioMixer::SetSourceVolume(int index, double volume) {
    if (index >= 0 && index < sources.GetCount()) {
        sources[index].second = clamp(volume, 0.0, 1.0);
    }
}

double AudioMixer::GetSourceVolume(int index) const {
    if (index >= 0 && index < sources.GetCount()) {
        return sources[index].second;
    }
    return 0.0;
}

SpatialAudioSystem::SpatialAudioSystem() {
}

SpatialAudioSystem::~SpatialAudioSystem() {
    ClearSources();
}

bool SpatialAudioSystem::Initialize(int sampleRate) {
    return mixer.Initialize(sampleRate, 2); // Stereo output
}

void SpatialAudioSystem::SetListenerPosition(const Point3& pos) {
    listenerPosition = pos;
}

void SpatialAudioSystem::SetListenerOrientation(const Vector3& forward, const Vector3& up) {
    listenerForward = forward.Normalize();
    listenerUp = up.Normalize();
}

int SpatialAudioSystem::AddSource(std::shared_ptr<SoundSource> source) {
    AudioSourceInfo info;
    info.source = source;
    sources.Add(info);
    return sources.GetCount() - 1;
}

void SpatialAudioSystem::RemoveSource(int index) {
    if (index >= 0 && index < sources.GetCount()) {
        sources.Remove(index);
    }
}

void SpatialAudioSystem::ClearSources() {
    sources.Clear();
}

void SpatialAudioSystem::SetSourcePosition(int index, const Point3& pos) {
    if (index >= 0 && index < sources.GetCount()) {
        sources[index].position = pos;
    }
}

Point3 SpatialAudioSystem::GetSourcePosition(int index) const {
    if (index >= 0 && index < sources.GetCount()) {
        return sources[index].position;
    }
    return Point3(0, 0, 0);
}

Vector<float> SpatialAudioSystem::Process(int numSamples) {
    // Update each source with spatial properties
    for (auto& sourceInfo : sources) {
        if (sourceInfo.source) {
            // Calculate distance attenuation
            double distance = (sourceInfo.position - listenerPosition).Length();
            sourceInfo.distanceAttenuation = CalculateAttenuation(sourceInfo.position, listenerPosition);
            
            // For this implementation, we'll just set the volume based on distance
            sourceInfo.source->SetVolume(sourceInfo.distanceAttenuation);
            
            // Doppler effect calculation would go here
            // (simplified for this example)
        }
    }
    
    // Mix all sources
    return mixer.Mix(numSamples);
}

void SpatialAudioSystem::SetAttenuationModel(double refDistance, double maxDistance, 
                                            double rolloffFactor) {
    this->refDistance = refDistance;
    this->maxDistance = maxDistance;
    this->rolloffFactor = rolloffFactor;
}

double SpatialAudioSystem::CalculateAttenuation(const Point3& sourcePos, const Point3& listenerPos) const {
    double distance = (sourcePos - listenerPos).Length();
    
    // Clamp distance to valid range
    distance = clamp(distance, refDistance, maxDistance);
    
    // Calculate attenuation using inverse square law (modified with rolloff factor)
    if (distance <= refDistance) {
        return 1.0; // No attenuation at reference distance or closer
    }
    
    // Inverse distance law with rolloff factor
    double attenuation = refDistance / (refDistance + rolloffFactor * (distance - refDistance));
    
    // Clamp between 0 and 1
    return clamp(attenuation, 0.0, 1.0);
}

END_UPP_NAMESPACE