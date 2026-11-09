# Test A* pathfinding (Track 2, Phase 1, Task 1)

print("Testing A* pathfinding...")

# First, discover level geometry
dims = get_level_dimensions()
print("Level dimensions:", dims)
print("Grid size:", dims["grid_size"])
cols = dims["columns"]
rows = dims["rows"]
grid = dims["grid_size"]

# ============================================================
# Test 1: Basic path - same row (horizontal walk)
# ============================================================

print("\n--- Test 1: Horizontal Walk ---")

# Scan bottom rows to find two walkable tiles for a path
# Walkable = passable tile + solid below
start_col = -1
start_row = -1
end_col = -1

# Scan rows 15-25 (where the player walks in world1-stage1)
# IsWalkable: tile empty, tile above empty, tile below solid
scan_row = -1
for try_row in range(15, 26):
    found_start = False
    for c in range(1, min(cols - 1, 50)):
        tile = get_tile_type(c, try_row)
        above = get_tile_type(c, try_row - 1)
        below = get_tile_type(c, try_row + 1)
        walkable = (tile == "Empty") and (above == "Empty") and (below == "Wall" or below == "FullBlock")
        if walkable:
            if not found_start:
                start_col = c
                start_row = try_row
                scan_row = try_row
                found_start = True
            elif c > start_col + 2:
                end_col = c
                break
    if end_col >= 0:
        break

print("Walkable start:", start_col, start_row)
print("Walkable end:", end_col, scan_row)

if start_col >= 0 and end_col >= 0:
    path = find_path(start_col, start_row, end_col, scan_row)
    print("Path length:", len(path))
    if len(path) > 0:
        print("First node:", path[0])
        print("Last node:", path[len(path) - 1])
        last = path[len(path) - 1]
        if last["col"] == end_col and last["row"] == scan_row:
            print("  v Path reaches goal correctly")
        else:
            print("  x Path goal mismatch: expected", end_col, scan_row, "got", last["col"], last["row"])
        # Verify all nodes are valid
        valid = True
        for i in range(len(path)):
            n = path[i]
            if n["col"] < 0 or n["row"] < 0:
                valid = False
        if valid:
            print("  v All nodes have valid coordinates")
        else:
            print("  x Some nodes have invalid coordinates")
    else:
        print("  Note: No path found between", start_col, start_row, "and", end_col, scan_row)
else:
    print("  Note: Could not find walkable tiles in scan row", scan_row)

# ============================================================
# Test 2: Same-tile path (trivial)
# ============================================================

print("\n--- Test 2: Trivial Path (same tile) ---")

if start_col >= 0:
    path = find_path(start_col, start_row, start_col, start_row)
    print("Trivial path length:", len(path))
    if len(path) == 1:
        print("  v Trivial path has 1 node (start == goal)")
    elif len(path) > 0:
        print("  Note: Trivial path has", len(path), "nodes")
    else:
        print("  Note: Trivial path returned empty")

# ============================================================
# Test 3: Unreachable destination (in a wall)
# ============================================================

print("\n--- Test 3: Unreachable Goal (solid tile) ---")

if start_col >= 0:
    # Find a solid tile
    solid_col = -1
    solid_row = -1
    for c in range(cols):
        t = get_tile_type(c, 0)
        if t == "Wall" or t == "FullBlock":
            solid_col = c
            solid_row = 0
            break

    if solid_col >= 0:
        path = find_path(start_col, start_row, solid_col, solid_row)
        print("Path to solid tile at", solid_col, solid_row, ":", len(path), "nodes")
        if len(path) == 0:
            print("  v Correctly returns empty path for unreachable goal")
        else:
            print("  Note: Returned", len(path), "nodes (unexpected)")
    else:
        print("  Note: No solid tile found")

# ============================================================
# Test 4: find_path returns dicts with correct keys
# ============================================================

print("\n--- Test 4: Path Node Structure ---")

if start_col >= 0 and end_col >= 0:
    path = find_path(start_col, start_row, end_col, scan_row)
    if len(path) > 0:
        node = path[0]
        print("First node:", node)
        nc = node["col"]
        nr = node["row"]
        mt = node["move_type"]
        print("  col:", nc, "  row:", nr, "  move_type:", mt)
        if mt == "walk" or mt == "jump" or mt == "fall":
            print("  v Node has correct structure, move_type:", mt)
        else:
            print("  x Unknown move_type:", mt)
    else:
        print("  Note: Path was empty, cannot check node structure")

# ============================================================
# Test 5: Cross-platform jump path
# Platforms at row 17 (lower) and row 13 (upper), both at col 6.
# Height diff = 4 = MAX_JUMP_HEIGHT, same column -> requires a jump.
# ============================================================

print("\n--- Test 5: Cross-platform Jump ---")

# Verify both tiles are actually walkable before testing
lower_col = 6
lower_row = 17
upper_col = 6
upper_row = 13

lt = get_tile_type(lower_col, lower_row)
la = get_tile_type(lower_col, lower_row - 1)
lb = get_tile_type(lower_col, lower_row + 1)
ut = get_tile_type(upper_col, upper_row)
ua = get_tile_type(upper_col, upper_row - 1)
ub = get_tile_type(upper_col, upper_row + 1)

lower_walkable = (lt == "Empty") and (la == "Empty") and (lb == "Wall" or lb == "FullBlock")
upper_walkable = (ut == "Empty") and (ua == "Empty") and (ub == "Wall" or ub == "FullBlock")

print("Lower platform (" + str(lower_col) + "," + str(lower_row) + ") walkable:", lower_walkable)
print("Upper platform (" + str(upper_col) + "," + str(upper_row) + ") walkable:", upper_walkable)

if lower_walkable and upper_walkable:
    path = find_path(lower_col, lower_row, upper_col, upper_row)
    print("Jump path length:", len(path))
    if len(path) > 0:
        last = path[len(path) - 1]
        if last["col"] == upper_col and last["row"] == upper_row:
            print("  v Path reaches upper platform")
        else:
            print("  x Path goal mismatch: expected", upper_col, upper_row, "got", last["col"], last["row"])
        # Check that at least one node uses jump move_type
        has_jump = False
        for i in range(len(path)):
            if path[i]["move_type"] == "jump":
                has_jump = True
        if has_jump:
            print("  v Path contains a jump node")
        else:
            print("  Note: Path found but no jump node (unexpected for height diff 4)")
    else:
        print("  x No path found between platforms (height diff 4 should be jumpable)")
else:
    print("  Note: Platform tiles not walkable as expected - level may have changed")

print("\nAll pathfinding tests completed!")
