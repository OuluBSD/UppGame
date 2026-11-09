#ifndef UPP_UISYSTEM_H
#define UPP_UISYSTEM_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <CtrlCore/CtrlCore.h>
#include <Geometry/Geometry.h>
#include <GameLib/GameLib.h>

NAMESPACE_UPP

// Forward declarations
class UIElement;
class UIImage;
class UILabel;
class UIButton;
class UIContainer;

// UI System class for rendering HUD and UI elements
class UISystem {
public:
	UISystem();
	virtual ~UISystem();
	
	// Initialize the UI system
	bool Initialize();
	void Uninitialize();
	
	// Update UI system (for animations, input handling, etc.)
	void Update(double dt);
	
	// Render UI elements
	void Render(Draw& draw, const Rect& viewport);
	
	// Create UI elements
	std::shared_ptr<UIElement> CreateElement();
	std::shared_ptr<UIImage> CreateImage();
	std::shared_ptr<UILabel> CreateLabel(const String& text = String());
	std::shared_ptr<UIButton> CreateButton(const String& text = String());
	std::shared_ptr<UIContainer> CreateContainer();
	
	// Add/remove elements from the system
	void AddElement(std::shared_ptr<UIElement> element);
	void RemoveElement(std::shared_ptr<UIElement> element);
	void ClearElements();
	
	// Input handling - returns true if UI consumed the event
	bool HandleInput(int x, int y, dword keyflags);  // For keyboard input
	bool HandleMouseMove(int x, int y);
	bool HandleMouseDown(int x, int y, int button);  // 0=left, 1=right, 2=middle
	bool HandleMouseUp(int x, int y, int button);
	bool HandleMouseWheel(int x, int y, int delta);
	
	// Set/get viewport for UI rendering
	void SetViewport(const Rect& viewport) { this->viewport = viewport; }
	Rect GetViewport() const { return viewport; }
	
	// Enable/disable the UI system
	void SetEnabled(bool enabled) { this->enabled = enabled; }
	bool IsEnabled() const { return enabled; }

private:
	// UI elements
	Vector<std::shared_ptr<UIElement>> elements;
	
	// Viewport and rendering state
	Rect viewport;
	bool enabled = true;
	
	// Mouse state
	Point mouse_pos;
	bool mouse_buttons[3] = {false, false, false};
	
	// Find element at position
	std::shared_ptr<UIElement> FindElementAt(int x, int y);
};

// Base UI element class
class UIElement : public Moveable<UIElement> {
public:
	UIElement();
	virtual ~UIElement() = default;
	
	// Element properties
	void SetPosition(int x, int y) { pos = Point(x, y); }
	void SetSize(int w, int h) { size = Size(w, h); }
	void SetPositionAndSize(int x, int y, int w, int h) { SetPosition(x, y); SetSize(w, h); }
	
	void SetVisible(bool visible) { this->visible = visible; }
	void SetEnabled(bool enabled) { this->enabled = enabled; }
	
	Point GetPosition() const { return pos; }
	Size GetSize() const { return size; }
	Rect GetRect() const { return Rect(pos, size); }
	bool IsVisible() const { return visible; }
	bool IsEnabled() const { return enabled; }
	
	// Transform methods
	void SetTransform(const Point& position, double rotation = 0.0, const Point& scale = Point(1, 1));
	Point GetTransformPosition() const { return pos; }
	double GetTransformRotation() const { return rotation; }
	Point GetTransformScale() const { return scale; }
	
	// Rendering
	virtual void Render(Draw& draw, const Rect& viewport) = 0;
	
	// Input handling - return true if element consumed the event
	virtual bool HandleInput(int x, int y, dword keyflags) { return false; }
	virtual bool HandleMouseMove(int x, int y) { return false; }
	virtual bool HandleMouseDown(int x, int y, int button) { return false; }
	virtual bool HandleMouseUp(int x, int y, int button) { return false; }
	virtual bool HandleMouseWheel(int x, int y, int delta) { return false; }
	
	// Check if point is inside this element
	virtual bool Contains(int x, int y) const;
	
	// Set/get z-order (higher z-order renders on top)
	void SetZOrder(int z) { z_order = z; }
	int GetZOrder() const { return z_order; }

protected:
	Point pos = Point(0, 0);
	Size size = Size(100, 30);
	bool visible = true;
	bool enabled = true;
	int z_order = 0;
	
	// Transform properties
	double rotation = 0.0;
	Point scale = Point(1, 1);
};

// Image UI element
class UIImage : public UIElement {
public:
	UIImage();
	virtual ~UIImage() = default;
	
	// Set/get image
	void SetImage(const Image& img) { image = img; }
	const Image& GetImage() const { return image; }
	
	// Set/get color tint
	void SetTintColor(Color color) { tintColor = color; }
	Color GetTintColor() const { return tintColor; }
	
	void Render(Draw& draw, const Rect& viewport) override;

private:
	Image image;
	Color tintColor = Color(255, 255, 255);  // No tint by default
};

// Label UI element
class UILabel : public UIElement {
public:
	UILabel(const String& text = String());
	virtual ~UILabel() = default;
	
	// Set/get text
	void SetText(const String& text) { this->text = text; }
	const String& GetText() const { return text; }
	
	// Set/get text properties
	void SetTextColor(Color color) { textColor = color; }
	void SetTextFont(Font font) { textFont = font; }
	Color GetTextColor() const { return textColor; }
	Font GetTextFont() const { return textFont; }
	
	void Render(Draw& draw, const Rect& viewport) override;

private:
	String text;
	Color textColor = Color(255, 255, 255);  // White by default
	Font textFont = StdFont(12);
};

// Button UI element
class UIButton : public UIElement {
public:
	UIButton(const String& text = String());
	virtual ~UIButton() = default;
	
	// Set/get text
	void SetText(const String& text) { this->text = text; }
	const String& GetText() const { return text; }
	
	// Set/get button properties
	void SetTextColor(Color color) { textColor = color; }
	void SetBackgroundColor(Color color) { backgroundColor = color; }
	void SetBorderColor(Color color) { borderColor = color; }
	
	Color GetTextColor() const { return textColor; }
	Color GetBackgroundColor() const { return backgroundColor; }
	Color GetBorderColor() const { return borderColor; }
	
	// Set callback for when button is clicked
	void SetClickCallback(std::function<void()> callback) { click_callback = callback; }
	
	void Render(Draw& draw, const Rect& viewport) override;
	bool HandleMouseDown(int x, int y, int button) override;
	bool HandleMouseUp(int x, int y, int button) override;

private:
	String text;
	Color textColor = Color(0, 0, 0);         // Black text by default
	Color backgroundColor = Color(200, 200, 200);  // Light gray by default
	Color borderColor = Color(100, 100, 100);     // Dark gray by default
	
	// State
	bool is_pressed = false;
	
	// Callbacks
	std::function<void()> click_callback;
};

// Container UI element (for grouping other elements)
class UIContainer : public UIElement {
public:
	UIContainer();
	virtual ~UIContainer() = default;
	
	// Add/remove child elements
	void AddChild(std::shared_ptr<UIElement> element);
	void RemoveChild(std::shared_ptr<UIElement> element);
	void ClearChildren();
	
	// Get child count
	int GetChildCount() const { return children.GetCount(); }
	std::shared_ptr<UIElement> GetChild(int index) const { return children[index]; }
	
	void Render(Draw& draw, const Rect& viewport) override;
	
	// Input handling - propagate to children
	bool HandleInput(int x, int y, dword keyflags) override;
	bool HandleMouseMove(int x, int y) override;
	bool HandleMouseDown(int x, int y, int button) override;
	bool HandleMouseUp(int x, int y, int button) override;
	bool HandleMouseWheel(int x, int y, int delta) override;

private:
	Vector<std::shared_ptr<UIElement>> children;
};

END_UPP_NAMESPACE

#endif