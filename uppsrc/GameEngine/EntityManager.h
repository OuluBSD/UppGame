#ifndef UPP_GAMEENGINE_ENTITYMANAGER_H
#define UPP_GAMEENGINE_ENTITYMANAGER_H

#include <Core/Core.h>
#include <Vfs/Ecs/Ecs.h>
#include <GameEngine/Components.h>
#include <GameEngine/Systems.h>

NAMESPACE_UPP

// Entity factory for creating common entity types
class EntityFactory {
public:
    // Create a basic entity with transform
    static EntityPtr CreateEntity(Engine& engine, const String& name);
    
    // Create a game object with transform and render components
    static EntityPtr CreateGameObject(Engine& engine, 
                                     const String& tag,
                                     const Point3& position = Point3(0, 0, 0),
                                     const Point3& scale = Point3(1, 1, 1));
    
    // Create a physics object with transform, render and physics components
    static EntityPtr CreatePhysicsObject(Engine& engine,
                                        const String& tag,
                                        const Point3& position = Point3(0, 0, 0),
                                        const Point3& scale = Point3(1, 1, 1),
                                        double mass = 1.0);
    
    // Create a UI element
    static EntityPtr CreateUIElement(Engine& engine,
                                    const String& tag,
                                    const Point& position = Point(0, 0),
                                    const Size& size = Size(100, 100));
    
    // Create a player entity with components for player behavior
    static EntityPtr CreatePlayer(Engine& engine,
                                 const String& tag,
                                 const Point3& position = Point3(0, 0, 0));
    
    // Create a camera entity
    static EntityPtr CreateCamera(Engine& engine,
                                 const String& tag,
                                 const Point3& position = Point3(0, 0, 0));
    
    // Create a light entity
    static EntityPtr CreateLight(Engine& engine,
                                const String& tag,
                                const Point3& position = Point3(0, 0, 0));
};

// Entity query system for finding entities based on components
class EntityQuery {
public:
    EntityQuery(Engine& engine);
    
    // Find entities with a specific component type
    template<typename T>
    Vector<EntityPtr> WithComponent();
    
    // Find entities with multiple component types
    template<typename T1, typename T2>
    Vector<EntityPtr> WithComponents();
    
    template<typename T1, typename T2, typename T3>
    Vector<EntityPtr> WithComponents();
    
    // Find entities with a specific tag
    Vector<EntityPtr> WithTag(const String& tag);
    
    // Find entities with a specific tag and component
    template<typename T>
    Vector<EntityPtr> WithTagAndComponent(const String& tag);
    
private:
    Engine& engine;
};

// Main Entity Manager to coordinate entity lifecycle
class EntityManager {
public:
    EntityManager(Engine& engine);
    ~EntityManager();
    
    // Initialize the entity manager
    bool Initialize();
    
    // Update all managed entities
    void Update(double dt);
    
    // Create a new entity through the factory
    EntityPtr CreateEntity(const String& name);
    
    // Create a game object
    EntityPtr CreateGameObject(const String& tag,
                              const Point3& position = Point3(0, 0, 0),
                              const Point3& scale = Point3(1, 1, 1));
    
    // Create a physics object
    EntityPtr CreatePhysicsObject(const String& tag,
                                 const Point3& position = Point3(0, 0, 0),
                                 const Point3& scale = Point3(1, 1, 1),
                                 double mass = 1.0);
    
    // Find entity by ID
    EntityPtr FindEntityById(int64 id);
    
    // Find entities by tag
    Vector<EntityPtr> FindEntitiesByTag(const String& tag);
    
    // Find entities with a specific component
    template<typename T>
    Vector<EntityPtr> FindEntitiesWithComponent();
    
    // Remove an entity
    void RemoveEntity(EntityPtr entity);
    
    // Remove all entities
    void Clear();
    
    // Get the total count of entities
    int GetEntityCount() const { return entity_count; }
    
    // Get all entities
    const Vector<EntityPtr>& GetAllEntities() const { return entities; }
    
    // Register a component type with the factory system
    static void RegisterComponentTypes();
    
private:
    Engine& engine;
    Vector<EntityPtr> entities;
    Index<int64> entity_ids;  // Track entity IDs to prevent duplicates
    int entity_count = 0;
};

// Template implementations
template<typename T>
Vector<EntityPtr> EntityQuery::WithComponent() {
    Vector<EntityPtr> result;
    
    auto& entityManagerSys = engine.Get<EntityManagerSystem>();
    if (!entityManagerSys) return result;
    
    // Find all entities and check if they have the required component
    for (auto& entity : entityManagerSys->GetEntities()) {
        if (entity && entity->IsEnabled()) {
            T* component = entity->Find<T>();
            if (component) {
                result.Add(entity);
            }
        }
    }
    
    return result;
}

template<typename T1, typename T2>
Vector<EntityPtr> EntityQuery::WithComponents() {
    Vector<EntityPtr> result;
    
    auto& entityManagerSys = engine.Get<EntityManagerSystem>();
    if (!entityManagerSys) return result;
    
    // Find all entities and check if they have both required components
    for (auto& entity : entityManagerSys->GetEntities()) {
        if (entity && entity->IsEnabled()) {
            T1* comp1 = entity->Find<T1>();
            T2* comp2 = entity->Find<T2>();
            
            if (comp1 && comp2) {
                result.Add(entity);
            }
        }
    }
    
    return result;
}

template<typename T1, typename T2, typename T3>
Vector<EntityPtr> EntityQuery::WithComponents() {
    Vector<EntityPtr> result;
    
    auto& entityManagerSys = engine.Get<EntityManagerSystem>();
    if (!entityManagerSys) return result;
    
    // Find all entities and check if they have all three required components
    for (auto& entity : entityManagerSys->GetEntities()) {
        if (entity && entity->IsEnabled()) {
            T1* comp1 = entity->Find<T1>();
            T2* comp2 = entity->Find<T2>();
            T3* comp3 = entity->Find<T3>();
            
            if (comp1 && comp2 && comp3) {
                result.Add(entity);
            }
        }
    }
    
    return result;
}

template<typename T>
Vector<EntityPtr> EntityQuery::WithTagAndComponent(const String& tag) {
    Vector<EntityPtr> result;
    
    auto& entityManagerSys = engine.Get<EntityManagerSystem>();
    if (!entityManagerSys) return result;
    
    // Find all entities with the specified tag
    Vector<EntityPtr> taggedEntities = entityManagerSys->FindEntitiesByTag(tag);
    
    // Filter to only those that also have the specified component
    for (auto& entity : taggedEntities) {
        if (entity && entity->IsEnabled()) {
            T* component = entity->Find<T>();
            if (component) {
                result.Add(entity);
            }
        }
    }
    
    return result;
}

template<typename T>
Vector<EntityPtr> EntityManager::FindEntitiesWithComponent() {
    Vector<EntityPtr> result;
    
    for (auto& entity : entities) {
        if (entity && entity->IsEnabled()) {
            T* component = entity->Find<T>();
            if (component) {
                result.Add(entity);
            }
        }
    }
    
    return result;
}

END_UPP_NAMESPACE

#endif