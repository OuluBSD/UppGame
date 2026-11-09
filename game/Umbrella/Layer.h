#ifndef _Umbrella_Layer_h_
#define _Umbrella_Layer_h_

#include "MapGrid.h"

using namespace Upp;

enum LayerType {
	LAYER_TERRAIN,
	LAYER_BACKGROUND,
	LAYER_ENTITY,
	LAYER_ANNOTATION
};

class Layer {
private:
	String name;
	LayerType type;
	bool visible;
	bool locked;
	int opacity;  // 0-100%
	MapGrid grid; // Each layer has its own grid

public:
	Layer(const String& name, LayerType type, int cols, int rows);

	// Getters
	const String& GetName() const { return name; }
	LayerType GetType() const { return type; }
	bool IsVisible() const { return visible; }
	bool IsLocked() const { return locked; }
	int GetOpacity() const { return opacity; }
	MapGrid& GetGrid() { return grid; }
	const MapGrid& GetGrid() const { return grid; }

	// Setters
	void SetName(const String& n) { name = n; }
	void SetVisible(bool v) { visible = v; }
	void SetLocked(bool l) { locked = l; }
	void SetOpacity(int o) { opacity = clamp(o, 0, 100); }

	// Operations
	void Clear();
};

#endif
