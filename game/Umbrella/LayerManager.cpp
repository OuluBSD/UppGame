#include "Umbrella.h"
#include "LayerManager.h"

using namespace Upp;

LayerManager::LayerManager()
	: activeLayerIndex(0) {
}

Layer& LayerManager::AddLayer(const String& name, LayerType type, int cols, int rows) {
	Layer* layer = new Layer(name, type, cols, rows);
	layers.Add(layer);
	if(layers.GetCount() == 1) {
		activeLayerIndex = 0;
	}
	return *layer;
}

void LayerManager::RemoveLayer(int index) {
	if(index < 0 || index >= layers.GetCount()) return;

	layers.Remove(index);

	// Adjust active layer index
	if(activeLayerIndex >= layers.GetCount()) {
		activeLayerIndex = layers.GetCount() - 1;
	}
	if(layers.GetCount() == 0) {
		activeLayerIndex = -1;
	}
}

void LayerManager::MoveLayerUp(int index) {
	if(index <= 0 || index >= layers.GetCount()) return;
	layers.Swap(index, index - 1);
	if(activeLayerIndex == index) {
		activeLayerIndex = index - 1;
	} else if(activeLayerIndex == index - 1) {
		activeLayerIndex = index;
	}
}

void LayerManager::MoveLayerDown(int index) {
	if(index < 0 || index >= layers.GetCount() - 1) return;
	layers.Swap(index, index + 1);
	if(activeLayerIndex == index) {
		activeLayerIndex = index + 1;
	} else if(activeLayerIndex == index + 1) {
		activeLayerIndex = index;
	}
}

void LayerManager::SetActiveLayer(int index) {
	if(index >= 0 && index < layers.GetCount()) {
		activeLayerIndex = index;
	}
}

void LayerManager::Clear() {
	for(int i = 0; i < layers.GetCount(); i++) {
		layers[i].Clear();
	}
}

void LayerManager::ClearAllLayers() {
	layers.Clear();
	activeLayerIndex = -1;
}

void LayerManager::InitializeDefaultLayers(int cols, int rows) {
	ClearAllLayers();
	AddLayer("Terrain", LAYER_TERRAIN, cols, rows);
	AddLayer("Background", LAYER_BACKGROUND, cols, rows);
	AddLayer("Entities", LAYER_ENTITY, cols, rows);
	AddLayer("Annotations", LAYER_ANNOTATION, cols, rows);
	SetActiveLayer(0);  // Set terrain as active
}

Layer* LayerManager::FindLayerByType(LayerType type) {
	for(int i = 0; i < layers.GetCount(); i++) {
		if(layers[i].GetType() == type) {
			return &layers[i];
		}
	}
	return nullptr;
}

const Layer* LayerManager::FindLayerByType(LayerType type) const {
	for(int i = 0; i < layers.GetCount(); i++) {
		if(layers[i].GetType() == type) {
			return &layers[i];
		}
	}
	return nullptr;
}
