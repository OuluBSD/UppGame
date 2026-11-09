#ifndef _Umbrella_LayerManager_h_
#define _Umbrella_LayerManager_h_

#include "Layer.h"

using namespace Upp;

class LayerManager {
private:
	Array<Layer> layers;
	int activeLayerIndex;

public:
	LayerManager();

	// Layer management
	Layer& AddLayer(const String& name, LayerType type, int cols, int rows);
	void RemoveLayer(int index);
	void MoveLayerUp(int index);
	void MoveLayerDown(int index);

	// Getters
	int GetLayerCount() const { return layers.GetCount(); }
	Layer& GetLayer(int index) { return layers[index]; }
	const Layer& GetLayer(int index) const { return layers[index]; }
	Layer* GetActiveLayer() { return activeLayerIndex >= 0 && activeLayerIndex < layers.GetCount() ? &layers[activeLayerIndex] : nullptr; }
	const Layer* GetActiveLayer() const { return activeLayerIndex >= 0 && activeLayerIndex < layers.GetCount() ? &layers[activeLayerIndex] : nullptr; }
	int GetActiveLayerIndex() const { return activeLayerIndex; }
	Layer* FindLayerByType(LayerType type);
	const Layer* FindLayerByType(LayerType type) const;

	// Setters
	void SetActiveLayer(int index);

	// Operations
	void Clear();
	void ClearAllLayers();

	// Initialization helper
	void InitializeDefaultLayers(int cols, int rows);
};

#endif
