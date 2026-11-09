#ifndef _SweetSandy_GLPrimitives_h_
#define _SweetSandy_GLPrimitives_h_

// Small immediate-mode GL shape-drawing helpers shared across whatever
// draws simple flat-colored geometry. Promoted here from a PlayScreen.cpp-
// local static (used for the player vehicle's own chassis box) once AI
// traffic (Task 2, 006-ai-traffic-and-pedestrians) became a second real
// consumer (a simplified AI car body) - the same "wait for a second real
// consumer before sharing" call already made for SandyMath.h's own
// RandomRange, not written speculatively ahead of time.
//
// Call only from inside a current GL context, between a color/material
// set-up and the matrix that positions this box - these draw in local,
// origin-centered space only.

// A box of size sx*sy*sz, centered on the origin, with correct outward
// per-face normals for flat/lit shading.
void DrawUnitBox(double sx, double sy, double sz);

// Unified Ground phase, Task 1: loads path via StreamRaster (same call
// Terrain::Load already uses for heightmaps) and uploads it as a GL
// texture. Returns 0 (logs, does not ASSERT_) on failure - a missing
// texture file should draw untextured, not crash the game. Call only
// from inside a current GL context (same rule as DrawUnitBox above) -
// never from a headless self-test.
GLuint LoadGLTexture(const String& path);

// Island Terrain phase, Task 3: an unlit, translucent flat quad at
// world y=water_level, re-centered under camera_pos every call (not a
// fixed-size world object) so it always reaches the horizon regardless
// of camera position, without needing real infinite geometry - not
// real water simulation, just a simple sea-level marker. Depth-tested
// (correctly hidden behind anything nearer the camera, e.g. a hill
// above sea level) but does not write depth itself - standard
// translucency practice. Unlike DrawUnitBox above, this positions
// itself in world space directly (camera_pos), not local/origin-
// centered space - call it without any extra glTranslated of your own.
void DrawWaterGL(Vec3 camera_pos, double water_level, double view_radius, Color tint, byte alpha);

#endif
