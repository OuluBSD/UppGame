#include "UISystem.h"

NAMESPACE_UPP

// UIElement implementation
UIElement::UIElement() {
}

void UIElement::SetTransform(const Point& position, double rotation, const Point& scale) {
	pos = position;
	this->rotation = rotation;
	this->scale = scale;
}

bool UIElement::Contains(int x, int y) const {
	Rect element_rect = GetRect();
	return element_rect.Contains(Point(x, y));
}

// UIImage implementation
UIImage::UIImage() {
}

void UIImage::Render(Draw& draw, const Rect& viewport) {
	if (!visible || !enabled) return;
	
	if (image && !image.IsEmpty()) {
		// Draw the image with tinting if needed
		if (tintColor == Color(255, 255, 255)) {
			// No tinting needed
			draw.DrawImage(pos.x, pos.y, size.cx, size.cy, image);
		} else {
			// Apply tinting
			ImageBuffer ib(image.GetSize());
			for (int y = 0; y < ib.GetHeight(); y++) {
				RGBA* line = ib[y];
				const RGBA* src_line = image[y];
				for (int x = 0; x < ib.GetWidth(); x++) {
					// Apply tint by blending with the tint color
					line[x].r = (byte)((src_line[x].r * tintColor.r) / 255);
					line[x].g = (byte)((src_line[x].g * tintColor.g) / 255);
					line[x].b = (byte)((src_line[x].b * tintColor.b) / 255);
					line[x].a = src_line[x].a; // Preserve alpha
				}
			}
			draw.DrawImage(pos.x, pos.y, size.cx, size.cy, Image(ib));
		}
	} else {
		// Draw a placeholder rectangle
		draw.DrawRect(Rect(pos, size), Color(100, 100, 100));
	}
}

// UILabel implementation
UILabel::UILabel(const String& text) : text(text) {
	// Adjust size based on text content
	Size text_size = GetTextSize(text, textFont);
	size = text_size + Size(10, 10); // Add padding
}

void UILabel::Render(Draw& draw, const Rect& viewport) {
	if (!visible || !enabled) return;
	
	// Draw text
	draw.DrawText(pos.x, pos.y, text, textFont, textColor);
}

// UIButton implementation
UIButton::UIButton(const String& text) : text(text) {
	// Adjust size based on text content
	Size text_size = GetTextSize(text, textFont);
	size = text_size + Size(20, 10); // Add padding
}

void UIButton::Render(Draw& draw, const Rect& viewport) {
	if (!visible || !enabled) return;
	
	// Choose background color based on state
	Color bg_color = backgroundColor;
	if (is_pressed) {
		// Darken the button when pressed
		bg_color = Color(
			max(0, (int)backgroundColor.r - 30),
			max(0, (int)backgroundColor.g - 30), 
			max(0, (int)backgroundColor.b - 30)
		);
	}
	
	// Draw button background
	draw.DrawRect(Rect(pos, size), bg_color);
	
	// Draw button border
	draw.DrawRect(Rect(pos, size), 1, borderColor);
	
	// Draw text centered in the button
	if (!text.IsEmpty()) {
		Size text_size = GetTextSize(text, textFont);
		Point text_pos = Point(
			pos.x + (size.cx - text_size.cx) / 2,
			pos.y + (size.cy - text_size.cy) / 2
		);
		draw.DrawText(text_pos.x, text_pos.y, text, textFont, textColor);
	}
}

bool UIButton::HandleMouseDown(int x, int y, int button) {
	if (!visible || !enabled) return false;
	
	if (button == 0 && Contains(x, y)) {  // Left mouse button
		is_pressed = true;
		return true;
	}
	return false;
}

bool UIButton::HandleMouseUp(int x, int y, int button) {
	if (!visible || !enabled) return false;
	
	if (button == 0 && is_pressed && Contains(x, y)) {  // Left mouse button
		is_pressed = false;
		if (click_callback) {
			click_callback();
		}
		return true;
	}
	
	is_pressed = false;  // Reset if mouse was released outside the button
	return button == 0;
}

// UIContainer implementation
UIContainer::UIContainer() {
}

void UIContainer::AddChild(std::shared_ptr<UIElement> element) {
	if (element) {
		children.Add(element);
		// Sort children by z-order
		Sort(children, [](const std::shared_ptr<UIElement>& a, const std::shared_ptr<UIElement>& b) {
			return a->GetZOrder() < b->GetZOrder();
		});
	}
}

void UIContainer::RemoveChild(std::shared_ptr<UIElement> element) {
	if (element) {
		for (int i = 0; i < children.GetCount(); i++) {
			if (children[i] == element) {
				children.Remove(i);
				break;
			}
		}
	}
}

void UIContainer::ClearChildren() {
	children.Clear();
}

void UIContainer::Render(Draw& draw, const Rect& viewport) {
	if (!visible || !enabled) return;
	
	// Render all child elements
	for (auto& child : children) {
		if (child) {
			child->Render(draw, viewport);
		}
	}
}

bool UIContainer::HandleInput(int x, int y, dword keyflags) {
	if (!visible || !enabled) return false;
	
	// Propagate to children
	for (auto& child : children) {
		if (child && child->HandleInput(x, y, keyflags)) {
			return true;
		}
	}
	return false;
}

bool UIContainer::HandleMouseMove(int x, int y) {
	if (!visible || !enabled) return false;
	
	// Propagate to children
	for (auto& child : children) {
		if (child && child->HandleMouseMove(x, y)) {
			return true;
		}
	}
	return false;
}

bool UIContainer::HandleMouseDown(int x, int y, int button) {
	if (!visible || !enabled) return false;
	
	// Propagate to children (in reverse order to handle topmost elements first)
	for (int i = children.GetCount() - 1; i >= 0; i--) {
		auto& child = children[i];
		if (child && child->HandleMouseDown(x, y, button)) {
			return true;
		}
	}
	return false;
}

bool UIContainer::HandleMouseUp(int x, int y, int button) {
	if (!visible || !enabled) return false;
	
	// Propagate to children (in reverse order to handle topmost elements first)
	for (int i = children.GetCount() - 1; i >= 0; i--) {
		auto& child = children[i];
		if (child && child->HandleMouseUp(x, y, button)) {
			return true;
		}
	}
	return false;
}

bool UIContainer::HandleMouseWheel(int x, int y, int delta) {
	if (!visible || !enabled) return false;
	
	// Propagate to children
	for (int i = children.GetCount() - 1; i >= 0; i--) {
		auto& child = children[i];
		if (child && child->HandleMouseWheel(x, y, delta)) {
			return true;
		}
	}
	return false;
}

// UISystem implementation
UISystem::UISystem() {
}

UISystem::~UISystem() {
	Uninitialize();
}

bool UISystem::Initialize() {
	// Initialize UI system - nothing needed for basic functionality
	return true;
}

void UISystem::Uninitialize() {
	ClearElements();
}

void UISystem::Update(double dt) {
	// Update UI animations, states, etc. here if needed
	// For now, we just keep the system up to date
}

void UISystem::Render(Draw& draw, const Rect& viewport) {
	if (!enabled) return;
	
	// Sort elements by z-order before rendering
	Sort(elements, [](const std::shared_ptr<UIElement>& a, const std::shared_ptr<UIElement>& b) {
		return a->GetZOrder() < b->GetZOrder();
	});
	
	// Render all UI elements
	for (auto& element : elements) {
		if (element) {
			element->Render(draw, viewport);
		}
	}
}

std::shared_ptr<UIElement> UISystem::CreateElement() {
	return std::make_shared<UIElement>();
}

std::shared_ptr<UIImage> UISystem::CreateImage() {
	return std::make_shared<UIImage>();
}

std::shared_ptr<UILabel> UISystem::CreateLabel(const String& text) {
	return std::make_shared<UILabel>(text);
}

std::shared_ptr<UIButton> UISystem::CreateButton(const String& text) {
	return std::make_shared<UIButton>(text);
}

std::shared_ptr<UIContainer> UISystem::CreateContainer() {
	return std::make_shared<UIContainer>();
}

void UISystem::AddElement(std::shared_ptr<UIElement> element) {
	if (element) {
		elements.Add(element);
		// Sort elements by z-order
		Sort(elements, [](const std::shared_ptr<UIElement>& a, const std::shared_ptr<UIElement>& b) {
			return a->GetZOrder() < b->GetZOrder();
		});
	}
}

void UISystem::RemoveElement(std::shared_ptr<UIElement> element) {
	if (element) {
		for (int i = 0; i < elements.GetCount(); i++) {
			if (elements[i] == element) {
				elements.Remove(i);
				break;
			}
		}
	}
}

void UISystem::ClearElements() {
	elements.Clear();
}

bool UISystem::HandleInput(int x, int y, dword keyflags) {
	if (!enabled) return false;
	
	// Check if any UI element consumes the input
	for (int i = elements.GetCount() - 1; i >= 0; i--) {  // Process from top to bottom
		auto& element = elements[i];
		if (element && element->HandleInput(x, y, keyflags)) {
			return true;
		}
	}
	return false;
}

bool UISystem::HandleMouseMove(int x, int y) {
	if (!enabled) return false;
	
	mouse_pos = Point(x, y);
	
	// Check if any UI element consumes the mouse move
	for (int i = elements.GetCount() - 1; i >= 0; i--) {  // Process from top to bottom
		auto& element = elements[i];
		if (element && element->HandleMouseMove(x, y)) {
			return true;
		}
	}
	return false;
}

bool UISystem::HandleMouseDown(int x, int y, int button) {
	if (!enabled) return false;
	
	mouse_pos = Point(x, y);
	if (button >= 0 && button < 3) {
		mouse_buttons[button] = true;
	}
	
	// Check if any UI element consumes the mouse down
	for (int i = elements.GetCount() - 1; i >= 0; i--) {  // Process from top to bottom
		auto& element = elements[i];
		if (element && element->HandleMouseDown(x, y, button)) {
			return true;
		}
	}
	return false;
}

bool UISystem::HandleMouseUp(int x, int y, int button) {
	if (!enabled) return false;
	
	mouse_pos = Point(x, y);
	if (button >= 0 && button < 3) {
		mouse_buttons[button] = false;
	}
	
	// Check if any UI element consumes the mouse up
	for (int i = elements.GetCount() - 1; i >= 0; i--) {  // Process from top to bottom
		auto& element = elements[i];
		if (element && element->HandleMouseUp(x, y, button)) {
			return true;
		}
	}
	return false;
}

bool UISystem::HandleMouseWheel(int x, int y, int delta) {
	if (!enabled) return false;
	
	// Check if any UI element consumes the mouse wheel
	for (int i = elements.GetCount() - 1; i >= 0; i--) {  // Process from top to bottom
		auto& element = elements[i];
		if (element && element->HandleMouseWheel(x, y, delta)) {
			return true;
		}
	}
	return false;
}

std::shared_ptr<UIElement> UISystem::FindElementAt(int x, int y) {
	// Find topmost element at position (x, y)
	for (int i = elements.GetCount() - 1; i >= 0; i--) {  // Check from top to bottom
		auto& element = elements[i];
		if (element && element->IsVisible() && element->IsEnabled() && element->Contains(x, y)) {
			return element;
		}
	}
	return nullptr;
}

END_UPP_NAMESPACE