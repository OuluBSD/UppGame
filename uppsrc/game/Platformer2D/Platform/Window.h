#ifndef _Platformer2D_Platform_Window_h_
#define _Platformer2D_Platform_Window_h_

#include "Platform.h"
#include "Timer.h"
#include <CtrlLib/CtrlLib.h>

NAMESPACE_UPP

// ============================================================================
// Main Window Class
// ============================================================================

class Platformer2DWindow : public TopWindow
{
public:
	static bool Create(const Platformer2DConfig& cfg);
	static void Destroy();
	static Platformer2DWindow* GetInstance() { return s_instance; }
	
	// Rendering
	void Paint(Draw& w) override;
	void RenderFrame();
	
	// Window events
	void Layout() override;
	void Close() override;
	bool Key(dword key, int count) override;
	void MouseMove(Point p, dword flags) override;
	Image MouseEvent(int event, Point p, int zdelta, dword flags) override;
	
	// Configuration
	void SetFullscreen(bool fs);
	void SetAspectCorrected(bool ac);
	void SetBorder(bool b);
	void SetIntegerScaled(bool is);
	void SetVSync(bool vs);
	
private:
	Platformer2DWindow();
	~Platformer2DWindow();
	
	static Platformer2DWindow* s_instance;
	
	// Display settings
	bool fullscreen = false;
	bool aspectCorrected = true;
	bool border = true;
	bool integerScaled = false;
	bool vsync = true;
	
	// Scaling
	int scale = 1;
	Rect clientRect;
	
	// Framebuffer (from renderer)
	IndexedSurface* framebuffer = nullptr;
	
	// U++ Image for Draw
	Image displayImage;
	
	void UpdateDisplayImage();
	void CalculateScale();
};

END_UPP_NAMESPACE

#endif // _Platformer2D_Platform_Window_h_