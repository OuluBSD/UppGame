# Platformer2D/Render Package Guide

## Scope
- Applies to `uppsrc/game/Platformer2D/Render/`.
- Indexed EGA surface model, palette conversion, sprite/tile/text drawing, scaling, border, presentation-independent framebuffer.

## Responsibilities

1. **Indexed Framebuffer (8-bit, 320x200)**
   - Core rendering target: `uint8_t[320*200]` palette indices
   - Double-buffered for smooth presentation
   - Palette: 256 entries (16 EGA colors + duplicates for fading)

2. **EGA Palette Management**
   - 6 palette sets (id 0-5) × 17 entries (16 colors + border)
   - Fade to/from black/white
   - Palette cycling for water/animation effects
   - Border color (overscan) separate from palette

3. **Sprite/Tile Drawing (8-bit indexed)**
   - Unmasked blit (solid rectangles, tiles)
   - Masked blit (sprites with transparency)
   - 1bpp blit (font characters, masked with color)
   - XOR blit (cursors, selection)
   - Clipped blitting

4. **Text Rendering**
   - Proportional font (FON_MAINFONT)
   - Multi-line measurement and drawing
   - Color mapping via palette

5. **Screen Effects**
   - Pixel panning (smooth horizontal scroll)
   - Screen shake
   - Fade in/out
   - Border color change

6. **Scaling & Presentation (Platform-Independent)**
   - Integer scaling (1x, 2x, 3x...)
   - Aspect correction (320x200 -> 4:3)
   - Overscan border emulation
   - Output: 8-bit indexed framebuffer (platform layer converts to RGB)

7. **Status Bar / UI Surfaces**
   - Separate surfaces for status panel, dropdown menus
   - Backup surface for menu overlay

## Key Types

- `IndexedSurface` - 8-bit surface with palette index
- `EGAPalette` - 256 RGB colors + border
- `VL_Backend` interface adapted to U++ Draw

## Binary Compatibility

- Palette format matches OmniSpeak `vl_palette[6][17]`
- Surface blit semantics match `id_vl.h` exactly
- Framebuffer layout: 320x200, row-major, 1 byte per pixel

## Dependencies

- Uses: `Platformer2D`, `Core` (for game state), `Resource` (for graphics chunks)
- Used by: `Platform`, `Platformer2DTest`

## U++ Draw Integration

- Platform layer creates U++ `Image` from indexed framebuffer
- Palette applied via `SetPalette` on `Image`
- Draw::DrawImage for presentation with scaling