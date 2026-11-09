#ifndef UPP_TABLE_H
#define UPP_TABLE_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <GameLib/GameLib.h>
#include <GameEngine/Group.h>
#include <GameEngine/Container.h>
#include <Vector/Vector.h>
#include <memory>

NAMESPACE_UPP

// Cell class to represent a single cell in the table
template<typename T>
class Cell {
public:
    Cell(T content) : actor(content) {}
    
    // Alignment (using bit flags)
    Cell& Top();
    Cell& Left();
    Cell& Bottom();
    Cell& Right();
    Cell& Center();
    Cell& Fill();
    Cell& FillX();
    Cell& FillY();
    
    // Size constraints
    Cell& Size(float width, float height);
    Cell& MinSize(float minWidth, float minHeight);
    Cell& PrefSize(float prefWidth, float prefHeight);
    Cell& MaxSize(float maxWidth, float maxHeight);
    
    // Padding
    Cell& Pad(float pad);
    Cell& PadTop(float padTop);
    Cell& PadLeft(float padLeft);
    Cell& PadBottom(float padBottom);
    Cell& PadRight(float padRight);
    
    // Spacing
    Cell& Space(float space);
    Cell& SpaceTop(float spaceTop);
    Cell& SpaceLeft(float spaceLeft);
    Cell& SpaceBottom(float spaceBottom);
    Cell& SpaceRight(float spaceRight);
    
    // Get the contained actor
    T GetActor() const { return actor; }

private:
    T actor;
    int align = 0;
    float minWidth = -1, minHeight = -1;
    float prefWidth = -1, prefHeight = -1;
    float maxWidth = -1, maxHeight = -1;
    float padTop = 0, padLeft = 0, padBottom = 0, padRight = 0;
    float spaceTop = 0, spaceLeft = 0, spaceBottom = 0, spaceRight = 0;
    bool fillX = false, fillY = false;
};

// Table class - similar to libgdx's Table
class Table : public Group {
public:
    Table();
    virtual ~Table();

    // Add actors to the table
    Cell<std::shared_ptr<Actor>> Add(std::shared_ptr<Actor> actor);
    Cell<std::shared_ptr<Actor>> Add();
    void Row();  // Move to the next row
    void Clear();  // Clear all cells and reset

    // Table settings
    void SetTransform(boolean transform);
    boolean GetTransform() const { return transform; }
    
    // Background (would be implemented as needed)
    // void SetBackground(Drawable background);

    // Override lifecycle methods
    void Act(double delta) override;
    void Draw(Draw& draw, double parentAlpha) override;

    // Layout methods
    void Invalidate() override;
    void Validate() override;
    void Pack();

protected:
    struct CellInfo {
        std::shared_ptr<Actor> actor;
        int row, col;
        int align;
        float minWidth, minHeight;
        float prefWidth, prefHeight;
        float maxWidth, maxHeight;
        float padTop, padLeft, padBottom, padRight;
        float spaceTop, spaceLeft, spaceBottom, spaceRight;
        bool fillX, fillY;
    };
    
    Vector<CellInfo> cells;
    Vector<int> rowCounts;  // Number of columns in each row
    int currentRow = 0;
    int currentCol = 0;
    boolean transform = false;
    boolean needsLayout = true;

    void Layout();
    void CalculatePrefSize();
    float GetPrefWidth() const;
    float GetPrefHeight() const;
};

// Implementation of Cell methods
template<typename T>
Cell<T>& Cell<T>::Top() {
    align |= 32;  // top alignment
    return *this;
}

template<typename T>
Cell<T>& Cell<T>::Left() {
    align |= 1;  // left alignment
    return *this;
}

template<typename T>
Cell<T>& Cell<T>::Bottom() {
    align |= 8;  // bottom alignment
    return *this;
}

template<typename T>
Cell<T>& Cell<T>::Right() {
    align |= 4;  // right alignment
    return *this;
}

template<typename T>
Cell<T>& Cell<T>::Center() {
    align |= 2 | 16;  // center x and y
    return *this;
}

template<typename T>
Cell<T>& Cell<T>::Fill() {
    fillX = fillY = true;
    return *this;
}

template<typename T>
Cell<T>& Cell<T>::FillX() {
    fillX = true;
    return *this;
}

template<typename T>
Cell<T>& Cell<T>::FillY() {
    fillY = true;
    return *this;
}

template<typename T>
Cell<T>& Cell<T>::Size(float width, float height) {
    prefWidth = width;
    prefHeight = height;
    return *this;
}

template<typename T>
Cell<T>& Cell<T>::MinSize(float minWidth, float minHeight) {
    this->minWidth = minWidth;
    this->minHeight = minHeight;
    return *this;
}

template<typename T>
Cell<T>& Cell<T>::PrefSize(float prefWidth, float prefHeight) {
    this->prefWidth = prefWidth;
    this->prefHeight = prefHeight;
    return *this;
}

template<typename T>
Cell<T>& Cell<T>::MaxSize(float maxWidth, float maxHeight) {
    this->maxWidth = maxWidth;
    this->maxHeight = maxHeight;
    return *this;
}

template<typename T>
Cell<T>& Cell<T>::Pad(float pad) {
    padTop = padLeft = padBottom = padRight = pad;
    return *this;
}

template<typename T>
Cell<T>& Cell<T>::PadTop(float padTop) {
    this->padTop = padTop;
    return *this;
}

template<typename T>
Cell<T>& Cell<T>::PadLeft(float padLeft) {
    this->padLeft = padLeft;
    return *this;
}

template<typename T>
Cell<T>& Cell<T>::PadBottom(float padBottom) {
    this->padBottom = padBottom;
    return *this;
}

template<typename T>
Cell<T>& Cell<T>::PadRight(float padRight) {
    this->padRight = padRight;
    return *this;
}

template<typename T>
Cell<T>& Cell<T>::Space(float space) {
    spaceTop = spaceLeft = spaceBottom = spaceRight = space;
    return *this;
}

template<typename T>
Cell<T>& Cell<T>::SpaceTop(float spaceTop) {
    this->spaceTop = spaceTop;
    return *this;
}

template<typename T>
Cell<T>& Cell<T>::SpaceLeft(float spaceLeft) {
    this->spaceLeft = spaceLeft;
    return *this;
}

template<typename T>
Cell<T>& Cell<T>::SpaceBottom(float spaceBottom) {
    this->spaceBottom = spaceBottom;
    return *this;
}

template<typename T>
Cell<T>& Cell<T>::SpaceRight(float spaceRight) {
    this->spaceRight = spaceRight;
    return *this;
}

END_UPP_NAMESPACE

#endif