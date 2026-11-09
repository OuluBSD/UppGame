#include "Skin.h"

NAMESPACE_UPP

// TextureRegion implementation
TextureRegion::TextureRegion() {
}

TextureRegion::TextureRegion(const Image& img, int x, int y, int width, int height) {
    SetRegion(img, x, y, width, height);
}

TextureRegion::TextureRegion(const Image& img) {
    SetRegion(img);
}

void TextureRegion::SetRegion(const Image& img, int x, int y, int width, int height) {
    this->image = img;
    this->region = Rect(x, y, width, height);
}

void TextureRegion::SetRegion(const Image& img) {
    this->image = img;
    this->region = Rect(0, 0, img.GetWidth(), img.GetHeight());
}

// NinePatch implementation
NinePatch::NinePatch() : left(0), right(0), top(0), bottom(0) {
}

NinePatch::NinePatch(const Image& img, int left, int right, int top, int bottom) 
    : left(left), right(right), top(top), bottom(bottom) {
    TextureRegion region(img);
    SetRegion(region, left, right, top, bottom);
}

NinePatch::NinePatch(const TextureRegion& region, int left, int right, int top, int bottom)
    : region(region), left(left), right(right), top(top), bottom(bottom) {
    SetRegion(region, left, right, top, bottom);
}

void NinePatch::SetRegion(const Image& img, int left, int right, int top, int bottom) {
    this->region.SetRegion(img, 0, 0, img.GetWidth(), img.GetHeight());
    SetRegion(this->region, left, right, top, bottom);
}

void NinePatch::SetRegion(const TextureRegion& region, int left, int right, int top, int bottom) {
    this->region = region;
    this->left = left;
    this->right = right;
    this->top = top;
    this->bottom = bottom;

    int width = region.GetWidth();
    int height = region.GetHeight();

    // Define slice coordinates
    slicesX.Clear();
    slicesX.Add(0);
    slicesX.Add(left);
    slicesX.Add(width - right);
    slicesX.Add(width);

    slicesY.Clear();
    slicesY.Add(0);
    slicesY.Add(top);
    slicesY.Add(height - bottom);
    slicesY.Add(height);

    // Create patches
    patches.Clear();
    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 3; x++) {
            int x1 = slicesX[x];
            int y1 = slicesY[y];
            int x2 = slicesX[x+1];
            int y2 = slicesY[y+1];
            
            // Extract patch from the original image
            ImageBuffer ib(x2 - x1, y2 - y1);
            for (int py = 0; py < ib.GetHeight(); py++) {
                RGBA* dest_line = ib[py];
                const RGBA* src_line = region.GetImage()[y1 + py] + x1;
                for (int px = 0; px < ib.GetWidth(); px++) {
                    dest_line[px] = src_line[px];
                }
            }
            
            patches.Add(Image(ib));
        }
    }
}

void NinePatch::Draw(Draw& draw, const Rect& rect) const {
    int width = rect.Width();
    int height = rect.Height();

    // Calculate dimensions for each patch
    int centerWidth = width - left - right;
    int centerHeight = height - top - bottom;

    // Draw corners
    // Top-left
    if (patches.GetCount() > 0) {
        draw.DrawImage(rect.left, rect.top, patches[0]);
    }
    
    // Top-right
    if (patches.GetCount() > 2) {
        draw.DrawImage(rect.left + width - right, rect.top, patches[2]);
    }
    
    // Bottom-left
    if (patches.GetCount() > 6) {
        draw.DrawImage(rect.left, rect.top + height - bottom, patches[6]);
    }
    
    // Bottom-right
    if (patches.GetCount() > 8) {
        draw.DrawImage(rect.left + width - right, rect.top + height - bottom, patches[8]);
    }

    // Draw edges with stretching
    if (patches.GetCount() > 1) {
        // Top edge
        draw.DrawImageRect(Rect(rect.left + left, rect.top, centerWidth, top), patches[1]);
    }
    
    if (patches.GetCount() > 7) {
        // Bottom edge
        draw.DrawImageRect(Rect(rect.left + left, rect.top + height - bottom, centerWidth, bottom), patches[7]);
    }
    
    if (patches.GetCount() > 3) {
        // Left edge
        draw.DrawImageRect(Rect(rect.left, rect.top + top, left, centerHeight), patches[3]);
    }
    
    if (patches.GetCount() > 5) {
        // Right edge
        draw.DrawImageRect(Rect(rect.left + width - right, rect.top + top, right, centerHeight), patches[5]);
    }

    // Draw center
    if (patches.GetCount() > 4) {
        draw.DrawImageRect(Rect(rect.left + left, rect.top + top, centerWidth, centerHeight), patches[4]);
    }
}

// Skin implementation
Skin::Skin() {
    // Initialize default values
    colors.Add("default.textColor", defaultTextColor);
    colors.Add("default.backgroundColor", defaultBackgroundColor);
    colors.Add("default.borderColor", defaultBorderColor);
    fonts.Add("default.font", defaultFont);
}

Skin::~Skin() {
    Clear();
}

bool Skin::Load(const String& skinPath) {
    // For now, we'll just set up default skin values
    // In a real implementation, this would parse a skin file
    Clear();
    
    // Add default UI element styles
    colors.Add("button.default.textColor", Color(0, 0, 0));
    colors.Add("button.default.backgroundColor", Color(200, 200, 200));
    colors.Add("button.default.borderColor", Color(100, 100, 100));
    colors.Add("button.pressed.backgroundColor", Color(170, 170, 170));
    
    colors.Add("label.default.textColor", Color(255, 255, 255));
    colors.Add("label.default.backgroundColor", Color(0, 0, 0, 0)); // Transparent
    
    colors.Add("textfield.default.textColor", Color(0, 0, 0));
    colors.Add("textfield.default.backgroundColor", Color(255, 255, 255));
    colors.Add("textfield.default.borderColor", Color(100, 100, 100));
    
    fonts.Add("default.font", StdFont(12));
    fonts.Add("large.font", StdFont(16));
    fonts.Add("small.font", StdFont(10));
    
    return true;
}

void Skin::Clear() {
    colors.Clear();
    fonts.Clear();
    textures.Clear();
    ninePatches.Clear();
    regions.Clear();
}

void Skin::SetColor(const String& name, Color color) {
    colors.GetAdd(name) = color;
}

Color Skin::GetColor(const String& name) const {
    const Color* c = colors.GetPtr(name);
    return c ? *c : Color(255, 255, 255); // Default to white if not found
}

void Skin::SetFont(const String& name, Font font) {
    fonts.GetAdd(name) = font;
}

Font Skin::GetFont(const String& name) const {
    const Font* f = fonts.GetPtr(name);
    return f ? *f : defaultFont; // Default to standard font if not found
}

void Skin::SetTexture(const String& name, const Image& texture) {
    textures.GetAdd(name) = texture;
}

Image Skin::GetTexture(const String& name) const {
    const Image* img = textures.GetPtr(name);
    return img ? *img : Image(); // Return empty image if not found
}

void Skin::SetNinePatch(const String& name, const NinePatch& ninePatch) {
    ninePatches.GetAdd(name) = ninePatch;
}

NinePatch Skin::GetNinePatch(const String& name) const {
    const NinePatch* np = ninePatches.GetPtr(name);
    return np ? *np : NinePatch(); // Return empty NinePatch if not found
}

Font Skin::GetDefaultFont() const {
    return defaultFont;
}

Color Skin::GetDefaultTextColor() const {
    return defaultTextColor;
}

Color Skin::GetDefaultBackgroundColor() const {
    return defaultBackgroundColor;
}

Color Skin::GetDefaultBorderColor() const {
    return defaultBorderColor;
}

void Skin::ApplyToButton(std::shared_ptr<UIButton> button, const String& style) {
    if (!button) return;
    
    String prefix = "button." + style + ".";
    
    Color textColor = GetColor(prefix + "textColor");
    Color backgroundColor = GetColor(prefix + "backgroundColor");
    Color borderColor = GetColor(prefix + "borderColor");
    
    button->SetTextColor(textColor);
    button->SetBackgroundColor(backgroundColor);
    button->SetBorderColor(borderColor);
    
    // Set font
    Font font = GetFont("default.font");
    // Note: UIButton doesn't currently have font setting method, would need to add one
}

void Skin::ApplyToLabel(std::shared_ptr<UILabel> label, const String& style) {
    if (!label) return;
    
    String prefix = "label." + style + ".";
    
    Color textColor = GetColor(prefix + "textColor");
    Color backgroundColor = GetColor(prefix + "backgroundColor");
    
    label->SetTextColor(textColor);
    // Note: UILabel doesn't currently have background color, would need to add one
}

void Skin::ApplyToTextField(std::shared_ptr<UITextField> textField, const String& style) {
    if (!textField) return;
    
    String prefix = "textfield." + style + ".";
    
    Color textColor = GetColor(prefix + "textColor");
    Color backgroundColor = GetColor(prefix + "backgroundColor");
    Color borderColor = GetColor(prefix + "borderColor");
    
    textField->SetTextColor(textColor);
    textField->SetBackgroundColor(backgroundColor);
    textField->SetBorderColor(borderColor);
    
    // Set font
    Font font = GetFont("default.font");
    textField->SetTextFont(font);
}

void Skin::ApplyToImage(std::shared_ptr<UIImage> image, const String& style) {
    if (!image) return;
    
    // For now, just apply tinting based on color if specified
    String colorName = "image." + style + ".tintColor";
    Color tintColor = GetColor(colorName);
    image->SetTintColor(tintColor);
}

TextureRegion Skin::GetRegion(const String& name) const {
    const TextureRegion* region = regions.GetPtr(name);
    return region ? *region : TextureRegion(); // Return empty region if not found
}

END_UPP_NAMESPACE