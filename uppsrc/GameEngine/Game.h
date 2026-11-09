#ifndef UPP_GAME_H
#define UPP_GAME_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <CtrlCore/CtrlCore.h>
#include <Geometry/Geometry.h>
#include <GameEngine/GameEngine.h>
#include <GameEngine/GameWindow.h>
#include <memory>
#include <Eon/Eon.h>  // For Eon integration

NAMESPACE_UPP_BEGIN

class Game {
public:
	Game();
	virtual ~Game();

	// Game lifecycle methods
	virtual void Initialize();
	virtual void LoadContent();
	virtual void UnloadContent();
	virtual void Update(double deltaTime);
	virtual void Render(Draw& draw);

	// Game management
	void Run();
	void Exit();

	// Accessors
	GameWindow& GetMainWindow() { return main_window; }
	const GameWindow& GetMainWindow() const { return main_window; }

	// Game timing
	double GetDeltaTime() const { return delta_time; }

	// Input system
	void SetInputSystem(std::shared_ptr<InputSystem> input_system) {
		input_system_ = input_system;
		main_window.SetInputSystem(input_system);
	}
	std::shared_ptr<InputSystem> GetInputSystem() const {
		return input_system_;
	}

	// ECS integration
	void SetEcsIntegration(std::shared_ptr<GameEcsIntegration> ecs_integration) {
		ecs_integration_ = ecs_integration;
	}
	std::shared_ptr<GameEcsIntegration> GetEcsIntegration() const {
		return ecs_integration_;
	}

	// Eon integration
	void InitializeEonSystem();
	Engine& GetEonEngine() { return eon_engine; }

protected:
	GameWindow main_window;
	bool running = false;
	double delta_time = 0.0;

	// ECS integration
	std::shared_ptr<GameEcsIntegration> ecs_integration_;

	// Eon Engine integration
	Engine eon_engine;

private:
	void GameLoop();
};

NAMESPACE_UPP_END

#endif