#ifndef UPP_PARTICLESYSTEM_H
#define UPP_PARTICLESYSTEM_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <GameLib/GameLib.h>
#include <Geometry/Geometry.h>
#include <GameEngine/VFS.h>
#include <GameEngine/TextureAtlas.h>

NAMESPACE_UPP

// Structure to represent a single particle
struct Particle {
    Point3 position;
    Vector3 velocity;
    Color color;
    double size;
    double rotation;
    double life;           // Current life [0, 1], where 1.0 is full life, 0.0 is dead
    double max_life;       // Maximum life of the particle
    int texture_index;     // Index in the texture array (for animated particles)
    
    Particle() 
        : position(0, 0, 0), velocity(0, 0, 0), color(White()), size(1.0), 
          rotation(0.0), life(1.0), max_life(1.0), texture_index(0) {}
    
    bool IsAlive() const { return life > 0.0; }
    void Update(double dt) { 
        if (life > 0.0) {
            life -= dt / max_life;
            position += velocity * dt;
        }
    }
};

// Structure to define particle emitter properties
struct EmitterConfig {
    // Emission properties
    double emission_rate = 10.0;      // Particles per second
    int max_particles = 1000;         // Maximum particles in the system
    double particle_life = 2.0;       // Base life of particles (seconds)
    double life_variance = 0.5;       // Variance in particle life
    
    // Initial velocity properties
    Vector3 min_velocity = Vector3(-50, -50, 0);
    Vector3 max_velocity = Vector3(50, 50, 0);
    
    // Size properties
    double min_size = 10.0;
    double max_size = 20.0;
    
    // Color properties
    Color start_color = Color(255, 255, 255);
    Color end_color = Color(255, 255, 255);
    
    // Position properties
    Rect spawn_area = Rect(0, 0, 10, 10);  // Area where particles spawn
    
    // Physics properties
    Vector3 gravity = Vector3(0, 0, 0);    // Gravity affecting particles
    double drag = 0.0;                     // Air resistance
    
    // Texture properties
    String texture_path;                   // Texture for particles
    String atlas_region;                   // Region in texture atlas (optional)
    
    // Emitter properties
    bool looping = true;                   // Whether to continuously emit
    double duration = -1.0;                // Duration of emission (-1 for infinite)
};

// Particle system class that manages particle effects
class ParticleSystem {
public:
    ParticleSystem();
    virtual ~ParticleSystem();

    // Initialize with configuration
    bool Initialize(const EmitterConfig& config);
    
    // Update the particle system
    void Update(double dt);

    // Render the particles
    void Render(Draw& draw);

    // Set the position of the particle emitter
    void SetPosition(const Point3& pos);
    Point3 GetPosition() const { return position; }

    // Get/set active state
    void SetActive(bool active) { this->active = active; }
    bool IsActive() const { return active; }

    // Get number of active particles
    int GetActiveParticleCount() const;

    // Reset the particle system
    void Reset();

    // Get/set the texture atlas
    void SetTextureAtlas(std::shared_ptr<TextureAtlas> atlas) { 
        texture_atlas = atlas; 
    }
    std::shared_ptr<TextureAtlas> GetTextureAtlas() const { 
        return texture_atlas; 
    }

    // Get the current configuration
    const EmitterConfig& GetConfig() const { return config; }

private:
    // Configuration
    EmitterConfig config;
    
    // Particles
    Vector<Particle> particles;
    
    // Emitter state
    Point3 position = Point3(0, 0, 0);
    bool active = true;
    bool initialized = false;
    
    // Emission timing
    double time_since_last_emit = 0.0;
    double total_emission_time = 0.0;
    
    // Texture atlas
    std::shared_ptr<TextureAtlas> texture_atlas;
    
    // Initialize particles array
    void InitializeParticles();
    
    // Emit new particles
    void EmitParticles(double dt);
    
    // Get a dead particle to reuse, or create a new one if possible
    int GetAvailableParticleIndex();
    
    // Initialize a single particle with random properties
    void InitializeParticle(Particle& p);
};

END_UPP_NAMESPACE

#endif