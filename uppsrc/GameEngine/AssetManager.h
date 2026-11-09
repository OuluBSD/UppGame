#ifndef UPP_ASSET_MANAGER_H
#define UPP_ASSET_MANAGER_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <CtrlCore/CtrlCore.h>
#include <Geometry/Geometry.h>
#include <GameLib/GameLib.h>
#include <GameEngine/GameEngine.h>
#include <map>
#include <memory>
#include <string>
#include <functional>

NAMESPACE_UPP_BEGIN

// Base Asset class - all assets should inherit from this
class Asset {
public:
	virtual ~Asset() = default;
	std::string id;
};

// Common asset types
class TextureAsset : public Asset {
public:
	Image image;
	Size size;
	
	TextureAsset() = default;
	TextureAsset(const Image& img) : image(img), size(img.GetSize()) {}
};

class AudioAsset : public Asset {
public:
	std::vector<uint8> audio_data;
	int sample_rate = 44100;
	int channels = 2;
	
	AudioAsset() = default;
	AudioAsset(const std::vector<uint8>& data, int rate = 44100, int ch = 2) 
		: audio_data(data), sample_rate(rate), channels(ch) {}
};

class ModelAsset : public Asset {
public:
	// Simplified model representation
	Vector<Point3> vertices;
	Vector<Point3> normals;
	Vector<Point2> tex_coords;
	Vector<int> indices;
	
	ModelAsset() = default;
};

// Asset loader function type
template<typename T>
using AssetLoader = std::function<SharedPtr<T>(const std::string& path)>;

// Asset Manager class
class AssetManager {
public:
	AssetManager();
	~AssetManager();
	
	// Load an asset if not already loaded, or return cached version
	template<typename T>
	SharedPtr<T> LoadAsset(const std::string& id, const std::string& path);
	
	// Get an asset that's already loaded (returns null if not loaded)
	template<typename T>
	SharedPtr<T> GetAsset(const std::string& id);
	
	// Unload a specific asset
	void UnloadAsset(const std::string& id);
	
	// Unload all assets
	void UnloadAllAssets();
	
	// Register a custom loader for a specific asset type
	template<typename T>
	void RegisterLoader(AssetLoader<T> loader);
	
	// Get or set memory budget (in bytes)
	uint64 GetMemoryBudget() const { return memory_budget; }
	void SetMemoryBudget(uint64 budget) { memory_budget = budget; }
	
	// Get current memory usage (in bytes)
	uint64 GetMemoryUsage() const { return memory_used; }

private:
	// Store all assets in a generic map
	std::map<std::string, std::shared_ptr<Asset>> assets;
	
	// Track memory usage
	uint64 memory_used = 0;
	uint64 memory_budget = 100 * 1024 * 1024; // 100MB default budget
	
	// Default loaders
	AssetLoader<TextureAsset> texture_loader;
	AssetLoader<AudioAsset> audio_loader;
	AssetLoader<ModelAsset> model_loader;
	
	// Helper to calculate asset size
	size_t CalculateAssetSize(Asset* asset) const;
	
	// Helper to register default loaders
	void RegisterDefaultLoaders();
};

// Implementation of AssetManager methods

inline AssetManager::AssetManager() {
	RegisterDefaultLoaders();
}

inline AssetManager::~AssetManager() {
	UnloadAllAssets();
}

inline size_t AssetManager::CalculateAssetSize(Asset* asset) const {
	if (auto tex = dynamic_cast<TextureAsset*>(asset)) {
		// For images, size is width * height * 4 bytes (RGBA)
		return tex->size.cx * tex->size.cy * 4;
	} else if (auto audio = dynamic_cast<AudioAsset*>(asset)) {
		return audio->audio_data.size();
	} else if (auto model = dynamic_cast<ModelAsset*>(asset)) {
		// Calculate size based on vertex data
		return (model->vertices.size() * sizeof(Point3)) +
			   (model->normals.size() * sizeof(Point3)) +
			   (model->tex_coords.size() * sizeof(Point2)) +
			   (model->indices.size() * sizeof(int));
	}
	return 0; // Unknown asset type
}

inline void AssetManager::RegisterDefaultLoaders() {
	// Default texture loader
	texture_loader = [](const std::string& path) -> SharedPtr<TextureAsset> {
		Image img = LoadImageFile(path);
		if (!img.IsEmpty()) {
			auto asset = MakeSharedPtr<TextureAsset>(img);
			asset->id = path;
			return asset;
		}
		return nullptr;
	};
	
	// Default audio loader (placeholder - would need proper audio loading)
	audio_loader = [](const std::string& path) -> SharedPtr<AudioAsset> {
		// Placeholder implementation
		auto asset = MakeSharedPtr<AudioAsset>(std::vector<uint8>());
		asset->id = path;
		return asset;
	};
	
	// Default model loader (placeholder - would need proper 3D model loading)
	model_loader = [](const std::string& path) -> SharedPtr<ModelAsset> {
		// Placeholder implementation
		auto asset = MakeSharedPtr<ModelAsset>();
		asset->id = path;
		return asset;
	};
}

template<typename T>
inline SharedPtr<T> AssetManager::LoadAsset(const std::string& id, const std::string& path) {
	// First check if asset is already loaded
	auto existing = GetAsset<T>(id);
	if (existing) {
		return existing;
	}
	
	// Check if we're over budget
	if (memory_used >= memory_budget) {
		LOG("AssetManager: Memory budget exceeded. Consider unloading unused assets.");
		return nullptr;
	}
	
	// Load using the appropriate loader
	SharedPtr<T> asset;
	if constexpr (std::is_same_v<T, TextureAsset>) {
		auto typed_asset = texture_loader(path);
		asset = std::static_pointer_cast<T>(typed_asset);
	} else if constexpr (std::is_same_v<T, AudioAsset>) {
		auto typed_asset = audio_loader(path);
		asset = std::static_pointer_cast<T>(typed_asset);
	} else if constexpr (std::is_same_v<T, ModelAsset>) {
		auto typed_asset = model_loader(path);
		asset = std::static_pointer_cast<T>(typed_asset);
	} else {
		// Custom asset type - would need a custom loader
		return nullptr;
	}
	
	if (asset) {
		// Add to cache and update memory tracking
		assets[id] = asset;
		size_t size = CalculateAssetSize(asset.get());
		memory_used += size;
	}
	
	return asset;
}

template<typename T>
inline SharedPtr<T> AssetManager::GetAsset(const std::string& id) {
	auto it = assets.find(id);
	if (it != assets.end()) {
		// Check if the asset is of the requested type
		auto typed_asset = std::dynamic_pointer_cast<T>(it->second);
		return typed_asset;
	}
	return nullptr;
}

inline void AssetManager::UnloadAsset(const std::string& id) {
	auto it = assets.find(id);
	if (it != assets.end()) {
		// Update memory tracking
		size_t size = CalculateAssetSize(it->second.get());
		memory_used -= size;
		
		assets.erase(it);
	}
}

inline void AssetManager::UnloadAllAssets() {
	assets.clear();
	memory_used = 0;
}

template<typename T>
inline void AssetManager::RegisterLoader(AssetLoader<T> loader) {
	if constexpr (std::is_same_v<T, TextureAsset>) {
		texture_loader = std::move(loader);
	} else if constexpr (std::is_same_v<T, AudioAsset>) {
		audio_loader = std::move(loader);
	} else if constexpr (std::is_same_v<T, ModelAsset>) {
		model_loader = std::move(loader);
	}
}

NAMESPACE_UPP_END

#endif