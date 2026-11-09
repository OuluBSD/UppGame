#include "GameEngine.h"
#include "Actor.h"
#include "Stage.h"

NAMESPACE_UPP

Actor::Actor() 
    : x(0), y(0), width(100), height(100), 
      originX(0), originY(0), 
      scaleX(1), scaleY(1), 
      rotation(0), 
      color(White()), 
      visible(true), 
      touchable(true),
      zIndex(0)
{
}

Actor::~Actor()
{
}

Point Actor::GetPositionInParent() const 
{
    auto parentActor = GetParent();
    if (parentActor) {
        return Point((int)(x - parentActor->GetX()), (int)(y - parentActor->GetY()));
    }
    return Point((int)x, (int)y);
}

Matrix3 Actor::GetLocalTransformMatrix() const 
{
    Matrix3 translation = Matrix3::Translation(x, y);
    Matrix3 scaling = Matrix3::Scale(scaleX, scaleY);
    Matrix3 rotationMat = Matrix3::Rotation(rotation * M_PI / 180.0); // Convert degrees to radians
    Matrix3 originTranslation = Matrix3::Translation(-originX, -originY);

    // Apply transformations in order: origin offset, scale, rotation, then translation
    return translation * rotationMat * scaling * originTranslation;
}

Matrix3 Actor::GetParentTransformMatrix() const 
{
    auto parentActor = GetParent();
    if (parentActor) {
        return parentActor->GetTransformationMatrix();
    }
    return Matrix3::Identity();
}

Matrix3 Actor::GetTransformationMatrix() const 
{
    Matrix3 local = GetLocalTransformMatrix();
    Matrix3 parent = GetParentTransformMatrix();
    return parent * local;
}

Stage* Actor::GetStage() const 
{
    auto parentActor = GetParent();
    if (parentActor) {
        // Keep traversing up the hierarchy until we find a Stage
        // Implementation will depend on how Stage extends or interfaces with Actor
        if (auto stage = dynamic_cast<Stage*>(parentActor.get())) {
            return stage;
        }
        // If parent isn't a Stage, call GetStage recursively on the parent
        return parentActor->GetStage();
    }
    return nullptr;
}

bool Actor::Contains(double pointX, double pointY) const 
{
    return (pointX >= x && pointX < x + width && 
            pointY >= y && pointY < y + height);
}

bool Actor::Intersects(std::shared_ptr<Actor> other) const 
{
    if (!other) return false;
    
    Rect thisRect((int)x, (int)y, (int)width, (int)height);
    Rect otherRect((int)other->x, (int)other->y, (int)other->width, (int)other->height);
    
    return thisRect.Intersects(otherRect);
}

void Actor::Act(double delta) 
{
    // Default implementation - override in subclasses
    // This is called once per frame to update actor state
}

void Actor::Draw(Draw& draw, double parentAlpha) 
{
    // Default implementation - override in subclasses
    // This is called when the actor is being rendered
    if (!visible) return;
    
    // Save current transform
    // Apply actor's transformation matrix
    // Render the actor
    // Restore previous transform
    
    // For now, just draw a simple rectangle to visualize the actor
    if (visible) {
        Color drawColor = Color(
            (int)(color.GetR() * parentAlpha),
            (int)(color.GetG() * parentAlpha), 
            (int)(color.GetB() * parentAlpha)
        );
        draw.DrawRect((int)x, (int)y, (int)width, (int)height, drawColor);
    }
}

Point Actor::LocalToStageCoordinates(const Point& localCoords) const 
{
    // Transform a point from local coordinates to stage coordinates
    // This involves applying all parent transformations cumulatively
    Point currentPoint = localCoords;
    auto currentParent = GetParent();
    
    // Apply local transformation
    Matrix3 localTransform = GetLocalTransformMatrix();
    currentPoint = localTransform.TransformPoint(currentPoint);
    
    // Apply all parent transformations
    while (currentParent) {
        Matrix3 parentTransform = currentParent->GetLocalTransformMatrix();
        currentPoint = parentTransform.TransformPoint(currentPoint);
        currentParent = currentParent->GetParent();
    }
    
    return currentPoint;
}

Point Actor::StageToLocalCoordinates(const Point& stageCoords) const 
{
    // Transform a point from stage coordinates to local coordinates
    // This is the inverse of LocalToStageCoordinates
    
    // First, we need to collect all transforms from this actor up to the stage
    std::vector<Matrix3> transforms;
    auto current = std::const_pointer_cast<Actor>(shared_from_this());
    
    // Collect transforms from this actor to root
    while (current) {
        transforms.push_back(current->GetLocalTransformMatrix());
        current = current->GetParent();
    }
    
    // Apply inverse transformations in reverse order
    Point currentPoint = stageCoords;
    for (auto it = transforms.rbegin(); it != transforms.rend(); ++it) {
        currentPoint = it->TransformPointInverse(currentPoint);
    }
    
    return currentPoint;
}

Point Actor::LocalToParentCoordinates(const Point& localCoords) const 
{
    // Transform from local coordinates to parent coordinates
    Matrix3 localTransform = GetLocalTransformMatrix();
    return localTransform.TransformPoint(localCoords);
}

Point Actor::ParentToLocalCoordinates(const Point& parentCoords) const 
{
    // Transform from parent coordinates to local coordinates
    Matrix3 localTransform = GetLocalTransformMatrix();
    return localTransform.TransformPointInverse(parentCoords);
}

bool Actor::TouchDown(int screenX, int screenY, int pointer, int button) 
{
    // Default implementation - override in subclasses
    // Return true to indicate the event was handled
    return false;
}

void Actor::TouchUp(int screenX, int screenY, int pointer, int button) 
{
    // Default implementation - override in subclasses
}

void Actor::TouchDragged(int screenX, int screenY, int pointer) 
{
    // Default implementation - override in subclasses
}

bool Actor::MouseMoved(int screenX, int screenY) 
{
    // Default implementation - override in subclasses
    return false;
}

bool Actor::Scrolled(int amount) 
{
    // Default implementation - override in subclasses
    return false;
}

bool Actor::HandleEvent(std::shared_ptr<Event> event)
{
    // Default implementation - override in subclasses
    return false;
}

bool Actor::HandleUIEvent(std::shared_ptr<UIEvent> event)
{
    // Default implementation - override in subclasses
    return false;
}

Event::Event() : bubbles(false), handled(false), touchX(0), touchY(0)
{
}

END_UPP_NAMESPACE