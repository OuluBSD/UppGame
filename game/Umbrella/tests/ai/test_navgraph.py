# Test NavGraph - Level Navigation Graph (Track 2, Phase 1, Task 2)
# Known level geometry from scan_platforms.py:
#   Walkable rows: 4 (col 6), 8 (col 2), 13 (col 6), 17 (col 6)
#   Platforms at row 13 and 17 share col 6 with dh=4 (jumpable)

print("Testing NavGraph...")

# ============================================================
# Test 1: Build succeeds and returns sensible stats
# ============================================================

print("\n--- Test 1: Build ---")

stats = build_nav_graph()
print("Stats:", stats)

wc = stats["walkable"]
cc = stats["components"]
ec = stats["edges"]

print("  walkable tiles:", wc)
print("  components:", cc)
print("  edges:", ec)

if wc > 0:
    print("  v walkable count > 0")
else:
    print("  x walkable count is 0 - level may not be loaded")

if cc > 0:
    print("  v component count > 0")
else:
    print("  x component count is 0")

if ec > 0:
    print("  v edge count > 0")
else:
    print("  x edge count is 0")

# ============================================================
# Test 2: Component ID of a known walkable tile is non-negative
# ============================================================

print("\n--- Test 2: Component ID of walkable tile ---")

cid = get_component_id(6, 17)
print("Component ID at (6,17):", cid)
if cid >= 0:
    print("  v Walkable tile has valid component ID")
else:
    print("  x Walkable tile returned -1 (not in any component)")

# Non-walkable tile (solid wall at row 0, col 0)
cid_wall = get_component_id(0, 0)
print("Component ID at (0,0) [wall]:", cid_wall)
if cid_wall < 0:
    print("  v Solid tile correctly returns -1")
else:
    print("  x Solid tile should return -1, got:", cid_wall)

# ============================================================
# Test 3: is_reachable - same platform (trivially true)
# ============================================================

print("\n--- Test 3: Same-platform reachability ---")

r = is_reachable(6, 17, 9, 17)
print("is_reachable((6,17), (9,17)):", r)
if r:
    print("  v Tiles on same row are reachable")
else:
    print("  x Same-row tiles should be reachable")

# ============================================================
# Test 4: is_reachable - one-way jump (row 17 -> row 13)
# Geometry: (6,13) floor=Wall at row 14. Falling from (6,13) to (6,17)
# is BLOCKED by the Wall at row 14 (the platform's own floor).
# So the jump 17->13 is one-way. These are DIFFERENT components.
# ============================================================

print("\n--- Test 4: One-way jump (different components) ---")

r2 = is_reachable(6, 17, 6, 13)
print("is_reachable((6,17), (6,13)) [one-way jump]:", r2)
if not r2:
    print("  v Correctly different components: fall 13->17 blocked by platform floor at row 14")
else:
    print("  x Expected different components (fall path blocked by wall at row 14)")

# ============================================================
# Test 5: is_reachable - tile inside a wall (unreachable)
# ============================================================

print("\n--- Test 5: Unreachable tile (wall) ---")

r3 = is_reachable(6, 17, 0, 0)
print("is_reachable((6,17), (0,0)) [wall]:", r3)
if not r3:
    print("  v Wall tile correctly not reachable")
else:
    print("  x Wall tile should not be reachable")

# ============================================================
# Test 6: Consistency - component IDs match is_reachable
# ============================================================

print("\n--- Test 6: Component ID consistency ---")

cid_17 = get_component_id(6, 17)
cid_13 = get_component_id(6, 13)
print("Component(6,17):", cid_17, "  Component(6,13):", cid_13)

reach = is_reachable(6, 17, 6, 13)
ids_match = (cid_17 == cid_13)
if reach and ids_match:
    print("  v is_reachable and component IDs agree (same component)")
elif not reach and not ids_match:
    print("  v is_reachable and component IDs agree (different components)")
else:
    print("  x Inconsistency: is_reachable =", reach, "ids_match =", ids_match)

print("\nAll NavGraph tests completed!")
