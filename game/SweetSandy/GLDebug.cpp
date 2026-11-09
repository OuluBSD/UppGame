#include "SweetSandy.h"

NAMESPACE_UPP

GLFrameStats GLReadFrameStats(int width, int height, RGBA bg, int tolerance)
{
	GLFrameStats st;
	st.width = width;
	st.height = height;
	if(width <= 0 || height <= 0)
		return st;

	Buffer<byte> pixels(width * height * 3);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);

	auto Near = [tolerance](int a, int b) { return abs(a - b) <= tolerance; };

	const byte *p = pixels;
	for(int i = 0; i < width * height; i++, p += 3) {
		bool is_bg = Near(p[0], bg.r) && Near(p[1], bg.g) && Near(p[2], bg.b);
		if(is_bg)
			st.background_pixels++;
		else
			st.object_pixels++;
	}
	return st;
}

bool GLLogProjectedPoint(const char *label, double wx, double wy, double wz)
{
	double modelview[16], proj[16];
	int viewport[4];
	glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
	glGetDoublev(GL_PROJECTION_MATRIX, proj);
	glGetIntegerv(GL_VIEWPORT, viewport);

	double sx, sy, sz;
	int ok = gluProject(wx, wy, wz, modelview, proj, viewport, &sx, &sy, &sz);

	bool onscreen = ok && sx >= viewport[0] && sx <= viewport[0] + viewport[2]
	             && sy >= viewport[1] && sy <= viewport[1] + viewport[3]
	             && sz >= 0.0 && sz <= 1.0;

	LOG("GLDebug: " << label << " world=(" << wx << ", " << wy << ", " << wz << ")"
	    << " -> screen=(" << sx << ", " << sy << ") depth=" << sz
	    << " gluProject_ok=" << (ok ? "yes" : "no")
	    << " onscreen=" << (onscreen ? "yes" : "no")
	    << " viewport=(" << viewport[0] << "," << viewport[1] << "," << viewport[2] << "," << viewport[3] << ")");
	return onscreen;
}

void GLLogFrameStats(const char *label, const GLFrameStats& st)
{
	LOG("GLDebug: " << label << " " << st.width << "x" << st.height
	    << " background=" << Format("%.1f", st.BackgroundPercent()) << "%"
	    << " object=" << Format("%.1f", st.ObjectPercent()) << "%"
	    << " (bg_px=" << st.background_pixels << " obj_px=" << st.object_pixels << ")");
}

bool GLSaveFramebufferPNG(int width, int height, const String& path)
{
	if(width <= 0 || height <= 0)
		return false;

	Buffer<byte> pixels(width * height * 3);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);

	ImageBuffer ib(width, height);
	for(int y = 0; y < height; y++) {
		// OpenGL's row 0 is the bottom of the image; ImageBuffer's row 0
		// is the top - flip while copying.
		const byte *src = pixels + (size_t)(height - 1 - y) * width * 3;
		RGBA *dst = ib[y];
		for(int x = 0; x < width; x++) {
			dst[x].r = src[x * 3 + 0];
			dst[x].g = src[x * 3 + 1];
			dst[x].b = src[x * 3 + 2];
			dst[x].a = 255;
		}
	}

	Image img(ib);
	bool ok = PNGEncoder().SaveFile(path, img);
	LOG("GLDebug: GLSaveFramebufferPNG " << width << "x" << height << " -> " << path
	    << " ok=" << (ok ? "yes" : "no"));
	return ok;
}

RGBA GLReadPixel(int width, int height, int x, int y)
{
	RGBA c = MakeRGBA(0, 0, 0);
	if(x < 0 || y < 0 || x >= width || y >= height)
		return c;
	byte px[3];
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(x, y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, px);
	c.r = px[0]; c.g = px[1]; c.b = px[2];
	return c;
}

END_UPP_NAMESPACE
