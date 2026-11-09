#include "Systems.h"
#include <Eon/Ecs/Ecs.h>

NAMESPACE_UPP

// TransformSystem implementation
bool TransformSystem::Initialize(const WorldState& ws) {
    GameSystem::Initialize(ws);
    return true;
}

void TransformSystem::Uninitialize() {
    GameSystem::Uninitialize();
}

void TransformSystem::Update(double dt) {
    // Update any transform-specific logic here
    // For now, just maintain the component list
    for (auto& comp : transform_components) {
        if (!comp || !comp->IsEnabled()) continue;
        // Apply any transform updates if needed
    }
}

void TransformSystem::Add(Component& comp) {
    if (auto* tcomp = dynamic_cast<TransformComponent*>(&comp)) {
        transform_components.Add(tcomp);
    }
}

void TransformSystem::Remove(Component& comp) {
    if (auto* tcomp = dynamic_cast<TransformComponent*>(&comp)) {
        int idx = transform_components.Find(tcomp);
        if (idx >= 0) {
            transform_components.Remove(idx);
        }
    }
}

// RenderSystem implementation
bool RenderSystem::Initialize(const WorldState& ws) {
    GameSystem::Initialize(ws);
    return true;
}

void RenderSystem::Uninitialize() {
    GameSystem::Uninitialize();
}

void RenderSystem::Update(double dt) {
    // Render all entities with RenderComponent
    for (auto& comp : render_components) {
        if (!comp || !comp->IsEnabled() || !comp->IsVisible()) continue;
        
        // Here we would integrate with the rendering pipeline
        // For now, just process the component
        TransformComponent* transform = comp->GetEntity()->Find<TransformComponent>();
        if (transform) {
            // Use transform data for rendering
            Matrix4 model_matrix = transform->GetTransformationMatrix();
            // Rendering would happen here using model_matrix
        }
    }
}

void RenderSystem::Add(Component& comp) {
    if (auto* rcomp = dynamic_cast<RenderComponent*>(&comp)) {
        render_components.Add(rcomp);
    }
}

void RenderSystem::Remove(Component& comp) {
    if (auto* rcomp = dynamic_cast<RenderComponent*>(&comp)) {
        int idx = render_components.Find(rcomp);
        if (idx >= 0) {
            render_components.Remove(idx);
        }
    }
}

// PhysicsSystem implementation
bool PhysicsSystem::Initialize(const WorldState& ws) {
    GameSystem::Initialize(ws);
    return true;
}

void PhysicsSystem::Uninitialize() {
    GameSystem::Uninitialize();
}

void PhysicsSystem::Update(double dt) {
    accumulated_time += dt;
    
    // Use a fixed timestep for physics to ensure consistency
    while (accumulated_time >= fixed_timestep) {
        accumulated_time -= fixed_timestep;
        
        for (auto& comp : physics_components) {
            if (!comp || !comp->IsEnabled() || comp->IsStatic()) continue;
            
            // Skip kinematic objects which are controlled by animation/transform
            if (comp->IsKinematic()) continue;
            
            TransformComponent* transform = comp->GetEntity()->Find<TransformComponent>();
            if (transform) {
                // Simple physics integration
                Vector3 acceleration = comp->GetAcceleration();
                Vector3 velocity = comp->GetVelocity();
                
                // Apply gravity if not specified otherwise
                if (acceleration.GetLength() == 0) {
                    acceleration = Vector3(0, -9.81, 0); // Earth gravity
                }
                
                // Update velocity and position
                velocity += acceleration * fixed_timestep;
                Point3 position = transform->GetPosition();
                position = position + velocity * fixed_timestep;
                
                transform->SetPosition(position);
                comp->SetVelocity(velocity);
            }
        }
    }
}

void PhysicsSystem::Add(Component& comp) {
    if (auto* pcomp = dynamic_cast<PhysicsComponent*>(&comp)) {
        physics_components.Add(pcomp);
    }
}

void PhysicsSystem::Remove(Component& comp) {
    if (auto* pcomp = dynamic_cast<PhysicsComponent*>(&comp)) {
        int idx = physics_components.Find(pcomp);
        if (idx >= 0) {
            physics_components.Remove(idx);
        }
    }
}

// AnimationSystem implementation
bool AnimationSystem::Initialize(const WorldState& ws) {
    GameSystem::Initialize(ws);
    return true;
}

void AnimationSystem::Uninitialize() {
    GameSystem::Uninitialize();
}

void AnimationSystem::Update(double dt) {
    for (auto& comp : animation_components) {
        if (!comp || !comp->IsEnabled() || !comp->IsPlaying()) continue;
        
        // Update animation time
        double currentTime = comp->GetTime();
        currentTime += dt * comp->GetPlaybackSpeed();
        comp->SetTime(currentTime);
        
        // For now, just update the time
        // In a real system, we would interpolate animation frames
    }
}

void AnimationSystem::Add(Component& comp) {
    if (auto* acomp = dynamic_cast<AnimationComponent*>(&comp)) {
        animation_components.Add(acomp);
    }
}

void AnimationSystem::Remove(Component& comp) {
    if (auto* acomp = dynamic_cast<AnimationComponent*>(&comp)) {
        int idx = animation_components.Find(acomp);
        if (idx >= 0) {
            animation_components.Remove(idx);
        }
    }
}

// AudioSystem implementation
bool AudioSystem::Initialize(const WorldState& ws) {
    GameSystem::Initialize(ws);
    return true;
}

void AudioSystem::Uninitialize() {
    GameSystem::Uninitialize();
}

void AudioSystem::Update(double dt) {
    for (auto& comp : audio_components) {
        if (!comp || !comp->IsEnabled()) continue;
        
        // Update audio states
        // In a real implementation, this would interact with audio APIs
        if (comp->IsPlaying()) {
            // Audio playback logic would go here
        }
    }
}

void AudioSystem::Add(Component& comp) {
    if (auto* acomp = dynamic_cast<AudioComponent*>(&comp)) {
        audio_components.Add(acomp);
    }
}

void AudioSystem::Remove(Component& comp) {
    if (auto* acomp = dynamic_cast<AudioComponent*>(&comp)) {
        int idx = audio_components.Find(acomp);
        if (idx >= 0) {
            audio_components.Remove(idx);
        }
    }
}

// InputSystem implementation
bool InputSystem::Initialize(const WorldState& ws) {
    GameSystem::Initialize(ws);
    return true;
}

void InputSystem::Uninitialize() {
    GameSystem::Uninitialize();
}

void InputSystem::Update(double dt) {
    for (auto& comp : input_components) {
        if (!comp || !comp->IsEnabled()) continue;
        
        // Handle input logic
        // In a real system, this would process input events
    }
}

void InputSystem::Add(Component& comp) {
    if (auto* icomp = dynamic_cast<InputComponent*>(&comp)) {
        input_components.Add(icomp);
    }
}

void InputSystem::Remove(Component& comp) {
    if (auto* icomp = dynamic_cast<InputComponent*>(&comp)) {
        int idx = input_components.Find(icomp);
        if (idx >= 0) {
            input_components.Remove(idx);
        }
    }
}

// UISystem implementation
bool UISystem::Initialize(const WorldState& ws) {
    GameSystem::Initialize(ws);
    return true;
}

void UISystem::Uninitialize() {
    GameSystem::Uninitialize();
}

void UISystem::Update(double dt) {
    for (auto& comp : ui_components) {
        if (!comp || !comp->IsEnabled() || !comp->IsVisible()) continue;
        
        // Update UI elements
        // In a real system, this would render UI components
    }
}

void UISystem::Add(Component& comp) {
    if (auto* uicomp = dynamic_cast<UIComponent*>(&comp)) {
        ui_components.Add(uicomp);
    }
}

void UISystem::Remove(Component& comp) {
    if (auto* uicomp = dynamic_cast<UIComponent*>(&comp)) {
        int idx = ui_components.Find(uicomp);
        if (idx >= 0) {
            ui_components.Remove(idx);
        }
    }
}

// EntityManagerSystem implementation
bool EntityManagerSystem::Initialize(const WorldState& ws) {
    GameSystem::Initialize(ws);
    return true;
}

void EntityManagerSystem::Uninitialize() {
    GameSystem::Uninitialize();
}

void EntityManagerSystem::Update(double dt) {
    // Update all entities
    for (auto& entity : entities) {
        if (entity && entity->IsEnabled()) {
            // Entity update logic would go here
        }
    }
}

EntityPtr EntityManagerSystem::CreateEntity(const String& name) {
    Entity& entity = val.Add<Entity>();
    entity.SetIdx(Entity::GetNextIdx());
    entity.val.id = name;
    
    EntityPtr ptr = &entity;
    entities.Add(ptr);
    
    return ptr;
}

EntityPtr EntityManagerSystem::CreateGameObject(const String& tag,
                                               const Point3& position,
                                               const Point3& scale) {
    EntityPtr entity = CreateEntity(tag);
    
    if (entity) {
        // Add default components for a game object
        TransformComponent* transform = entity->Add<TransformComponent>();
        if (transform) {
            transform->SetPosition(position);
            transform->SetScale(scale);
        }
        
        RenderComponent* render = entity->Add<RenderComponent>();
        if (render) {
            render->SetVisible(true);
        }
        
        TagComponent* tagComp = entity->Add<TagComponent>();
        if (tagComp) {
            tagComp->SetTag(tag);
        }
    }
    
    return entity;
}

Vector<EntityPtr> EntityManagerSystem::FindEntitiesByTag(const String& tag) {
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

void EntityManagerSystem::RemoveEntity(EntityPtr entity) {
    if (!entity) return;
    
    int idx = entities.Find(entity);
    if (idx >= 0) {
        entities.Remove(idx);
    }
}

END_UPP_NAMESPACE