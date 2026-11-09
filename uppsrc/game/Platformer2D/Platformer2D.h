#ifndef _Platformer2D_Platformer2D_h_
#define _Platformer2D_Platformer2D_h_

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <CtrlLib/CtrlLib.h>
#include <Geometry/Geometry.h>
#include <Sound/Sound.h>

NAMESPACE_UPP

// Forward declarations for core types
class Platformer2DCore;
class Platformer2DResource;
class Platformer2DRender;
class Platformer2DAudio;
class Platformer2DPlatform;

// Episode identifiers
enum class Episode : uint8_t
{
	KEEN4 = 4,
	KEEN5 = 5,
	KEEN6 = 6
};

// Fixed-step simulation clock contract
// Target: 70 Hz (140 Hz for some timing) matching original PIT model
inline constexpr int PLATFORMER2D_TICKS_PER_SECOND = 70;
inline constexpr int PLATFORMER2D_MS_PER_TICK = 1000 / PLATFORMER2D_TICKS_PER_SECOND;

// Fixed-point math constants (16.16 format matching original)
using Fixed16 = int32_t;
inline constexpr Fixed16 FIXED16_ONE = 0x10000;
inline constexpr Fixed16 FIXED16_HALF = 0x8000;

// Palette and display constants matching original EGA 320x200
inline constexpr int EGA_WIDTH = 320;
inline constexpr int EGA_HEIGHT = 200;
inline constexpr int EGA_PALETTE_SIZE = 256;
inline constexpr int EGA_PLANES = 4;

// Object system constants
inline constexpr int MAX_OBJECTS = 100;
inline constexpr int OBJECT_FIELDS = 38;

// Game state dump schema version (matches dumpprinter)
inline constexpr int DUMP_SCHEMA_VERSION = 1;

END_UPP_NAMESPACE

#endif // _Platformer2D_Platformer2D_h_