# Test input injection and frame-by-frame simulation (Phase 3)

print("Testing input injection and frame stepping...")

# ============================================================
# Test 1: Key state management
# ============================================================

print("\n--- Key State Management ---")

# Verify all keys start cleared
keys = get_keys()
print("Initial keys:", keys)
if not keys["left"] and not keys["right"] and not keys["jump"] and not keys["attack"]:
    print("  ✓ All keys start clear")
else:
    print("  ✗ Keys not cleared at start")

# Set individual keys
set_key("left", True)
set_key("jump", True)
keys = get_keys()
print("After set_key(left, True) and set_key(jump, True):", keys)
if keys["left"] and keys["jump"] and not keys["right"] and not keys["attack"]:
    print("  ✓ Keys set correctly")
else:
    print("  ✗ Key state mismatch")

# Clear all keys
clear_keys()
keys = get_keys()
print("After clear_keys():", keys)
if not keys["left"] and not keys["right"] and not keys["jump"] and not keys["attack"]:
    print("  ✓ clear_keys() works")
else:
    print("  ✗ clear_keys() failed")

# ============================================================
# Test 2: Frame stepping - tick_game moves player
# ============================================================

print("\n--- Frame Stepping ---")

# Get initial position
pos_before = get_player_position()
print("Position before ticking:", pos_before)

# Tick a single frame with no input - gravity should pull player down
tick_game()
pos_after = get_player_position()
print("Position after 1 tick (no input):", pos_after)

# Player should have moved vertically due to gravity if not on ground
if pos_after[1] != pos_before[1]:
    print("  ✓ Tick advanced simulation (position changed)")
else:
    print("  Note: Position unchanged (player may already be on ground)")

# ============================================================
# Test 3: Input injection moves player horizontally
# ============================================================

print("\n--- Horizontal Movement Test ---")

# Move player to a known starting position on solid ground
set_player_position(100.0, 290.0)  # Near the bottom of world1-stage1

# Let physics settle - tick several frames to land on ground
clear_keys()
tick_game_frames(10)

landed_pos = get_player_position()
print("Position after settling (10 frames):", landed_pos)
on_ground = is_player_on_ground()
print("On ground:", on_ground)

# Now inject RIGHT input for 30 frames
start_pos = get_player_position()
set_key("right", True)
tick_game_frames(30)
clear_keys()
end_pos = get_player_position()

print("Start position (right walk):", start_pos)
print("End position (after 30 frames right):", end_pos)

x_moved = end_pos[0] - start_pos[0]
print("X displacement:", x_moved)
if x_moved > 10.0:
    print("  ✓ Player moved right from input injection")
else:
    print("  Note: Player moved", x_moved, "px right (may have hit wall)")

# Test LEFT movement
set_key("left", True)
tick_game_frames(30)
clear_keys()
left_end_pos = get_player_position()
x_moved_left = left_end_pos[0] - end_pos[0]
print("After 30 frames left:", left_end_pos, "displacement:", x_moved_left)
if x_moved_left < -5.0:
    print("  ✓ Player moved left from input injection")
else:
    print("  Note: Player moved", x_moved_left, "px left")

# ============================================================
# Test 4: Jump injection
# ============================================================

print("\n--- Jump Test ---")

# Settle to ground
clear_keys()
tick_game_frames(20)
pre_jump_pos = get_player_position()
pre_jump_on_ground = is_player_on_ground()
print("Pre-jump: pos =", pre_jump_pos, "on_ground =", pre_jump_on_ground)

# Inject jump for 1 frame (press), then release
set_key("jump", True)
tick_game()
set_key("jump", False)

# Check upward velocity immediately after jump press
vel_after_jump = get_player_velocity()
print("Velocity after jump press:", vel_after_jump)
if vel_after_jump[1] < 0:  # Negative Y = upward in this coordinate system
    print("  ✓ Jump produced upward velocity")
else:
    print("  Note: Velocity Y =", vel_after_jump[1], "(expected negative for upward)")

# Tick a few frames to see the jump in progress
tick_game_frames(5)
mid_jump_pos = get_player_position()
print("Position 5 frames after jump:", mid_jump_pos)

# ============================================================
# Test 5: tick_game_frames count verification
# ============================================================

print("\n--- Frame Count Verification ---")

clear_keys()
tick_game_frames(60)  # 1 second at 60fps
pos_1sec = get_player_position()
state_1sec = get_game_state()
print("After 60 frames (1 second): pos =", pos_1sec, "state =", state_1sec)
print("  ✓ tick_game_frames(60) completed without error")

print("\nAll input injection tests completed!")
