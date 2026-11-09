#ifndef UPP_CONTAINER_H
#define UPP_CONTAINER_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <GameLib/GameLib.h>
#include <GameEngine/Actor.h>
#include <GameEngine/Group.h>
#include <memory>

NAMESPACE_UPP

// Container class - similar to libgdx's Container
// A container that holds a single actor with various layout options
class Container : public Group {
public:
    Container();
    explicit Container(std::shared_ptr<Actor> actor);
    virtual ~Container();

    // Set/get the contained actor
    void SetActor(std::shared_ptr<Actor> actor);
    std::shared_ptr<Actor> GetActor() const;
    
    // Clear the contained actor
    void Clear();
    
    // Size control
    void SetSize(float width, float height) override;
    void SetWidth(float width) override;
    void SetHeight(float height) override;

    // Pref size - the "ideal" size for this container
    float GetPrefWidth() const;
    float GetPrefHeight() const;
    
    // Min size - the smallest acceptable size
    float GetMinWidth() const;
    float GetMinHeight() const;
    
    // Max size - the largest acceptable size
    float GetMaxWidth() const;
    float GetMaxHeight() const;
    
    // Set size limits
    void SetPrefSize(float prefWidth, float prefHeight);
    void SetMinSize(float minWidth, float minHeight);
    void SetMaxSize(float maxWidth, float maxHeight);
    
    void SetPrefWidth(float prefWidth);
    void SetPrefHeight(float prefHeight);
    void SetMinWidth(float minWidth);
    void SetMinHeight(float minHeight);
    void SetMaxWidth(float maxWidth);
    void SetMaxHeight(float maxHeight);

    // Alignment of the contained widget within the container
    void SetAlign(int align);
    int GetAlign() const { return align; }

    // Pad from the container's edges
    void SetPad(float pad);
    void SetPadTop(float padTop);
    void SetPadLeft(float padLeft);
    void SetPadBottom(float padBottom);
    void SetPadRight(float padRight);
    
    float GetPadTop() const { return padTop; }
    float GetPadLeft() const { return padLeft; }
    float GetPadBottom() const { return padBottom; }
    float GetPadRight() const { return padRight; }

    // Fill in the X and Y directions (0 to 1)
    void SetFill(float x, float y);
    void SetFillX(float x);
    void SetFillY(float y);
    float GetFillX() const { return fillX; }
    float GetFillY() const { return fillY; }

    // Expand in the X and Y directions
    void SetExpand(bool x, bool y);
    void SetExpandX(bool x);
    void SetExpandY(bool y);
    bool GetExpandX() const { return expandX; }
    bool GetExpandY() const { return expandY; }

    // Position within the container
    void SetSpace(float space);
    void SetSpaceTop(float spaceTop);
    void SetSpaceLeft(float spaceLeft);
    void SetSpaceBottom(float spaceBottom);
    void SetSpaceRight(float spaceRight);

    // Update the layout of the contained actor
    void Invalidate(); // Mark layout as invalid
    void Validate();   // Update the layout if invalid
    void Pack();       // Size the container to its preferred size and validate

    // Override methods to properly handle contained actor
    void Act(double delta) override;
    void Draw(Draw& draw, double parentAlpha) override;

protected:
    std::shared_ptr<Actor> actor;
    bool needsLayout = true;

    // Size constraints
    float prefWidth = -1, prefHeight = -1;
    float minWidth = 0, minHeight = 0;
    float maxWidth = 0, maxHeight = 0;  // 0 means no max

    // Alignment
    int align = 0;  // Use U++ alignment constants

    // Padding
    float padTop = 0, padLeft = 0, padBottom = 0, padRight = 0;

    // Fill amount (0 to 1)
    float fillX = -1, fillY = -1;  // -1 means not set

    // Expand
    bool expandX = false, expandY = false;

    // Space (for widgets that support it)
    float spaceTop = 0, spaceLeft = 0, spaceBottom = 0, spaceRight = 0;

    // Internal method to update the layout
    void Layout();
    
    // Get the space available for the actor after padding
    float GetPadWidth() const { return padLeft + padRight; }
    float GetPadHeight() const { return padTop + padBottom; }
};

END_UPP_NAMESPACE

#endif