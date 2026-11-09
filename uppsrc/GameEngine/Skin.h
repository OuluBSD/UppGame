#ifndef UPP_SKIN_H
#define UPP_SKIN_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <CtrlCore/CtrlCore.h>
#include <GameLib/GameLib.h>
#include <GameEngine/UISystem.h>

NAMESPACE_UPP

// Forward declarations
class TextureRegion;
class NinePatch;

// Skin class for visual customization of UI elements
class Skin {
public:
    Skin();
    virtual ~Skin();

    // Load skin from a file or resource
    bool Load(const String& skinPath);
    void Clear();

    // Set/get colors for different UI states
    void SetColor(const String& name, Color color);
    Color GetColor(const String& name) const;

    // Set/get fonts for different UI elements
    void SetFont(const String& name, Font font);
    Font GetFont(const String& name) const;

    // Set/get textures for different UI elements
    void SetTexture(const String& name, const Image& texture);
    Image GetTexture(const String& name) const;

    // Set/get nine-patch textures for scalable UI elements
    void SetNinePatch(const String& name, const NinePatch& ninePatch);
    NinePatch GetNinePatch(const String& name) const;

    // Get default UI styles
    Font GetDefaultFont() const;
    Color GetDefaultTextColor() const;
    Color GetDefaultBackgroundColor() const;
    Color GetDefaultBorderColor() const;

    // Apply skin to a UI element
    void ApplyToButton(std::shared_ptr<UIButton> button, const String& style = "default");
    void ApplyToLabel(std::shared_ptr<UILabel> label, const String& style = "default");
    void ApplyToTextField(std::shared_ptr<UITextField> textField, const String& style = "default");
    void ApplyToImage(std::shared_ptr<UIImage> image, const String& style = "default");

    // Get texture region for a specific UI element state
    TextureRegion GetRegion(const String& name) const;

private:
    HashMap<String, Color> colors;
    HashMap<String, Font> fonts;
    HashMap<String, Image> textures;
    HashMap<String, NinePatch> ninePatches;
    HashMap<String, TextureRegion> regions;

    // Default values
    Font defaultFont = StdFont(12);
    Color defaultTextColor = Color(255, 255, 255);
    Color defaultBackgroundColor = Color(200, 200, 200);
    Color defaultBorderColor = Color(100, 100, 100);
};

// TextureRegion - represents a rectangular region of a texture
class TextureRegion {
public:
    TextureRegion();
    TextureRegion(const Image& img, int x, int y, int width, int height);
    TextureRegion(const Image& img);

    void SetRegion(const Image& img, int x, int y, int width, int height);
    void SetRegion(const Image& img);

    Image GetImage() const { return image; }
    Rect GetRegion() const { return region; }

    int GetX() const { return region.left; }
    int GetY() const { return region.top; }
    int GetWidth() const { return region.Width(); }
    int GetHeight() const { return region.Height(); }

private:
    Image image;
    Rect region;
};

// NinePatch - for scalable UI elements
class NinePatch {
public:
    NinePatch();
    NinePatch(const Image& img, int left, int right, int top, int bottom);
    NinePatch(const TextureRegion& region, int left, int right, int top, int bottom);

    void SetRegion(const Image& img, int left, int right, int top, int bottom);
    void SetRegion(const TextureRegion& region, int left, int right, int top, int bottom);

    void Draw(Draw& draw, const Rect& rect) const;

    int GetLeft() const { return left; }
    int GetRight() const { return right; }
    int GetTop() const { return top; }
    int GetBottom() const { return bottom; }

private:
    TextureRegion region;
    int left, right, top, bottom;
    Vector<int> slicesX;
    Vector<int> slicesY;
    Vector<Image> patches;
};

END_UPP_NAMESPACE

#endif