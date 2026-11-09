#ifndef UPP_TEXTURE_PACKER_H
#define UPP_TEXTURE_PACKER_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <GameLib/GameLib.h>
#include <GameEngine/VFS.h>
#include <Vector/Vector.h>

NAMESPACE_UPP

// Rectangle packing node for texture atlas generation
struct PackNode {
    int x, y, width, height;
    bool used;
    PackNode* left;
    PackNode* right;
    
    PackNode(int x = 0, int y = 0, int width = 0, int height = 0)
        : x(x), y(y), width(width), height(height), used(false), left(nullptr), right(nullptr) {}
    
    ~PackNode() {
        if (left) delete left;
        if (right) delete right;
    }
};

// TextureAtlasInfo - contains information about packed textures
struct TextureAtlasInfo {
    String name;
    Rect bounds;      // Position and size in the atlas
    Size originalSize; // Original size of the texture
    
    TextureAtlasInfo() {}
    TextureAtlasInfo(const String& name, const Rect& bounds, const Size& originalSize)
        : name(name), bounds(bounds), originalSize(originalSize) {}
};

// TexturePacker - for creating texture atlases from multiple images
class TexturePacker {
public:
    TexturePacker();
    ~TexturePacker();
    
    // Add an image to be packed
    void AddImage(const String& name, const Image& img);
    
    // Add all images from a directory
    void AddImagesFromDirectory(const String& directory, std::shared_ptr<VFS> vfs = nullptr);
    
    // Pack all added images into a texture atlas
    Image Pack(int atlasWidth = 1024, int atlasHeight = 1024, int padding = 1);
    
    // Get information about packed textures
    const Vector<TextureAtlasInfo>& GetAtlasInfo() const { return atlasInfo; }
    
    // Save the atlas and corresponding data file
    bool SaveAtlas(const String& atlasPath, const String& infoPath, std::shared_ptr<VFS> vfs = nullptr);
    
    // Clear all added images
    void Clear();
    
    // Set packing options
    void SetPadding(int padding) { this->padding = padding; }
    int GetPadding() const { return padding; }
    
    void SetPowerOfTwo(bool pot) { powerOfTwo = pot; }
    bool GetPowerOfTwo() const { return powerOfTwo; }
    
    void SetAllowRotation(bool rot) { allowRotation = rot; }
    bool GetAllowRotation() const { return allowRotation; }

private:
    struct ImageEntry {
        String name;
        Image image;
        Size size;
    };
    
    Vector<ImageEntry> images;
    Vector<TextureAtlasInfo> atlasInfo;
    int padding = 1;
    bool powerOfTwo = true;
    bool allowRotation = false;
    
    // Find the best position to place an image
    PackNode* Insert(PackNode* node, int width, int height);
    
    // Try to pack images using a best-fit algorithm
    bool PackInternal(int atlasWidth, int atlasHeight);
};

// Font generation utilities
class FontGenerator {
public:
    // Generate a font atlas from a font and character set
    static Image GenerateFontAtlas(Font font, const String& characters, 
                                  int& cellWidth, int& cellHeight);
                                  
    // Generate font atlas with custom character ranges
    static Image GenerateFontAtlas(Font font, const Vector<int>& characterCodes,
                                  int& cellWidth, int& cellHeight);
    
    // Save font atlas and info
    static bool SaveFontAtlas(const Image& atlas, const String& characters, 
                             int cellWidth, int cellHeight, const String& outputPrefix,
                             std::shared_ptr<VFS> vfs = nullptr);
};

END_UPP_NAMESPACE

#endif