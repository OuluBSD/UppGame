#include "GameEngine.h"
#include "Stage.h"

NAMESPACE_UPP

Stage::Stage() : stageWidth(800), stageHeight(600)
{
    viewport = Rect(0, 0, stageWidth, stageHeight);
    x = 0;
    y = 0;
    width = stageWidth;
    height = stageHeight;
    visible = true;
    touchable = true;
    eventDispatcher = std::make_shared<EventDispatcher>();
}

Stage::Stage(int width, int height) : stageWidth(width), stageHeight(height)
{
    viewport = Rect(0, 0, stageWidth, stageHeight);
    x = 0;
    y = 0;
    this->width = stageWidth;
    this->height = stageHeight;
    visible = true;
    touchable = true;
    eventDispatcher = std::make_shared<EventDispatcher>();
}

Stage::~Stage()
{
    Clear();
}

void Stage::AddActor(std::shared_ptr<Actor> actor) 
{
    if (!actor) return;
    
    // Check if actor is already in this stage
    for (int i = 0; i < actors.GetCount(); i++) {
        if (actors[i] == actor) return;
    }
    
    actors.Add(actor);
    actor->SetParent(shared_from_this());
    
    // Sort actors by z-index
    SortActors();
}

void Stage::RemoveActor(std::shared_ptr<Actor> actor) 
{
    if (!actor) return;
    
    for (int i = 0; i < actors.GetCount(); i++) {
        if (actors[i] == actor) {
            // If this actor has focus, clear the focus
            if (keyboardFocus == actor) keyboardFocus = nullptr;
            if (scrollFocus == actor) scrollFocus = nullptr;
            
            actors.Remove(i);
            actor->SetParent(nullptr);
            return;
        }
    }
}

void Stage::Clear() 
{
    keyboardFocus = nullptr;
    scrollFocus = nullptr;
    
    for (int i = 0; i < actors.GetCount(); i++) {
        actors[i]->SetParent(nullptr);
    }
    
    actors.Clear();
}

std::shared_ptr<Actor> Stage::FindActor(const String& name) const 
{
    for (int i = 0; i < actors.GetCount(); i++) {
        if (actors[i]->GetName() == name) {
            return actors[i];
        }
        
        // If direct child doesn't match, recursively search in its children
        // (Assuming there's a way to get children of an actor)
        // This would require extending the Actor class to support child management
    }
    return nullptr;
}

void Stage::Act(double delta) 
{
    // Update this stage first
    Actor::Act(delta);
    
    // Update all root actors
    for (int i = 0; i < actors.GetCount(); i++) {
        if (actors[i]->IsVisible()) {
            actors[i]->Act(delta);
        }
    }
}

void Stage::Draw(Draw& draw) 
{
    if (!visible) return;
    
    // Draw all visible root actors
    for (int i = 0; i < actors.GetCount(); i++) {
        if (actors[i]->IsVisible()) {
            actors[i]->Draw(draw, 1.0); // parentAlpha is 1.0 at stage level
        }
    }
}

bool Stage::TouchDown(int screenX, int screenY, int pointer, int button) 
{
    // Call the parent implementation first
    Actor::TouchDown(screenX, screenY, pointer, button);
    
    // Find actor at touch position
    std::shared_ptr<Actor> target = Hit(screenX, screenY, true);
    
    if (target) {
        // Deliver event to the target
        return target->TouchDown(screenX, screenY, pointer, button);
    }
    
    return false;
}

void Stage::TouchUp(int screenX, int screenY, int pointer, int button) 
{
    // Call the parent implementation first
    Actor::TouchUp(screenX, screenY, pointer, button);
    
    // Find actor at touch position
    std::shared_ptr<Actor> target = Hit(screenX, screenY, true);
    
    if (target) {
        // Deliver event to the target
        target->TouchUp(screenX, screenY, pointer, button);
    }
}

void Stage::TouchDragged(int screenX, int screenY, int pointer) 
{
    // Call the parent implementation first
    Actor::TouchDragged(screenX, screenY, pointer);
    
    // For drag events, we might continue with the same actor that received the touch down
    // This would require tracking the actor that received the initial touch down
}

bool Stage::MouseMoved(int screenX, int screenY) 
{
    // Call the parent implementation first
    Actor::MouseMoved(screenX, screenY);
    
    // Find actor at mouse position
    std::shared_ptr<Actor> target = Hit(screenX, screenY, true);
    
    if (target) {
        // Deliver event to the target
        return target->MouseMoved(screenX, screenY);
    }
    
    return false;
}

bool Stage::Scrolled(int amount) 
{
    // Call the parent implementation first
    Actor::Scrolled(amount);
    
    // Try delivering to scroll focus first
    if (scrollFocus && scrollFocus->IsVisible() && scrollFocus->IsTouchable()) {
        if (scrollFocus->Scrolled(amount)) {
            return true;
        }
    }
    
    // Otherwise, try the actor at the mouse position
    // For this, we'd need to track the current mouse position
    // For now, we'll just return false
    return false;
}

void Stage::SetSize(int width, int height) 
{
    stageWidth = width;
    stageHeight = height;
    this->width = width;
    this->height = height;
    UpdateViewport();
}

void Stage::SetViewport(int x, int y, int width, int height) 
{
    viewport = Rect(x, y, width, height);
}

void Stage::SetKeyboardFocus(std::shared_ptr<Actor> actor) 
{
    keyboardFocus = actor;
}

void Stage::SetScrollFocus(std::shared_ptr<Actor> actor) 
{
    scrollFocus = actor;
}

std::shared_ptr<Actor> Stage::Hit(int x, int y, bool touchable) const 
{
    // Check if the stage itself is hit first (for input processing)
    if (!Contains(x, y)) return nullptr;
    
    // Check child actors in reverse order (last added/most on top first)
    for (int i = actors.GetCount() - 1; i >= 0; i--) {
        std::shared_ptr<Actor> actor = actors[i];
        
        if (!actor->IsVisible()) continue;
        if (touchable && !actor->IsTouchable()) continue;
        
        // Convert stage coordinates to actor's local coordinates
        Point localCoords = actor->StageToLocalCoordinates(Point(x, y));
        
        // Check if the point is within the actor
        if (actor->Contains(localCoords.x, localCoords.y)) {
            // Recursively check children of this actor
            std::shared_ptr<Actor> childHit = actor->HandleEvent(nullptr); // This would need to be implemented differently
            if (childHit) return childHit;
            
            return actor;
        }
    }
    
    return nullptr;
}

void Stage::UpdateViewport() 
{
    viewport = Rect(0, 0, stageWidth, stageHeight);
}

void Stage::SortActors() 
{
    // Simple bubble sort by z-index
    for (int i = 0; i < actors.GetCount() - 1; i++) {
        for (int j = 0; j < actors.GetCount() - 1 - i; j++) {
            if (actors[j]->GetZIndex() > actors[j + 1]->GetZIndex()) {
                std::swap(actors[j], actors[j + 1]);
            }
        }
    }
}

bool Stage::DeliverEvent(std::shared_ptr<Actor> target, std::shared_ptr<Event> event) 
{
    if (!target || !event) return false;
    
    event->SetTarget(target);
    
    bool handled = target->HandleEvent(event);
    
    // If event bubbles and wasn't handled, try parents
    if (!handled && event->GetBubbles()) {
        auto parent = target->GetParent();
        while (parent) {
            event->SetTarget(parent);
            handled = parent->HandleEvent(event);
            if (handled) break;
            parent = parent->GetParent();
        }
    }
    
    return handled;
}

bool Stage::DispatchEvent(std::shared_ptr<UIEvent> event) {
    if (!event || !eventDispatcher) return false;

    // Find the target actor for this event
    if (!event->GetTarget()) {
        // If no target is set, try to find an actor at the event coordinates
        Point pos = event->GetTouchPosition();
        std::shared_ptr<Actor> target = Hit(pos.x, pos.y, true);
        if (target) {
            event->SetTarget(target);
        }
    }

    return eventDispatcher->DispatchEvent(event);
}

bool Stage::DeliverUIEvent(std::shared_ptr<Actor> target, std::shared_ptr<UIEvent> event) {
    if (!target || !event) return false;

    event->SetTarget(target);
    return target->HandleUIEvent(event);
}

END_UPP_NAMESPACE