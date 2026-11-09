#include "GameEngine.h"
#include "Group.h"

NAMESPACE_UPP

Group::Group()
{
    // Initialize as an Actor that can contain other actors
}

Group::~Group()
{
    Clear(); // Clean up all children
}

void Group::AddActor(std::shared_ptr<Actor> actor) 
{
    if (!actor) return;
    
    // Check if actor is already a child
    for (int i = 0; i < children.GetCount(); i++) {
        if (children[i] == actor) return;
    }
    
    children.Add(actor);
    actor->SetParent(shared_from_this());
}

void Group::RemoveActor(std::shared_ptr<Actor> actor) 
{
    if (!actor) return;
    
    for (int i = 0; i < children.GetCount(); i++) {
        if (children[i] == actor) {
            children.Remove(i);
            actor->SetParent(nullptr);
            return;
        }
    }
}

void Group::RemoveActorAt(int index) 
{
    if (index >= 0 && index < children.GetCount()) {
        children[index]->SetParent(nullptr);
        children.Remove(index);
    }
}

void Group::Clear() 
{
    // Remove parent reference from all children
    for (int i = 0; i < children.GetCount(); i++) {
        children[i]->SetParent(nullptr);
    }
    children.Clear();
}

bool Group::HasChild(std::shared_ptr<Actor> actor) const 
{
    if (!actor) return false;
    
    for (int i = 0; i < children.GetCount(); i++) {
        if (children[i] == actor) {
            return true;
        }
    }
    return false;
}

int Group::IndexOf(std::shared_ptr<Actor> actor) const 
{
    if (!actor) return -1;
    
    for (int i = 0; i < children.GetCount(); i++) {
        if (children[i] == actor) {
            return i;
        }
    }
    return -1;
}

std::shared_ptr<Actor> Group::FindActor(const String& name) const 
{
    for (int i = 0; i < children.GetCount(); i++) {
        if (children[i]->GetName() == name) {
            return children[i];
        }
    }
    return nullptr;
}

void Group::Act(double delta) 
{
    // Update this group first
    Actor::Act(delta);
    
    // Update all children
    for (int i = 0; i < children.GetCount(); i++) {
        if (children[i]->IsVisible()) {
            children[i]->Act(delta);
        }
    }
}

void Group::Draw(Draw& draw, double parentAlpha) 
{
    if (!visible) return;
    
    // If clipping is enabled, we might want to set up a clipping rectangle here
    if (clip) {
        // Implementation for clipping would go here
        // This would depend on the Draw API's clipping capabilities
    }
    
    // Draw all visible children
    for (int i = 0; i < children.GetCount(); i++) {
        if (children[i]->IsVisible()) {
            // Apply the parent alpha to the child's rendering
            children[i]->Draw(draw, parentAlpha);
        }
    }
}

bool Group::TouchDown(int screenX, int screenY, int pointer, int button) 
{
    // Call parent implementation
    Actor::TouchDown(screenX, screenY, pointer, button);
    
    // Check if this group should handle the touch event first
    // (for example, if it has specific touch behavior)
    bool handled = Actor::TouchDown(screenX, screenY, pointer, button);
    if (handled) return true;
    
    // Propagate to children in reverse order (topmost first)
    for (int i = children.GetCount() - 1; i >= 0; i--) {
        std::shared_ptr<Actor> child = children[i];
        
        if (!child->IsVisible() || !child->IsTouchable()) continue;
        
        // Convert screen coordinates to child's local coordinates
        Point localCoords = child->ParentToLocalCoordinates(Point(screenX, screenY));
        
        // Check if the touch is within the child's bounds
        if (child->Contains(localCoords.x, localCoords.y)) {
            if (child->TouchDown(localCoords.x, localCoords.y, pointer, button)) {
                return true; // Event handled by child
            }
        }
    }
    
    return false;
}

void Group::TouchUp(int screenX, int screenY, int pointer, int button) 
{
    // Call parent implementation
    Actor::TouchUp(screenX, screenY, pointer, button);
    
    // Propagate to children in reverse order (topmost first)
    for (int i = children.GetCount() - 1; i >= 0; i--) {
        std::shared_ptr<Actor> child = children[i];
        
        if (!child->IsVisible() || !child->IsTouchable()) continue;
        
        // Convert screen coordinates to child's local coordinates
        Point localCoords = child->ParentToLocalCoordinates(Point(screenX, screenY));
        
        // Check if the touch is within the child's bounds
        if (child->Contains(localCoords.x, localCoords.y)) {
            child->TouchUp(localCoords.x, localCoords.y, pointer, button);
            return; // Event processed
        }
    }
}

void Group::TouchDragged(int screenX, int screenY, int pointer) 
{
    // Call parent implementation
    Actor::TouchDragged(screenX, screenY, pointer);
    
    // For drag events, we would typically continue with the same actor that received the initial touch down
    // For simplicity here, we'll skip propagation unless specifically needed
}

bool Group::MouseMoved(int screenX, int screenY) 
{
    // Call parent implementation
    Actor::MouseMoved(screenX, screenY);
    
    // Propagate to children in reverse order (topmost first)
    for (int i = children.GetCount() - 1; i >= 0; i--) {
        std::shared_ptr<Actor> child = children[i];
        
        if (!child->IsVisible() || !child->IsTouchable()) continue;
        
        // Convert screen coordinates to child's local coordinates
        Point localCoords = child->ParentToLocalCoordinates(Point(screenX, screenY));
        
        // Check if the mouse is within the child's bounds
        if (child->Contains(localCoords.x, localCoords.y)) {
            if (child->MouseMoved(localCoords.x, localCoords.y)) {
                return true; // Event handled by child
            }
        }
    }
    
    return false;
}

bool Group::Scrolled(int amount) 
{
    // Call parent implementation
    Actor::Scrolled(amount);
    
    // Propagate to children in reverse order (topmost first)
    for (int i = children.GetCount() - 1; i >= 0; i--) {
        std::shared_ptr<Actor> child = children[i];
        
        if (!child->IsVisible() || !child->IsTouchable()) continue;
        
        // For scroll events, we might deliver to the child under the mouse cursor
        // Or to a specifically focused child
        // For now, we'll just deliver to the topmost child under the cursor
        Point mousePos = GetPosition(); // In a real implementation, you'd have the actual mouse position
        Point localCoords = child->ParentToLocalCoordinates(mousePos);
        
        if (child->Contains(localCoords.x, localCoords.y)) {
            if (child->Scrolled(amount)) {
                return true; // Event handled by child
            }
        }
    }
    
    return false;
}

Point Group::ActorToStageCoordinates(std::shared_ptr<Actor> child, const Point& actorCoords) const 
{
    if (!child) return actorCoords;
    
    // Transform from child's local coordinates to this group's coordinates
    Point groupCoords = child->LocalToParentCoordinates(actorCoords);
    
    // Then transform from this group's coordinates to stage coordinates
    return LocalToStageCoordinates(groupCoords);
}

Point Group::StageToActorCoordinates(std::shared_ptr<Actor> child, const Point& stageCoords) const 
{
    if (!child) return stageCoords;
    
    // Transform from stage coordinates to this group's coordinates
    Point groupCoords = StageToLocalCoordinates(stageCoords);
    
    // Then transform from this group's coordinates to child's local coordinates
    return child->ParentToLocalCoordinates(groupCoords);
}

std::shared_ptr<Actor> Group::FindActorAt(float x, float y, bool touchable) const 
{
    // Check children in reverse order (topmost first)
    for (int i = children.GetCount() - 1; i >= 0; i--) {
        std::shared_ptr<Actor> child = children[i];
        
        if (!child->IsVisible()) continue;
        if (touchable && !child->IsTouchable()) continue;
        
        // Convert the point to the child's local coordinates
        Point localCoords = child->ParentToLocalCoordinates(Point((int)x, (int)y));
        
        if (child->Contains(localCoords.x, localCoords.y)) {
            // Recursively check if there's a more specific child at this point
            std::shared_ptr<Group> childGroup = std::dynamic_pointer_cast<Group>(child);
            if (childGroup) {
                std::shared_ptr<Actor> nestedActor = childGroup->FindActorAt(localCoords.x, localCoords.y, touchable);
                if (nestedActor) return nestedActor;
            }
            
            return child;
        }
    }
    
    return nullptr;
}

END_UPP_NAMESPACE