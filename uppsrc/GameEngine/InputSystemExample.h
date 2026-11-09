#ifndef UPP_INPUTSYSTEM_EXAMPLE_H
#define UPP_INPUTSYSTEM_EXAMPLE_H

#include <GameEngine/GameEngine.h>
#include <Eon/Eon.h>  // For Eon engine

NAMESPACE_UPP

// Example class showing how to use InputSystem with Eon patterns
class GameInputExample {
public:
    GameInputExample();
    virtual ~GameInputExample();
    
    // Initialize the input system using HAL with Eon patterns
    bool InitializeWithEon(Engine& engine);
    void Uninitialize();
    
    // Update the input system
    void Update(double dt);
    
    // Get current input state
    const InputState& GetInputState() const;
    
    // Access to the input system
    InputSystem& GetInputSystem() { return *input_system; }
    const InputSystem& GetInputSystem() const { return *input_system; }

private:
    std::unique_ptr<InputSystem> input_system;
    
    // HAL atoms
    AtomBase* hal_context = nullptr;
    AtomBase* hal_events = nullptr;
    
    // Initialize HAL components
    bool InitializeHAL(Engine& engine);
};

END_UPP_NAMESPACE

#endif