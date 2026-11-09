#ifndef UPP_GAMEENGINE_SYSTEMS_H
#define UPP_GAMEENGINE_SYSTEMS_H

#include <Core/Core.h>
#include <Vfs/Ecs/Ecs.h>
#include <GameEngine/Components.h>
#include <Eon/Lib/RenderingSystem.h>
#include <Eon/Ecs/EcsPhysicsSystem.h>

NAMESPACE_UPP

// Base Game System class
class GameSystem : public System {
public:
    GameSystem() : System() {}
    GameSystem(VfsValue& m) : System(m) {}
    
    virtual ~GameSystem() {}
    
    // Initialize the system with world state
    virtual bool Initialize(const WorldState& ws) { 
        SetInitialized();
        return true; 
    }
    
    // Called when system is uninitialized
    virtual void Uninitialize() {}
    
    // Update the system each frame
    virtual void Update(double dt) = 0;
    
    TypeCls GetTypeCls() const override { return TypeCls(*this); }
};

// Transform system for updating transform components
class TransformSystem : public GameSystem {
public:
    ECS_SYS_CTOR(TransformSystem)

    bool Initialize(const WorldState& ws) override;
    void Uninitialize() override;
    void Update(double dt) override;

    // Add a component to this system for processing
    void Add(Component& comp);
    
    // Remove a component from this system
    void Remove(Component& comp);

private:
    Vector<Ptr<TransformComponent>> transform_components;
};

// Render system for rendering entities
class RenderSystem : public GameSystem {
public:
    ECS_SYS_CTOR(RenderSystem)

    bool Initialize(const WorldState& ws) override;
    void Uninitialize() override;
    void Update(double dt) override;

    // Add a component to this system for processing
    void Add(Component& comp);
    
    // Remove a component from this system
    void Remove(Component& comp);

private:
    Vector<Ptr<RenderComponent>> render_components;
};

// Physics system for simulating physics
class PhysicsSystem : public GameSystem {
public:
    ECS_SYS_CTOR(PhysicsSystem)

    bool Initialize(const WorldState& ws) override;
    void Uninitialize() override;
    void Update(double dt) override;

    // Add a component to this system for processing
    void Add(Component& comp);
    
    // Remove a component from this system
    void Remove(Component& comp);

private:
    Vector<Ptr<PhysicsComponent>> physics_components;
    double accumulated_time = 0.0;
    const double fixed_timestep = 1.0/60.0; // 60 FPS fixed timestep
};

// Animation system for updating animations
class AnimationSystem : public GameSystem {
public:
    ECS_SYS_CTOR(AnimationSystem)

    bool Initialize(const WorldState& ws) override;
    void Uninitialize() override;
    void Update(double dt) override;

    // Add a component to this system for processing
    void Add(Component& comp);
    
    // Remove a component from this system
    void Remove(Component& comp);

private:
    Vector<Ptr<AnimationComponent>> animation_components;
};

// Audio system for playing sounds
class AudioSystem : public GameSystem {
public:
    ECS_SYS_CTOR(AudioSystem)

    bool Initialize(const WorldState& ws) override;
    void Uninitialize() override;
    void Update(double dt) override;

    // Add a component to this system for processing
    void Add(Component& comp);
    
    // Remove a component from this system
    void Remove(Component& comp);

private:
    Vector<Ptr<AudioComponent>> audio_components;
};

// Input system for handling input events
class InputSystem : public GameSystem {
public:
    ECS_SYS_CTOR(InputSystem)

    bool Initialize(const WorldState& ws) override;
    void Uninitialize() override;
    void Update(double dt) override;

    // Add a component to this system for processing
    void Add(Component& comp);
    
    // Remove a component from this system
    void Remove(Component& comp);

private:
    Vector<Ptr<InputComponent>> input_components;
};

// UI system for handling UI elements
class UISystem : public GameSystem {
public:
    ECS_SYS_CTOR(UISystem)

    bool Initialize(const WorldState& ws) override;
    void Uninitialize() override;
    void Update(double dt) override;

    // Add a component to this system for processing
    void Add(Component& comp);
    
    // Remove a component from this system
    void Remove(Component& comp);

private:
    Vector<Ptr<UIComponent>> ui_components;
};

// Entity management system that coordinates other systems
class EntityManagerSystem : public GameSystem {
public:
    ECS_SYS_CTOR(EntityManagerSystem)

    bool Initialize(const WorldState& ws) override;
    void Uninitialize() override;
    void Update(double dt) override;

    // Create a new entity with specified components
    EntityPtr CreateEntity(const String& name);
    
    // Create a game object with common components
    EntityPtr CreateGameObject(const String& tag,
                              const Point3& position = Point3(0, 0, 0),
                              const Point3& scale = Point3(1, 1, 1));

    // Find entities by tag
    Vector<EntityPtr> FindEntitiesByTag(const String& tag);
    
    // Remove an entity
    void RemoveEntity(EntityPtr entity);

private:
    Vector<EntityPtr> entities;
};

END_UPP_NAMESPACE

#endif