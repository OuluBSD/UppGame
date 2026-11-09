#include "TextureAtlas.h"
#include <plugin/png/png.h>
#include <plugin/jpg/jpg.h>

NAMESPACE_UPP

TextureAtlas::TextureAtlas() {
}

TextureAtlas::~TextureAtlas() {
}

bool TextureAtlas::LoadFromFile(const String& atlas_path) {
    // Parse a libgdx-style atlas file
    // Atlas files typically contain texture info and region coordinates
    String content = LoadFile(atlas_path);
    if (content.IsEmpty()) {
        LOG("Could not load atlas file: " << atlas_path);
        return false;
    }

    // For now, we'll implement a basic parser
    // A real implementation would parse the full atlas format
    Vector<String> lines = Split(content, '\n');
    
    String texture_path;
    bool in_pages = false;
    bool in_regions = false;
    
    for (const String& line : lines) {
        String trimmed = Nvl(TrimLeft(line), "");
        if (trimmed.IsEmpty()) continue;
        
        if (!trimmed.Contains(':')) {
            // This might be a section header
            if (trimmed == "pages") {
                in_pages = true;
                in_regions = false;
            } else if (trimmed == "regions") {
                in_pages = false;
                in_regions = true;
            } else if (in_pages) {
                // This should be the texture file name
                texture_path = AppendFileName(GetFileDirectory(atlas_path), trimmed);
            }
        } else {
            // Key-value pair
            int colon_pos = trimmed.Find(':');
            String key = TrimLeft(trimmed.Mid(0, colon_pos));
            String value = TrimLeft(trimmed.Mid(colon_pos + 1));
            
            if (in_pages && key == "file") {
                texture_path = AppendFileName(GetFileDirectory(atlas_path), value);
            }
        }
    }
    
    // Load the atlas texture
    if (!texture_path.IsEmpty()) {
        // Try different image formats
        Vector<String> extensions = {".png", ".jpg", ".jpeg", ".bmp"};
        
        for (const String& ext : extensions) {
            String full_path = texture_path + ext;
            if (FileExists(full_path)) {
                texture_path = full_path;
                break;
            }
        }
        
        // Load the actual texture
        String ext = ToLower(GetFileExt(texture_path));
        if (ext == ".png") {
            PNGDecoder decoder;
            atlas_texture = decoder.LoadFile(texture_path);
        } else if (ext == ".jpg" || ext == ".jpeg") {
            JPEGDecoder decoder;
            atlas_texture = decoder.LoadFile(texture_path);
        } else {
            atlas_texture = StreamRaster::LoadFile(texture_path);
        }
    }
    
    return !atlas_texture.IsEmpty();
}

bool TextureAtlas::LoadFromTextureAndRegions(const String& texture_path, 
                                            const Vector<AtlasRegion>& regions) {
    // Load the main atlas texture
    String ext = ToLower(GetFileExt(texture_path));
    if (ext == ".png") {
        PNGDecoder decoder;
        atlas_texture = decoder.LoadFile(texture_path);
    } else if (ext == ".jpg" || ext == ".jpeg") {
        JPEGDecoder decoder;
        atlas_texture = decoder.LoadFile(texture_path);
    } else {
        atlas_texture = StreamRaster::LoadFile(texture_path);
    }
    
    if (atlas_texture.IsEmpty()) {
        LOG("Could not load atlas texture: " << texture_path);
        return false;
    }
    
    // Copy the regions
    this->regions = regions;
    
    // Create the lookup map
    for (int i = 0; i < regions.GetCount(); i++) {
        region_map.GetAdd(regions[i].name) = i;
    }
    
    return true;
}

bool TextureAtlas::LoadFromImages(const VectorMap<String, String>& image_paths, 
                                 int atlas_width, int atlas_height, int padding) {
    return PackAndCreateAtlas(image_paths, atlas_width, atlas_height, padding);
}

const AtlasRegion* TextureAtlas::GetRegion(const String& name) const {
    int idx = region_map.Find(name);
    if (idx >= 0) {
        return &regions[idx];
    }
    return nullptr;
}

void TextureAtlas::AddRegion(const String& name, const Image& image, const Rect& bounds) {
    AtlasRegion region;
    region.name = name;
    region.image = image;
    region.bounds = bounds;
    region.original_size = image.GetSize();
    region.original_bounds = bounds;
    region.offset = Point(0, 0);
    
    regions.Add(region);
    region_map.GetAdd(name) = regions.GetCount() - 1;
}

Image TextureAtlas::PackImages(const Vector<Image>& images, 
                              Vector<Rect>& output_rects, 
                              int atlas_width, int atlas_height, int padding) {
    // Create an image buffer for the atlas
    ImageBuffer atlas(atlas_width, atlas_height);
    atlas.Clear(0); // Clear to transparent black
    
    // Simple shelf packing algorithm
    Vector<int> shelves;  // Top of each shelf
    Vector<int> shelf_heights;  // Height of each shelf
    Vector<Rect> placements;  // Where each image was placed
    
    int current_y = 0;
    int current_x = 0;
    int current_shelf_height = 0;
    
    for (int i = 0; i < images.GetCount(); i++) {
        Size img_size = images[i].GetSize();
        
        // Check if image is too large for the atlas
        if (img_size.cx + padding > atlas_width || img_size.cy + padding > atlas_height) {
            LOG("Image " << i << " is too large for atlas");
            continue;
        }
        
        // Check if we need a new shelf
        if (img_size.cy + padding > current_shelf_height) {
            // Start a new shelf
            if (current_y + current_shelf_height + img_size.cy + padding > atlas_height) {
                // No more space in atlas
                break;
            }
            
            // Save the current shelf if it had content
            if (current_shelf_height > 0) {
                shelves.Add(current_y);
                shelf_heights.Add(current_shelf_height);
            }
            
            // Start a new shelf
            current_y += current_shelf_height;
            current_x = 0;
            current_shelf_height = img_size.cy + padding;
        }
        
        // Check if we need to go to the next row in current shelf
        if (current_x + img_size.cx + padding > atlas_width) {
            // Start a new row in the same shelf
            current_x = 0;
            current_y += current_shelf_height;  // Move to next shelf position
            current_shelf_height = img_size.cy + padding;  // Adjust shelf height if needed
        }
        
        // Place the image
        if (current_y + img_size.cy <= atlas_height && current_x + img_size.cx <= atlas_width) {
            // Copy image to atlas
            for (int y = 0; y < img_size.cy; y++) {
                const RGBA* src_row = images[i][y];
                RGBA* dst_row = atlas[current_y + y] + current_x;
                memcpy(dst_row, src_row, img_size.cx * sizeof(RGBA));
            }
            
            // Record placement
            Rect placement(current_x, current_y, current_x + img_size.cx, current_y + img_size.cy);
            placements.Add(placement);
            
            // Move to next position
            current_x += img_size.cx + padding;
            
            // Update shelf height if this image is taller
            if (img_size.cy + padding > current_shelf_height) {
                current_shelf_height = img_size.cy + padding;
            }
        }
    }
    
    // Save the last shelf
    if (current_shelf_height > 0) {
        shelves.Add(current_y);
        shelf_heights.Add(current_shelf_height);
    }
    
    output_rects = placements;
    return Image(atlas);
}

bool TextureAtlas::PackAndCreateAtlas(const VectorMap<String, String>& image_paths,
                                     int atlas_width, int atlas_height, int padding) {
    // Load all images first
    Vector<Image> images;
    Vector<String> names;
    Vector<String> paths;
    
    for (int i = 0; i < image_paths.GetCount(); i++) {
        String name = image_paths.GetKey(i);
        String path = image_paths[i];
        
        Image img;
        String ext = ToLower(GetFileExt(path));
        if (ext == ".png") {
            PNGDecoder decoder;
            img = decoder.LoadFile(path);
        } else if (ext == ".jpg" || ext == ".jpeg") {
            JPEGDecoder decoder;
            img = decoder.LoadFile(path);
        } else {
            img = StreamRaster::LoadFile(path);
        }
        
        if (!img.IsEmpty()) {
            images.Add(img);
            names.Add(name);
            paths.Add(path);
        }
    }
    
    if (images.IsEmpty()) {
        LOG("No images could be loaded for atlas packing");
        return false;
    }
    
    // Pack the images
    Vector<Rect> placements;
    atlas_texture = PackImages(images, placements, atlas_width, atlas_height, padding);
    
    if (atlas_texture.IsEmpty()) {
        LOG("Could not create atlas texture");
        return false;
    }
    
    // Create AtlasRegion entries
    for (int i = 0; i < min(placements.GetCount(), names.GetCount()); i++) {
        AtlasRegion region;
        region.name = names[i];
        region.image = images[i];
        region.bounds = placements[i];
        region.original_size = images[i].GetSize();
        region.original_bounds = placements[i];
        region.offset = Point(0, 0);
        
        regions.Add(region);
        region_map.GetAdd(region.name) = regions.GetCount() - 1;
    }
    
    return true;
}

END_UPP_NAMESPACE