# Platformer2D/Resource Package Guide

## Scope
- Applies to `uppsrc/game/Platformer2D/Resource/`.
- Filesystem paths, episode variables, endian-safe readers, resource archives, decompression, cache/purge policy, maps, graphics, fonts, audio chunks.

## Responsibilities

1. **Filesystem Abstraction**
   - Search paths: game data (KEENPATH), user data (USERPATH), omni data (OMNIPATH)
   - XDG support on Linux
   - Case-insensitive file lookup (DOS compatibility)

2. **Endian-Safe Binary Readers**
   - LE/BE uint8/16/32 readers with bounds checking
   - Strict error handling (no silent truncation)

3. **Resource Archives (GAMEMAPS, EGAGRAPH, AUDIO)**
   - Chunk-based format with EGAHEAD/MAPHEAD/AUDIOHHD/AUDIODCT indexes
   - Chunk loading with decompression pipeline
   - Cache with LRU purge policy (configurable limits)

4. **Decompression Suite**
   - Huffman (EGAGRAPH, AUDIO)
   - Carmack (GAMEMAPS tile data)
   - RLEW (GAMEMAPS RLE words)
   - LZW (EGAGRAPH sprite data)
   - All with strict bounds checking

5. **Episode Variables (EPISODE.CKx)**
   - Parse variable definitions (bool, int, string, arrays, actions)
   - Generated declarations for game logic

6. **Map System**
   - MAPHEAD chunk index
   - Plane decoding (background, foreground, info, collision)
   - Tile info (TILEINFO) and sprite info (EGAHEAD/EGADICT)

7. **Graphics Resources**
   - 8-bit indexed chunks (tiles, sprites, fonts, pics)
   - Palette handling (EGA 16-color + border)
   - Masked/unmasked blit formats

8. **Audio Resources**
   - AdLib/OPL music chunks (IMF format)
   - Digitized sound chunks (PC speaker / Sound Blaster)
   - AudioHuffman decoding

## Key Types

- `CA_CacheEntry` - Cache entry with refcount, purgeable flag, size
- `CA_ChunkInfo` - Chunk metadata (offset, length, compressed, type)
- `MAP_MapHeader` - Map plane offsets and dimensions
- `EGA_Sprite` - 8-bit indexed sprite with mask
- `IMF_MusicTrack` - AdLib music sequence

## Cache Policy

- Mark/lock/unlock pattern (MarkGrChunk, CacheMarks, LockGrChunk, UnlockGrChunk)
- Purgeable when unlocked and memory pressure
- Configurable max cache size (default: 4MB matching original)

## Binary Compatibility

- All chunk formats match OmniSpeak `id_ca.c` exactly
- Huffman trees from EGAHEAD/AUDIOHHD
- Carmack/RLEW decoding produces identical tile data
- LZW produces identical sprite data

## Dependencies

- Uses: `Platformer2D` (main package), `Core` (for game state types)
- Used by: `Core`, `Render`, `Audio`, `Platformer2DTest`

## Error Handling

- All readers return `bool` success + output parameter
- Invalid offsets/lengths return false, do not crash
- Corrupted data logs warning and returns empty/default