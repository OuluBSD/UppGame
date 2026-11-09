#include "EcsIntegration.h"
#include <Eon/Lib/Lib.h>

NAMESPACE_UPP

// GameEntityManager implementation
GameEntityManager::GameEntityManager(VfsValue& owner) : owner_value(&owner) {}

GameEntityManager::~GameEntityManager() {
    entities.Clear();
}

void GameEntityManager::AddEntity(EntityPtr entity) {
    if (entity) {
        entities.Add(entity);
    }
}

void GameEntityManager::RemoveEntity(EntityPtr entity) {
    if (entity) {
        int index = entities.Find(entity);
        if (index >= 0) {
            entities.Remove(index);
        }
    }
}

EntityPtr GameEntityManager::FindEntityByTag(const String& tag) {
    int index = FindEntityIndexByTag(tag);
    if (index >= 0) {
        return entities[index];
    }
    return nullptr;
}

Vector<EntityPtr> GameEntityManager::FindEntitiesByTag(const String& tag) {
    Vector<EntityPtr> result;
    for (EntityPtr& entity : entities) {
        if (TagComponent* tag_comp = entity->Find<TagComponent>()) {
            if (tag_comp->GetTag() == tag) {
                result.Add(entity);
            }
        }
    }
    return result;
}

void GameEntityManager::Update(double dt) {
    // Perform any entity-specific updates here
    // For now, we just maintain the entities
}

int GameEntityManager::FindEntityIndexByTag(const String& tag) {
    for (int i = 0; i < entities.GetCount(); i++) {
        if (TagComponent* tag_comp = entities[i]->Find<TagComponent>()) {
            if (tag_comp->GetTag() == tag) {
                return i;
            }
        }
    }
    return -1;
}

// GameEcsSystem implementation
bool GameEcsSystem::Initialize(const WorldState& ws) {
    // Initialize this system
    System::Initialize(ws);
    
    // Initialize internal systems
    InitializeSystems();
    
    // Create the entity manager
    entity_manager = std::make_shared<GameEntityManager>(val);
    
    return true;
}

void GameEcsSystem::Uninitialize() {
    entity_manager.reset();
    System::Uninitialize();
}

void GameEcsSystem::Update(double dt) {
    // Update all entities managed by the entity manager
    if (entity_manager) {
        entity_manager->Update(dt);
    }
}

void GameEcsSystem::InitializeSystems() {
    // Register components with the factory
    GameEcsIntegration::RegisterComponents();
    
    // Add required systems to the engine
    engine.GetAdd<RenderingSystem>();
    engine.GetAdd<EventSystem>();
    engine.GetAdd<PhysicsSystem>(val);  // Physics system needs VfsValue
    engine.GetAdd<InteractionSystem>();
    engine.GetAdd<GameEcsSystem>(val);
}

EntityPtr GameEcsSystem::CreateEntity(const String& name) {
    VfsValue& entity_value = val.Add();
    EntityPtr entity = new Entity(entity_value);
    entity_value.ext = entity;
    entity->SetIdx(Entity::GetNextIdx());
    
    // Add a tag component with the entity name
    if (TagComponent* tag_comp = entity->Add0<TagComponent>(ws)) {
        tag_comp->SetTag(name);
    }
    
    // Add to entity manager if available
    if (entity_manager) {
        entity_manager->AddEntity(entity);
    }
    
    return entity;
}

EntityPtr GameEcsSystem::CreateGameObject(const String& tag, 
                                         const Point3& position,
                                         const Point3& scale) {
    VfsValue& entity_value = val.Add();
    EntityPtr entity = new Entity(entity_value);
    entity_value.ext = entity;
    entity->SetIdx(Entity::GetNextIdx());
    
    // Add components
    if (TransformComponent* transform = entity->Add0<TransformComponent>(ws)) {
        transform->SetPosition(position);
        transform->SetScale(scale);
    }
    
    if (RenderComponent* render = entity->Add0<RenderComponent>(ws)) {
        // Set default model, texture, and material
        render->SetModel("default_model");
        render->SetTexture("default_texture");
        render->SetMaterial("default_material");
    }
    
    if (PhysicsComponent* physics = entity->Add0<PhysicsComponent>(ws)) {
        physics->SetMass(1.0);
        physics->SetKinematic(false);
    }
    
    if (TagComponent* tag_comp = entity->Add0<TagComponent>(ws)) {
        tag_comp->SetTag(tag);
    }
    
    // Add to entity manager
    if (entity_manager) {
        entity_manager->AddEntity(entity);
    }
    
    return entity;
}

// GameEcsIntegration implementation
GameEcsIntegration::GameEcsIntegration() {
    // Initialize the engine
    VfsValue& engine_val = val.Add();
    engine_val.ext = &engine;
    engine.val = engine_val;
}

GameEcsIntegration::~GameEcsIntegration() {
    ecs_system.reset();
    entity_manager.reset();
}

bool GameEcsIntegration::Initialize() {
    // Register components
    RegisterComponents();
    
    // Create the ECS system
    VfsValue& ecs_val = val.Add();
    ecs_system = std::make_shared<GameEcsSystem>(ecs_val);
    ecs_val.ext = ecs_system.get();
    
    // Create the entity manager
    entity_manager = std::make_shared<GameEntityManager>(ecs_val);
    
    // Add required systems to the main engine
    engine.GetAdd<RenderingSystem>();  // We might need to create VfsValue for these
    engine.GetAdd<EventSystem>();
    
    return true;
}

void GameEcsIntegration::Update(double dt) {
    if (ecs_system) {
        ecs_system->Update(dt);
    }
    
    if (entity_manager) {
        entity_manager->Update(dt);
    }
}

EntityPtr GameEcsIntegration::CreateEntity(const String& name) {
    if (ecs_system) {
        return ecs_system->CreateEntity(name);
    }
    return nullptr;
}

EntityPtr GameEcsIntegration::CreateGameObject(const String& tag, 
                                              const Point3& position,
                                              const Point3& scale) {
    if (ecs_system) {
        return ecs_system->CreateGameObject(tag, position, scale);
    }
    return nullptr;
}

void GameEcsIntegration::RegisterComponents() {
    // Register our custom components with the VFS factory
    static bool registered = false;
    if (registered) return;
    
    REGISTER_EON_COMPONENT(TransformComponent, "game.transform", "Game|ECS");
    REGISTER_EON_COMPONENT(RenderComponent, "game.render", "Game|ECS");
    REGISTER_EON_COMPONENT(PhysicsComponent, "game.physics", "Game|ECS");
    REGISTER_EON_COMPONENT(TagComponent, "game.tag", "Game|ECS");
    REGISTER_EON_COMPONENT(GameEcsSystem, "game.ecs_system", "Game|ECS");
    
    registered = true;
}

END_UPP_NAMESPACE