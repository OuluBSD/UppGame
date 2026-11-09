#ifndef UPP_SCENE_H
#define UPP_SCENE_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <CtrlCore/CtrlCore.h>
#include <Geometry/Geometry.h>
#include <GameLib/GameLib.h>
#include <GameEngine/GameEngine.h>
#include <vector>
#include <string>
#include <memory>

NAMESPACE_UPP_BEGIN

// Forward declaration
// Note: We will use existing ECS from ai-upp when upptst/Eon* packages are working
// For now, we'll define a basic structure that can integrate with it later

// Scene base class
class Scene {
public:
	Scene();
	virtual ~Scene();
	
	// Scene lifecycle methods
	virtual void OnEnter();      // Called when scene becomes active
	virtual void OnExit();       // Called when scene is deactivated
	virtual void Update(double deltaTime);
	virtual void Render(Draw& draw);
	
	// Scene properties
	void SetName(const std::string& name) { name_ = name; }
	const std::string& GetName() const { return name_; }
	
	// Scene management
	void SetActive(bool active) { active_ = active; }
	bool IsActive() const { return active_; }

protected:
	std::string name_;
	bool active_ = false;
};

// Scene manager for switching between scenes
class SceneManager {
public:
	SceneManager();
	~SceneManager();
	
	// Add a scene to the manager
	void AddScene(std::shared_ptr<Scene> scene);
	
	// Switch to a specific scene by name
	void SwitchToScene(const std::string& sceneName);
	
	// Get current scene
	std::shared_ptr<Scene> GetCurrentScene() const { return current_scene_; }
	
	// Update the current scene
	void Update(double deltaTime);
	
	// Render the current scene
	void Render(Draw& draw);
	
	// Get a scene by name
	std::shared_ptr<Scene> GetScene(const std::string& name) const;
	
private:
	std::vector<std::shared_ptr<Scene>> scenes_;
	std::shared_ptr<Scene> current_scene_;
};

// Implementation of Scene methods

inline Scene::Scene() {
	// Initialize scene properties
}

inline Scene::~Scene() {
	// Clean up scene resources
}

inline void Scene::OnEnter() {
	active_ = true;
	LOG("Scene " + name_ + " entered");
}

inline void Scene::OnExit() {
	active_ = false;
	LOG("Scene " + name_ + " exited");
}

inline void Scene::Update(double deltaTime) {
	// To be implemented with ECS integration later
}

inline void Scene::Render(Draw& draw) {
	// To be implemented with ECS integration later
}

// Implementation of SceneManager methods

inline SceneManager::SceneManager() {
	// Initialize scene manager
}

inline SceneManager::~SceneManager() {
	// Clean up all scenes
	scenes_.clear();
	current_scene_.reset();
}

inline void SceneManager::AddScene(std::shared_ptr<Scene> scene) {
	scenes_.push_back(scene);
}

inline void SceneManager::SwitchToScene(const std::string& sceneName) {
	// If we have a current scene, call its OnExit
	if (current_scene_) {
		current_scene_->OnExit();
	}
	
	// Find and activate the new scene
	for (auto& scene : scenes_) {
		if (scene->GetName() == sceneName) {
			current_scene_ = scene;
			current_scene_->OnEnter();
			return;
		}
	}
	
	// If scene not found, log error
	LOG("Scene not found: " + sceneName);
}

inline void SceneManager::Update(double deltaTime) {
	if (current_scene_ && current_scene_->IsActive()) {
		current_scene_->Update(deltaTime);
	}
}

inline void SceneManager::Render(Draw& draw) {
	if (current_scene_ && current_scene_->IsActive()) {
		current_scene_->Render(draw);
	}
}

inline std::shared_ptr<Scene> SceneManager::GetScene(const std::string& name) const {
	for (const auto& scene : scenes_) {
		if (scene->GetName() == name) {
			return scene;
		}
	}
	return nullptr;
}

NAMESPACE_UPP_END

#endif