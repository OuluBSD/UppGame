# Test collision/physics queries (Task 3) and level/map state (Task 4)

print("Testing level state and collision API...")

# ============================================================
# Task 4: Level/Map State
# ============================================================

print("\n--- Level/Map State ---")

# Get level dimensions
dims = get_level_dimensions()
print("Level dimensions:", dims)
print("  Columns:", dims["columns"])
print("  Rows:", dims["rows"])
print("  Grid size:", dims["grid_size"])

if dims["columns"] > 0 and dims["rows"] > 0 and dims["grid_size"] > 0:
    print("  ✓ Level dimensions valid")
else:
    print("  ✗ Level dimensions invalid")

# Get level path
path = get_level_path()
print("Level path:", path)
if len(path) > 0:
    print("  ✓ Level path set")
else:
    print("  ✗ Level path empty")

# Get game state
state = get_game_state()
print("Game state:", state)
if state == "PLAYING":
    print("  ✓ Game is PLAYING")
else:
    print("  Note: Game state is", state, "(expected PLAYING for headless test)")

# Get droplet counts
collected = get_droplet_count()
remaining = get_droplet_remaining()
print("Droplets collected:", collected)
print("Droplets remaining:", remaining)
print("  Total droplets:", collected + remaining)

# ============================================================
# Task 3: Collision/Physics Queries
# ============================================================

print("\n--- Collision/Physics Queries ---")

grid_size = dims["grid_size"]
cols = dims["columns"]
rows = dims["rows"]

# Test world_to_tile conversion
print("\nTesting coordinate conversion...")
tile_coord = world_to_tile(0.0, 0.0)
print("world_to_tile(0, 0):", tile_coord)
if tile_coord[0] == 0 and tile_coord[1] == 0:
    print("  ✓ Origin maps to tile (0,0)")
else:
    print("  ✗ Origin should map to (0,0), got", tile_coord)

# Test tile_to_world conversion (should be inverse)
world_coord = tile_to_world(1, 1)
print("tile_to_world(1, 1):", world_coord)
expected_x = grid_size * 1
expected_y = grid_size * 1
if abs(world_coord[0] - expected_x) < 0.1 and abs(world_coord[1] - expected_y) < 0.1:
    print("  ✓ Tile (1,1) maps to world correctly")
else:
    print("  ✗ Expected", expected_x, expected_y, "got", world_coord)

# Test round-trip conversion
test_x = grid_size * 3 + grid_size / 2
test_y = grid_size * 5 + grid_size / 4
tile = world_to_tile(test_x, test_y)
print("\nRound-trip test: world (" + str(test_x) + ", " + str(test_y) + ") -> tile", tile)
if tile[0] == 3 and tile[1] == 5:
    print("  ✓ Round-trip conversion correct")
else:
    print("  ✗ Expected (3, 5), got", tile)

# Test tile type queries
print("\nTesting tile type queries...")
tile_type = get_tile_type(0, 0)
print("Tile type at (0,0):", tile_type)

# Query a range of tiles to find non-empty ones
print("\nScanning for non-empty tiles in first 10 columns and rows...")
wall_count = 0
bg_count = 0
fullblock_count = 0
empty_count = 0
scan_limit = 10
for r in range(scan_limit):
    for c in range(scan_limit):
        t = get_tile_type(c, r)
        if t == "Wall":
            wall_count = wall_count + 1
        elif t == "Background":
            bg_count = bg_count + 1
        elif t == "FullBlock":
            fullblock_count = fullblock_count + 1
        else:
            empty_count = empty_count + 1

print("  Wall tiles:", wall_count)
print("  Background tiles:", bg_count)
print("  Full block tiles:", fullblock_count)
print("  Empty tiles:", empty_count)
print("  Total scanned:", scan_limit * scan_limit)

# Test solid/wall predicates at a known solid location
# world1-stage1 typically has floor tiles at the bottom rows
# Scan bottom few rows to find solid tiles
print("\nScanning bottom rows for solid tiles...")
found_solid = 0
bottom_row = rows - 1
for c in range(min(cols, 20)):
    if is_tile_solid(c, bottom_row):
        found_solid = found_solid + 1
print("  Solid tiles in bottom row (" + str(bottom_row) + "):", found_solid)

# Test out-of-bounds (should return EMPTY/false gracefully)
print("\nTesting out-of-bounds handling...")
oob_type = get_tile_type(-1, -1)
print("  get_tile_type(-1, -1):", oob_type)
oob_solid = is_tile_solid(-1, -1)
print("  is_tile_solid(-1, -1):", oob_solid)
if oob_type == "EMPTY" and not oob_solid:
    print("  ✓ Out-of-bounds handled gracefully")
else:
    print("  Note: OOB returns", oob_type, oob_solid)

print("\nAll level state and collision tests completed!")
