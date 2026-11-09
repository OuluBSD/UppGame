# Test ActionExecutor - path execution with input injection (Track 2, Phase 2)
# Stable walkable rows in world1-stage1 (Y-UP physics):
#   Row 15: cols 6-9+  (floor tile row 14, wide platform)
#   Row 20: cols 6-9+  (floor tile row 19, lower area)
# Player placed at row 17 falls DOWN to row 15 (60 frames to settle).

print("Testing ActionExecutor / navigate_to...")

GRID = 14

# ============================================================
# Test 1: navigate_to same tile (trivial - already there)
# ============================================================

print("\n--- Test 1: Navigate to current position (trivial) ---")

# Place at row 17 and let gravity settle to row 15 (60 frames)
set_player_position(6 * GRID + 7, 17 * GRID + 7)
tick_game_frames(60)

pos = get_player_position()
tile = world_to_tile(pos[0], pos[1])
start_col = tile[0]
start_row = tile[1]
print("Player settled at tile:", start_col, start_row, "on_ground:", is_player_on_ground())

result = navigate_to(start_col, start_row)
print("Result:", result)
if result["success"]:
    print("  v Trivial navigate succeeds")
else:
    print("  x Trivial navigate failed:", result["reason"])

# ============================================================
# Test 2: Walk right on same platform row
# ============================================================

print("\n--- Test 2: Walk right to (9, 15) ---")

set_player_position(6 * GRID + 7, 17 * GRID + 7)
tick_game_frames(60)

pos = get_player_position()
tile = world_to_tile(pos[0], pos[1])
print("Start tile:", tile[0], tile[1], "on_ground:", is_player_on_ground())

result = navigate_to(9, 15)
print("Result:", result)

pos2 = get_player_position()
tile2 = world_to_tile(pos2[0], pos2[1])
print("Player ended at tile:", tile2[0], tile2[1])

if result["success"]:
    print("  v Walk right succeeded in", result["frames"], "frames")
    if tile2[1] == 15:
        print("  v Player is on row 15")
    else:
        print("  Note: Player at row", tile2[1], "(expected 15)")
else:
    print("  x Walk right failed:", result["reason"])

# ============================================================
# Test 3: navigate_to unreachable tile (in wall) - should return no_path
# ============================================================

print("\n--- Test 3: Navigate to wall (0,0) - should fail gracefully ---")

set_player_position(6 * GRID + 7, 17 * GRID + 7)
tick_game_frames(60)

result = navigate_to(0, 0)
print("Result:", result)

if not result["success"] and result["reason"] == "no_path":
    print("  v Correctly returns no_path for wall tile")
else:
    print("  Note: Unexpected result:", result)

# ============================================================
# Test 4: Walk left across the platform (negative direction)
# ============================================================

print("\n--- Test 4: Walk left to (6, 15) from (9, 15) ---")

set_player_position(9 * GRID + 7, 17 * GRID + 7)
tick_game_frames(60)

pos4 = get_player_position()
tile4 = world_to_tile(pos4[0], pos4[1])
print("Start tile:", tile4[0], tile4[1], "on_ground:", is_player_on_ground())

result = navigate_to(6, 15)
print("Result:", result)

pos4b = get_player_position()
tile4b = world_to_tile(pos4b[0], pos4b[1])
print("Player ended at tile:", tile4b[0], tile4b[1])

if result["success"]:
    print("  v Walk left succeeded in", result["frames"], "frames")
else:
    print("  x Walk left failed:", result["reason"])

print("\nAll navigation tests completed!")
