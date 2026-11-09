print("Tiles at col 6, rows 12-19:")
for r in range(12, 20):
    t = get_tile_type(6, r)
    print("  (6," + str(r) + "):", t)

print("Tiles at col 6, rows 3-9:")
for r in range(3, 10):
    t = get_tile_type(6, r)
    print("  (6," + str(r) + "):", t)
