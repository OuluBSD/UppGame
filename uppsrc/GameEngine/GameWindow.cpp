#include "GameEngine.h"
#include "GameWindow.h"
#include <Core/Core.h>
#include <Draw/Draw.h>
#include <CtrlCore/CtrlCore.h>
#include <Geometry/Geometry.h>
#include <thread>
#include <chrono>

NAMESPACE_UPP

// The class GameWindow is defined within the Upp namespace via GameEngine.h
// All method implementations are thus in the Upp namespace automatically

GameWindow::GameWindow() {
	Title("Game Window");
	Sizeable(true);
	NoCenter();
	
	// Initialize default matrices
	projection_matrix.data.SetIdentity();
	view_matrix.data.SetIdentity();
	model_matrix.data.SetIdentity();
	
	// Set default viewport to full window size
	Size sz = GetSize();
	viewport = Rect(0, 0, sz.cx, sz.cy);
}

GameWindow::~GameWindow() {
	StopGameLoop();
}

void GameWindow::Paint(Draw& w) {
	// Call custom render callback if set
	if (render_callback) {
		render_callback(w);
	} else {
		// Default rendering - clear with black
		w.DrawRect(GetSize(), White());
	}
	
	// If game is running, we might want to handle rendering via the game loop
	// instead of the UI paint cycle for better performance
}

bool GameWindow::Key(dword key, int count) {
	// Handle game-specific key events
	switch(key) {
		case K_ESCAPE:
			Break();
			return true;
		default:
			break;
	}
	
	// Call base class implementation
	return TopWindow::Key(key, count);
}

void GameWindow::LeftDown(Point p, dword flags) {
	// Handle mouse input for game
	TopWindow::LeftDown(p, flags);
}

void GameWindow::LeftUp(Point p, dword flags) {
	// Handle mouse input for game
	TopWindow::LeftUp(p, flags);
}

void GameWindow::MouseMove(Point p, dword flags) {
	// Handle mouse input for game
	TopWindow::MouseMove(p, flags);
}

void GameWindow::MouseWheel(Point p, int zdelta, dword flags) {
	// Handle mouse wheel input for game
	TopWindow::MouseWheel(p, zdelta, flags);
}

void GameWindow::GotFocus() {
	// Handle game focus events
	TopWindow::GotFocus();
}

void GameWindow::LostFocus() {
	// Handle game focus events
	TopWindow::LostFocus();
}

void GameWindow::SetGameLoopCallback(std::function<void()> callback) {
	game_loop_callback = callback;
}

void GameWindow::SetRenderCallback(std::function<void(Draw&)> callback) {
	render_callback = callback;
}

void GameWindow::SetUpdateCallback(std::function<void(double)> callback) {
	update_callback = callback;
}

void GameWindow::StartGameLoop() {
	if (game_running) return;
	
	game_running = true;
	
	// Start game thread
	game_thread.Run([this]() { 
		GameLoop(); 
	});
}

void GameWindow::StopGameLoop() {
	if (!game_running) return;
	
	game_running = false;
	// Check if thread is running by attempting to join if it's active
	// Check the game running state instead of directly checking thread
	if (game_running) {
		game_thread.Wait();
	}
}

void GameWindow::SetTargetFPS(int fps) {
	target_fps = fps;
	if (target_fps > 0) {
		frame_time_ms = 1000 / target_fps;
	} else {
		frame_time_ms = 0; // Unlimited FPS
	}
}

void GameWindow::SetViewport(int x, int y, int width, int height) {
	viewport = Rect(x, y, x + width, y + height);
}

void GameWindow::SetProjectionMatrix(const Matrix4& proj_matrix) {
	projection_matrix = proj_matrix;
}

void GameWindow::SetViewMatrix(const Matrix4& view_matrix_param) {
	view_matrix = view_matrix_param;
}

void GameWindow::SetModelMatrix(const Matrix4& model_matrix_param) {
	model_matrix = model_matrix_param;
}

Point3 GameWindow::TransformPoint(const Point3& point) const {
	// Apply model, then view, then projection transformation
	// TODO: Actual transformation implementation 
	return point; // Placeholder
}

Vector3 GameWindow::TransformVector(const Vector3& vector) const {
	// Apply model, then view, then projection transformation to vector
	// TODO: Actual transformation implementation 
	return vector; // Placeholder
}

void GameWindow::GameLoop() {
	using namespace std::chrono;
	
	time_point<steady_clock> last_time = steady_clock::now();
	
	while (game_running) {
		auto current_time = steady_clock::now();
		auto elapsed_time = duration_cast<microseconds>(current_time - last_time).count() / 1000000.0;
		last_time = current_time;
		
		// Call update callback with delta time
		if (update_callback) {
			update_callback(elapsed_time);
		}
		
		// Call the main game loop callback
		if (game_loop_callback) {
			game_loop_callback();
		}
		
		// Refresh the window to trigger repaint
		Refresh();
		
		// Control frame rate if limited
		if (frame_time_ms > 0) {
			auto frame_end = steady_clock::now();
			auto actual_frame_time = duration_cast<milliseconds>(frame_end - current_time).count();
			int sleep_time = frame_time_ms - actual_frame_time;
			
			if (sleep_time > 0) {
				Sleep(sleep_time);
			}
		}
	}
}

void GameWindow::SetScreenContext(ScrContext* context) {
	screen_context = context;
}

void GameWindow::SetScreenSink(ScrSinkDevice* sink) {
	screen_sink = sink;
}

void GameWindow::SetScreenEvents(ScrEventsBase* events) {
	screen_events = events;
}

void* GameWindow::GetPlatformWindowHandle() const {
	// This would typically return the native window handle
	// The implementation depends on the platform and would need to access
	// the underlying window system (Win32 HWND, X11 Window, etc.)
	// For now, return nullptr, but in a real implementation this would
	// interface with the Screen API to get the handle
	return nullptr;
}

void GameWindow::SetRawInputMode(bool enabled) {
	raw_input_mode = enabled;
	// This would interface with the Screen API to enable/disable raw input
	// depending on the platform
}

void GameWindow::SetRenderingContext(void* context) {
	rendering_context = context;
}

void GameWindow::SetClearColor(double r, double g, double b, double a) {
	clear_color[0] = r;
	clear_color[1] = g;
	clear_color[2] = b;
	clear_color[3] = a;
}

void GameWindow::ClearBuffers() {
	// This would interface with the Graphics API to clear the rendering buffers
	// depending on the backend being used (OpenGL, DirectX, etc.)
}

void GameWindow::SwapBuffers() {
	// This would interface with the Graphics API to swap the front/back buffers
	// for double buffering
}

void GameWindow::EnableDepthTest(bool enable) {
	depth_test_enabled = enable;
	// This would interface with the Graphics API to enable/disable depth testing
}

void GameWindow::EnableCulling(bool enable) {
	culling_enabled = enable;
	// This would interface with the Graphics API to enable/disable face culling
}

void GameWindow::SetCullFace(int face) {
	cull_face = face;
	// This would interface with the Graphics API to set which faces to cull
}

void GameWindow::SetBlendMode(int mode) {
	blend_mode = mode;
	// This would interface with the Graphics API to set the blending mode
}

void GameWindow::RenderLine(const Point3& start, const Point3& end, Color color) {
	// This would interface with the Graphics API to render a 3D line
}

void GameWindow::RenderTriangle(const Point3& v1, const Point3& v2, const Point3& v3, Color color) {
	// This would interface with the Graphics API to render a 3D triangle
}

void GameWindow::RenderQuad(const Point3& v1, const Point3& v2, const Point3& v3, const Point3& v4, Color color) {
	// This would interface with the Graphics API to render a 3D quad
}

END_UPP_NAMESPACE

