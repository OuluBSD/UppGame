#include "Umbrella.h"
#include "Layer.h"

using namespace Upp;

Layer::Layer(const String& name, LayerType type, int cols, int rows)
	: name(name), type(type), visible(true), locked(false), opacity(100), grid(cols, rows) {
}

void Layer::Clear() {
	grid.Clear();
}
