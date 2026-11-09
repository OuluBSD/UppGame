#ifndef UPP_ACTOR_H
#define UPP_ACTOR_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <GameLib/GameLib.h>
#include <Geometry/Geometry.h>
#include <functional>
#include <memory>
#include <vector>

NAMESPACE_UPP

class Stage;
class Event;

// Base Actor class - similar to libgdx's Actor
class Actor : public Moveable<Actor>, public std::enable_shared_from_this<Actor> {
public:
    Actor();
    virtual ~Actor();

    // Actor properties
    void SetName(const String& name) { this->name = name; }
    const String& GetName() const { return name; }

    void SetPosition(double x, double y) { this->x = x; this->y = y; }
    void SetX(double x) { this->x = x; }
    void SetY(double y) { this->y = y; }
    double GetX() const { return x; }
    double GetY() const { return y; }
    Point GetPosition() const { return Point((int)x, (int)y); }

    void SetSize(double width, double height) { this->width = width; this->height = height; }
    void SetWidth(double width) { this->width = width; }
    void SetHeight(double height) { this->height = height; }
    double GetWidth() const { return width; }
    double GetHeight() const { return height; }
    Size GetSize() const { return Size((int)width, (int)height); }

    void SetOrigin(double originX, double originY) { this->originX = originX; this->originY = originY; }
    void SetOriginX(double originX) { this->originX = originX; }
    void SetOriginY(double originY) { this->originY = originY; }
    double GetOriginX() const { return originX; }
    double GetOriginY() const { return originY; }

    void SetScale(double scale) { this->scaleX = scale; this->scaleY = scale; }
    void SetScale(double scaleX, double scaleY) { this->scaleX = scaleX; this->scaleY = scaleY; }
    void SetScaleX(double scaleX) { this->scaleX = scaleX; }
    void SetScaleY(double scaleY) { this->scaleY = scaleY; }
    double GetScaleX() const { return scaleX; }
    double GetScaleY() const { return scaleY; }

    void SetRotation(double rotation) { this->rotation = rotation; }
    double GetRotation() const { return rotation; }

    void SetColor(Color color) { this->color = color; }
    Color GetColor() const { return color; }

    void SetVisible(bool visible) { this->visible = visible; }
    bool IsVisible() const { return visible; }

    void SetTouchable(bool touchable) { this->touchable = touchable; }
    bool IsTouchable() const { return touchable; }

    // Coordinates
    double GetRight() const { return x + width; }
    double GetTop() const { return y + height; }
    Point GetCenter() const { return Point((int)(x + width / 2), (int)(y + height / 2)); }
    Point GetPositionInParent() const; // Get position relative to parent

    // Transformations
    Matrix3 GetTransformationMatrix() const; // Get combined transformation matrix
    Matrix3 GetLocalTransformMatrix() const; // Get local transformation matrix
    Matrix3 GetParentTransformMatrix() const; // Get parent's transformation matrix

    // Hierarchical methods
    void SetParent(std::shared_ptr<Actor> parent) { this->parent = parent; }
    std::shared_ptr<Actor> GetParent() const { return parent.lock(); }
    Stage* GetStage() const; // Get the stage this actor is in

    // Bounds
    Rect GetBounds() const { return Rect((int)x, (int)y, (int)width, (int)height); }
    bool Contains(double pointX, double pointY) const;
    bool Intersects(std::shared_ptr<Actor> other) const;

    // Actor lifecycle methods (to be overridden)
    virtual void Act(double delta); // Called each frame to update the actor
    virtual void Draw(Draw& draw, double parentAlpha); // Called to draw the actor

    // Coordinate conversion (to be overridden with transformation matrices)
    Point LocalToStageCoordinates(const Point& localCoords) const;
    Point StageToLocalCoordinates(const Point& stageCoords) const;
    Point LocalToParentCoordinates(const Point& localCoords) const;
    Point ParentToLocalCoordinates(const Point& parentCoords) const;

    // Event handling
    virtual bool TouchDown(int screenX, int screenY, int pointer, int button);
    virtual void TouchUp(int screenX, int screenY, int pointer, int button);
    virtual void TouchDragged(int screenX, int screenY, int pointer);
    virtual bool MouseMoved(int screenX, int screenY);
    virtual bool Scrolled(int amount);

    // Custom event handling
    virtual bool HandleEvent(std::shared_ptr<Event> event);
    virtual bool HandleUIEvent(std::shared_ptr<UIEvent> event);

    // Z-index for rendering order
    void SetZIndex(int index) { zIndex = index; }
    int GetZIndex() const { return zIndex; }

protected:
    // Actor properties
    String name;
    double x = 0, y = 0;
    double width = 100, height = 100;
    double originX = 0, originY = 0;
    double scaleX = 1, scaleY = 1;
    double rotation = 0; // in degrees
    Color color = White();
    bool visible = true;
    bool touchable = true;
    int zIndex = 0;

    // Hierarchy
    std::weak_ptr<Actor> parent;
};

// Event class for handling input and other events
class Event {
public:
    Event();
    virtual ~Event() = default;

    void SetBubbles(bool bubbles) { this->bubbles = bubbles; }
    bool GetBubbles() const { return bubbles; }
    
    void SetTarget(std::shared_ptr<Actor> target) { this->target = target; }
    std::shared_ptr<Actor> GetTarget() const { return target.lock(); }
    
    void SetListenerActor(std::shared_ptr<Actor> actor) { this->listenerActor = actor; }
    std::shared_ptr<Actor> GetListenerActor() const { return listenerActor.lock(); }

    bool IsHandled() const { return handled; }
    void Handle() { handled = true; }

    // For input events
    void SetTouchPos(int x, int y) { touchX = x; touchY = y; }
    Point GetTouchPos() const { return Point(touchX, touchY); }
    int GetTouchX() const { return touchX; }
    int GetTouchY() const { return touchY; }

protected:
    std::weak_ptr<Actor> target;
    std::weak_ptr<Actor> listenerActor;
    bool bubbles = false;
    bool handled = false;
    int touchX = 0, touchY = 0;
};

END_UPP_NAMESPACE

#endif