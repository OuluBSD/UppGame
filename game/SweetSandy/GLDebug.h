#ifndef _SweetSandy_GLDebug_h_
#define _SweetSandy_GLDebug_h_

// Small OpenGL rendering diagnostics, built to debug "the window renders
// nothing sensible" reports without relying on screenshots: read the
// framebuffer back with glReadPixels and report what fraction of pixels are
// background vs. something else, and log gluProject'd screen positions for
// known world points so expected vs. actual can be compared by eye in the
// log. Call only from inside a current GL context (i.e. from GLPaint, after
// drawing, before the buffer swap).
struct GLFrameStats {
	int width = 0, height = 0;
	int background_pixels = 0;
	int object_pixels = 0;

	double ObjectPercent() const {
		int total = width * height;
		return total > 0 ? 100.0 * object_pixels / total : 0.0;
	}
	double BackgroundPercent() const {
		int total = width * height;
		return total > 0 ? 100.0 * background_pixels / total : 0.0;
	}
};

// RGBA's field order is platform-dependent (see uppsrc/Core/Color.h) - build
// it by field name instead of a positional constructor it doesn't have.
inline RGBA MakeRGBA(byte r, byte g, byte b, byte a = 255)
{
	RGBA c;
	c.r = r; c.g = g; c.b = b; c.a = a;
	return c;
}

// Classifies every pixel as "background" (within tolerance of bg on all
// three channels) or "object" and counts them. width/height must match the
// current viewport.
GLFrameStats GLReadFrameStats(int width, int height, RGBA bg, int tolerance = 10);

// Projects a world point with the current MODELVIEW/PROJECTION/VIEWPORT and
// logs world -> screen (x, y, z in [0,1] depth) - "sijainnit näytöllä" /
// "z arvot" per the debugging request. Returns false (and still logs) if
// gluProject fails.
bool GLLogProjectedPoint(const char *label, double wx, double wy, double wz);

// Logs GLFrameStats plus a sanity verdict (LOG only, no assertion - callers
// decide what "sane" means for their scene).
void GLLogFrameStats(const char *label, const GLFrameStats& st);

// Saves the current GL framebuffer as a PNG via glReadPixels - reads only
// this application's own rendered pixels, never the desktop/X display, so
// it's safe to use despite this project otherwise avoiding screenshots
// (see the top of this file): the risk that established the no-screenshot
// policy was unrelated content elsewhere on the display, which this never
// touches - same reasoning GLReadFrameStats above already relies on. Call
// only from inside a current GL context (i.e. from GLPaint, after drawing,
// before the buffer swap - call glFinish() first).
bool GLSaveFramebufferPNG(int width, int height, const String& path);

// Island Terrain phase, Task 3: reads back a single pixel - x/y in
// OpenGL's own bottom-up viewport convention (same as glReadPixels and
// gluProject's own returned screen y, so a GLLogProjectedPoint result
// can be fed straight in with no flip). Used to directly verify
// blending happened (e.g. GL_BLEND water over a known-colored surface
// produces a color between the two, not either pure color) - something
// GLReadFrameStats' own background/object split can't tell apart from
// a solid opaque quad.
RGBA GLReadPixel(int width, int height, int x, int y);

#endif
