#ifndef UPP_UIEVENT_H
#define UPP_UIEVENT_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <GameLib/GameLib.h>
#include <GameEngine/Actor.h>
#include <functional>

NAMESPACE_UPP

// Event types for UI interactions
enum class EventType {
    TOUCH_DOWN,
    TOUCH_UP,
    TOUCH_DRAGGED,
    MOUSE_MOVED,
    SCROLLED,
    KEY_DOWN,
    KEY_UP,
    KEY_TYPED,
    RESIZE,
    SHOW,
    HIDE,
    PAUSE,
    RESUME,
    DISPOSE
};

// UI Event class - more comprehensive than the basic Event in Actor
class UIEvent {
public:
    UIEvent(EventType type);
    virtual ~UIEvent() = default;

    // Get/set event type
    EventType GetType() const { return type; }
    void SetType(EventType newType) { type = newType; }

    // Get/set target actor
    void SetTarget(std::shared_ptr<Actor> target) { this->target = target; }
    std::shared_ptr<Actor> GetTarget() const { return target.lock(); }

    // Get/set listener
    void SetListener(std::shared_ptr<Actor> listener) { this->listener = listener; }
    std::shared_ptr<Actor> GetListener() const { return listener.lock(); }

    // Capture phase (handled at target before bubbling up)
    void SetCapture(boolean capture) { this->capture = capture; }
    boolean IsCapture() const { return capture; }

    // Bubbling (event propagates up the hierarchy after target)
    void SetBubbles(boolean bubbles) { this->bubbles = bubbles; }
    boolean GetBubbles() const { return bubbles; }

    // Event status
    boolean IsHandled() const { return handled; }
    void Handle() { handled = true; }
    void Stop() { stopped = true; }  // Stop propagation
    boolean IsStopped() const { return stopped; }

    // Coordinates
    void SetTouchPosition(int x, int y) { touchX = x; touchY = y; }
    Point GetTouchPosition() const { return Point(touchX, touchY); }
    int GetTouchX() const { return touchX; }
    int GetTouchY() const { return touchY; }

    // Input information
    void SetPointer(int pointer) { this->pointer = pointer; }  // For multi-touch
    int GetPointer() const { return pointer; }

    void SetButton(int button) { this->button = button; }  // Mouse button
    int GetButton() const { return button; }

    void SetScrollAmount(int amount) { this->scrollAmount = amount; }
    int GetScrollAmount() const { return scrollAmount; }

    // Time information
    void SetTimeStamp(dword timestamp) { this->timestamp = timestamp; }
    dword GetTimeStamp() const { return timestamp; }

    // Cancel the event
    void Cancel() { cancelled = true; }
    boolean IsCancelled() const { return cancelled; }
    boolean IsCancelable() const { return cancelable; }
    void SetCancelable(boolean cancelable) { this->cancelable = cancelable; }

protected:
    EventType type;
    std::weak_ptr<Actor> target;
    std::weak_ptr<Actor> listener;
    boolean capture = false;
    boolean bubbles = false;
    boolean handled = false;
    boolean stopped = false;
    boolean cancelled = false;
    boolean cancelable = true;
    int touchX = 0, touchY = 0;
    int pointer = 0;
    int button = 0;
    int scrollAmount = 0;
    dword timestamp = 0;
};

// Event listener interface
class EventListener {
public:
    virtual ~EventListener() = default;

    virtual bool HandleEvent(std::shared_ptr<UIEvent> event) = 0;
};

// Touch event listener
class TouchEventListener : public EventListener {
public:
    virtual bool TouchDown(std::shared_ptr<UIEvent> event) { return false; }
    virtual void TouchUp(std::shared_ptr<UIEvent> event) {}
    virtual void TouchDragged(std::shared_ptr<UIEvent> event) {}
    virtual bool MouseMoved(std::shared_ptr<UIEvent> event) { return false; }
    virtual bool Scrolled(std::shared_ptr<UIEvent> event) { return false; }

    bool HandleEvent(std::shared_ptr<UIEvent> event) override {
        switch (event->GetType()) {
            case EventType::TOUCH_DOWN:
                return TouchDown(event);
            case EventType::TOUCH_UP:
                TouchUp(event);
                return true;
            case EventType::TOUCH_DRAGGED:
                TouchDragged(event);
                return true;
            case EventType::MOUSE_MOVED:
                return MouseMoved(event);
            case EventType::SCROLLED:
                return Scrolled(event);
            default:
                return false;
        }
    }
};

// Event dispatcher/handler for the stage
class EventDispatcher {
public:
    EventDispatcher();
    virtual ~EventDispatcher();

    // Add/remove event listeners
    void AddListener(EventType type, std::function<bool(std::shared_ptr<UIEvent>)> listener);
    void RemoveListener(EventType type, std::function<bool(std::shared_ptr<UIEvent>)> listener);
    
    // Add typed event listener
    template<typename T>
    void AddTypedListener(EventType type, std::function<bool(std::shared_ptr<T>)> listener);

    // Dispatch an event
    bool DispatchEvent(std::shared_ptr<UIEvent> event);

    // Handle event bubbling
    bool BubbleEvent(std::shared_ptr<UIEvent> event);

protected:
    struct ListenerInfo {
        std::function<bool(std::shared_ptr<UIEvent>)> callback;
        std::shared_ptr<EventListener> listener;
    };

    HashMap<EventType, Vector<ListenerInfo>> listeners;
};

END_UPP_NAMESPACE

#endif