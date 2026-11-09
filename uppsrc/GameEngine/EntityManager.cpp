#include "EntityManager.h"

NAMESPACE_UPP

// EntityFactory implementation
EntityPtr EntityFactory::CreateEntity(Engine& engine, const String& name) {
    auto* entityManagerSys = engine.Get<EntityManagerSystem>();
    if (!entityManagerSys) {
        // If no EntityManagerSystem exists, create it
        entityManagerSys = engine.Add<EntityManagerSystem>();
    }
    
    return entityManagerSys->CreateEntity(name);
}

EntityPtr EntityFactory::CreateGameObject(Engine& engine,
                                         const String& tag,
                                         const Point3& position,
                                         const Point3& scale) {
    auto* entityManagerSys = engine.Get<EntityManagerSystem>();
    if (!entityManagerSys) {
        entityManagerSys = engine.Add<EntityManagerSystem>();
    }
    
    EntityPtr entity = entityManagerSys->CreateGameObject(tag, position, scale);
    
    // Add a tag component if not already added
    if (entity) {
        TagComponent* tagComp = entity->Find<TagComponent>();
        if (!tagComp) {
            tagComp = entity->Add<TagComponent>();
            if (tagComp) {
                tagComp->SetTag(tag);
            }
        }
    }
    
    return entity;
}

EntityPtr EntityFactory::CreatePhysicsObject(Engine& engine,
                                           const String& tag,
                                           const Point3& position,
                                           const Point3& scale,
                                           double mass) {
    EntityPtr entity = CreateGameObject(engine, tag, position, scale);
    
    if (entity) {
        // Add physics component
        PhysicsComponent* physics = entity->Add<PhysicsComponent>();
        if (physics) {
            physics->SetMass(mass);
        }
    }
    
    return entity;
}

EntityPtr EntityFactory::CreateUIElement(Engine& engine,
                                        const String& tag,
                                        const Point& position,
                                        const Size& size) {
    auto* entityManagerSys = engine.Get<EntityManagerSystem>();
    if (!entityManagerSys) {
        entityManagerSys = engine.Add<EntityManagerSystem>();
    }
    
    EntityPtr entity = entityManagerSys->CreateEntity(tag);
    
    if (entity) {
        // Add UI component
        UIComponent* ui = entity->Add<UIComponent>();
        if (ui) {
            ui->SetPosition(position);
            ui->SetSize(size);
            ui->SetUIType("panel");
        }
        
        // Add tag component
        TagComponent* tagComp = entity->Add<TagComponent>();
        if (tagComp) {
            tagComp->SetTag(tag);
        }
    }
    
    return entity;
}

EntityPtr EntityFactory::CreatePlayer(Engine& engine,
                                     const String& tag,
                                     const Point3& position) {
    EntityPtr entity = CreatePhysicsObject(engine, tag, position, Point3(1, 2, 1), 75.0);  // 75kg player
    
    if (entity) {
        // Add input component for player control
        InputComponent* input = entity->Add<InputComponent>();
        if (input) {
            input->SetInputMode("player");
        }
    }
    
    return entity;
}

EntityPtr EntityFactory::CreateCamera(Engine& engine,
                                     const String& tag,
                                     const Point3& position) {
    EntityPtr entity = CreateGameObject(engine, tag, position, Point3(1, 1, 1));
    
    if (entity) {
        // Add a tag specifically for cameras
        TagComponent* tagComp = entity->Find<TagComponent>();
        if (tagComp) {
            tagComp->SetGroup("camera");
        }
    }
    
    return entity;
}

EntityPtr EntityFactory::CreateLight(Engine& engine,
                                    const String& tag,
                                    const Point3& position) {
    EntityPtr entity = CreateGameObject(engine, tag, position, Point3(1, 1, 1));
    
    if (entity) {
        // Add a tag specifically for lights
        TagComponent* tagComp = entity->Find<TagComponent>();
        if (tagComp) {
            tagComp->SetGroup("light");
        }
    }
    
    return entity;
}

// EntityQuery implementation
EntityQuery::EntityQuery(Engine& engine) : engine(engine) {
}

Vector<EntityPtr> EntityQuery::WithTag(const String& tag) {
    Vector<EntityPtr> result;
    
    auto* entityManagerSys = engine.Get<EntityManagerSystem>();
    if (entityManagerSys) {
        result = entityManagerSys->FindEntitiesByTag(tag);
    }
    
    return result;
}

// EntityManager implementation
EntityManager::EntityManager(Engine& engine) : engine(engine) {
}

EntityManager::~EntityManager() {
    Clear();
}

bool EntityManager::Initialize() {
    // Ensure EntityManagerSystem exists
    auto* entityManagerSys = engine.Get<EntityManagerSystem>();
    if (!entityManagerSys) {
        entityManagerSys = engine.Add<EntityManagerSystem>();
    }
    
    return true;
}

void EntityManager::Update(double dt) {
    // The systems handle updates, but we could add entity-specific updates here
}

EntityPtr EntityManager::CreateEntity(const String& name) {
    auto* entityManagerSys = engine.Get<EntityManagerSystem>();
    if (!entityManagerSys) {
        entityManagerSys = engine.Add<EntityManagerSystem>();
    }
    
    EntityPtr entity = entityManagerSys->CreateEntity(name);
    if (entity) {
        entities.Add(entity);
        entity_count++;
    }
    
    return entity;
}

EntityPtr EntityManager::CreateGameObject(const String& tag,
                                         const Point3& position,
                                         const Point3& scale) {
    auto* entityManagerSys = engine.Get<EntityManagerSystem>();
    if (!entityManagerSys) {
        entityManagerSys = engine.Add<EntityManagerSystem>();
    }
    
    EntityPtr entity = entityManagerSys->CreateGameObject(tag, position, scale);
    if (entity) {
        entities.Add(entity);
        entity_count++;
    }
    
    return entity;
}

EntityPtr EntityManager::CreatePhysicsObject(const String& tag,
                                            const Point3& position,
                                            const Point3& scale,
                                            double mass) {
    EntityPtr entity = CreateGameObject(tag, position, scale);
    
    if (entity) {
        PhysicsComponent* physics = entity->Add<PhysicsComponent>();
        if (physics) {
            physics->SetMass(mass);
        }
    }
    
    return entity;
}

EntityPtr EntityManager::FindEntityById(int64 id) {
    // Note: We're using the ID as a simple index here, in a real system you'd want a proper ID mapping
    // For now, just return the first entity that exists
    for (auto& entity : entities) {
        if (entity && entity->IsEnabled()) {
            if (entity->idx == id) {
                return entity;
            }
        }
    }
    
    return nullptr;
}

Vector<EntityPtr> EntityManager::FindEntitiesByTag(const String& tag) {
    Vector<EntityPtr> result;
    
    for (auto& entity : entities) {
        if (entity && entity->IsEnabled()) {
            TagComponent* tagComp = entity->Find<TagComponent>();
            if (tagComp && tagComp->GetTag() == tag) {
                result.Add(entity);
            }
        }
    }
    
    return result;
}

void EntityManager::RemoveEntity(EntityPtr entity) {
    if (!entity) return;
    
    int idx = entities.Find(entity);
    if (idx >= 0) {
        auto* entityManagerSys = engine.Get<EntityManagerSystem>();
        if (entityManagerSys) {
            entityManagerSys->RemoveEntity(entity);
        }
        
        entities.Remove(idx);
        entity_count--;
    }
}

void EntityManager::Clear() {
    entities.Clear();
    entity_count = 0;
}

void EntityManager::RegisterComponentTypes() {
    // Register the game-specific components with the ECS factory system
    // This would typically be done during engine initialization
}

END_UPP_NAMESPACE