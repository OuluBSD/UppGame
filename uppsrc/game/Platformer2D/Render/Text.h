#ifndef _Platformer2D_Render_Text_h_
#define _Platformer2D_Render_Text_h_

// Types are defined in Render.h which includes this header
#include <algorithm>
#include <cstdint>

// ============================================================================
// Font Implementation
// ============================================================================

inline PF2DFont::PF2DFont()
{
	for (int i = 0; i < 256; ++i)
	{
		charData[i] = FontChar{0, 0, 0, 0, nullptr};
	}
	fontHeight = 0;
	spaceWidth = 0;
}

inline bool PF2DFont::LoadFromChunk(int chunkNum)
{
	// TODO: Load from resource chunk (FON_MAINFONT)
	// Format: 256 characters, each with width, height, data
	// For now, create a basic 8x8 font
	fontHeight = 8;
	spaceWidth = 4;
	
	for (int i = 32; i < 127; ++i)
	{
		charData[i].width = 8;
		charData[i].height = 8;
		charData[i].xOffset = 0;
		charData[i].yOffset = 0;
		// Data would come from chunk
	}
	
	return true;
}

inline int PF2DFont::GetCharWidth(char c) const
{
	uint8_t idx = (uint8_t)c;
	if (idx < 256 && charData[idx].width > 0)
		return charData[idx].width;
	return spaceWidth;
}

inline void PF2DFont::DrawText(IndexedSurface* dst, int x, int y, const char* text, uint8_t color) const
{
	if (!dst || !text) return;
	
	int cx = x;
	while (*text)
	{
		char c = *text++;
		if (c == '\n')
		{
			cx = x;
			y += fontHeight;
			continue;
		}
		
		int cw = GetCharWidth(c);
		// TODO: Actual 1bpp blit from font data
		// Blit::Blit1bpp(dst, charData[(uint8_t)c].data, cx, y, cw, fontHeight, color);
		cx += cw;
	}
}

inline void PF2DFont::DrawTextClipped(IndexedSurface* dst, int x, int y, const char* text, uint8_t color, int clipW, int clipH) const
{
	// Similar to DrawText but with clipping
	DrawText(dst, x, y, text, color);
}

inline void PF2DFont::MeasureMultiline(const char* text, int* outW, int* outH) const
{
	if (!text) { if (outW) *outW = 0; if (outH) *outH = 0; return; }
	
	int maxW = 0, curW = 0;
	int h = fontHeight;
	
	while (*text)
	{
		char c = *text++;
		if (c == '\n')
		{
			maxW = std::max(maxW, curW);
			curW = 0;
			h += fontHeight;
		}
		else
		{
			curW += GetCharWidth(c);
		}
	}
	maxW = std::max(maxW, curW);
	
	if (outW) *outW = maxW;
	if (outH) *outH = h;
}

inline void PF2DFont::DrawMultiline(IndexedSurface* dst, int x, int y, const char* text, uint8_t color, int maxWidth) const
{
	if (!dst || !text) return;
	
	int cx = x;
	int cy = y;
	
	while (*text)
	{
		char c = *text++;
		if (c == '\n')
		{
			cx = x;
			cy += fontHeight;
			continue;
		}
		
		if (maxWidth > 0 && cx + GetCharWidth(c) > x + maxWidth)
		{
			cx = x;
			cy += fontHeight;
		}
		
		// DrawChar(dst, cx, cy, c, color);
		cx += GetCharWidth(c);
	}
}


#endif // _Platformer2D_Render_Text_h_