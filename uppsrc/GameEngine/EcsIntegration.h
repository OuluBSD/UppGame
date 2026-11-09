#ifndef UPP_ECSINTEGRATION_H
#define UPP_ECSINTEGRATION_H

#include <Core/Core.h>
#include <GameEngine/GameEngine.h>
#include <Vfs/Ecs/Engine.h>  // ECS Engine
#include <Vfs/Ecs/Entity.h>  // ECS Entity
#include <Vfs/Ecs/Component.h>  // ECS Component
#include <Eon/Lib/RenderingSystem.h>  // Rendering system
#include <Eon/Draw/EventSystem.h>  // Event system
#include <Eon/Ecs/EcsPhysicsSystem.h>  // Physics system
#include <Eon/Interaction/InteractionSystem.h>  // Interaction system

NAMESPACE_UPP

// Forward declarations
class GameEcsSystem;
class GameEntityManager;

// Component for game objects with transform
class TransformComponent : public Component {
public:
    TransformComponent(VfsValue& e) : Component(e) {}

    void SetPosition(const Point3& pos) { position = pos; }
    Point3 GetPosition() const { return position; }

    void SetRotation(const Quaternion& rot) { rotation = rot; }
    Quaternion GetRotation() const { return rotation; }

    void SetScale(const Point3& scale) { scale3d = scale; }
    Point3 GetScale() const { return scale3d; }

    void Visit(Vis& v) override {
        v("position", position)("rotation", rotation)("scale", scale3d);
    }

private:
    Point3 position = Point3(0, 0, 0);
    Quaternion rotation = Quaternion(0, 0, 0, 1);  // Identity quaternion
    Point3 scale3d = Point3(1, 1, 1);
};

// Component for rendering game objects
class RenderComponent : public Component {
public:
    RenderComponent(VfsValue& e) : Component(e) {}

    void SetModel(const String& model_path) { model_path_ = model_path; }
    String GetModel() const { return model_path_; }

    void SetTexture(const String& texture_path) { texture_path_ = texture_path; }
    String GetTexture() const { return texture_path_; }

    void SetMaterial(const String& material_path) { material_path_ = material_path; }
    String GetMaterial() const { return material_path_; }

    void Visit(Vis& v) override {
        v("model", model_path_)("texture", texture_path_)("material", material_path_);
    }

private:
    String model_path_;
    String texture_path_;
    String material_path_;
};

// Component for physics
class PhysicsComponent : public Component {
public:
    PhysicsComponent(VfsValue& e) : Component(e) {}

    void SetMass(double mass) { mass_ = mass; }
    double GetMass() const { return mass_; }

    void SetVelocity(const Vector3& vel) { velocity = vel; }
    Vector3 GetVelocity() const { return velocity; }

    void SetKinematic(bool kinematic) { kinematic_ = kinematic; }
    bool IsKinematic() const { return kinematic_; }

    void Visit(Vis& v) override {
        v("mass", mass_)("velocity", velocity)("kinematic", kinematic_);
    }

private:
    double mass_ = 1.0;
    Vector3 velocity = Vector3(0, 0, 0);
    bool kinematic_ = false;
};

// Component for game object tags
class TagComponent : public Component {
public:
    TagComponent(VfsValue& e) : Component(e) {}

    void SetTag(const String& tag) { tag_ = tag; }
    String GetTag() const { return tag_; }

    void Visit(Vis& v) override {
        v("tag", tag_);
    }

private:
    String tag_;
};

// GameEcsSystem - Main system to manage ECS integration
class GameEcsSystem : public System {
public:
    ECS_SYS_CTOR(GameEcsSystem)

    // Initialize the ECS system
    bool Initialize(const WorldState& ws) override;
    void Uninitialize() override;

    // Update the ECS system (called every frame)
    void Update(double dt) override;

    // Create a basic entity with transform and render components
    EntityPtr CreateEntity(const String& name);

    // Create an entity with common components for a game object
    EntityPtr CreateGameObject(const String& tag, 
                              const Point3& position = Point3(0, 0, 0),
                              const Point3& scale = Point3(1, 1, 1));

    // Get the ECS engine
    Engine& GetEngine() { return engine; }

    // Get the entity manager
    GameEntityManager& GetEntityManager() { return *entity_manager; }

    void Visit(Vis& v) override {
        System::Visit(v);
    }

private:
    Engine engine;  // ECS Engine
    std::shared_ptr<GameEntityManager> entity_manager;

    // Initialize internal systems
    void InitializeSystems();
};

// GameEntityManager - Manage entities in the game world
class GameEntityManager {
public:
    GameEntityManager(VfsValue& owner);
    ~GameEntityManager();

    // Add an entity to the manager
    void AddEntity(EntityPtr entity);

    // Remove an entity from the manager
    void RemoveEntity(EntityPtr entity);

    // Find an entity by tag
    EntityPtr FindEntityByTag(const String& tag);

    // Find all entities with a specific tag
    Vector<EntityPtr> FindEntitiesByTag(const String& tag);

    // Update all entities
    void Update(double dt);

    // Get all entities
    const Vector<EntityPtr>& GetEntities() const { return entities; }

private:
    Vector<EntityPtr> entities;
    VfsValue* owner_value;

    // Helper to find entity by tag
    int FindEntityIndexByTag(const String& tag);
};

// GameEcsIntegration - Main ECS integration class for GameEngine
class GameEcsIntegration {
public:
    GameEcsIntegration();
    ~GameEcsIntegration();

    // Initialize the ECS integration
    bool Initialize();

    // Update the ECS systems
    void Update(double dt);

    // Create a new entity
    EntityPtr CreateEntity(const String& name);

    // Create a game object with transform and render components
    EntityPtr CreateGameObject(const String& tag, 
                              const Point3& position = Point3(0, 0, 0),
                              const Point3& scale = Point3(1, 1, 1));

    // Get the ECS system
    GameEcsSystem* GetEcsSystem() { return ecs_system.get(); }

    // Get the entity manager
    GameEntityManager* GetEntityManager() { return entity_manager.get(); }

    // Get the main ECS engine
    ::Engine* GetEngine() { return &engine; }

    // Register common components with the factory
    static void RegisterComponents();

private:
    std::shared_ptr<GameEcsSystem> ecs_system;
    std::shared_ptr<GameEntityManager> entity_manager;
    ::Engine engine;  // Main ECS engine
};

END_UPP_NAMESPACE

#endif