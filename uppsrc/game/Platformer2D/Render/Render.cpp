#include <game/Platformer2D/Render/Render.h>
#include <game/Platformer2D/Render/Surface.h>
#include <game/Platformer2D/Render/Palette.h>
#include <game/Platformer2D/Render/Blit.h>
#include <game/Platformer2D/Render/Text.h>

NAMESPACE_UPP

// ============================================================================
// Renderer Static Members
// ============================================================================

IndexedSurface* Renderer::g_screen = nullptr;
IndexedSurface* Renderer::g_backupSurface = nullptr;
IndexedSurface* Renderer::g_statusSurface = nullptr;
EGAPalette Renderer::g_palette;
int Renderer::g_scrollX = 0;
int Renderer::g_scrollY = 0;
bool Renderer::g_fullscreen = false;
bool Renderer::g_aspectCorrected = true;
bool Renderer::g_border = true;
bool Renderer::g_integerScaled = false;
int Renderer::g_swapInterval = 1;

PF2DFont g_mainFont;

// ============================================================================
// Renderer Implementation
// ============================================================================

void Renderer::Startup()
{
	LOG("Renderer::Startup");
	
	// Create main screen surface (320x200)
	g_screen = new IndexedSurface(EGA_WIDTH, EGA_HEIGHT);
	g_backupSurface = new IndexedSurface(EGA_WIDTH, EGA_HEIGHT);
	g_statusSurface = new IndexedSurface(EGA_WIDTH, 32); // Status bar height
	
	g_palette.SetDefaults();
	g_mainFont.LoadFromChunk(CK_CHUNKNUM(EGAGraphChunk::FON_MAINFONT));
}

void Renderer::Shutdown()
{
	LOG("Renderer::Shutdown");
	
	delete g_screen; g_screen = nullptr;
	delete g_backupSurface; g_backupSurface = nullptr;
	delete g_statusSurface; g_statusSurface = nullptr;
}

void Renderer::SetScreen(IndexedSurface* surface)
{
	g_screen = surface;
}

IndexedSurface* Renderer::GetScreen()
{
	return g_screen;
}

IndexedSurface* Renderer::GetBackupSurface()
{
	return g_backupSurface;
}

IndexedSurface* Renderer::GetStatusSurface()
{
	return g_statusSurface;
}

void Renderer::SetPalette(int id)
{
	if (id >= 0 && id < 6)
		g_palette.ToUppPalette(nullptr); // TODO: Apply to screen
}

void Renderer::SetPaletteFromArray(const EGAPaletteEntry* palette)
{
	// Copy to current palette set
	for (int i = 0; i < 17; ++i)
		g_palette.palettes[0][i] = palette[i];
}

void Renderer::SetBorderColor(uint16_t colorIndex)
{
	// Palette index for border
	if (colorIndex < 16)
		g_palette.palettes[0][16] = g_palette.palettes[0][colorIndex];
}

void Renderer::ColorBorder(uint16_t colorIndex)
{
	SetBorderColor(colorIndex);
}

void Renderer::SetDefaultPalette()
{
	g_palette.SetDefaults();
}

void Renderer::FadeToBlack()
{
	g_palette.FadeToBlack(0);
}

void Renderer::FadeFromBlack()
{
	g_palette.FadeFromBlack(0, g_palette.palettes[0]);
}

void Renderer::ClearScreen(uint8_t color)
{
	if (g_screen) g_screen->Clear(color);
}

void Renderer::Rect(int x, int y, int w, int h, uint8_t color)
{
	if (g_screen) g_screen->FillRect(x, y, w, h, color);
}

void Renderer::RectPM(int x, int y, int w, int h, uint8_t color, int mapMask)
{
	// Map mask not implemented yet
	Rect(x, y, w, h, color);
}

void Renderer::UnmaskedToScreen(const uint8_t* src, int x, int y, int w, int h)
{
	Blit::Unmasked(g_screen, src, x, y, w, h);
}

void Renderer::UnmaskedToScreenPM(const uint8_t* src, int x, int y, int w, int h, int mapMask)
{
	Blit::UnmaskedPM(g_screen, src, x, y, w, h, mapMask);
}

void Renderer::MaskedToScreen(const uint8_t* src, int x, int y, int w, int h)
{
	Blit::Masked(g_screen, src, x, y, w, h);
}

void Renderer::MaskedBlitToScreen(const uint8_t* src, int x, int y, int w, int h)
{
	Blit::MaskedBlit(g_screen, src, x, y, w, h);
}

void Renderer::MaskedBlitClipToScreen(const uint8_t* src, int x, int y, int w, int h, int dw, int dh)
{
	Blit::MaskedBlitClip(g_screen, src, x, y, w, h, dw, dh);
}

void Renderer::Blit1bppToScreen(const uint8_t* src, int x, int y, int w, int h, uint8_t color)
{
	Blit::Blit1bpp(g_screen, src, x, y, w, h, color);
}

void Renderer::Blit1bppToScreenPM(const uint8_t* src, int x, int y, int w, int h, uint8_t color, int mapMask)
{
	Blit::Blit1bppPM(g_screen, src, x, y, w, h, color, mapMask);
}

void Renderer::Xor1bppToScreen(const uint8_t* src, int x, int y, int w, int h, uint8_t color)
{
	Blit::Xor1bpp(g_screen, src, x, y, w, h, color);
}

void Renderer::Blit1bppClipToScreen(const uint8_t* src, int x, int y, int w, int h, int dw, int dh, uint8_t color)
{
	Blit::Blit1bppClip(g_screen, src, x, y, w, h, dw, dh, color);
}

void Renderer::InvBlit1bppToScreen(const uint8_t* src, int x, int y, int w, int h, uint8_t color)
{
	Blit::InvBlit1bpp(g_screen, src, x, y, w, h, color);
}

void Renderer::InvBlit1bppClipToScreen(const uint8_t* src, int x, int y, int w, int h, int dw, int dh, uint8_t color)
{
	Blit::InvBlit1bppClip(g_screen, src, x, y, w, h, dw, dh, color);
}

void Renderer::ScreenToScreen(int x, int y, int sx, int sy, int sw, int sh)
{
	if (g_screen) g_screen->CopyFrom(*g_screen, x, y, sx, sy, sw, sh);
}

void Renderer::SurfaceToScreen(const IndexedSurface* src, int x, int y, int sx, int sy, int sw, int sh)
{
	if (g_screen && src) g_screen->CopyFrom(*src, x, y, sx, sy, sw, sh);
}

void Renderer::SurfaceToSurface(IndexedSurface* dst, const IndexedSurface* src, int x, int y, int sx, int sy, int sw, int sh)
{
	if (dst && src) dst->CopyFrom(*src, x, y, sx, sy, sw, sh);
}

void Renderer::ScrollScreen(int x, int y)
{
	g_scrollX = x;
	g_scrollY = y;
	if (g_screen) g_screen->Scroll(x, y);
}

void Renderer::SetScrollCoords(int x, int y)
{
	g_scrollX = x;
	g_scrollY = y;
}

int Renderer::GetScrollX() { return g_scrollX; }
int Renderer::GetScrollY() { return g_scrollY; }

void Renderer::Present()
{
	// Platform layer will handle actual presentation
	LOG("Renderer::Present - frame ready");
}

void Renderer::UpdateRect(int x, int y, int w, int h)
{
	// Partial update - platform handles
}

void Renderer::SwapBuffers()
{
	// Double buffer swap
	if (g_screen && g_backupSurface)
	{
		// Swap pointers
		IndexedSurface* tmp = g_screen;
		g_screen = g_backupSurface;
		g_backupSurface = tmp;
	}
}

void Renderer::DelayTics(int tics)
{
	// TODO: Use simulation clock
	Sleep(tics * PLATFORMER2D_MS_PER_TICK);
}

uint32_t Renderer::GetTics(int wait)
{
	// TODO: Use simulation clock
	return 0;
}

void Renderer::Yield()
{
	Sleep(1);
}

void Renderer::WaitVBLs(int vbls)
{
	DelayTics(vbls * 2); // Approximate
}

void Renderer::SetParams(bool fullscreen, bool aspectCorrected, bool border, bool integerScaled)
{
	g_fullscreen = fullscreen;
	g_aspectCorrected = aspectCorrected;
	g_border = border;
	g_integerScaled = integerScaled;
	LOG("Renderer::SetParams: fs=" << fullscreen << " aspect=" << aspectCorrected << " border=" << border << " int=" << integerScaled);
}

void Renderer::ToggleFullscreen() { SetParams(!g_fullscreen, g_aspectCorrected, g_border, g_integerScaled); }
void Renderer::ToggleAspect() { SetParams(g_fullscreen, !g_aspectCorrected, g_border, g_integerScaled); }
void Renderer::ToggleBorder() { SetParams(g_fullscreen, g_aspectCorrected, !g_border, g_integerScaled); }
void Renderer::ToggleInteger() { SetParams(g_fullscreen, g_aspectCorrected, g_border, !g_integerScaled); }
void Renderer::ToggleVSync() { g_swapInterval = 1 - g_swapInterval; }

IndexedSurface* Renderer::CreateSurface(int w, int h)
{
	return new IndexedSurface(w, h);
}

void Renderer::DestroySurface(IndexedSurface* surf)
{
	delete surf;
}

END_UPP_NAMESPACE