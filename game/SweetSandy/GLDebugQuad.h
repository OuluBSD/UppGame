#ifndef _SweetSandy_GLDebugQuad_h_
#define _SweetSandy_GLDebugQuad_h_

// Minimal, isolated OpenGL sanity check: one quad, known camera, known
// background. Used by `--gltest` to verify basic GL rendering works at all
// (viewport/projection/clear/draw/readback) before trusting the much more
// complex PlayScreen scene. Logs world/screen positions, z, and the
// background-vs-object pixel percentage on every paint.
class GLDebugQuad : public GLCtrl {
public:
	typedef GLDebugQuad CLASSNAME;

	int    frames_painted = 0;
	double last_object_percent = -1;

	void GLPaint() override;
};

#endif
