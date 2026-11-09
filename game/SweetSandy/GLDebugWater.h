#ifndef _SweetSandy_GLDebugWater_h_
#define _SweetSandy_GLDebugWater_h_

// Island Terrain phase, Task 3: isolated OpenGL sanity check for
// DrawWaterGL, same shape as GLDebugQuad/--gltest - a known opaque
// "seafloor" quad, the water plane drawn over it, and a single sampled
// pixel in the overlap region to directly verify blending actually
// happened (not just "some object pixels exist").
class GLDebugWater : public GLCtrl {
public:
	typedef GLDebugWater CLASSNAME;

	int    frames_painted = 0;
	double last_object_percent = -1;
	RGBA   last_overlap_pixel = MakeRGBA(0, 0, 0);
	bool   last_overlap_pixel_valid = false;
	String pending_framebuffer_dump; // see RequestFramebufferDump

	void GLPaint() override;

	// One-shot: saves the *next* painted frame's framebuffer to path -
	// same convention as PlayScreen::RequestFramebufferDump.
	void RequestFramebufferDump(const String& path) { pending_framebuffer_dump = path; }
};

#endif
