#include "Window.h"
#include "Input.h"
#include <game/Platformer2D/Render/Render.h>

NAMESPACE_UPP

Platformer2DWindow* Platformer2DWindow::s_instance = nullptr;

Platformer2DWindow::Platformer2DWindow()
{
	Title("Platformer2D - Commander Keen");
	Sizeable().Zoomable().CenterScreen();
	
	// Set minimum size
	SetMinSize(Size(320, 200));
	
	// Icon
	// Icon(Platformer2DImg::icon());
}

Platformer2DWindow::~Platformer2DWindow()
{
	s_instance = nullptr;
}

bool Platformer2DWindow::Create(const Platformer2DConfig& cfg)
{
	if (s_instance) return true;
	
	s_instance = new Platformer2DWindow();
	
	// Apply config
	s_instance->fullscreen = cfg.fullscreen;
	s_instance->aspectCorrected = cfg.aspectCorrected;
	s_instance->border = cfg.border;
	s_instance->integerScaled = cfg.integerScaled;
	s_instance->vsync = cfg.vsync;
	
	s_instance->CalculateScale();
	
	// Get framebuffer from renderer
	s_instance->framebuffer = Renderer::GetScreen();
	
	if (cfg.fullscreen)
		s_instance->FullScreen(true);
	else
		s_instance->Open();
	
	// Start the fixed-step timer
	FixedStepTimer::StartTimer();
	
	return true;
}

void Platformer2DWindow::Destroy()
{
	if (s_instance)
	{
		delete s_instance;
		s_instance = nullptr;
	}
}

void Platformer2DWindow::Paint(Draw& w)
{
	// Clear background
	w.DrawRect(GetSize(), Black());
	
	// Render framebuffer to display image
	RenderFrame();
	
	// Draw display image centered
	if (!displayImage.IsEmpty())
	{
		Rect src(0, 0, displayImage.GetWidth(), displayImage.GetHeight());
		Rect dst = clientRect;
		w.DrawImage(dst.left, dst.top, displayImage, src);
	}
}

void Platformer2DWindow::RenderFrame()
{
	if (!framebuffer) return;
	
	// Convert indexed framebuffer to U++ Image
	// Apply current palette
	RGBA palette[256];
	Renderer::GetPalette().ToUppPalette(palette);
	
	// Create image from indexed data
	// This is a simplified conversion - real implementation would be more efficient
	int w = framebuffer->GetWidth();
	int h = framebuffer->GetHeight();
	
	ImageBuffer ib(w, h);
	for (int y = 0; y < h; ++y)
	{
		const uint8_t* row = framebuffer->GetRow(y);
		RGBA* out = ib[y];
		for (int x = 0; x < w; ++x)
		{
			out[x] = palette[row[x]];
		}
	}
	
	displayImage = ib;
	
	// Apply scaling
	UpdateDisplayImage();
}

void Platformer2DWindow::UpdateDisplayImage()
{
	// Already done in RenderFrame for now
}

void Platformer2DWindow::CalculateScale()
{
	Size sz = GetSize();
	
	if (integerScaled)
	{
		// Integer scaling: find largest integer scale that fits
		scale = 1;
		while (320 * (scale + 1) <= sz.cx && 200 * (scale + 1) <= sz.cy)
			scale++;
	}
	else
	{
		// Stretch to fit (with or without aspect correction)
		if (aspectCorrected)
		{
			// 320x200 -> 4:3 aspect
			double aspect = 4.0 / 3.0;
			double winAspect = (double)sz.cx / sz.cy;
			
			if (winAspect > aspect)
			{
				// Window wider - fit height
				scale = max(1, sz.cy / 200);
			}
			else
			{
				// Window taller - fit width
				scale = max(1, sz.cx / 320);
			}
		}
		else
		{
			// Stretch to fill
			scale = max(1, min(sz.cx / 320, sz.cy / 200));
		}
	}
	
	// Calculate centered client rect
	int dw = 320 * scale;
	int dh = 200 * scale;
	
	// Add border if enabled
	if (border)
	{
		dw += 16 * scale; // 8 pixels each side
		dh += 16 * scale;
	}
	
	clientRect = Rect(
		(sz.cx - dw) / 2,
		(sz.cy - dh) / 2,
		dw, dh
	);
}

void Platformer2DWindow::Layout()
{
	CalculateScale();
	Refresh();
	TopWindow::Layout();
}

void Platformer2DWindow::Close()
{
	Platformer2D_PlatformShutdown();
	TopWindow::Close();
}

bool Platformer2DWindow::Key(dword key, int count)
{
	if (count > 0)
		InputMapper::OnKeyDown(key);
	else
		InputMapper::OnKeyUp(key);
	return TopWindow::Key(key, count);
}

void Platformer2DWindow::MouseMove(Point p, dword flags)
{
	InputMapper::OnMouseMove(p);
	TopWindow::MouseMove(p, flags);
}

Image Platformer2DWindow::MouseEvent(int event, Point p, int zdelta, dword flags)
{
	InputMapper::OnMouseEvent(event, p, zdelta);
	return TopWindow::MouseEvent(event, p, zdelta, flags);
}

void Platformer2DWindow::SetFullscreen(bool fs)
{
	fullscreen = fs;
	FullScreen(fs);
	CalculateScale();
	Refresh();
}

void Platformer2DWindow::SetAspectCorrected(bool ac)
{
	aspectCorrected = ac;
	CalculateScale();
	Refresh();
}

void Platformer2DWindow::SetBorder(bool b)
{
	border = b;
	CalculateScale();
	Refresh();
}

void Platformer2DWindow::SetIntegerScaled(bool is)
{
	integerScaled = is;
	CalculateScale();
	Refresh();
}

void Platformer2DWindow::SetVSync(bool vs)
{
	vsync = vs;
	// TODO: Apply to renderer
}

END_UPP_NAMESPACE