#ifndef _Platformer2D_Render_Blit_h_
#define _Platformer2D_Render_Blit_h_

// Types are defined in Render.h which includes this header
// Forward declaration in UPP namespace
class IndexedSurface;

#include <algorithm>
#include <cstdint>

// ============================================================================
// Blit Namespace - All blitting operations
// ============================================================================

namespace Blit
{
	// Unmasked blit (solid copy)
	void Unmasked(IndexedSurface* dst, const uint8_t* src, int x, int y, int w, int h);
	void UnmaskedPM(IndexedSurface* dst, const uint8_t* src, int x, int y, int w, int h, int mapMask);
	
	// Masked blit (transparency: color 0 = transparent)
	void Masked(IndexedSurface* dst, const uint8_t* src, int x, int y, int w, int h);
	void MaskedBlit(IndexedSurface* dst, const uint8_t* src, int x, int y, int w, int h);
	void MaskedBlitClip(IndexedSurface* dst, const uint8_t* src, int x, int y, int w, int h, int dw, int dh);
	
	// 1bpp blit (font, 1 bit per pixel, expanded to color)
	void Blit1bpp(IndexedSurface* dst, const uint8_t* src, int x, int y, int w, int h, uint8_t color);
	void Blit1bppPM(IndexedSurface* dst, const uint8_t* src, int x, int y, int w, int h, uint8_t color, int mapMask);
	void Xor1bpp(IndexedSurface* dst, const uint8_t* src, int x, int y, int w, int h, uint8_t color);
	void Blit1bppClip(IndexedSurface* dst, const uint8_t* src, int x, int y, int w, int h, int dw, int dh, uint8_t color);
	void InvBlit1bpp(IndexedSurface* dst, const uint8_t* src, int x, int y, int w, int h, uint8_t color);
	void InvBlit1bppClip(IndexedSurface* dst, const uint8_t* src, int x, int y, int w, int h, int dw, int dh, uint8_t color);
	
	// Surface to surface
	void SurfaceToSurface(IndexedSurface* dst, const IndexedSurface* src, int x, int y, int sx, int sy, int sw, int sh);
	
	// Clipping helper
	struct ClipRect
	{
		int x, y, w, h;
		ClipRect() : x(0), y(0), w(0), h(0) {}
		ClipRect(int x_, int y_, int w_, int h_) : x(x_), y(y_), w(w_), h(h_) {}
		
		bool Intersect(const ClipRect& other)
		{
			int x1 = std::max(x, other.x);
			int y1 = std::max(y, other.y);
			int x2 = std::min(x + w, other.x + other.w);
			int y2 = std::min(y + h, other.y + other.h);
			if (x1 >= x2 || y1 >= y2) { w = h = 0; return false; }
			x = x1; y = y1; w = x2 - x1; h = y2 - y1;
			return true;
		}
	};
	
	ClipRect GetSurfaceClipRect(const IndexedSurface* surf);
}


#endif // _Platformer2D_Render_Blit_h_