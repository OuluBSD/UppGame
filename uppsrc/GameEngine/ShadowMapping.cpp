#include "ShadowMapping.h"

NAMESPACE_UPP

ShadowMap::ShadowMap() : width(1024), height(1024) {
}

bool ShadowMap::Initialize(int width, int height) {
    this->width = width;
    this->height = height;
    UpdateShadowMapBuffer();
    return true;
}

void ShadowMap::UpdateShadowMapBuffer() {
    shadowMapBuffer = ImageBuffer(width, height);
    // Initialize with maximum depth (far plane)
    for (int y = 0; y < height; y++) {
        RGBA* line = shadowMapBuffer[y];
        for (int x = 0; x < width; x++) {
            // Use RGB to store depth information (simplified approach)
            line[x] = RGBA(255, 255, 255, 255); // Max depth value
        }
    }
    shadowMapTexture = Image(shadowMapBuffer);
}

Matrix4 ShadowMap::CalculateLightViewProjection(std::shared_ptr<Light> light) {
    if (!light) return Matrix4::Identity();

    Matrix4 view, projection;

    switch (light->GetType()) {
        case LightType::DIRECTIONAL: {
            // For directional lights, use an orthographic projection
            // that encompasses the view frustum
            Point3 lightDir = Point3(light->GetDirection().x, 
                                   light->GetDirection().y, 
                                   light->GetDirection().z);
            
            // Create a look-at matrix for the light
            Point3 lightPos = -lightDir * 10.0; // Position the light behind the scene
            Point3 target = Point3(0, 0, 0);
            Vector3 up = Vector3(0, 1, 0);
            
            view = Matrix4::LookAt(lightPos, target, up);
            
            // Orthographic projection to cover the scene
            projection = Matrix4::Ortho(-maxDistance, maxDistance, 
                                       -maxDistance, maxDistance,
                                       0.1, maxDistance * 2);
            break;
        }
        case LightType::POINT:
            // Point lights require cube shadow maps or multiple passes
            // For simplicity, we'll handle this as a special case
            view = Matrix4::Identity();
            projection = Matrix4::Perspective(M_PI / 2.0, 1.0, 0.1, maxDistance);
            break;
        case LightType::SPOT:
            // Spot lights use perspective projection matching the light cone
            view = Matrix4::LookAt(light->GetPosition(),
                                  light->GetPosition() + light->GetDirection(),
                                  Vector3(0, 1, 0));
            projection = Matrix4::Perspective(light->GetOuterConeAngle() * 2,
                                            1.0, 0.1, maxDistance);
            break;
    }

    return projection * view;
}

void ShadowMap::RenderShadowMap(Draw& draw, 
                               const Vector<std::shared_ptr<SceneNode>>& objects,
                               std::shared_ptr<Light> light) {
    if (!light) return;

    // Calculate the light's view-projection matrix
    lightViewProjection = CalculateLightViewProjection(light);

    // In a real implementation, this is where we would:
    // 1. Set up a depth-only render target (the shadow map)
    // 2. Render all objects from the light's perspective
    // 3. Store the depth information in the shadow map
    
    // For this implementation, we'll simulate the process by updating 
    // our shadow map buffer based on the light's view
    
    // Update the shadow map texture
    UpdateShadowMapBuffer();
    
    // In a full implementation, we would render objects from the light's perspective
    // and store only the depth information in the shadow map
}

void ShadowMap::ApplyShadows(Draw& draw, 
                            const Matrix4& viewProjection,
                            const Rect& viewport) {
    // In a real implementation, this is where we would:
    // 1. Pass the shadow map texture to the shader
    // 2. Pass the light's view-projection matrix to the shader
    // 3. Calculate shadow coordinates for each fragment
    // 4. Compare depths to determine if fragment is in shadow
    
    // For this implementation, we'll just apply a basic shadow effect
    // based on the shadow map and light positions
    
    // In a real implementation, this would be handled via shader programs
    // that perform the shadow coordinate transformation and depth comparison
}

ShadowMappingSystem::ShadowMappingSystem() {
}

ShadowMappingSystem::~ShadowMappingSystem() {
    ClearShadowMaps();
}

bool ShadowMappingSystem::Initialize() {
    // In a real implementation, this would initialize GPU resources
    return true;
}

std::shared_ptr<ShadowMap> ShadowMappingSystem::AddShadowMap(int width, int height) {
    auto shadowMap = std::make_shared<ShadowMap>();
    if (shadowMap->Initialize(width, height)) {
        shadowMaps.Add(shadowMap);
        return shadowMap;
    }
    return nullptr;
}

void ShadowMappingSystem::RemoveShadowMap(int index) {
    if (index >= 0 && index < shadowMaps.GetCount()) {
        shadowMaps.Remove(index);
    }
}

void ShadowMappingSystem::ClearShadowMaps() {
    shadowMaps.Clear();
}

void ShadowMappingSystem::UpdateShadowMaps(Draw& draw,
                                          const Vector<std::shared_ptr<SceneNode>>& sceneObjects,
                                          const Vector<std::shared_ptr<Light>>& lights) {
    if (!enabled) return;

    // For each light that casts shadows, update its shadow map
    for (int i = 0; i < lights.GetCount() && i < shadowMaps.GetCount(); i++) {
        auto light = lights[i];
        if (light) {  // Only handle lights that should cast shadows
            shadowMaps[i]->SetGlobalBias(globalBias);
            shadowMaps[i]->SetShadowIntensity(globalIntensity);
            shadowMaps[i]->RenderShadowMap(draw, sceneObjects, light);
        }
    }
}

void ShadowMappingSystem::ApplyShadows(Draw& draw,
                                      const Matrix4& viewProjection,
                                      const Rect& viewport) {
    if (!enabled) return;

    // Apply all shadow maps to the scene
    for (auto& shadowMap : shadowMaps) {
        if (shadowMap) {
            shadowMap->ApplyShadows(draw, viewProjection, viewport);
        }
    }
}

END_UPP_NAMESPACE