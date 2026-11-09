#ifndef _Umbrella_GameSettings_h_
#define _Umbrella_GameSettings_h_

namespace Upp {

// Game configuration constants
struct GameSettings {
	// Thrown Enemy Capture Settings
	static constexpr float THROWN_ENEMY_SIZE_TOLERANCE = 1.75f;  // Target can be up to 75% larger than thrower

	// Enemy Visual Effects
	static constexpr float ENEMY_ROTATION_SPEED = 720.0f;  // Degrees per second (2 full rotations)

	// Enemy State Colors (RGB)
	static constexpr int DEACTIVATED_TINT_R = 0;    // Green tint for captured/thrown enemies
	static constexpr int DEACTIVATED_TINT_G = 255;
	static constexpr int DEACTIVATED_TINT_B = 100;

	static constexpr int DEAD_TINT_R = 255;  // Red tint for dead enemies
	static constexpr int DEAD_TINT_G = 50;
	static constexpr int DEAD_TINT_B = 50;

	static constexpr int NORMAL_TINT_R = 255;  // No tint for normal enemies
	static constexpr int NORMAL_TINT_G = 255;
	static constexpr int NORMAL_TINT_B = 255;

	// Dead Enemy Physics
	static constexpr float DEAD_ENEMY_MIN_VX = -100.0f;  // Minimum horizontal velocity
	static constexpr float DEAD_ENEMY_MAX_VX = 100.0f;   // Maximum horizontal velocity
	static constexpr float DEAD_ENEMY_MIN_VY = 100.0f;   // Minimum upward velocity
	static constexpr float DEAD_ENEMY_MAX_VY = 200.0f;   // Maximum upward velocity

	// Level Completion Settings
	static constexpr float LEVEL_COMPLETE_TREAT_TIMEOUT = 7.0f;  // Seconds to collect treats after all enemies killed
	static constexpr float LEVEL_TRANSITION_SCROLL_SPEED = 200.0f;  // Pixels per second for level scroll
	static constexpr float PLAYER_HOVER_HEIGHT = 50.0f;  // How high player hovers during transition
	static constexpr float TRANSITION_HOVER_TIME = 1.0f;  // Seconds to hover before scrolling
	static constexpr float TRANSITION_DROP_TIME = 0.5f;  // Seconds to drop into new level

	// Debug/Development Settings
	static constexpr bool SHOW_COMPLETION_MESSAGES = true;  // Show level completion countdown and messages

	// TODO: Add other game settings here as they're identified:
	// - Enemy AI parameters (speeds, jump heights, etc.)
	// - Player physics constants
	// - Combat parameters (damage, knockback, etc.)
	// - Scoring values
};

}

#endif
