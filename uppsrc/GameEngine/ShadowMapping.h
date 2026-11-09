#ifndef UPP_SHADOW_MAPPING_H
#define UPP_SHADOW_MAPPING_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <GameLib/GameLib.h>
#include <GameEngine/GameEngine.h>
#include <GameEngine/Lighting.h>
#include <GameEngine/Rendering.h>

NAMESPACE_UPP

// Shadow mapping technique for realistic shadows
class ShadowMap {
public:
    ShadowMap();
    virtual ~ShadowMap() = default;

    // Initialize shadow map with specified resolution
    bool Initialize(int width = 1024, int height = 1024);

    // Get/set shadow map properties
    void SetResolution(int width, int height) { 
        this->width = width; 
        this->height = height; 
        UpdateShadowMapBuffer();
    }
    int GetWidth() const { return width; }
    int GetHeight() const { return height; }

    // Set shadow map bias to reduce shadow acne
    void SetBias(double bias) { this->bias = bias; }
    double GetBias() const { return bias; }

    // Set maximum distance for shadow casting
    void SetMaxDistance(double distance) { maxDistance = distance; }
    double GetMaxDistance() const { return maxDistance; }

    // Set shadow intensity/darkness
    void SetShadowIntensity(double intensity) { 
        shadowIntensity = clamp(intensity, 0.0, 1.0); 
    }
    double GetShadowIntensity() const { return shadowIntensity; }

    // Render shadow map from light's perspective
    void RenderShadowMap(Draw& draw, 
                        const Vector<std::shared_ptr<SceneNode>>& objects,
                        std::shared_ptr<Light> light);

    // Apply shadows during scene rendering
    void ApplyShadows(Draw& draw, 
                     const Matrix4& viewProjection,
                     const Rect& viewport);

    // Get the shadow map texture
    Image GetShadowMapTexture() const { return shadowMapTexture; }

    // Get light's view-projection matrix used for shadow mapping
    Matrix4 GetLightViewProjection() const { return lightViewProjection; }

private:
    int width, height;
    double bias = 0.005;           // Bias to reduce shadow acne
    double maxDistance = 50.0;     // Maximum shadow distance
    double shadowIntensity = 0.8;  // How dark the shadows are (0.0 = no shadow, 1.0 = black)

    // Shadow map texture
    Image shadowMapTexture;
    ImageBuffer shadowMapBuffer;

    // Light's view-projection matrix for shadow mapping
    Matrix4 lightViewProjection;

    // Update the shadow map buffer when resolution changes
    void UpdateShadowMapBuffer();

    // Calculate view-projection matrix for the light
    Matrix4 CalculateLightViewProjection(std::shared_ptr<Light> light);
};

// Shadow mapping system that manages multiple shadow maps
class ShadowMappingSystem {
public:
    ShadowMappingSystem();
    virtual ~ShadowMappingSystem();

    // Initialize the system
    bool Initialize();

    // Add/remove shadow maps
    std::shared_ptr<ShadowMap> AddShadowMap(int width = 1024, int height = 1024);
    void RemoveShadowMap(int index);
    void ClearShadowMaps();

    // Get shadow maps
    int GetShadowMapCount() const { return shadowMaps.GetCount(); }
    std::shared_ptr<ShadowMap> GetShadowMap(int index) const { return shadowMaps[index]; }

    // Update all shadow maps for current frame
    void UpdateShadowMaps(Draw& draw,
                         const Vector<std::shared_ptr<SceneNode>>& sceneObjects,
                         const Vector<std::shared_ptr<Light>>& lights);

    // Apply shadows to the scene during rendering
    void ApplyShadows(Draw& draw,
                     const Matrix4& viewProjection,
                     const Rect& viewport);

    // Enable/disable shadow mapping
    void SetEnabled(bool enabled) { this->enabled = enabled; }
    bool IsEnabled() const { return enabled; }

    // Set global shadow properties
    void SetGlobalBias(double bias) { globalBias = bias; }
    double GetGlobalBias() const { return globalBias; }

    void SetGlobalIntensity(double intensity) { 
        globalIntensity = clamp(intensity, 0.0, 1.0); 
    }
    double GetGlobalIntensity() const { return globalIntensity; }

private:
    Vector<std::shared_ptr<ShadowMap>> shadowMaps;
    bool enabled = true;
    double globalBias = 0.005;
    double globalIntensity = 0.8;
};

END_UPP_NAMESPACE

#endif