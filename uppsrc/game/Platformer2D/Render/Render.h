#ifndef _Platformer2D_Render_Render_h_
#define _Platformer2D_Render_Render_h_

#include <game/Platformer2D/Platformer2D.h>
#include <game/Platformer2D/Core/Core.h>
#include <game/Platformer2D/Resource/Resource.h>

NAMESPACE_UPP

// ============================================================================
// EGA Palette
// ============================================================================

struct EGAPaletteEntry
{
	uint8_t r, g, b;
	
	EGAPaletteEntry() : r(0), g(0), b(0) {}
	EGAPaletteEntry(uint8_t r_, uint8_t g_, uint8_t b_) : r(r_), g(g_), b(b_) {}
	
	bool operator==(const EGAPaletteEntry& other) const { return r == other.r && g == other.g && b == other.b; }
};

class EGAPalette
{
public:
	// 6 palette sets × 17 entries (16 colors + border)
	EGAPaletteEntry palettes[6][17];

	EGAPalette(); // Defined in Palette.h (calls SetDefaults)

	void SetDefaults();
	void SetPalette(int id, const EGAPaletteEntry* colors);
	void SetBorderColor(int paletteId, const EGAPaletteEntry& color);
	const EGAPaletteEntry* GetPalette(int id) const { return palettes[id]; }
	EGAPaletteEntry GetColor(int paletteId, int colorIndex) const;
	EGAPaletteEntry GetBorderColor(int paletteId) const { return palettes[paletteId][16]; }

	// Fade operations
	void FadeToBlack(int paletteId, int steps = 16);
	void FadeFromBlack(int paletteId, const EGAPaletteEntry* target, int steps = 16);
	void FadePalettes(int fromId, int toId, int steps);

	// Convert to U++ RGB palette for Image
	void ToUppPalette(RGBA* out256) const;
};

// ============================================================================
// Indexed Surface (8-bit, 320x200)
// ============================================================================

class IndexedSurface
{
public:
	IndexedSurface();
	IndexedSurface(int width, int height);
	~IndexedSurface();

	void Create(int width, int height);
	void Destroy();

	bool IsValid() const { return data != nullptr; }
	int GetWidth() const { return width; }
	int GetHeight() const { return height; }
	int GetPitch() const { return pitch; }
	Size GetSize() const { return Size(width, height); }

	// Direct pixel access
	uint8_t GetPixel(int x, int y) const;
	void SetPixel(int x, int y, uint8_t color);

	// Row access
	const uint8_t* GetRow(int y) const;
	uint8_t* GetRow(int y);

	// Clear/Fill
	void Clear(uint8_t color = 0);
	void FillRect(int x, int y, int w, int h, uint8_t color);

	// Copy
	void CopyFrom(const IndexedSurface& src, int dstX, int dstY, int srcX, int srcY, int w, int h);
	void CopyTo(IndexedSurface& dst, int dstX, int dstY, int srcX, int srcY, int w, int h) const;

	// Scroll
	void Scroll(int dx, int dy);

private:
	uint8_t* data = nullptr;
	int width = 0;
	int height = 0;
	int pitch = 0; // Bytes per row (aligned)
	bool owned = true;
};

// ============================================================================
// Renderer State
// ============================================================================

class Renderer
{
public:
	static void Startup();
	static void Shutdown();
	
	// Screen management
	static void SetScreen(IndexedSurface* surface);
	static IndexedSurface* GetScreen();
	static IndexedSurface* GetBackupSurface();
	static IndexedSurface* GetStatusSurface();
	
	// Palette
	static void SetPalette(int id);
	static void SetPaletteFromArray(const EGAPaletteEntry* palette);
	static void SetBorderColor(uint16_t colorIndex);
	static void ColorBorder(uint16_t colorIndex);
	static void SetDefaultPalette();
	static void FadeToBlack();
	static void FadeFromBlack();
	
	// Drawing primitives
	static void ClearScreen(uint8_t color = 0);
	static void Rect(int x, int y, int w, int h, uint8_t color);
	static void RectPM(int x, int y, int w, int h, uint8_t color, int mapMask);
	
	// Blitting
	static void UnmaskedToScreen(const uint8_t* src, int x, int y, int w, int h);
	static void UnmaskedToScreenPM(const uint8_t* src, int x, int y, int w, int h, int mapMask);
	static void MaskedToScreen(const uint8_t* src, int x, int y, int w, int h);
	static void MaskedBlitToScreen(const uint8_t* src, int x, int y, int w, int h);
	static void MaskedBlitClipToScreen(const uint8_t* src, int x, int y, int w, int h, int dw, int dh);
	
	// 1bpp (font) blitting
	static void Blit1bppToScreen(const uint8_t* src, int x, int y, int w, int h, uint8_t color);
	static void Blit1bppToScreenPM(const uint8_t* src, int x, int y, int w, int h, uint8_t color, int mapMask);
	static void Xor1bppToScreen(const uint8_t* src, int x, int y, int w, int h, uint8_t color);
	static void Blit1bppClipToScreen(const uint8_t* src, int x, int y, int w, int h, int dw, int dh, uint8_t color);
	static void InvBlit1bppToScreen(const uint8_t* src, int x, int y, int w, int h, uint8_t color);
	static void InvBlit1bppClipToScreen(const uint8_t* src, int x, int y, int w, int h, int dw, int dh, uint8_t color);
	
	// Screen-to-screen
	static void ScreenToScreen(int x, int y, int sx, int sy, int sw, int sh);
	static void SurfaceToScreen(const IndexedSurface* src, int x, int y, int sx, int sy, int sw, int sh);
	static void SurfaceToSurface(IndexedSurface* dst, const IndexedSurface* src, int x, int y, int sx, int sy, int sw, int sh);
	
	// Scrolling
	static void ScrollScreen(int x, int y);
	static void SetScrollCoords(int x, int y);
	static int GetScrollX();
	static int GetScrollY();
	
	// Presentation
	static void Present();
	static void UpdateRect(int x, int y, int w, int h);
	static void SwapBuffers();
	
	// Timing
	static void DelayTics(int tics);
	static uint32_t GetTics(int wait = 0);
	static void Yield();
	static void WaitVBLs(int vbls);
	
	// Video parameters
	static void SetParams(bool fullscreen, bool aspectCorrected, bool border, bool integerScaled);
	static void ToggleFullscreen();
	static void ToggleAspect();
	static void ToggleBorder();
	static void ToggleInteger();
	static void ToggleVSync();
	
	// Surface creation
	static IndexedSurface* CreateSurface(int w, int h);
	static void DestroySurface(IndexedSurface* surf);
	
	// Palette access
	static const EGAPalette& GetPalette() { return g_palette; }
	
private:
	static IndexedSurface* g_screen;
	static IndexedSurface* g_backupSurface;
	static IndexedSurface* g_statusSurface;
	static EGAPalette g_palette;
	static int g_scrollX, g_scrollY;
	static bool g_fullscreen, g_aspectCorrected, g_border, g_integerScaled;
	static int g_swapInterval;
};

// ============================================================================
// Font Rendering
// ============================================================================

struct FontChar
{
	uint8_t width;
	uint8_t height;
	uint8_t xOffset;
	uint8_t yOffset;
	const uint8_t* data; // 1bpp bitmap
};

class PF2DFont
{
public:
	PF2DFont();
	bool LoadFromChunk(int chunkNum); // FON_MAINFONT

	int GetCharWidth(char c) const;
	int GetHeight() const { return fontHeight; }

	void DrawText(IndexedSurface* dst, int x, int y, const char* text, uint8_t color) const;
	void DrawTextClipped(IndexedSurface* dst, int x, int y, const char* text, uint8_t color, int clipW, int clipH) const;

	// Multi-line
	void MeasureMultiline(const char* text, int* outW, int* outH) const;
	void DrawMultiline(IndexedSurface* dst, int x, int y, const char* text, uint8_t color, int maxWidth = 0) const;

private:
	FontChar charData[256];
	int fontHeight = 0;
	int spaceWidth = 0;
};

extern PF2DFont g_mainFont;

// Implementation headers (included after class definitions to avoid cycles)
#include <game/Platformer2D/Render/Surface.h>
#include <game/Platformer2D/Render/Palette.h>
#include <game/Platformer2D/Render/Blit.h>
#include <game/Platformer2D/Render/Text.h>

END_UPP_NAMESPACE

#endif // _Platformer2D_Render_Render_h_