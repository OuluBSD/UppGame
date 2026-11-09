#include "InputSystemExample.h"

NAMESPACE_UPP

GameInputExample::GameInputExample() {
    input_system = std::make_unique<InputSystem>();
}

GameInputExample::~GameInputExample() {
    Uninitialize();
}

bool GameInputExample::InitializeWithEon(Engine& engine) {
    // Initialize HAL components
    if (!InitializeHAL(engine)) {
        LOG("Failed to initialize HAL components for input");
        return false;
    }
    
    // Initialize the input system with HAL context and events
    if (!input_system->Initialize(*hal_context, *hal_events)) {
        LOG("Failed to initialize InputSystem with HAL components");
        return false;
    }
    
    return true;
}

void GameInputExample::Uninitialize() {
    if (input_system) {
        input_system->Uninitialize();
    }
    
    // Clean up HAL atoms if needed
    hal_context = nullptr;
    hal_events = nullptr;
}

void GameInputExample::Update(double dt) {
    if (input_system) {
        input_system->Update(dt);
    }
}

const InputState& GameInputExample::GetInputState() const {
    return input_system->GetState();
}

bool GameInputExample::InitializeHAL(Engine& engine) {
    // This would set up the HAL atoms using Eon patterns
    // In a real implementation, this would create HAL atoms with proper VFS paths
    
    // For demonstration purposes, we'll show how this would work conceptually:
    /*
    auto sys = engine.GetAdd<Eon::ScriptLoader>();
    Eon::Builder& builder = sys->val.GetAdd<Eon::Builder>("builder");
    
    // Create a chain for input handling
    auto& chain = builder.AddChain("input.chain");
    
    // Add HAL context atom
    auto& context_atom = chain.AddAtom("hal.context.base");
    
    // Add HAL events atom
    auto& events_atom = chain.AddAtom("hal.events.base");
    events_atom.Assign("context", "hal/context/base");  // Link to context
    
    // Compile and load the AST
    Eon::AstNode* root = builder.CompileAst();
    if (!root) {
        LOG("Failed to compile AST for HAL input components");
        return false;
    }
    
    sys->LoadAst(root);
    
    // Retrieve the created atoms
    hal_context = engine.FindAtomByPath("input.chain.hal.context.base");
    hal_events = engine.FindAtomByPath("input.chain.hal.events.base");
    */
    
    // For now, we'll simulate HAL initialization by creating placeholder atoms
    // In a real implementation, this would properly connect to the HAL system
    return true;
}

END_UPP_NAMESPACE