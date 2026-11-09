#ifndef UPP_GROUP_H
#define UPP_GROUP_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <GameLib/GameLib.h>
#include <GameEngine/Actor.h>
#include <Vector/Vector.h>
#include <memory>

NAMESPACE_UPP

// Group class - similar to libgdx's Group
// A Group is an Actor that can contain other actors, allowing for scene hierarchies
class Group : public Actor {
public:
    Group();
    virtual ~Group();

    // Child management
    void AddActor(std::shared_ptr<Actor> actor);
    void RemoveActor(std::shared_ptr<Actor> actor);
    void RemoveActorAt(int index);
    void Clear();
    int GetChildrenCount() const { return children.GetCount(); }
    std::shared_ptr<Actor> GetChildAt(int index) const { return children[index]; }
    
    // Check if actor is a child
    bool HasChild(std::shared_ptr<Actor> actor) const;
    int IndexOf(std::shared_ptr<Actor> actor) const;
    
    // Find child by name (non-recursive)
    std::shared_ptr<Actor> FindActor(const String& name) const;

    // Override lifecycle methods to affect children
    void Act(double delta) override;
    void Draw(Draw& draw, double parentAlpha) override;

    // Override input methods to propagate to children
    bool TouchDown(int screenX, int screenY, int pointer, int button) override;
    void TouchUp(int screenX, int screenY, int pointer, int button) override;
    void TouchDragged(int screenX, int screenY, int pointer) override;
    bool MouseMoved(int screenX, int screenY) override;
    bool Scrolled(int amount) override;

    // Coordinate translation for children
    Point ActorToStageCoordinates(std::shared_ptr<Actor> child, const Point& actorCoords) const;
    Point StageToActorCoordinates(std::shared_ptr<Actor> child, const Point& stageCoords) const;

    // Set whether this group clips its children
    void SetClip(boolean clip) { this->clip = clip; }
    boolean IsClip() const { return clip; }

protected:
    Vector<std::shared_ptr<Actor>> children;
    boolean clip = false;

    // Helper method to find actor at position
    std::shared_ptr<Actor> FindActorAt(float x, float y, bool touchable) const;
};

END_UPP_NAMESPACE

#endif