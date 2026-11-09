# Scan level for walkable platforms at different heights
# Goal: find two walkable tiles separated by a height difference (needs jump)

dims = get_level_dimensions()
cols = dims["columns"]
rows = dims["rows"]

print("Scanning all walkable tiles by row...")

# Collect one representative walkable col per row
walkable_rows = {}
for r in range(2, rows - 1):
    for c in range(1, cols - 1):
        tile  = get_tile_type(c, r)
        above = get_tile_type(c, r - 1)
        below = get_tile_type(c, r + 1)
        if tile == "Empty" and above == "Empty" and (below == "Wall" or below == "FullBlock"):
            if r not in walkable_rows:
                walkable_rows[r] = c

print("Walkable rows found:", len(walkable_rows))

row_list = []
for k in walkable_rows:
    row_list.Add(k)

# Sort ascending (bubble sort - no sorted() in ByteVM)
n = len(row_list)
for i in range(n):
    for j in range(n - i - 1):
        if row_list[j] > row_list[j + 1]:
            tmp = row_list[j]
            row_list[j] = row_list[j + 1]
            row_list[j + 1] = tmp

print("Sorted walkable rows:")
for i in range(len(row_list)):
    r = row_list[i]
    print("  row", r, "sample col", walkable_rows[r])

# Find pairs with height difference 1-4 (jumpable) and reasonable col distance
print("\nLooking for jump-worthy pairs (height diff 1-4, col diff <= 3):")
found = 0
for i in range(len(row_list)):
    for j in range(len(row_list)):
        r1 = row_list[i]
        r2 = row_list[j]
        dh = r1 - r2  # positive = r2 is higher (needs jump)
        if dh < 1 or dh > 4:
            continue
        c1 = walkable_rows[r1]
        c2 = walkable_rows[r2]
        dc = c2 - c1
        if dc < 0:
            dc = -dc
        if dc > 3:
            continue
        print("  lower=(" + str(c1) + "," + str(r1) + ") upper=(" + str(c2) + "," + str(r2) + ") dh=" + str(dh) + " dc=" + str(dc))
        found = found + 1
        if found >= 10:
            break
    if found >= 10:
        break

if found == 0:
    print("  No close pairs found")

print("Done.")
