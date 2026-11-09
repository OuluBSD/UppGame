#include "Lighting.h"

NAMESPACE_UPP

// Light implementation
Light::Light() : type(LightType::DIRECTIONAL) {
}

Light::Light(LightType type) : type(type) {
}

double Light::CalculateIllumination(const Point3& point) const {
    switch (type) {
        case LightType::DIRECTIONAL:
            // Directional lights have constant illumination
            return intensity;
            
        case LightType::POINT: {
            // Calculate distance-based attenuation
            Vector3 lightDir = position - point;
            double distance = lightDir.Length();
            
            if (distance > range) {
                return 0.0;
            }
            
            // Basic attenuation formula: 1/(constant + linear*distance + quadratic*distance^2)
            double attenuation = 1.0 / (1.0 + 0.1 * distance + 0.01 * distance * distance);
            return intensity * attenuation;
        }
        
        case LightType::SPOT: {
            // Calculate distance-based attenuation
            Vector3 lightDir = (position - point).Normalize();
            Vector3 spotDir = direction.Normalize();
            
            // Calculate angular attenuation
            double angle = acos(clamp(lightDir * spotDir, -1.0, 1.0));
            if (angle > outerConeAngle) {
                return 0.0;  // Outside outer cone
            }
            
            // Smooth transition between inner and outer cones
            double angularAttenuation = 1.0;
            if (angle > innerConeAngle) {
                angularAttenuation = (angle - innerConeAngle) / (outerConeAngle - innerConeAngle);
                angularAttenuation = 1.0 - clamp(angularAttenuation, 0.0, 1.0);
            }
            
            // Calculate distance-based attenuation
            double distance = (position - point).Length();
            if (distance > range) {
                return 0.0;
            }
            
            double distanceAttenuation = 1.0 / (1.0 + 0.1 * distance + 0.01 * distance * distance);
            
            return intensity * angularAttenuation * distanceAttenuation;
        }
    }
    
    return 0.0;  // Should not reach here
}

bool Light::AffectsPoint(const Point3& point) const {
    return CalculateIllumination(point) > 0.01;  // Threshold to determine if light affects point
}

Vector<Value> Light::GetUniforms() const {
    Vector<Value> uniforms;
    uniforms.Add(AsValue((int)type));
    uniforms.Add(AsValue(position));
    uniforms.Add(AsValue(direction));
    uniforms.Add(AsValue(color));
    uniforms.Add(AsValue(intensity));
    uniforms.Add(AsValue(range));
    uniforms.Add(AsValue(innerConeAngle));
    uniforms.Add(AsValue(outerConeAngle));
    return uniforms;
}

// LightingSystem implementation
LightingSystem::LightingSystem() {
}

void LightingSystem::AddLight(std::shared_ptr<Light> light) {
    if (light) {
        lights.Add(light);
    }
}

void LightingSystem::RemoveLight(int index) {
    if (index >= 0 && index < lights.GetCount()) {
        lights.Remove(index);
    }
}

void LightingSystem::ClearLights() {
    lights.Clear();
}

Color LightingSystem::CalculateLightingAt(const Point3& point, 
                                         const Vector3& normal,
                                         const Color& materialColor) const {
    // Start with ambient light
    double r = ambientLight.r / 255.0;
    double g = ambientLight.g / 255.0;
    double b = ambientLight.b / 255.0;
    
    // Add contributions from each light
    for (const auto& light : lights) {
        if (!light || !light->AffectsPoint(point)) continue;
        
        double illumination = light->CalculateIllumination(point);
        
        if (illumination > 0.01) {  // Only calculate if light affects point
            Vector3 lightDir;
            if (light->GetType() == LightType::DIRECTIONAL) {
                // For directional lights, use the opposite of the direction
                lightDir = (-light->GetDirection()).Normalize();
            } else {
                // For point/spot lights, calculate direction from point to light
                lightDir = (light->GetPosition() - point).Normalize();
            }
            
            // Calculate diffuse lighting using Lambert's cosine law
            double NdotL = max(0.0, normal * lightDir);
            
            // Add diffuse contribution
            Color lightColor = light->GetColor();
            r += (lightColor.r / 255.0) * NdotL * illumination;
            g += (lightColor.g / 255.0) * NdotL * illumination;
            b += (lightColor.b / 255.0) * NdotL * illumination;
        }
    }
    
    // Clamp values to [0, 1] range
    r = clamp(r, 0.0, 1.0);
    g = clamp(g, 0.0, 1.0);
    b = clamp(b, 0.0, 1.0);
    
    // Apply to material color
    int finalR = (int)(r * materialColor.r);
    int finalG = (int)(g * materialColor.g);
    int finalB = (int)(b * materialColor.b);
    
    return Color(
        min(255, max(0, finalR)),
        min(255, max(0, finalG)),
        min(255, max(0, finalB))
    );
}

Mesh LightingSystem::ApplyLightingToMesh(const Mesh& mesh, const Matrix4& transform) const {
    Mesh litMesh = mesh;  // Start with a copy of the original mesh
    
    Vector<Vertex>& vertices = const_cast<Vector<Vertex>&>(litMesh.GetVertices());
    
    for (auto& vertex : vertices) {
        // Transform the vertex position using the given transform
        Point4 pos4(vertex.position.x, vertex.position.y, vertex.position.z, 1.0);
        Point4 transformedPos = transform.Transform(pos4);
        Point3 worldPos(transformedPos.x, transformedPos.y, transformedPos.z);
        
        // Calculate final color with lighting
        Color litColor = CalculateLightingAt(worldPos, vertex.normal, vertex.color);
        vertex.color = litColor;
    }
    
    return litMesh;
}

void LightingSystem::Update(double dt) {
    // Here we can update animated lights if needed
    // For now, we just pass through
}

END_UPP_NAMESPACE