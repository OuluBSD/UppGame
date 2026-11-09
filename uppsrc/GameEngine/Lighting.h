#ifndef UPP_LIGHTING_H
#define UPP_LIGHTING_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <GameLib/GameLib.h>
#include <Geometry/Geometry.h>
#include <GameEngine/GameEngine.h>

NAMESPACE_UPP

// Light types for 3D rendering
enum class LightType {
    DIRECTIONAL,
    POINT,
    SPOT
};

// Light class for 3D scene lighting
class Light {
public:
    Light();
    explicit Light(LightType type);
    virtual ~Light() = default;

    // Get/set light type
    void SetType(LightType type) { this->type = type; }
    LightType GetType() const { return type; }

    // Positional properties
    void SetPosition(const Point3& pos) { this->position = pos; }
    Point3 GetPosition() const { return position; }

    // Direction for directional and spot lights
    void SetDirection(const Vector3& dir) { this->direction = dir; }
    Vector3 GetDirection() const { return direction; }

    // Color properties
    void SetColor(const Color& color) { this->color = color; }
    Color GetColor() const { return color; }

    // Intensity
    void SetIntensity(double intensity) { this->intensity = intensity; }
    double GetIntensity() const { return intensity; }

    // Range for point and spot lights
    void SetRange(double range) { this->range = range; }
    double GetRange() const { return range; }

    // Spot light properties
    void SetInnerConeAngle(double angle) { this->innerConeAngle = angle; }
    void SetOuterConeAngle(double angle) { this->outerConeAngle = angle; }
    double GetInnerConeAngle() const { return innerConeAngle; }
    double GetOuterConeAngle() const { return outerConeAngle; }

    // Calculate lighting contribution at a point
    double CalculateIllumination(const Point3& point) const;

    // Check if this light affects a given point
    bool AffectsPoint(const Point3& point) const;

    // Get light properties as shader uniforms
    Vector<Value> GetUniforms() const;

private:
    LightType type = LightType::DIRECTIONAL;
    Point3 position = Point3(0, 0, 0);
    Vector3 direction = Vector3(0, -1, 0);  // Default down
    Color color = Color(255, 255, 255);     // White light
    double intensity = 1.0;
    double range = 10.0;                    // For point/spot lights
    double innerConeAngle = M_PI / 6;       // 30 degrees
    double outerConeAngle = M_PI / 4;       // 45 degrees
};

// Lighting system for managing scene lights
class LightingSystem {
public:
    LightingSystem();
    virtual ~LightingSystem() = default;

    // Add/remove lights
    void AddLight(std::shared_ptr<Light> light);
    void RemoveLight(int index);
    void ClearLights();

    // Get the number of lights
    int GetLightCount() const { return lights.GetCount(); }
    std::shared_ptr<Light> GetLight(int index) const { return lights[index]; }

    // Calculate combined lighting at a point
    Color CalculateLightingAt(const Point3& point, 
                             const Vector3& normal,
                             const Color& materialColor = White()) const;

    // Apply lighting to a mesh
    Mesh ApplyLightingToMesh(const Mesh& mesh, const Matrix4& transform) const;

    // Update lighting system (for animated lights)
    void Update(double dt);

    // Set ambient light level
    void SetAmbientLight(const Color& ambient) { this->ambientLight = ambient; }
    Color GetAmbientLight() const { return ambientLight; }

private:
    Vector<std::shared_ptr<Light>> lights;
    Color ambientLight = Color(50, 50, 50);  // Dim ambient light
};

END_UPP_NAMESPACE

#endif