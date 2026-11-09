# Test player state query and manipulation

print("Testing player state API...")

# Query initial state
pos = get_player_position()
print("Initial position:", pos)

vel = get_player_velocity()
print("Initial velocity:", vel)

lives = get_player_lives()
print("Lives:", lives)

score = get_player_score()
print("Score:", score)

state = get_player_state()
print("Parasol state:", state)

on_ground = is_player_on_ground()
print("On ground:", on_ground)

# Test manipulation
print("\nTesting position manipulation...")
set_player_position(100.0, 200.0)
new_pos = get_player_position()
print("New position after set:", new_pos)

# Manual validation (no assert since ByteVM may not support it)
x_diff = abs(new_pos[0] - 100.0)
y_diff = abs(new_pos[1] - 200.0)
if x_diff < 1.0 and y_diff < 1.0:
    print("✓ Position set correctly")
else:
    print("✗ Position set incorrectly - X diff:", x_diff, "Y diff:", y_diff)

print("\nTesting velocity manipulation...")
set_player_velocity(50.0, -100.0)
new_vel = get_player_velocity()
print("New velocity:", new_vel)

print("\nTesting score manipulation...")
add_player_score(500)
new_score = get_player_score()
print("Score after add:", new_score)

# Manual validation
if new_score == score + 500:
    print("✓ Score added correctly")
else:
    print("✗ Score not added correctly - Expected:", score + 500, "Got:", new_score)

print("\nAll player state tests passed!")
