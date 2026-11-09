#include "SweetSandy.h"

NAMESPACE_UPP

namespace {
const RGBA kSeafloorColor = MakeRGBA(150, 120, 70);
const Color kWaterTint(60, 110, 180);
const byte kWaterAlpha = 200;
const double kWaterLevel = 0.0;
const double kSeafloorY = -3.0;
}

void GLDebugWater::GLPaint()
{
	frames_painted++;
	Size sz = GetSize();
	LOG("GLDebugWater::GLPaint: frame=" << frames_painted << " widget_size=" << sz.cx << "x" << sz.cy);
	if(sz.cx <= 0 || sz.cy <= 0) {
		LOG("GLDebugWater::GLPaint: size is zero - widget was never laid out");
		return;
	}

	RGBA bg = MakeRGBA(10, 10, 10); // distinct from both the seafloor and water colors
	glViewport(0, 0, (GLsizei)sz.cx, (GLsizei)sz.cy);
	glClearColor(bg.r / 255.0f, bg.g / 255.0f, bg.b / 255.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDisable(GL_LIGHTING);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, (double)sz.cx / sz.cy, 0.5, 200.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	Vec3 eye(0, 15, 15);
	Vec3 look_at(0, kSeafloorY, 0);
	gluLookAt(eye.x, eye.y, eye.z, look_at.x, look_at.y, look_at.z, 0, 1, 0);

	// Opaque "seafloor" - drawn first (establishes both color and depth
	// for the water plane to blend/test against). Deliberately smaller
	// than the water plane's own footprint (view_radius=60 below) so a
	// framebuffer capture shows a real boundary - background+water in
	// the corners, seafloor+water in the middle - not just one solid
	// blended color filling the whole frame.
	glColor3ub(kSeafloorColor.r, kSeafloorColor.g, kSeafloorColor.b);
	glBegin(GL_QUADS);
		glVertex3d(-12, kSeafloorY, -12);
		glVertex3d( 12, kSeafloorY, -12);
		glVertex3d( 12, kSeafloorY,  12);
		glVertex3d(-12, kSeafloorY,  12);
	glEnd();

	DrawWaterGL(eye, kWaterLevel, 60.0, kWaterTint, kWaterAlpha);

	// The look-at point sits on the seafloor, directly under the water
	// plane from the camera's own view - so its projected screen
	// position is exactly where both surfaces overlap in the final
	// rendered image.
	GLLogProjectedPoint("look_at (on seafloor, under water)", look_at.x, look_at.y, look_at.z);
	double modelview[16], proj[16];
	int viewport[4];
	glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
	glGetDoublev(GL_PROJECTION_MATRIX, proj);
	glGetIntegerv(GL_VIEWPORT, viewport);
	double sx, sy, sz2;
	glFinish();
	if(gluProject(look_at.x, look_at.y, look_at.z, modelview, proj, viewport, &sx, &sy, &sz2)) {
		last_overlap_pixel = GLReadPixel(sz.cx, sz.cy, (int)sx, (int)sy);
		last_overlap_pixel_valid = true;
		LOG("GLDebugWater::GLPaint: overlap pixel at (" << (int)sx << "," << (int)sy << ") = ("
		    << (int)last_overlap_pixel.r << "," << (int)last_overlap_pixel.g << "," << (int)last_overlap_pixel.b << ")");
	} else {
		last_overlap_pixel_valid = false;
		LOG("GLDebugWater::GLPaint: gluProject failed for the overlap sample point");
	}

	GLFrameStats st = GLReadFrameStats(sz.cx, sz.cy, bg);
	GLLogFrameStats("GLDebugWater", st);
	last_object_percent = st.ObjectPercent();

	if(!pending_framebuffer_dump.IsEmpty()) {
		GLSaveFramebufferPNG(sz.cx, sz.cy, pending_framebuffer_dump);
		pending_framebuffer_dump.Clear();
	}
}

END_UPP_NAMESPACE
