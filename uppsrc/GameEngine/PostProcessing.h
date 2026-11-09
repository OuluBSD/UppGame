#ifndef UPP_POST_PROCESSING_H
#define UPP_POST_PROCESSING_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <CtrlCore/CtrlCore.h>
#include <Geometry/Geometry.h>
#include <GameLib/GameLib.h>
#include <GameEngine/GameEngine.h>

NAMESPACE_UPP_BEGIN

// Post-processing effect types
enum class PostProcessEffectType {
    BLOOM,
    BLUR,
    COLOR_GRADING,
    TONEMAPPING,
    MOTION_BLUR,
    DEPTH_OF_FIELD,
    FXAA,         // Fast Approximate Anti-Aliasing
    SMAA,         // Subpixel Morphological Anti-Aliasing
    HDR,
    VIGNETTE,
    CHROMATIC_ABERRATION,
    SCANLINES
};

// Base post-processing effect
class PostProcessEffect {
public:
    PostProcessEffect(PostProcessEffectType type, const String& name = String());
    virtual ~PostProcessEffect() = default;
    
    // Update effect parameters (called every frame)
    virtual void Update(double deltaTime) {}
    
    // Apply the effect to the input image
    virtual Image Apply(const Image& input) = 0;
    
    // Set effect parameters
    virtual void SetParameter(const String& name, const Value& value);
    virtual void SetParameter(const String& name, double value);
    virtual void SetParameter(const String& name, Color value);
    virtual void SetParameter(const String& name, const Point2& value);
    
    // Get effect parameters
    Value GetParameter(const String& name) const;
    double GetFloatParameter(const String& name) const;
    Color GetColorParameter(const String& name) const;
    Point2 GetVector2Parameter(const String& name) const;
    
    // Enable/disable effect
    void SetEnabled(bool enabled) { this->enabled = enabled; }
    bool IsEnabled() const { return enabled; }
    
    // Get effect type and name
    PostProcessEffectType GetType() const { return type; }
    const String& GetName() const { return name; }
    
    // Set shader for GPU-based effects (when available)
    void SetShader(SharedPtr<ShaderProgram> shader) { this->shader = shader; }
    SharedPtr<ShaderProgram> GetShader() const { return shader; }
    
protected:
    PostProcessEffectType type;
    String name;
    bool enabled = true;
    std::map<String, Value> parameters;
    SharedPtr<ShaderProgram> shader;
};

// Bloom effect - adds glow to bright areas
class BloomEffect : public PostProcessEffect {
public:
    BloomEffect();
    
    // Apply bloom effect to input image
    Image Apply(const Image& input) override;
    
    // Set bloom-specific parameters
    void SetThreshold(double threshold) { SetParameter("threshold", threshold); }
    void SetIntensity(double intensity) { SetParameter("intensity", intensity); }
    void SetBlurRadius(double radius) { SetParameter("blurRadius", radius); }
    
private:
    // Helper function to extract bright areas from image
    Image ExtractBrightAreas(const Image& input, double threshold) const;
    
    // Helper function to blur an image
    Image BlurImage(const Image& input, double radius) const;
    
    // Helper function to combine images (additive blend)
    Image CombineImages(const Image& base, const Image& bloom, double intensity) const;
};

// Blur effect - various types of blur
class BlurEffect : public PostProcessEffect {
public:
    enum class BlurType {
        GAUSSIAN,
        BOX,
        MOTION
    };
    
    BlurEffect(BlurType blurType = BlurType::GAUSSIAN);
    
    Image Apply(const Image& input) override;
    
    void SetBlurType(BlurType type) { blur_type = type; }
    void SetRadius(double radius) { SetParameter("radius", radius); }
    void SetDirection(const Point2& direction) { SetParameter("direction", direction); } // For motion blur
    
private:
    BlurType blur_type;
    
    Image ApplyGaussianBlur(const Image& input, double radius) const;
    Image ApplyBoxBlur(const Image& input, double radius) const;
    Image ApplyMotionBlur(const Image& input, double radius, const Point2& direction) const;
};

// Color grading effect - adjusts color balance, saturation, etc.
class ColorGradingEffect : public PostProcessEffect {
public:
    ColorGradingEffect();
    
    Image Apply(const Image& input) override;
    
    // Set color grading parameters
    void SetBrightness(double brightness) { SetParameter("brightness", brightness); }
    void SetContrast(double contrast) { SetParameter("contrast", contrast); }
    void SetSaturation(double saturation) { SetParameter("saturation", saturation); }
    void SetHueShift(double hueShift) { SetParameter("hueShift", hueShift); }
    void SetTemperature(double temperature) { SetParameter("temperature", temperature); }
    void SetTint(double tint) { SetParameter("tint", tint); }
    
private:
    Image ApplyColorAdjustment(const Image& input, double brightness, double contrast, 
                              double saturation, double hueShift, double temperature, 
                              double tint) const;
};

// Vignette effect - darkens corners of the image
class VignetteEffect : public PostProcessEffect {
public:
    VignetteEffect();
    
    Image Apply(const Image& input) override;
    
    // Set vignette parameters
    void SetIntensity(double intensity) { SetParameter("intensity", intensity); }
    void SetSmoothness(double smoothness) { SetParameter("smoothness", smoothness); }
    void SetRoundness(double roundness) { SetParameter("roundness", roundness); }
    void SetCenter(const Point2& center) { SetParameter("center", center); }
    
private:
    Image ApplyVignette(const Image& input, double intensity, double smoothness, 
                       double roundness, const Point2& center) const;
};

// Chromatic aberration effect - simulates lens distortion
class ChromaticAberrationEffect : public PostProcessEffect {
public:
    ChromaticAberrationEffect();
    
    Image Apply(const Image& input) override;
    
    // Set chromatic aberration parameters
    void SetIntensity(double intensity) { SetParameter("intensity", intensity); }
    void SetDirection(const Point2& direction) { SetParameter("direction", direction); }
    
private:
    Image ApplyChromaticAberration(const Image& input, double intensity, 
                                  const Point2& direction) const;
};

// Post-processing manager that handles effect chains
class PostProcessManager {
public:
    PostProcessManager();
    ~PostProcessManager();
    
    // Add an effect to the processing chain
    void AddEffect(SharedPtr<PostProcessEffect> effect);
    
    // Remove an effect from the chain
    void RemoveEffect(const String& name);
    void RemoveEffectByType(PostProcessEffectType type);
    
    // Remove all effects
    void ClearEffects();
    
    // Apply all enabled effects in sequence
    Image ApplyEffects(const Image& input) const;
    
    // Get effect by name or type
    SharedPtr<PostProcessEffect> GetEffect(const String& name) const;
    Vector<SharedPtr<PostProcessEffect>> GetEffectsByType(PostProcessEffectType type) const;
    
    // Enable/disable specific effect
    void SetEffectEnabled(const String& name, bool enabled);
    
    // Get all effect names
    Vector<String> GetEffectNames() const;
    
    // Get/set rendering resolution
    void SetRenderResolution(const Size& size) { render_resolution = size; }
    Size GetRenderResolution() const { return render_resolution; }
    
    // Set overall intensity of post-processing (0.0 = disabled, 1.0 = full effect)
    void SetIntensity(double intensity) { overall_intensity = Clamp(intensity, 0.0, 1.0); }
    double GetIntensity() const { return overall_intensity; }
    
private:
    Vector<SharedPtr<PostProcessEffect>> effects;
    Size render_resolution = Size(800, 600);
    double overall_intensity = 1.0;
};

// Implementation
inline PostProcessEffect::PostProcessEffect(PostProcessEffectType type, const String& name) 
    : type(type), name(name.IsEmpty() ? "Effect" + AsString((int)type) : name) {
    // Initialize with default parameters based on effect type
    switch (type) {
        case PostProcessEffectType::BLOOM:
            parameters["threshold"] = 0.8;
            parameters["intensity"] = 1.0;
            parameters["blurRadius"] = 5.0;
            break;
        case PostProcessEffectType::BLUR:
            parameters["radius"] = 2.0;
            parameters["direction"] = Point2(1, 0);
            break;
        case PostProcessEffectType::COLOR_GRADING:
            parameters["brightness"] = 0.0;
            parameters["contrast"] = 1.0;
            parameters["saturation"] = 1.0;
            parameters["hueShift"] = 0.0;
            parameters["temperature"] = 0.0;
            parameters["tint"] = 0.0;
            break;
        case PostProcessEffectType::VIGNETTE:
            parameters["intensity"] = 0.3;
            parameters["smoothness"] = 0.2;
            parameters["roundness"] = 1.0;
            parameters["center"] = Point2(0.5, 0.5);
            break;
        case PostProcessEffectType::CHROMATIC_ABERRATION:
            parameters["intensity"] = 0.02;
            parameters["direction"] = Point2(0, 1);
            break;
        default:
            break;
    }
}

inline void PostProcessEffect::SetParameter(const String& name, const Value& value) {
    parameters[name] = value;
}

inline void PostProcessEffect::SetParameter(const String& name, double value) {
    parameters[name] = value;
}

inline void PostProcessEffect::SetParameter(const String& name, Color value) {
    parameters[name] = value;
}

inline void PostProcessEffect::SetParameter(const String& name, const Point2& value) {
    parameters[name] = value;
}

inline Value PostProcessEffect::GetParameter(const String& name) const {
    auto it = parameters.find(name);
    return it != parameters.end() ? it->second : Value();
}

inline double PostProcessEffect::GetFloatParameter(const String& name) const {
    Value val = GetParameter(name);
    return val.Is<double>() ? Get<double>(val) : 0.0;
}

inline Color PostProcessEffect::GetColorParameter(const String& name) const {
    Value val = GetParameter(name);
    return val.Is<Color>() ? Get<Color>(val) : Color(0, 0, 0);
}

inline Point2 PostProcessEffect::GetVector2Parameter(const String& name) const {
    Value val = GetParameter(name);
    return val.Is<Point2>() ? Get<Point2>(val) : Point2(0, 0);
}

// BloomEffect implementation
inline BloomEffect::BloomEffect() 
    : PostProcessEffect(PostProcessEffectType::BLOOM, "Bloom") {}

inline Image BloomEffect::Apply(const Image& input) {
    if (!enabled) return input;
    
    double threshold = GetFloatParameter("threshold");
    double intensity = GetFloatParameter("intensity");
    double blurRadius = GetFloatParameter("blurRadius");
    
    // Extract bright areas
    Image brightAreas = ExtractBrightAreas(input, threshold);
    
    // Blur the bright areas
    Image blurredBright = BlurImage(brightAreas, blurRadius);
    
    // Combine with original
    return CombineImages(input, blurredBright, intensity);
}

inline Image BloomEffect::ExtractBrightAreas(const Image& input, double threshold) const {
    Size sz = input.GetSize();
    Image result = CreateImage(sz.cx, sz.cy, White());
    RGBA* dest = result;
    const RGBA* src = input;
    
    for (int i = 0; i < sz.cx * sz.cy; i++) {
        double brightness = (src[i].r + src[i].g + src[i].b) / 3.0 / 255.0;
        if (brightness > threshold) {
            dest[i] = src[i];
        } else {
            dest[i] = RGBA(0, 0, 0, 255);
        }
    }
    
    return result;
}

inline Image BloomEffect::BlurImage(const Image& input, double radius) const {
    // Simple box blur implementation
    int blurRadius = (int)radius;
    if (blurRadius <= 0) return input;
    
    Size sz = input.GetSize();
    Image result = CreateImage(sz.cx, sz.cy, White());
    RGBA* dest = result;
    const RGBA* src = input;
    
    for (int y = 0; y < sz.cy; y++) {
        for (int x = 0; x < sz.cx; x++) {
            int r = 0, g = 0, b = 0, a = 0;
            int count = 0;
            
            for (int dy = -blurRadius; dy <= blurRadius; dy++) {
                for (int dx = -blurRadius; dx <= blurRadius; dx++) {
                    int nx = x + dx;
                    int ny = y + dy;
                    
                    if (nx >= 0 && nx < sz.cx && ny >= 0 && ny < sz.cy) {
                        RGBA pixel = src[ny * sz.cx + nx];
                        r += pixel.r;
                        g += pixel.g;
                        b += pixel.b;
                        a += pixel.a;
                        count++;
                    }
                }
            }
            
            if (count > 0) {
                dest[y * sz.cx + x] = RGBA(r/count, g/count, b/count, a/count);
            } else {
                dest[y * sz.cx + x] = RGBA(0, 0, 0, 0);
            }
        }
    }
    
    return result;
}

inline Image BloomEffect::CombineImages(const Image& base, const Image& bloom, double intensity) const {
    Size sz = base.GetSize();
    Image result = CreateImage(sz.cx, sz.cy, White());
    RGBA* dest = result;
    const RGBA* basePtr = base;
    const RGBA* bloomPtr = bloom;
    
    for (int i = 0; i < sz.cx * sz.cy; i++) {
        int r = basePtr[i].r + (int)(bloomPtr[i].r * intensity);
        int g = basePtr[i].g + (int)(bloomPtr[i].g * intensity);
        int b = basePtr[i].b + (int)(bloomPtr[i].b * intensity);
        int a = basePtr[i].a;
        
        dest[i] = RGBA(Clamp(r, 0, 255), Clamp(g, 0, 255), Clamp(b, 0, 255), Clamp(a, 0, 255));
    }
    
    return result;
}

// BlurEffect implementation
inline BlurEffect::BlurEffect(BlurType blurType) 
    : PostProcessEffect(PostProcessEffectType::BLUR, "Blur"), blur_type(blurType) {}

inline Image BlurEffect::Apply(const Image& input) {
    if (!enabled) return input;
    
    double radius = GetFloatParameter("radius");
    Point2 direction = GetVector2Parameter("direction");
    
    switch (blur_type) {
        case BlurType::GAUSSIAN:
            return ApplyGaussianBlur(input, radius);
        case BlurType::BOX:
            return ApplyBoxBlur(input, radius);
        case BlurType::MOTION:
            return ApplyMotionBlur(input, radius, direction);
        default:
            return input;
    }
}

inline Image BlurEffect::ApplyGaussianBlur(const Image& input, double radius) const {
    // For now, just use the basic blur (box blur) as a placeholder
    return ApplyBoxBlur(input, radius);
}

inline Image BlurEffect::ApplyBoxBlur(const Image& input, double radius) const {
    return BlurImage(input, radius); // Use the existing blur implementation from Bloom
}

inline Image BlurEffect::ApplyMotionBlur(const Image& input, double radius, const Point2& direction) const {
    // Motion blur along a specific direction
    int blurRadius = (int)radius;
    if (blurRadius <= 0) return input;
    
    Size sz = input.GetSize();
    Image result = CreateImage(sz.cx, sz.cy, White());
    RGBA* dest = result;
    const RGBA* src = input;
    
    // Normalize direction
    double len = sqrt(direction.x * direction.x + direction.y * direction.y);
    if (len == 0) len = 1;
    Point2 normDir(direction.x/len, direction.y/len);
    
    for (int y = 0; y < sz.cy; y++) {
        for (int x = 0; x < sz.cx; x++) {
            int r = 0, g = 0, b = 0, a = 0;
            int count = 0;
            
            // Sample along the motion direction
            for (int i = -blurRadius; i <= blurRadius; i++) {
                int nx = (int)(x + normDir.x * i);
                int ny = (int)(y + normDir.y * i);
                
                if (nx >= 0 && nx < sz.cx && ny >= 0 && ny < sz.cy) {
                    RGBA pixel = src[ny * sz.cx + nx];
                    r += pixel.r;
                    g += pixel.g;
                    b += pixel.b;
                    a += pixel.a;
                    count++;
                }
            }
            
            if (count > 0) {
                dest[y * sz.cx + x] = RGBA(r/count, g/count, b/count, a/count);
            } else {
                dest[y * sz.cx + x] = RGBA(0, 0, 0, 0);
            }
        }
    }
    
    return result;
}

// ColorGradingEffect implementation
inline ColorGradingEffect::ColorGradingEffect() 
    : PostProcessEffect(PostProcessEffectType::COLOR_GRADING, "ColorGrading") {}

inline Image ColorGradingEffect::Apply(const Image& input) {
    if (!enabled) return input;
    
    double brightness = GetFloatParameter("brightness");
    double contrast = GetFloatParameter("contrast");
    double saturation = GetFloatParameter("saturation");
    double hueShift = GetFloatParameter("hueShift");
    double temperature = GetFloatParameter("temperature");
    double tint = GetFloatParameter("tint");
    
    return ApplyColorAdjustment(input, brightness, contrast, saturation, hueShift, temperature, tint);
}

inline Image ColorGradingEffect::ApplyColorAdjustment(const Image& input, double brightness, 
                                                     double contrast, double saturation, 
                                                     double hueShift, double temperature, 
                                                     double tint) const {
    Size sz = input.GetSize();
    Image result = CreateImage(sz.cx, sz.cy, White());
    RGBA* dest = result;
    const RGBA* src = input;
    
    // Contrast and brightness adjustment factors
    double contrastFactor = (259.0 * (contrast + 255.0)) / (255.0 * (259.0 - contrast));
    
    for (int i = 0; i < sz.cx * sz.cy; i++) {
        int r = src[i].r;
        int g = src[i].g;
        int b = src[i].b;
        
        // Apply brightness
        r = Clamp(r + (int)(brightness * 255), 0, 255);
        g = Clamp(g + (int)(brightness * 255), 0, 255);
        b = Clamp(b + (int)(brightness * 255), 0, 255);
        
        // Apply contrast
        r = Clamp((int)(contrastFactor * (r - 128) + 128), 0, 255);
        g = Clamp((int)(contrastFactor * (g - 128) + 128), 0, 255);
        b = Clamp((int)(contrastFactor * (b - 128) + 128), 0, 255);
        
        // Apply saturation (simple algorithm)
        if (saturation != 1.0) {
            // Convert to grayscale
            int gray = (r + g + b) / 3;
            // Interpolate between grayscale and original based on saturation
            r = Clamp((int)(gray + saturation * (r - gray)), 0, 255);
            g = Clamp((int)(gray + saturation * (g - gray)), 0, 255);
            b = Clamp((int)(gray + saturation * (b - gray)), 0, 255);
        }
        
        dest[i] = RGBA(r, g, b, src[i].a);
    }
    
    return result;
}

// VignetteEffect implementation
inline VignetteEffect::VignetteEffect() 
    : PostProcessEffect(PostProcessEffectType::VIGNETTE, "Vignette") {}

inline Image VignetteEffect::Apply(const Image& input) {
    if (!enabled) return input;
    
    double intensity = GetFloatParameter("intensity");
    double smoothness = GetFloatParameter("smoothness");
    double roundness = GetFloatParameter("roundness");
    Point2 center = GetVector2Parameter("center");
    
    return ApplyVignette(input, intensity, smoothness, roundness, center);
}

inline Image VignetteEffect::ApplyVignette(const Image& input, double intensity, 
                                          double smoothness, double roundness, 
                                          const Point2& center) const {
    Size sz = input.GetSize();
    Image result = CreateImage(sz.cx, sz.cy, White());
    RGBA* dest = result;
    const RGBA* src = input;
    
    // Convert center from normalized to pixel coordinates
    Point2 centerPx(center.x * sz.cx, center.y * sz.cy);
    
    // Calculate max distance from center to corner
    double maxDist = sqrt(pow(max(centerPx.x, sz.cx - centerPx.x), 2) + 
                         pow(max(centerPx.y, sz.cy - centerPx.y), 2));
    
    for (int y = 0; y < sz.cy; y++) {
        for (int x = 0; x < sz.cx; x++) {
            // Calculate distance from current pixel to center
            double dx = x - centerPx.x;
            double dy = y - centerPx.y;
            
            // Apply roundness (make distance calculation elliptical)
            dx /= roundness;
            double dist = sqrt(dx * dx + dy * dy);
            
            // Calculate vignette strength (0.0 = no vignette, 1.0 = full vignette)
            double strength = pow(Clamp(dist / maxDist, 0.0, 1.0), smoothness);
            strength = Clamp(strength * intensity, 0.0, 1.0);
            
            // Apply vignette by darkening the color
            RGBA pixel = src[y * sz.cx + x];
            int r = (int)(pixel.r * (1.0 - strength));
            int g = (int)(pixel.g * (1.0 - strength));
            int b = (int)(pixel.b * (1.0 - strength));
            
            dest[y * sz.cx + x] = RGBA(r, g, b, pixel.a);
        }
    }
    
    return result;
}

// ChromaticAberrationEffect implementation
inline ChromaticAberrationEffect::ChromaticAberrationEffect() 
    : PostProcessEffect(PostProcessEffectType::CHROMATIC_ABERRATION, "ChromaticAberration") {}

inline Image ChromaticAberrationEffect::Apply(const Image& input) {
    if (!enabled) return input;
    
    double intensity = GetFloatParameter("intensity");
    Point2 direction = GetVector2Parameter("direction");
    
    return ApplyChromaticAberration(input, intensity, direction);
}

inline Image ChromaticAberrationEffect::ApplyChromaticAberration(const Image& input, 
                                                               double intensity, 
                                                               const Point2& direction) const {
    Size sz = input.GetSize();
    Image result = CreateImage(sz.cx, sz.cy, White());
    RGBA* dest = result;
    const RGBA* src = input;
    
    // Calculate offset based on intensity and direction
    int offsetX = (int)(direction.x * intensity * sz.cx / 4); // Divide by 4 to limit offset
    int offsetY = (int)(direction.y * intensity * sz.cy / 4);
    
    for (int y = 0; y < sz.cy; y++) {
        for (int x = 0; x < sz.cx; x++) {
            // Calculate positions for red/green/blue channels
            int rX = Clamp(x - offsetX, 0, sz.cx - 1);
            int rY = Clamp(y - offsetY, 0, sz.cy - 1);
            int bX = Clamp(x + offsetX, 0, sz.cx - 1);
            int bY = Clamp(y + offsetY, 0, sz.cy - 1);
            
            // Get RGB values from different positions
            int r = src[rY * sz.cx + rX].r;
            int g = src[y * sz.cx + x].g;  // Green stays in original position
            int b = src[bY * sz.cx + bX].b;
            int a = src[y * sz.cx + x].a;
            
            dest[y * sz.cx + x] = RGBA(r, g, b, a);
        }
    }
    
    return result;
}

// PostProcessManager implementation
inline PostProcessManager::PostProcessManager() {
    // Initialize with default settings
}

inline PostProcessManager::~PostProcessManager() {
    ClearEffects();
}

inline void PostProcessManager::AddEffect(SharedPtr<PostProcessEffect> effect) {
    if (effect) {
        effects.Add(effect);
    }
}

inline void PostProcessManager::RemoveEffect(const String& name) {
    int index = -1;
    for (int i = 0; i < effects.GetCount(); i++) {
        if (effects[i]->GetName() == name) {
            index = i;
            break;
        }
    }
    
    if (index >= 0) {
        effects.Remove(index);
    }
}

inline void PostProcessManager::RemoveEffectByType(PostProcessEffectType type) {
    for (int i = effects.GetCount() - 1; i >= 0; i--) {
        if (effects[i]->GetType() == type) {
            effects.Remove(i);
        }
    }
}

inline void PostProcessManager::ClearEffects() {
    effects.Clear();
}

inline Image PostProcessManager::ApplyEffects(const Image& input) const {
    Image current = input;
    
    for (const auto& effect : effects) {
        if (effect->IsEnabled()) {
            current = effect->Apply(current);
        }
    }
    
    return current;
}

inline SharedPtr<PostProcessEffect> PostProcessManager::GetEffect(const String& name) const {
    for (const auto& effect : effects) {
        if (effect->GetName() == name) {
            return effect;
        }
    }
    return nullptr;
}

inline Vector<SharedPtr<PostProcessEffect>> PostProcessManager::GetEffectsByType(PostProcessEffectType type) const {
    Vector<SharedPtr<PostProcessEffect>> result;
    for (const auto& effect : effects) {
        if (effect->GetType() == type) {
            result.Add(effect);
        }
    }
    return result;
}

inline void PostProcessManager::SetEffectEnabled(const String& name, bool enabled) {
    auto effect = GetEffect(name);
    if (effect) {
        effect->SetEnabled(enabled);
    }
}

inline Vector<String> PostProcessManager::GetEffectNames() const {
    Vector<String> names;
    for (const auto& effect : effects) {
        names.Add(effect->GetName());
    }
    return names;
}

NAMESPACE_UPP_END

#endif