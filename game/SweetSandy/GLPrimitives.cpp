#include "SweetSandy.h"

NAMESPACE_UPP

void DrawUnitBox(double sx, double sy, double sz)
{
	double x = sx * 0.5, y = sy * 0.5, z = sz * 0.5;
	glBegin(GL_QUADS);
		glNormal3d(0, 0, 1);  glVertex3d(-x, -y, z); glVertex3d(x, -y, z); glVertex3d(x, y, z); glVertex3d(-x, y, z);
		glNormal3d(0, 0, -1); glVertex3d(-x, y, -z); glVertex3d(x, y, -z); glVertex3d(x, -y, -z); glVertex3d(-x, -y, -z);
		glNormal3d(0, 1, 0);  glVertex3d(-x, y, -z); glVertex3d(-x, y, z); glVertex3d(x, y, z); glVertex3d(x, y, -z);
		glNormal3d(0, -1, 0); glVertex3d(-x, -y, -z); glVertex3d(x, -y, -z); glVertex3d(x, -y, z); glVertex3d(-x, -y, z);
		glNormal3d(1, 0, 0);  glVertex3d(x, -y, -z); glVertex3d(x, y, -z); glVertex3d(x, y, z); glVertex3d(x, -y, z);
		glNormal3d(-1, 0, 0); glVertex3d(-x, -y, z); glVertex3d(-x, y, z); glVertex3d(-x, y, -z); glVertex3d(-x, -y, -z);
	glEnd();
}

GLuint LoadGLTexture(const String& path)
{
	Image img = StreamRaster::LoadFileAny(path);
	if(img.IsEmpty()) {
		LOG("LoadGLTexture: failed to load '" << path << "'");
		return 0;
	}

	GLuint tex = 0;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// U++'s own RGBA struct is NOT r,g,b,a in memory on this platform -
	// uppsrc/Core/Color.h defines it as b,g,r,a outside PLATFORM_MACOS -
	// so the source format below is GL_BGRA, matching Image::Begin()'s
	// own real byte layout directly instead of manually re-swizzling
	// every pixel. Caught by reading Color.h before writing this call,
	// not discovered later as a color-swapped-texture bug.
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.GetWidth(), img.GetHeight(), 0,
	             GL_BGRA, GL_UNSIGNED_BYTE, img.Begin());
	glBindTexture(GL_TEXTURE_2D, 0);
	return tex;
}

void DrawWaterGL(Vec3 camera_pos, double water_level, double view_radius, Color tint, byte alpha)
{
	glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);

	glColor4ub(tint.GetR(), tint.GetG(), tint.GetB(), alpha);
	double cx = camera_pos.x, cz = camera_pos.z;
	glBegin(GL_QUADS);
		glVertex3d(cx - view_radius, water_level, cz - view_radius);
		glVertex3d(cx + view_radius, water_level, cz - view_radius);
		glVertex3d(cx + view_radius, water_level, cz + view_radius);
		glVertex3d(cx - view_radius, water_level, cz + view_radius);
	glEnd();

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	glEnable(GL_LIGHTING);
}

END_UPP_NAMESPACE
