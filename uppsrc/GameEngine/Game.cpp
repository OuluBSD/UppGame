#include "GameEngine.h"
#include <memory>

NAMESPACE_UPP

Game::Game() {
	// Initialize Eon system first
	InitializeEonSystem();

	// Initialize ECS integration
	ecs_integration_ = std::make_shared<GameEcsIntegration>();
	ecs_integration_->Initialize();
	SetEcsIntegration(ecs_integration_);

	// Set up the main window callbacks
	main_window.SetRenderCallback([this](Draw& w) {
		Render(w);
	});

	main_window.SetUpdateCallback([this](double dt) {
		delta_time = dt;
		Update(dt);
	});

	// Set up the game loop callback
	main_window.SetGameLoopCallback([this]() {
		// Any additional game loop logic can go here
	});
}

Game::~Game() {
	UnloadContent();
}

void Game::InitializeEonSystem() {
	// Setup Eon system for game engine
	// Register any required Eon components
}

void Game::Initialize() {
	// Initialize game systems here
	if (ecs_integration_) {
		ecs_integration_->Initialize();
	}
	
	// Initialize Eon-based game systems using proper Eon patterns
	auto sys = eon_engine.GetAdd<Eon::ScriptLoader>();
	sys->SetEagerChainBuild(true);

	// Example: Create a game loop using Eon patterns
	// This creates a basic loop structure following Eon conventions
	try {
		Eon::Builder& builder = sys->val.GetAdd<Eon::Builder>("builder");
		
		// Create the main game loop
		auto& game_loop = builder.AddLoop("game.main");
		
		// Add game components as atoms in the loop
		auto& state_manager = game_loop.AddAtom("game.state.manager");
		auto& transform_sys = game_loop.AddAtom("game.system.transform.update");
		auto& physics_sys = game_loop.AddAtom("game.system.physics.update");
		auto& render_sys = game_loop.AddAtom("game.system.render.update");
		auto& audio_sys = game_loop.AddAtom("game.system.audio.update");
		auto& event_sys = game_loop.AddAtom("game.system.event.process");
		
		// Compile and load the AST
		Eon::AstNode* root = builder.CompileAst();
		if (root) {
			sys->LoadAst(root);
			
			// Initialize the system
			if (!sys->PostInitializeAll()) {
				LOG("Eon Game System PostInitialize failed");
			}
			
			if (!sys->StartAll()) {
				LOG("Eon Game System Start failed");
			}
		}
	}
	catch (Exc& e) {
		LOG("Error initializing Eon game systems: " << e);
	}
}

void Game::LoadContent() {
	// Load game assets here
	// You could also create initial game entities here
	if (ecs_integration_) {
		// Example: Create a player entity
		auto player = ecs_integration_->CreateGameObject("Player", Point3(0, 0, 0));
		if (player) {
			if (auto transform = player->Find<TransformComponent>()) {
				transform->SetPosition(Point3(0, 0, 0));
			}
		}
	}
}

void Game::UnloadContent() {
	// Unload game assets here
	ecs_integration_.reset();
}

void Game::Update(double deltaTime) {
	// Update game logic here
	// This is called from the game thread with the delta time
	
	// Update ECS systems
	if (ecs_integration_) {
		ecs_integration_->Update(deltaTime);
	}
}

void Game::Render(Draw& draw) {
	// Render game content here
	// This is called from the UI thread during Paint
	// Fallback rendering
	draw.DrawRect(main_window.GetSize(), Color(20, 20, 60)); // Dark blue background
}

void Game::Run() {
	running = true;

	// Initialize the game
	Initialize();
	LoadContent();

	// Start the game loop in our GameWindow
	main_window.StartGameLoop();

	// Run the main window modal
	main_window.Run();

	// When the window closes, stop the game loop
	main_window.StopGameLoop();
	running = false;

	// Clean up
	UnloadContent();
}

void Game::Exit() {
	// Properly stop Eon systems before exiting
	auto sys = eon_engine.GetAdd<Eon::ScriptLoader>();
	if (sys) {
		sys->StopAll();
		sys->UndoAll();
	}
	
	running = false;
	main_window.Break(); // Close the window
}

void Game::Exit() {
	running = false;
	main_window.Break(); // Close the window
}

END_UPP_NAMESPACE
