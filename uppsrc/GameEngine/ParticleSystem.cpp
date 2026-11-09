#include "ParticleSystem.h"

NAMESPACE_UPP

ParticleSystem::ParticleSystem() {
}

ParticleSystem::~ParticleSystem() {
}

bool ParticleSystem::Initialize(const EmitterConfig& cfg) {
    config = cfg;
    InitializeParticles();
    initialized = true;
    return true;
}

void ParticleSystem::InitializeParticles() {
    particles.SetCount(config.max_particles);
    for (auto& p : particles) {
        p.life = 0; // Start as dead
    }
}

void ParticleSystem::Update(double dt) {
    if (!active || !initialized) return;
    
    total_emission_time += dt;
    
    // Update existing particles
    for (auto& p : particles) {
        if (p.IsAlive()) {
            // Apply gravity
            p.velocity += config.gravity * dt;
            
            // Apply drag
            p.velocity *= (1.0 - config.drag * dt);
            
            // Update particle
            p.Update(dt);
        }
    }
    
    // Emit new particles if active and not exceeded duration
    if (config.looping || total_emission_time < config.duration) {
        EmitParticles(dt);
    }
}

void ParticleSystem::EmitParticles(double dt) {
    if (config.emission_rate <= 0) return;
    
    double particles_to_emit = config.emission_rate * dt;
    time_since_last_emit += dt;
    
    // Calculate how many particles we should emit in this frame
    int emit_count = (int)floor(time_since_last_emit * config.emission_rate);
    
    if (emit_count > 0) {
        time_since_last_emit -= emit_count / config.emission_rate;
    }
    
    // Actually emit the particles
    for (int i = 0; i < emit_count; i++) {
        int idx = GetAvailableParticleIndex();
        if (idx >= 0) {
            InitializeParticle(particles[idx]);
        }
    }
}

int ParticleSystem::GetAvailableParticleIndex() {
    // Find a dead particle to reuse
    for (int i = 0; i < particles.GetCount(); i++) {
        if (!particles[i].IsAlive()) {
            return i;
        }
    }
    
    // If no dead particles, try to reuse the oldest one
    // (only if we're okay with overriding existing particles)
    int oldest_idx = -1;
    double oldest_life = 1.0;  // Looking for the one closest to death
    
    for (int i = 0; i < particles.GetCount(); i++) {
        if (particles[i].life < oldest_life) {
            oldest_life = particles[i].life;
            oldest_idx = i;
        }
    }
    
    return oldest_idx;
}

void ParticleSystem::InitializeParticle(Particle& p) {
    // Set initial position with random offset in spawn area
    Point3 offset(
        Randomf() * config.spawn_area.GetWidth(),
        Randomf() * config.spawn_area.GetHeight(),
        0
    );
    p.position = position + offset;
    
    // Set random velocity between min and max
    p.velocity.x = config.min_velocity.x + 
                   Randomf() * (config.max_velocity.x - config.min_velocity.x);
    p.velocity.y = config.min_velocity.y + 
                   Randomf() * (config.max_velocity.y - config.min_velocity.y);
    p.velocity.z = config.min_velocity.z + 
                   Randomf() * (config.max_velocity.z - config.min_velocity.z);
    
    // Set random life with variance
    double life_variance = config.life_variance * Randomf();
    if (Random(2) == 0) life_variance = -life_variance;  // Random sign
    p.max_life = max(0.001, config.particle_life + life_variance);
    p.life = 1.0;  // Fresh particle
    
    // Set random size
    p.size = config.min_size + 
             Randomf() * (config.max_size - config.min_size);
    
    // Set initial color (interpolated between start and end based on life)
    p.color = config.start_color;  // For now, just use start color
    
    // Set random rotation
    p.rotation = Randomf() * 2.0 * M_PI;
    
    // Set texture index (for animated particles)
    p.texture_index = 0;
}

void ParticleSystem::Render(Draw& draw) {
    if (!initialized) return;
    
    // Draw all active particles
    for (const auto& p : particles) {
        if (p.IsAlive()) {
            // Calculate color based on life (interpolate between start and end)
            double life_ratio = p.life;  // Goes from 1.0 to 0.0
            Color particle_color = Color(
                (byte)(config.start_color.r * life_ratio + config.end_color.r * (1.0 - life_ratio)),
                (byte)(config.start_color.g * life_ratio + config.end_color.g * (1.0 - life_ratio)),
                (byte)(config.start_color.b * life_ratio + config.end_color.b * (1.0 - life_ratio))
            );
            
            // Draw the particle as a small rectangle (for now, until we implement texture support)
            // Calculate the rectangle for this particle
            Rect particle_rect(
                (int)(p.position.x - p.size/2),
                (int)(p.position.y - p.size/2),
                (int)(p.position.x + p.size/2),
                (int)(p.position.y + p.size/2)
            );
            
            // Draw the particle
            draw.DrawRect(particle_rect, particle_color);
        }
    }
}

void ParticleSystem::SetPosition(const Point3& pos) {
    position = pos;
}

int ParticleSystem::GetActiveParticleCount() const {
    int count = 0;
    for (const auto& p : particles) {
        if (p.IsAlive()) count++;
    }
    return count;
}

void ParticleSystem::Reset() {
    for (auto& p : particles) {
        p.life = 0; // Mark as dead
    }
    time_since_last_emit = 0.0;
    total_emission_time = 0.0;
    active = true;
}

END_UPP_NAMESPACE