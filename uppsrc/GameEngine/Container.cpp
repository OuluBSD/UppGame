#include "GameEngine.h"
#include "Container.h"

NAMESPACE_UPP

Container::Container() 
{
    SetSize(0, 0);  // Default to preferred size
    SetTouchable(false);  // Containers are not touchable by default
}

Container::Container(std::shared_ptr<Actor> actor) : Container()
{
    SetActor(actor);
}

Container::~Container()
{
    // Actor will be cleaned up when the Group's destructor runs
}

void Container::SetActor(std::shared_ptr<Actor> actor) 
{
    if (this->actor == actor) return;
    
    // Remove old actor
    if (this->actor) {
        RemoveActor(this->actor);
        this->actor = nullptr;
    }
    
    // Add new actor
    this->actor = actor;
    if (actor) {
        AddActor(actor);
        Invalidate();
    }
}

std::shared_ptr<Actor> Container::GetActor() const 
{
    return actor;
}

void Container::Clear() 
{
    SetActor(nullptr);
    Invalidate();
}

void Container::SetSize(float width, float height) 
{
    Actor::SetSize(width, height);
    Invalidate();
}

void Container::SetWidth(float width) 
{
    Actor::SetWidth(width);
    Invalidate();
}

void Container::SetHeight(float height) 
{
    Actor::SetHeight(height);
    Invalidate();
}

float Container::GetPrefWidth() const 
{
    if (prefWidth != -1) return prefWidth;
    
    if (!actor) return 0;
    
    // In a real implementation, you'd get the preferred size from the actor
    // This is a simplified version
    return actor->GetWidth() + padLeft + padRight;
}

float Container::GetPrefHeight() const 
{
    if (prefHeight != -1) return prefHeight;
    
    if (!actor) return 0;
    
    // In a real implementation, you'd get the preferred size from the actor
    // This is a simplified version
    return actor->GetHeight() + padTop + padBottom;
}

float Container::GetMinWidth() const 
{
    return minWidth;
}

float Container::GetMinHeight() const 
{
    return minHeight;
}

float Container::GetMaxWidth() const 
{
    return maxWidth > 0 ? maxWidth : FLT_MAX;
}

float Container::GetMaxHeight() const 
{
    return maxHeight > 0 ? maxHeight : FLT_MAX;
}

void Container::SetPrefSize(float prefWidth, float prefHeight) 
{
    this->prefWidth = prefWidth;
    this->prefHeight = prefHeight;
    Invalidate();
}

void Container::SetMinSize(float minWidth, float minHeight) 
{
    this->minWidth = minWidth;
    this->minHeight = minHeight;
    Invalidate();
}

void Container::SetMaxSize(float maxWidth, float maxHeight) 
{
    this->maxWidth = maxWidth;
    this->maxHeight = maxHeight;
    Invalidate();
}

void Container::SetPrefWidth(float prefWidth) 
{
    this->prefWidth = prefWidth;
    Invalidate();
}

void Container::SetPrefHeight(float prefHeight) 
{
    this->prefHeight = prefHeight;
    Invalidate();
}

void Container::SetMinWidth(float minWidth) 
{
    this->minWidth = minWidth;
    Invalidate();
}

void Container::SetMinHeight(float minHeight) 
{
    this->minHeight = minHeight;
    Invalidate();
}

void Container::SetMaxWidth(float maxWidth) 
{
    this->maxWidth = maxWidth;
    Invalidate();
}

void Container::SetMaxHeight(float maxHeight) 
{
    this->maxHeight = maxHeight;
    Invalidate();
}

void Container::SetAlign(int align) 
{
    this->align = align;
    Invalidate();
}

void Container::SetPad(float pad) 
{
    padTop = padLeft = padBottom = padRight = pad;
    Invalidate();
}

void Container::SetPadTop(float padTop) 
{
    this->padTop = padTop;
    Invalidate();
}

void Container::SetPadLeft(float padLeft) 
{
    this->padLeft = padLeft;
    Invalidate();
}

void Container::SetPadBottom(float padBottom) 
{
    this->padBottom = padBottom;
    Invalidate();
}

void Container::SetPadRight(float padRight) 
{
    this->padRight = padRight;
    Invalidate();
}

void Container::SetFill(float x, float y) 
{
    fillX = x;
    fillY = y;
    Invalidate();
}

void Container::SetFillX(float x) 
{
    fillX = x;
    Invalidate();
}

void Container::SetFillY(float y) 
{
    fillY = y;
    Invalidate();
}

void Container::SetExpand(bool x, bool y) 
{
    expandX = x;
    expandY = y;
    Invalidate();
}

void Container::SetExpandX(bool x) 
{
    expandX = x;
    Invalidate();
}

void Container::SetExpandY(bool y) 
{
    expandY = y;
    Invalidate();
}

void Container::SetSpace(float space) 
{
    spaceTop = spaceLeft = spaceBottom = spaceRight = space;
    Invalidate();
}

void Container::SetSpaceTop(float spaceTop) 
{
    this->spaceTop = spaceTop;
    Invalidate();
}

void Container::SetSpaceLeft(float spaceLeft) 
{
    this->spaceLeft = spaceLeft;
    Invalidate();
}

void Container::SetSpaceBottom(float spaceBottom) 
{
    this->spaceBottom = spaceBottom;
    Invalidate();
}

void Container::SetSpaceRight(float spaceRight) 
{
    this->spaceRight = spaceRight;
    Invalidate();
}

void Container::Invalidate() 
{
    needsLayout = true;
    // If this container is inside another layout container, 
    // it should propagate the invalidation up the hierarchy
    auto parent = GetParent();
    if (parent) {
        Group* parentGroup = dynamic_cast<Group*>(parent.get());
        if (parentGroup) {
            // In a full implementation, we might have an InvalidateHierarchy method
        }
    }
}

void Container::Validate() 
{
    if (!needsLayout) return;
    
    Layout();
    needsLayout = false;
}

void Container::Pack() 
{
    SetSize(GetPrefWidth(), GetPrefHeight());
    Validate();
}

void Container::Act(double delta) 
{
    Validate();  // Ensure layout is up to date
    Group::Act(delta);  // Update children
}

void Container::Draw(Draw& draw, double parentAlpha) 
{
    Validate();  // Ensure layout is up to date
    Group::Draw(draw, parentAlpha);  // Draw children
}

void Container::Layout() 
{
    if (!actor) return;
    
    // Calculate available space after padding
    float spaceWidth = width - GetPadWidth();
    float spaceHeight = height - GetPadHeight();
    
    // Apply size limits to the available space
    spaceWidth = max(spaceWidth, minWidth - GetPadWidth());
    spaceHeight = max(spaceHeight, minHeight - GetPadHeight());
    
    if (maxWidth > 0) spaceWidth = min(spaceWidth, maxWidth - GetPadWidth());
    if (maxHeight > 0) spaceHeight = min(spaceHeight, maxHeight - GetPadHeight());
    
    // Determine final size of the actor
    float actorWidth, actorHeight;
    
    if (fillX >= 0) {
        // Use fill factor
        actorWidth = spaceWidth * fillX;
    } else if (expandX) {
        // Expand to fill available space
        actorWidth = spaceWidth;
    } else {
        // Use preferred size, but don't exceed available space
        actorWidth = min(spaceWidth, GetPrefWidth() - GetPadWidth());
    }
    
    if (fillY >= 0) {
        // Use fill factor
        actorHeight = spaceHeight * fillY;
    } else if (expandY) {
        // Expand to fill available space
        actorHeight = spaceHeight;
    } else {
        // Use preferred size, but don't exceed available space
        actorHeight = min(spaceHeight, GetPrefHeight() - GetPadHeight());
    }
    
    // Apply minimum and maximum size constraints to the actor
    if (actor->GetMinWidth() > 0) actorWidth = max(actorWidth, actor->GetMinWidth());
    if (actor->GetMinHeight() > 0) actorHeight = max(actorHeight, actor->GetMinHeight());
    
    if (actor->GetMaxWidth() > 0) actorWidth = min(actorWidth, actor->GetMaxWidth());
    if (actor->GetMaxHeight() > 0) actorHeight = min(actorHeight, actor->GetMaxHeight());
    
    // Position the actor based on alignment
    float actorX = x + padLeft;
    float actorY = y + padBottom;  // Assuming y=0 is bottom in this coordinate system
    
    // Apply horizontal alignment
    if (align & 4) {  // right
        actorX = x + width - padRight - actorWidth;
    } else if (align & 2) {  // center x
        actorX = x + padLeft + (spaceWidth - actorWidth) / 2.0f;
    }
    // (1 would be left, which is default, no adjustment needed)
    
    // Apply vertical alignment
    if (align & 32) {  // top
        actorY = y + height - padTop - actorHeight;
    } else if (align & 16) {  // center y
        actorY = y + padBottom + (spaceHeight - actorHeight) / 2.0f;
    }
    // (8 would be bottom, which is default, no adjustment needed)
    
    // Set the position and size of the contained actor
    actor->SetPosition(actorX, actorY);
    actor->SetSize(actorWidth, actorHeight);
}

END_UPP_NAMESPACE