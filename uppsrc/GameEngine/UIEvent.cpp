#include "GameEngine.h"
#include "UIEvent.h"

NAMESPACE_UPP

UIEvent::UIEvent(EventType type) : type(type), handled(false), stopped(false), 
                                   touchX(0), touchY(0), pointer(0), button(0), 
                                   scrollAmount(0), timestamp(0), cancelled(false), 
                                   cancelable(true) {
    timestamp = GetTickCount();  // Use U++ time function
}

EventDispatcher::EventDispatcher() {
}

EventDispatcher::~EventDispatcher() {
}

void EventDispatcher::AddListener(EventType type, std::function<bool(std::shared_ptr<UIEvent>)> listener) {
    ListenerInfo info;
    info.callback = listener;
    listeners.GetAdd(type).Add(info);
}

void EventDispatcher::RemoveListener(EventType type, std::function<bool(std::shared_ptr<UIEvent>)> listener) {
    Vector<ListenerInfo>& list = listeners.GetAdd(type);
    for (int i = 0; i < list.GetCount(); i++) {
        // Since we have function objects, we can't directly compare them
        // In a real implementation, you'd need a different approach to identify listeners
    }
}

bool EventDispatcher::DispatchEvent(std::shared_ptr<UIEvent> event) {
    if (!event) return false;
    
    if (event->IsHandled() || event->IsStopped()) return false;
    
    // Get listeners for this event type
    const Vector<ListenerInfo>* list = listeners.Get(event->GetType());
    if (!list) return false;
    
    bool handled = false;
    
    // Call each listener for this event type
    for (int i = 0; i < list->GetCount(); i++) {
        if ((*list)[i].callback && (*list)[i].callback(event)) {
            handled = true;
            if (event->IsHandled()) break;  // Stop if event was marked as handled
        }
    }
    
    return handled;
}

bool EventDispatcher::BubbleEvent(std::shared_ptr<UIEvent> event) {
    if (!event || !event->GetBubbles()) return false;
    
    // Get the target actor and start bubbling up the hierarchy
    std::shared_ptr<Actor> current = event->GetTarget();
    if (!current) return false;
    
    bool handled = false;
    
    // Bubble up the parent hierarchy
    while (current && !event->IsStopped()) {
        // In a real implementation, we would check if current has event listeners
        // and call them to handle the event
        
        current = current->GetParent();
    }
    
    return handled;
}

END_UPP_NAMESPACE