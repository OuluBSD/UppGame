#include "SpriteBatch.h"

NAMESPACE_UPP

SpriteBatch::SpriteBatch(int initial_capacity) 
    : max_capacity(10000), is_batching(false), sprite_count(0) {
    SetCapacity(initial_capacity);
}

SpriteBatch::~SpriteBatch() {
    Clear();
}

void SpriteBatch::SetCapacity(int new_capacity) {
    capacity = min(new_capacity, max_capacity);
    // We'll store sprite information differently for U++ compatibility
    sprites.SetCount(capacity);
}

void SpriteBatch::Begin() {
    if (is_batching) {
        return; // Already in a batch
    }
    
    is_batching = true;
    sprite_count = 0;
    Clear();
}

void SpriteBatch::End(Draw& draw) {
    if (!is_batching) {
        return; // Not currently batching
    }
    
    // Draw all sprites in the batch
    for (int i = 0; i < sprite_count; i++) {
        const Sprite& sprite = sprites[i];
        // Apply rotation and transformation if needed
        if (sprite.rotation != 0.0) {
            // For rotated sprites, we need to apply transformation
            // This is more complex in U++ but possible with DrawTransform
            
            // Calculate transformation matrix based on rotation, origin, and scale
            double cos_rot = cos(sprite.rotation);
            double sin_rot = sin(sprite.rotation);
            
            // Create transformation matrix
            // This is a simplified transformation that includes rotation around origin
            AffineMatrix m;
            
            // Translate to origin
            m = m * AffineMatrix().Translate(-sprite.origin.x, -sprite.origin.y);
            
            // Apply rotation
            m = m * AffineMatrix().Rotate(sprite.rotation);
            
            // Translate back 
            m = m * AffineMatrix().Translate(sprite.origin.x + sprite.destRect.left, 
                                           sprite.origin.y + sprite.destRect.top);
            
            // Apply the transformation to draw the rotated sprite
            draw.DrawImage(sprite.destRect.left, sprite.destRect.top, 
                          sprite.destRect.GetSize().cx, sprite.destRect.GetSize().cy, 
                          sprite.image, m);
        } else {
            // Draw without rotation - direct and efficient
            if (sprite.sourceRect == Rect(0, 0, sprite.image.GetWidth(), sprite.image.GetHeight())) {
                // Draw full image
                draw.DrawImage(sprite.destRect.left, sprite.destRect.top, 
                              sprite.destRect.GetSize().cx, sprite.destRect.GetSize().cy, 
                              sprite.image);
            } else {
                // Draw sub-image (clipped)
                Image clipped = sprite.image; // In a real implementation, clip the image to sourceRect
                draw.DrawImage(sprite.destRect.left, sprite.destRect.top, 
                              sprite.destRect.GetSize().cx, sprite.destRect.GetSize().cy, 
                              clipped);
            }
        }
    }
    
    is_batching = false;
    sprite_count = 0; // Reset count after drawing
}

void SpriteBatch::Draw(const Image& image, const Rect& dest_rect, Color color) {
    Rect src_rect(0, 0, image.GetWidth(), image.GetHeight());
    Draw(image, src_rect, dest_rect, color);
}

void SpriteBatch::Draw(const Image& image, const Rect& src_rect, const Rect& dest_rect, Color color) {
    Draw(image, src_rect, dest_rect, 0.0, Point3(0, 0, 0), color);
}

void SpriteBatch::Draw(const Sprite& sprite) {
    if (!is_batching) {
        LOG("SpriteBatch::Draw called outside of Begin()/End() block");
        return;
    }
    
    if (sprite_count >= capacity) {
        LOG("Sprite batch capacity exceeded");
        return;
    }
    
    sprites[sprite_count] = sprite;
    sprite_count++;
}

void SpriteBatch::Draw(const Image& image, const Rect& src_rect, const Rect& dest_rect, 
                      double rotation, const Point3& origin, Color color) {
    if (!is_batching) {
        LOG("SpriteBatch::Draw called outside of Begin()/End() block");
        return;
    }
    
    if (sprite_count >= capacity) {
        LOG("Sprite batch capacity exceeded");
        return;
    }
    
    // Create and store the sprite
    Sprite sprite(image, src_rect, dest_rect, color);
    sprite.rotation = rotation;
    sprite.origin = origin;
    
    sprites[sprite_count] = sprite;
    sprite_count++;
}

void SpriteBatch::Clear() {
    sprite_count = 0;
}

END_UPP_NAMESPACE