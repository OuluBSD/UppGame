#ifndef UPP_GAMEWINDOW_H
#define UPP_GAMEWINDOW_H

/*
These are included indirectly in cpp files via main header:
#include <Core/Core.h>
#include <Draw/Draw.h>
#include <CtrlCore/CtrlCore.h>
#include <Geometry/Geometry.h>
#include <GameLib/GameLib.h>
#include <api/Screen/Screen.h>
#include <api/Graphics/Graphics.h>
#include <Eon/Eon.h>
*/

// NAMESPACE_UPP // We are in upp namespace by main header includes

// GameWindow class - intended to be included from GameEngine.h which opens the namespace

class GameWindow : public TopWindow {
public:
	virtual void     Paint(Draw& w) override;
	virtual bool     Key(dword key, int count) override;
	virtual void     LeftDown(Point p, dword flags) override;
	virtual void     LeftUp(Point p, dword flags) override;
	virtual void     MouseMove(Point p, dword flags) override;
	virtual void     MouseWheel(Point p, int zdelta, dword flags) override;
	virtual void     GotFocus() override;
	virtual void     LostFocus() override;

public:
	typedef GameWindow CLASSNAME;

	GameWindow();
	virtual ~GameWindow();

	// Game-specific methods
	void SetGameLoopCallback(std::function<void()> callback);
	void SetRenderCallback(std::function<void(Draw&)> callback);
	void SetUpdateCallback(std::function<void(double)> callback);

	void StartGameLoop();
	void StopGameLoop();
	bool IsGameRunning() const { return game_running; }
	
	void SetTargetFPS(int fps);
	int GetTargetFPS() const { return target_fps; }

	// Geometry integration methods
	void SetViewport(int x, int y, int width, int height);
	void SetProjectionMatrix(const Matrix4& proj_matrix);
	void SetViewMatrix(const Matrix4& view_matrix);
	void SetModelMatrix(const Matrix4& model_matrix);
	Matrix4 GetProjectionMatrix() const { return projection_matrix; }
	Matrix4 GetViewMatrix() const { return view_matrix; }
	Matrix4 GetModelMatrix() const { return model_matrix; }
	
	// Transform utilities
	Point3 TransformPoint(const Point3& point) const;
	Vector3 TransformVector(const Vector3& vector) const;

	// Screen API integration methods
	void SetScreenContext(ScrContext* context);
	void SetScreenSink(ScrSinkDevice* sink);
	void SetScreenEvents(ScrEventsBase* events);
	ScrContext* GetScreenContext() const { return screen_context; }
	ScrSinkDevice* GetScreenSink() const { return screen_sink; }
	ScrEventsBase* GetScreenEvents() const { return screen_events; }
	
	// Platform-specific window handle access
	void* GetPlatformWindowHandle() const;
	
	// Input handling integration
	void SetRawInputMode(bool enabled);
	bool IsRawInputEnabled() const { return raw_input_mode; }
	
	// Graphics API integration
	void SetRenderingContext(void* context);
	void* GetRenderingContext() const { return rendering_context; }
	
	// Input system integration
	void SetInputSystem(std::shared_ptr<InputSystem> input_system);
	std::shared_ptr<InputSystem> GetInputSystem() const { return input_system; }
	
	// Rendering methods
	void SetClearColor(double r, double g, double b, double a = 1.0);
	void ClearBuffers();
	void SwapBuffers();
	
	// Render state management
	void EnableDepthTest(bool enable);
	void EnableCulling(bool enable);
	void SetCullFace(int face); // GFX_FRONT, GFX_BACK, etc.
	void SetBlendMode(int mode); // Add, Multiply, etc.
	
	// Rendering utilities
	void RenderLine(const Point3& start, const Point3& end, Color color = White());
	void RenderTriangle(const Point3& v1, const Point3& v2, const Point3& v3, Color color = White());
	void RenderQuad(const Point3& v1, const Point3& v2, const Point3& v3, const Point3& v4, Color color = White());

private:
	void GameLoop();
	
	std::function<void()> game_loop_callback;
	std::function<void(Draw&)> render_callback;
	std::function<void(double)> update_callback;
	
	bool game_running = false;
	int target_fps = 60;
	int frame_time_ms = 1000 / 60;

	Thread game_thread;
	Mutex game_mutex;
	
	// Geometry-related properties
	Rect viewport;
	Matrix4 projection_matrix;
	Matrix4 view_matrix;
	Matrix4 model_matrix;
	
	// Screen API integration
	ScrContext* screen_context = nullptr;
	ScrSinkDevice* screen_sink = nullptr;
	ScrEventsBase* screen_events = nullptr;
	bool raw_input_mode = false;
	
	// Graphics API integration
	void* rendering_context = nullptr;
	double clear_color[4] = {0.0, 0.0, 0.0, 1.0}; // RGBA
	bool depth_test_enabled = true;
	bool culling_enabled = true;
	int cull_face = 0; // GFX_BACK by default
	int blend_mode = 0;
	
	// Input system
	std::shared_ptr<InputSystem> input_system;
};

//END_UPP_NAMESPACE

#endif
