#ifndef UPP_STAGE_H
#define UPP_STAGE_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <GameLib/GameLib.h>
#include <GameEngine/Actor.h>
#include <Vector/Vector.h>
#include <memory>

NAMESPACE_UPP

// Stage class - similar to libgdx's Stage
// Manages a hierarchy of actors and handles input events
class Stage : public Actor {
public:
    Stage();
    Stage(int width, int height);
    virtual ~Stage();

    // Actor management
    void AddActor(std::shared_ptr<Actor> actor);
    void RemoveActor(std::shared_ptr<Actor> actor);
    void Clear();
    int GetActorCount() const { return actors.GetCount(); }
    std::shared_ptr<Actor> GetActor(int index) const { return actors[index]; }

    // Get root actors (actors with no parent)
    const Vector<std::shared_ptr<Actor>>& GetRootActors() const { return actors; }

    // Find actor by name (recursive)
    std::shared_ptr<Actor> FindActor(const String& name) const;

    // Stage lifecycle methods
    void Act(double delta) override; // Update all actors
    void Draw(Draw& draw) override; // Draw all visible actors

    // Input handling
    bool TouchDown(int screenX, int screenY, int pointer, int button) override;
    void TouchUp(int screenX, int screenY, int pointer, int button) override;
    void TouchDragged(int screenX, int screenY, int pointer) override;
    bool MouseMoved(int screenX, int screenY) override;
    bool Scrolled(int amount) override;

    // Stage-specific properties
    void SetSize(int width, int height);
    int GetWidth() const { return stageWidth; }
    int GetHeight() const { return stageHeight; }

    void SetViewport(int x, int y, int width, int height);
    Rect GetViewport() const { return viewport; }

    // Focus management (for keyboard input)
    void SetKeyboardFocus(std::shared_ptr<Actor> actor);
    std::shared_ptr<Actor> GetKeyboardFocus() const { return keyboardFocus; }

    void SetScrollFocus(std::shared_ptr<Actor> actor);
    std::shared_ptr<Actor> GetScrollFocus() const { return scrollFocus; }

    // Get the root actor (this stage itself)
    std::shared_ptr<Actor> GetRoot() { return shared_from_this(); }

    // Picking - find actor at given coordinates
    std::shared_ptr<Actor> Hit(int x, int y, bool touchable) const;

    // Event system integration
    void SetEventDispatcher(std::shared_ptr<EventDispatcher> dispatcher) { this->eventDispatcher = dispatcher; }
    std::shared_ptr<EventDispatcher> GetEventDispatcher() const { return eventDispatcher; }
    bool DispatchEvent(std::shared_ptr<UIEvent> event);

protected:
    // List of root actors in the stage (actors that are directly added to the stage)
    Vector<std::shared_ptr<Actor>> actors;

    // Stage dimensions
    int stageWidth = 0;
    int stageHeight = 0;

    // Viewport for rendering
    Rect viewport;

    // Focus actors
    std::shared_ptr<Actor> keyboardFocus;
    std::shared_ptr<Actor> scrollFocus;

    // Input state
    Vector<bool> touchDown;
    Vector<Point> touchPos;

    // Event system
    std::shared_ptr<EventDispatcher> eventDispatcher;

    // Helper methods
    void UpdateViewport();
    void SortActors();
    bool DeliverEvent(std::shared_ptr<Actor> target, std::shared_ptr<Event> event);
    bool DeliverUIEvent(std::shared_ptr<Actor> target, std::shared_ptr<UIEvent> event);
};

END_UPP_NAMESPACE

#endif