#include "TexturePacker.h"
#include <plugin/png/png.h>
#include <plugin/jpg/jpg.h>

NAMESPACE_UPP

TexturePacker::TexturePacker() {
}

TexturePacker::~TexturePacker() {
}

void TexturePacker::AddImage(const String& name, const Image& img) {
    ImageEntry entry;
    entry.name = name;
    entry.image = img;
    entry.size = img.GetSize();
    images.Add(entry);
}

void TexturePacker::AddImagesFromDirectory(const String& directory, std::shared_ptr<VFS> vfs) {
    if (vfs) {
        // Use VFS to list files
        Vector<FileInfo> files = vfs->ListDirectory(directory);
        for (const auto& file : files) {
            if (file.is_file) {
                String ext = ToLower(GetFileExt(file.name));
                if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp") {
                    String fullPath = AppendFileName(directory, file.name);
                    Image img = vfs->LoadImage(fullPath);
                    if (img.GetWidth() > 0 && img.GetHeight() > 0) {
                        AddImage(file.name, img);
                    }
                }
            }
        }
    } else {
        // Use regular file system
        FindFile ff(AppendFileName(directory, "*.*"));
        while (ff.Find()) {
            if (!ff.IsDirectory()) {
                String ext = ToLower(GetFileExt(ff.GetName()));
                if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp") {
                    Image img = StreamRaster::LoadFile(ff.GetPath());
                    if (img.GetWidth() > 0 && img.GetHeight() > 0) {
                        AddImage(ff.GetName(), img);
                    }
                }
            }
        }
    }
}

PackNode* TexturePacker::Insert(PackNode* node, int width, int height) {
    if (node->used) {
        PackNode* newNode = Insert(node->right, width, height);
        if (newNode) return newNode;
        return Insert(node->left, width, height);
    } else if (width <= node->width && height <= node->height) {
        if (width == node->width && height == node->height) {
            node->used = true;
            return node;
        }
        
        // Split the node
        node->left = new PackNode();
        node->right = new PackNode();
        
        int dw = node->width - width;
        int dh = node->height - height;
        
        if (dw > dh) {
            // Split vertically
            node->left->x = node->x;
            node->left->y = node->y;
            node->left->width = width;
            node->left->height = node->height;
            
            node->right->x = node->x + width;
            node->right->y = node->y;
            node->right->width = node->width - width;
            node->right->height = node->height;
        } else {
            // Split horizontally
            node->left->x = node->x;
            node->left->y = node->y;
            node->left->width = node->width;
            node->left->height = height;
            
            node->right->x = node->x;
            node->right->y = node->y + height;
            node->right->width = node->width;
            node->right->height = node->height - height;
        }
        
        return Insert(node->left, width, height);
    }
    
    return nullptr;
}

bool TexturePacker::PackInternal(int atlasWidth, int atlasHeight) {
    // Sort images by area (largest first) for better packing
    struct ImageArea {
        int index;
        int area;
        Size size;
    };
    
    Vector<ImageArea> areas;
    for (int i = 0; i < images.GetCount(); i++) {
        ImageArea area;
        area.index = i;
        area.size = images[i].size;
        area.area = (area.size.cx + 2 * padding) * (area.size.cy + 2 * padding);
        areas.Add(area);
    }
    
    areas.Sort([](const ImageArea& a, const ImageArea& b) { return a.area > b.area; });
    
    // Create root node
    PackNode root(0, 0, atlasWidth, atlasHeight);
    
    atlasInfo.Clear();
    
    // Pack each image
    for (const auto& area : areas) {
        int width = area.size.cx + 2 * padding;
        int height = area.size.cy + 2 * padding;
        
        PackNode* node = Insert(&root, width, height);
        if (!node) {
            return false; // Could not fit all images
        }
        
        // Add to atlas info
        TextureAtlasInfo info;
        info.name = images[area.index].name;
        info.bounds = Rect(node->x + padding, node->y + padding, area.size.cx, area.size.cy);
        info.originalSize = area.size;
        atlasInfo.Add(info);
    }
    
    return true;
}

Image TexturePacker::Pack(int atlasWidth, int atlasHeight, int padding) {
    this->padding = padding;
    
    // Attempt different sizes until we find one that fits
    int attemptWidth = atlasWidth, attemptHeight = atlasHeight;
    
    while (true) {
        if (PackInternal(attemptWidth, attemptHeight)) {
            // Create the atlas image
            Image atlas = Image(attemptWidth, attemptHeight, White());
            
            // Draw each image in its position
            for (const auto& info : atlasInfo) {
                int imgIndex = -1;
                for (int i = 0; i < images.GetCount(); i++) {
                    if (images[i].name == info.name) {
                        imgIndex = i;
                        break;
                    }
                }
                
                if (imgIndex >= 0) {
                    const Image& src = images[imgIndex].image;
                    
                    // Draw the source image at the packed position
                    for (int y = 0; y < src.GetHeight(); y++) {
                        for (int x = 0; x < src.GetWidth(); x++) {
                            atlas[info.bounds.top + y][info.bounds.left + x] = src[y][x];
                        }
                    }
                }
            }
            
            return atlas;
        }
        
        // Try a larger atlas if the current one is too small
        if (attemptWidth < 4096 && attemptHeight < 4096) {
            attemptWidth *= 2;
            attemptHeight *= 2;
        } else {
            break;
        }
    }
    
    // If we couldn't pack at any size, return empty image
    return Image();
}

bool TexturePacker::SaveAtlas(const String& atlasPath, const String& infoPath, std::shared_ptr<VFS> vfs) {
    Image atlas = Pack();
    if (atlas.GetWidth() <= 0) return false;
    
    String atlasExt = ToLower(GetFileExt(atlasPath));
    bool saved = false;
    
    if (atlasExt == ".png") {
        PNGEncoder pngEnc;
        saved = pngEnc.SaveFile(atlasPath, atlas);
    } else if (atlasExt == ".jpg" || atlasExt == ".jpeg") {
        JPEGEncoder jpgEnc;
        saved = jpgEnc.SaveFile(atlasPath, atlas);
    }
    
    if (!saved && !vfs) return false;
    
    // If using VFS, save the atlas there as well
    if (vfs) {
        if (atlasExt == ".png") {
            PNGEncoder pngEnc;
            String data = pngEnc.SaveString(atlas);
            vfs->SaveString(atlasPath, data);
        } else if (atlasExt == ".jpg" || atlasExt == ".jpeg") {
            JPEGEncoder jpgEnc;
            String data = jpgEnc.SaveString(atlas);
            vfs->SaveString(atlasPath, data);
        }
    }
    
    // Save the atlas info as a simple text format
    String infoContent = "atlasWidth " + IntStr(atlas.GetWidth()) + "\n";
    infoContent += "atlasHeight " + IntStr(atlas.GetHeight()) + "\n";
    infoContent += "images " + IntStr(atlasInfo.GetCount()) + "\n";
    
    for (const auto& info : atlasInfo) {
        infoContent += info.name + " " + 
                      IntStr(info.bounds.left) + " " + 
                      IntStr(info.bounds.top) + " " + 
                      IntStr(info.bounds.right) + " " + 
                      IntStr(info.bounds.bottom) + " " + 
                      IntStr(info.originalSize.cx) + " " + 
                      IntStr(info.originalSize.cy) + "\n";
    }
    
    if (vfs) {
        vfs->SaveString(infoPath, infoContent);
    } else {
        FileOut out(infoPath);
        if (out.IsOpen()) {
            out.Write(infoContent);
        }
    }
    
    return true;
}

void TexturePacker::Clear() {
    images.Clear();
    atlasInfo.Clear();
}

// FontGenerator implementation
Image FontGenerator::GenerateFontAtlas(Font font, const String& characters, 
                                      int& cellWidth, int& cellHeight) {
    // Calculate cell size based on font
    Size textSize = GetTextSize("W", font); // Use 'W' as widest character approximation
    cellWidth = textSize.cx + 4; // Add padding
    cellHeight = textSize.cy + 4; // Add padding
    
    int cols = (int)ceil(sqrt(characters.GetLength()));
    if (cols == 0) cols = 1;
    int rows = (int)ceil((double)characters.GetLength() / cols);
    
    int atlasWidth = cols * cellWidth;
    int atlasHeight = rows * cellHeight;
    
    Image atlas = Image(atlasWidth, atlasHeight, White());
    
    // Draw each character
    int idx = 0;
    for (int i = 0; i < characters.GetLength(); i++) {
        int col = idx % cols;
        int row = idx / cols;
        
        int x = col * cellWidth + 2; // 2-pixel padding
        int y = row * cellHeight + 2; // 2-pixel padding
        
        Draw& d = Single<Draw>().Begin(atlas);
        d.DrawText(x, y, String(characters[i]), font, Black());
        Single<Draw>().End();
        
        idx++;
    }
    
    return atlas;
}

Image FontGenerator::GenerateFontAtlas(Font font, const Vector<int>& characterCodes,
                                      int& cellWidth, int& cellHeight) {
    // Calculate cell size based on font
    Size textSize = GetTextSize("W", font); // Use 'W' as widest character approximation
    cellWidth = textSize.cx + 4; // Add padding
    cellHeight = textSize.cy + 4; // Add padding
    
    int cols = (int)ceil(sqrt(characterCodes.GetCount()));
    if (cols == 0) cols = 1;
    int rows = (int)ceil((double)characterCodes.GetCount() / cols);
    
    int atlasWidth = cols * cellWidth;
    int atlasHeight = rows * cellHeight;
    
    Image atlas = Image(atlasWidth, atlasHeight, White());
    
    // Draw each character
    for (int i = 0; i < characterCodes.GetCount(); i++) {
        int col = i % cols;
        int row = i / cols;
        
        int x = col * cellWidth + 2; // 2-pixel padding
        int y = row * cellHeight + 2; // 2-pixel padding
        
        String charStr = String().Cat() << (int)characterCodes[i];
        
        Draw& d = Single<Draw>().Begin(atlas);
        d.DrawText(x, y, charStr, font, Black());
        Single<Draw>().End();
    }
    
    return atlas;
}

bool FontGenerator::SaveFontAtlas(const Image& atlas, const String& characters, 
                                 int cellWidth, int cellHeight, const String& outputPrefix,
                                 std::shared_ptr<VFS> vfs) {
    String atlasPath = outputPrefix + ".png";
    String infoPath = outputPrefix + ".fnt"; // Standard font file extension
    
    // Save atlas image
    PNGEncoder pngEnc;
    String imageData = pngEnc.SaveString(atlas);
    
    if (vfs) {
        vfs->SaveString(atlasPath, imageData);
    } else {
        FileOut out(atlasPath);
        if (out.IsOpen()) {
            out.Write(imageData);
        } else {
            return false;
        }
    }
    
    // Save font info
    String infoContent = "info face=\"Arial\" size=" + IntStr(cellHeight-4) + " bold=0 italic=0 charset=\"\" unicode=1 stretchH=100 smooth=1 aa=1 padding=0,0,0,0 spacing=2,2\n";
    infoContent += "common lineHeight=" + IntStr(cellHeight) + " base=" + IntStr(cellHeight-2) + " scaleW=" + IntStr(atlas.GetWidth()) + " scaleH=" + IntStr(atlas.GetHeight()) + " pages=1 packed=0\n";
    infoContent += "page id=0 file=\"" + atlasPath + "\"\n";
    infoContent += "chars count=" + IntStr(characters.GetLength()) + "\n";
    
    // Calculate character positions (simplified - assumes all characters are in a grid)
    int cols = (int)ceil(sqrt(characters.GetLength()));
    if (cols == 0) cols = 1;
    
    for (int i = 0; i < characters.GetLength(); i++) {
        int col = i % cols;
        int row = i / cols;
        
        int x = col * cellWidth;
        int y = row * cellHeight;
        
        infoContent += "char id=" + IntStr((int)characters[i]) + " x=" + IntStr(x) + " y=" + IntStr(y) + 
                      " width=" + IntStr(cellWidth-4) + " height=" + IntStr(cellHeight-4) + 
                      " xoffset=0 yoffset=0 xadvance=" + IntStr(cellWidth-4) + " page=0 chnl=15\n";
    }
    
    if (vfs) {
        vfs->SaveString(infoPath, infoContent);
    } else {
        FileOut out(infoPath);
        if (out.IsOpen()) {
            out.Write(infoContent);
        } else {
            return false;
        }
    }
    
    return true;
}

END_UPP_NAMESPACE