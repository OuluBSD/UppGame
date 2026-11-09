#ifndef UPP_SKYBOX_ENVIRONMENT_H
#define UPP_SKYBOX_ENVIRONMENT_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <GameLib/GameLib.h>
#include <GameEngine/GameEngine.h>
#include <GameEngine/SpriteMesh.h>

NAMESPACE_UPP

// Skybox class for rendering distant environment
class Skybox {
public:
    Skybox();
    virtual ~Skybox() = default;

    // Initialize with a cube texture or six separate textures
    bool Initialize(const String& texturePath);
    bool Initialize(const Image& posX, const Image& negX,
                   const Image& posY, const Image& negY,
                   const Image& posZ, const Image& negZ);

    // Initialize with default procedural sky
    bool InitializeProcedural();

    // Render the skybox
    void Render(Draw& draw, const Matrix4& viewMatrix, const Matrix4& projectionMatrix, 
                const Rect& viewport);

    // Set skybox properties
    void SetSize(double size) { this->size = size; }
    double GetSize() const { return size; }

    void SetPosition(const Point3& pos) { position = pos; }
    Point3 GetPosition() const { return position; }

    // Apply time-based changes (like day/night cycle)
    void SetTimeOfDay(double time) { timeOfDay = time; }  // 0.0-1.0, where 0.5 = noon
    double GetTimeOfDay() const { return timeOfDay; }

    // Get the cube mesh used for rendering
    const Mesh& GetMesh() const { return skyboxMesh; }

private:
    Mesh skyboxMesh;
    Vector<Image> textures;  // 6 textures for the cube faces
    double size = 100.0;     // Size of the skybox
    Point3 position = Point3(0, 0, 0);
    double timeOfDay = 0.5;  // 0.0-1.0 (0.5 = noon)

    // Create the cube mesh for the skybox
    void CreateSkyboxMesh();

    // Apply time-based sky effects
    void ApplyTimeEffects();
};

// Environment system managing sky, fog, atmospheric effects
class EnvironmentSystem {
public:
    EnvironmentSystem();
    virtual ~EnvironmentSystem() = default;

    // Initialize the environment system
    bool Initialize();

    // Skybox management
    bool SetSkybox(std::shared_ptr<Skybox> skybox);
    std::shared_ptr<Skybox> GetSkybox() const { return skybox; }

    // Fog properties
    void SetFogEnabled(bool enabled) { fogEnabled = enabled; }
    bool IsFogEnabled() const { return fogEnabled; }

    void SetFogColor(const Color& color) { fogColor = color; }
    Color GetFogColor() const { return fogColor; }

    void SetFogDensity(double density) { fogDensity = clamp(density, 0.0, 1.0); }
    double GetFogDensity() const { return fogDensity; }

    void SetFogStartDistance(double start) { fogStartDistance = start; }
    double GetFogStartDistance() const { return fogStartDistance; }

    void SetFogEndDistance(double end) { fogEndDistance = end; }
    double GetFogEndDistance() const { return fogEndDistance; }

    // Atmospheric scattering effects
    void SetAtmosphericScatteringEnabled(bool enabled) { atmosphericScatteringEnabled = enabled; }
    bool IsAtmosphericScatteringEnabled() const { return atmosphericScatteringEnabled; }

    // Day/night cycle
    void SetDayNightCycleEnabled(bool enabled) { dayNightCycleEnabled = enabled; }
    bool IsDayNightCycleEnabled() const { return dayNightCycleEnabled; }

    void SetTimeScale(double scale) { timeScale = scale; }  // How fast time passes
    double GetTimeScale() const { return timeScale; }

    // Update the environment (call each frame)
    void Update(double dt);

    // Render the environment
    void Render(Draw& draw, const Matrix4& viewMatrix, const Matrix4& projectionMatrix, 
                const Rect& viewport);

    // Get current environment time (0.0-1.0, where 0.5 = noon)
    double GetCurrentTime() const { return currentTime; }

    // Set current time directly
    void SetCurrentTime(double time) { currentTime = time; }

private:
    std::shared_ptr<Skybox> skybox;
    
    // Fog properties
    bool fogEnabled = false;
    Color fogColor = Color(135, 206, 235);  // Sky blue
    double fogDensity = 0.1;
    double fogStartDistance = 10.0;
    double fogEndDistance = 100.0;
    
    // Atmospheric effects
    bool atmosphericScatteringEnabled = false;
    
    // Day/night cycle
    bool dayNightCycleEnabled = false;
    double timeScale = 1.0;
    double currentTime = 0.5;  // Start at noon
    
    // Time tracking
    double lastUpdateTime = 0.0;
};

// Predefined environment presets
struct EnvironmentPreset {
    String name;
    Color fogColor;
    double fogDensity;
    bool fogEnabled;
    bool atmosphericScattering;
    bool dayNightCycle;
    double initialTimeOfDay;  // 0.0-1.0
    
    static EnvironmentPreset GetDaytimePreset();
    static EnvironmentPreset GetNighttimePreset();
    static EnvironmentPreset GetSunsetPreset();
    static EnvironmentPreset GetFoggyPreset();
};

END_UPP_NAMESPACE

#endif