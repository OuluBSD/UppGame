#ifndef UPP_TEXTUREATLAS_H
#define UPP_TEXTUREATLAS_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <GameLib/GameLib.h>
#include <GameEngine/VFS.h>

NAMESPACE_UPP

// Structure to represent a region in the texture atlas
struct AtlasRegion {
    String name;              // Name of the region (e.g. "player_walk_01")
    Image image;              // The sub-image reference
    Rect bounds;              // Bounds in the atlas texture
    Rect original_bounds;     // Original bounds (before packing)
    Point offset;             // Offset from original position
    Size original_size;       // Original size before cropping
    bool rotated;             // If the region was rotated during packing
    int index;                // For split regions with multiple parts with same name
    
    AtlasRegion() = default;
    AtlasRegion(const String& n, const Image& img, const Rect& b, const Rect& orig_b, 
                const Point& off, const Size& orig_s, bool rot = false, int idx = 0)
        : name(n), image(img), bounds(b), original_bounds(orig_b), 
          offset(off), original_size(orig_s), rotated(rot), index(idx) {}
};

// TextureAtlas class for efficient texture management
class TextureAtlas {
public:
    TextureAtlas();
    virtual ~TextureAtlas();

    // Load atlas from an atlas file (typically a .atlas file with coordinates)
    bool LoadFromFile(const String& atlas_path);
    
    // Load from a packed texture and an array of regions
    bool LoadFromTextureAndRegions(const String& texture_path, 
                                  const Vector<AtlasRegion>& regions);

    // Load from individual images that will be packed into an atlas
    bool LoadFromImages(const VectorMap<String, String>& image_paths, 
                       int atlas_width = 1024, int atlas_height = 1024,
                       int padding = 2);

    // Get a specific region from the atlas
    const AtlasRegion* GetRegion(const String& name) const;

    // Get the full atlas texture
    const Image& GetAtlasTexture() const { return atlas_texture; }

    // Add a region to the atlas (if creating dynamically)
    void AddRegion(const String& name, const Image& image, const Rect& bounds);

    // Get all regions in the atlas
    const Vector<AtlasRegion>& GetRegions() const { return regions; }

    // Get number of regions
    int GetRegionCount() const { return regions.GetCount(); }

    // Pack multiple images into a single atlas texture
    static Image PackImages(const Vector<Image>& images, 
                           Vector<Rect>& output_rects, 
                           int atlas_width = 1024, 
                           int atlas_height = 1024,
                           int padding = 2);

private:
    Image atlas_texture;                      // The packed texture atlas
    Vector<AtlasRegion> regions;              // List of regions in the atlas
    VectorMap<String, int> region_map;        // Map from name to region index

    // Pack images using a simple shelf packing algorithm
    bool PackAndCreateAtlas(const VectorMap<String, String>& image_paths,
                           int atlas_width, int atlas_height, int padding);

    // Helper to find position for a new image
    Rect FindPositionForImage(const Size& img_size, int atlas_width, int atlas_height, int padding);
};

END_UPP_NAMESPACE

#endif