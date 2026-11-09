# CarverTest - Python version
# Converted from CarverTest.esc
# Room carving test content

# ============================================================================
# Carver Test Script
# ============================================================================

def make_map():
    """Create a test map using room carving"""
    global r, split

    r = carve_room(1, 1)
    split = 1.0 / 5.0

    # Use pre-made tileset theme
    use_tiles(r, "kitchen")

    # Add a door to the back wall in the left side
    # r: add door to room
    # 1: back wall (left, back, right, invisible-front)
    # split: fraction of the wall from left to right when looking from inside
    d1 = carve_door(r, 1, split)


# Run the map creation
make_map()
