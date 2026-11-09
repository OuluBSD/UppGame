#ifndef UPP_SPRITEBATCH_H
#define UPP_SPRITEBATCH_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <GameLib/GameLib.h>
#include <Geometry/Geometry.h>
#include <GameEngine/VFS.h>

NAMESPACE_UPP

// Structure to represent a single sprite's data for batching
struct SpriteVertex {
    Point3 position;  // x, y, z
    Point3 texCoord;  // u, v, unused
    Color color;      // RGBA color
    
    SpriteVertex() = default;
    SpriteVertex(const Point3& pos, const Point3& tex, Color c = White())
        : position(pos), texCoord(tex), color(c) {}
};

// Structure to represent a sprite for rendering
struct Sprite {
    Image image;
    Rect sourceRect;      // Source rectangle in the texture
    Rect destRect;        // Destination rectangle on screen
    Color color = White(); // Tint color
    double rotation = 0.0; // Rotation in radians
    Point3 origin = Point3(0, 0, 0); // Origin point for rotation/scaling
    
    // Construct from image and rectangles
    Sprite(const Image& img, const Rect& src, const Rect& dst, Color c = White())
        : image(img), sourceRect(src), destRect(dst), color(c) {}
    
    // Simple constructor for full image
    Sprite(const Image& img, const Rect& dst, Color c = White())
        : image(img), sourceRect(0, 0, img.GetWidth(), img.GetHeight()), 
          destRect(dst), color(c) {}
};

// SpriteBatch class for efficient 2D rendering
class SpriteBatch {
public:
    SpriteBatch(int initial_capacity = 1000);
    virtual ~SpriteBatch();

    // Begin a batch - call before drawing sprites
    void Begin();
    
    // End a batch - call after drawing sprites to render them all at once
    void End(Draw& draw);

    // Draw a sprite
    void Draw(const Image& image, const Rect& dest_rect, Color color = White());
    void Draw(const Image& image, const Rect& src_rect, const Rect& dest_rect, Color color = White());
    void Draw(const Sprite& sprite);

    // Draw with rotation, scaling, and origin
    void Draw(const Image& image, const Rect& src_rect, const Rect& dest_rect, 
              double rotation, const Point3& origin, Color color = White());

    // Get/Set capacity
    int GetCapacity() const { return capacity; }
    void SetCapacity(int new_capacity);

    // Get current number of sprites in the batch
    int GetSpriteCount() const { return sprite_count; }

    // Clear the current batch (doesn't render)
    void Clear();

    // Get max capacity
    int GetMaxCapacity() const { return max_capacity; }

private:
    // Storage for sprites in the batch
    Vector<Sprite> sprites;
    
    // Maximum sprites we can batch
    int max_capacity;
    
    // Current capacity and count
    int capacity;
    int sprite_count;
    
    // Are we currently in a batch?
    bool is_batching;
    
    // Transform matrices for rendering
    Matrix4 projection_matrix;
    Matrix4 view_matrix;
    
    // Render the batched sprites
    void Flush(Draw& draw);
};

END_UPP_NAMESPACE

#endif