#ifndef _Platformer2D_Render_Palette_h_
#define _Platformer2D_Render_Palette_h_

// Types are defined in Render.h which includes this header

// ============================================================================
// EGAPalette Implementation
// ============================================================================

inline EGAPalette::EGAPalette()
{
	SetDefaults();
}

inline void EGAPalette::SetDefaults()
{
	// Standard EGA 16-color palette (RGB values 0-63 scaled to 0-255)
	static const uint8_t egaColors[16][3] = {
		{0, 0, 0},       // 0: Black
		{0, 0, 170},     // 1: Blue
		{0, 170, 0},     // 2: Green
		{0, 170, 170},   // 3: Cyan
		{170, 0, 0},     // 4: Red
		{170, 0, 170},   // 5: Magenta
		{170, 85, 0},    // 6: Brown
		{170, 170, 170}, // 7: Light Gray
		{85, 85, 85},    // 8: Dark Gray
		{85, 85, 255},   // 9: Light Blue
		{85, 255, 85},   // 10: Light Green
		{85, 255, 255},  // 11: Light Cyan
		{255, 85, 85},   // 12: Light Red
		{255, 85, 255},  // 13: Light Magenta
		{255, 255, 85},  // 14: Yellow
		{255, 255, 255}, // 15: White
	};
	
	for (int p = 0; p < 6; ++p)
	{
		for (int c = 0; c < 16; ++c)
		{
			palettes[p][c] = EGAPaletteEntry(egaColors[c][0], egaColors[c][1], egaColors[c][2]);
		}
		// Border defaults to black
		palettes[p][16] = EGAPaletteEntry(0, 0, 0);
	}
}

inline void EGAPalette::SetPalette(int id, const EGAPaletteEntry* colors)
{
	if (id >= 0 && id < 6 && colors)
	{
		for (int i = 0; i < 17; ++i)
			palettes[id][i] = colors[i];
	}
}

inline void EGAPalette::SetBorderColor(int paletteId, const EGAPaletteEntry& color)
{
	if (paletteId >= 0 && paletteId < 6)
		palettes[paletteId][16] = color;
}

inline EGAPaletteEntry EGAPalette::GetColor(int paletteId, int colorIndex) const
{
	if (paletteId >= 0 && paletteId < 6 && colorIndex >= 0 && colorIndex < 16)
		return palettes[paletteId][colorIndex];
	return EGAPaletteEntry(0, 0, 0);
}

inline void EGAPalette::FadeToBlack(int paletteId, int steps)
{
	if (paletteId < 0 || paletteId >= 6) return;
	
	for (int step = 0; step <= steps; ++step)
	{
		float factor = 1.0f - (float)step / steps;
		for (int c = 0; c < 16; ++c)
		{
			palettes[paletteId][c].r = (uint8_t)(palettes[paletteId][c].r * factor);
			palettes[paletteId][c].g = (uint8_t)(palettes[paletteId][c].g * factor);
			palettes[paletteId][c].b = (uint8_t)(palettes[paletteId][c].b * factor);
		}
		// Border also fades
		palettes[paletteId][16].r = (uint8_t)(palettes[paletteId][16].r * factor);
		palettes[paletteId][16].g = (uint8_t)(palettes[paletteId][16].g * factor);
		palettes[paletteId][16].b = (uint8_t)(palettes[paletteId][16].b * factor);
	}
}

inline void EGAPalette::FadeFromBlack(int paletteId, const EGAPaletteEntry* target, int steps)
{
	if (paletteId < 0 || paletteId >= 6 || !target) return;
	
	for (int step = 0; step <= steps; ++step)
	{
		float factor = (float)step / steps;
		for (int c = 0; c < 16; ++c)
		{
			palettes[paletteId][c].r = (uint8_t)(target[c].r * factor);
			palettes[paletteId][c].g = (uint8_t)(target[c].g * factor);
			palettes[paletteId][c].b = (uint8_t)(target[c].b * factor);
		}
		palettes[paletteId][16].r = (uint8_t)(target[16].r * factor);
		palettes[paletteId][16].g = (uint8_t)(target[16].g * factor);
		palettes[paletteId][16].b = (uint8_t)(target[16].b * factor);
	}
}

inline void EGAPalette::FadePalettes(int fromId, int toId, int steps)
{
	if (fromId < 0 || fromId >= 6 || toId < 0 || toId >= 6) return;
	
	for (int step = 0; step <= steps; ++step)
	{
		float factor = (float)step / steps;
		for (int c = 0; c < 17; ++c)
		{
			palettes[fromId][c].r = (uint8_t)(palettes[fromId][c].r * (1 - factor) + palettes[toId][c].r * factor);
			palettes[fromId][c].g = (uint8_t)(palettes[fromId][c].g * (1 - factor) + palettes[toId][c].g * factor);
			palettes[fromId][c].b = (uint8_t)(palettes[fromId][c].b * (1 - factor) + palettes[toId][c].b * factor);
		}
	}
}

inline void EGAPalette::ToUppPalette(RGBA* out256) const
{
	if (!out256) return;
	
	// Use palette 0 as current
	for (int i = 0; i < 16; ++i)
	{
		out256[i].r = palettes[0][i].r;
		out256[i].g = palettes[0][i].g;
		out256[i].b = palettes[0][i].b;
		out256[i].a = 255;
	}
	// Fill rest with duplicates of first 16 (for 256-color mode)
	for (int i = 16; i < 256; ++i)
	{
		out256[i] = out256[i % 16];
	}
}


#endif // _Platformer2D_Render_Palette_h_