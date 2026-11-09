#include "SweetSandy.h"

NAMESPACE_UPP

void GLDebugQuad::GLPaint()
{
	frames_painted++;
	Size sz = GetSize();
	LOG("GLDebugQuad::GLPaint: frame=" << frames_painted << " widget_size=" << sz.cx << "x" << sz.cy);
	if(sz.cx <= 0 || sz.cy <= 0) {
		LOG("GLDebugQuad::GLPaint: size is zero - widget was never laid out "
		    "(missing .SizePos() when it was added to its parent)");
		return;
	}

	RGBA bg = MakeRGBA(10, 10, 10);
	glViewport(0, 0, (GLsizei)sz.cx, (GLsizei)sz.cy);
	glClearColor(bg.r / 255.0f, bg.g / 255.0f, bg.b / 255.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, (double)sz.cx / sz.cy, 0.5, 100.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0, 0, 5,  0, 0, 0,  0, 1, 0);

	// Quad from (-1,-1,0) to (1,1,0), centered on the camera's look-at point.
	double half = 1.0;
	GLLogProjectedPoint("quad corner (-1,-1,0)", -half, -half, 0);
	GLLogProjectedPoint("quad corner ( 1,-1,0)",  half, -half, 0);
	GLLogProjectedPoint("quad corner ( 1, 1,0)",  half,  half, 0);
	GLLogProjectedPoint("quad corner (-1, 1,0)", -half,  half, 0);
	GLLogProjectedPoint("camera eye  (0,0,5)",    0, 0, 5);

	glColor3ub(220, 40, 40);
	glBegin(GL_QUADS);
		glVertex3d(-half, -half, 0);
		glVertex3d( half, -half, 0);
		glVertex3d( half,  half, 0);
		glVertex3d(-half,  half, 0);
	glEnd();

	glFinish();
	GLFrameStats st = GLReadFrameStats(sz.cx, sz.cy, bg);
	GLLogFrameStats("GLDebugQuad", st);
	last_object_percent = st.ObjectPercent();
}

END_UPP_NAMESPACE
