# C8 Intro - Python version
# Converted from C8_Intro.esc (Scumm-8 by Paul Nicholls)
# PICO-8 intro sequence

# ============================================================================
# Global Variables
# ============================================================================

# Debug flags
show_debuginfo = False
show_collision = False
show_pathfinding = False
show_perfinfo = False
enable_mouse = True
enable_diag_squeeze = True  # Allow squeeze through diag gap?
enable_gfx_load = True

# Game verbs (used in room definitions and UI)
verbs = [
    {"name": "open", "text": "open"},
    {"name": "close", "text": "close"},
    {"name": "give", "text": "give"},
    {"name": "pickup", "text": "pick-up"},
    {"name": "lookat", "text": "look-at"},
    {"name": "talkto", "text": "talk-to"},
    {"name": "push", "text": "push"},
    {"name": "pull", "text": "pull"},
    {"name": "use", "text": "use"}
]

# Verb to use when just clicking around (e.g. move actor)
verb_default = {"name": "walkto", "text": "walk to"}

# UI colors
verb_maincol = 12   # main color (lt blue)
verb_hovcol = 7     # hover color (white)
verb_shadcol = 1    # shadow (dk blue)
verb_defcol = 10    # default action (yellow)

# Cursor sprites
ui_cursorspr = 96
ui_uparrowspr = 80
ui_dnarrowspr = 112

# Default colors to use when animating cursor
ui_cursor_cols = [7, 12, 13, 13, 12, 7]

# Graphics loading
req_gfx_num = -1
curr_gfx_num = -1

# ============================================================================
# Initialization Functions
# ============================================================================

def reset_ui():
    """Reset UI colors and cursors"""
    global verb_maincol, verb_hovcol, verb_shadcol, verb_defcol
    global ui_cursorspr, ui_uparrowspr, ui_dnarrowspr, ui_cursor_cols

    verb_maincol = 12   # main color (lt blue)
    verb_hovcol = 7     # hover color (white)
    verb_shadcol = 1    # shadow (dk blue)
    verb_defcol = 10    # default action (yellow)
    ui_cursorspr = 96   # default cursor sprite
    ui_uparrowspr = 80  # default up arrow sprite
    ui_dnarrowspr = 112 # default down arrow sprite
    # Default colors to use when animating cursor
    ui_cursor_cols = [7, 12, 13, 13, 12, 7]


def startup_script():
    """Initial game setup"""
    # Initial UI setup
    reset_ui()

    # Change to first room
    change_room(rm_liquidream, 1)  # iris fade

    # Reset all game state
    cartdata("pn_code8")
    for d in range(64):
        dset(d, 0)

    # ########################################
    # Testing
    #
    # dset(20, 1)  # main_actor.fixed_ship
    # dset(22, 1)  # main_actor.disabled_signal = True
    # dset(23, 1)  # main_actor.engine_cover_replace = True
    # dset(24, 1)  # main_actor.crystal_replace = True
    #
    # ########################################

    music(1)

    # For any other room
    # selected_actor = main_actor
    # put_at(selected_actor, 48, 46, rm_map)
    # camera_follow(selected_actor)
    # change_room(rm_map)  # iris fade


# ============================================================================
# Graphics Loading Functions
# ============================================================================

def load_gfx_page(gfx_num):
    """Request graphics page load"""
    global req_gfx_num
    req_gfx_num = gfx_num


def remap(i, w, h):
    """Remap index for decompression"""
    sx = flr((i / 64) % (w / 8))
    sy = flr((i / 64) / (w / 8))
    x = (i % 8)
    y = flr(flr(i % 64) / 8)
    return (sx * 8 + x) + (sy * 8 + y) * w


def decomp(src, px, py, xget, xset):
    """Decompress graphics data"""
    global pn
    pn = {}
    src = src - 1
    bit = 256
    b = 0
    byte = 0

    def getval(bits):
        nonlocal bit, src, byte
        val = 0
        for i in range(bits):
            # Get next bit from stream
            if bit == 256:
                bit = 1
                src = src + 1
                byte = peek(src)
            if band(byte, bit) > 0:
                val = val + shl(1, i)
            bit = bit * 2
        return val

    # Read header
    w = getval(8)
    h = getval(8)
    cbits = getval(3)
    rmp = getval(1)
    maxci = getval(8)
    bpp = getval(3) + 1
    clist = []
    for i in range(maxci + 1):
        clist.append(getval(bpp))

    # Spans
    i = 0
    span = 0
    while i < w * h:
        # Span length
        bl = 1
        while getval(1) == 0:
            bl = bl + 1
        minv = shl(1, bl - 1)
        if bl == 1:
            minv = 0
        length = getval(max(1, bl - 1)) + minv + 1

        for j in range(length):
            i1 = i
            if rmp == 1:
                i1 = remap(i, w, h)
            x = px + (i1) % w
            y = py + flr(i1 / w)

            # Predict colour
            t = xget(x + 0, y - 1) / 16
            l = xget(x - 1, y + 0) * 16
            if y == py:
                t = 0
            if x == px:
                l = 0
            pc = pn.get(t + l) or pn.get(t) or pn.get(l)

            if span % 2 == 0:
                # Raw literal
                index = 0
                while True:
                    v = getval(cbits)
                    index = index + v
                    if v < shl(1, cbits) - 1:
                        break
                pindex = 999
                for i_idx in range(maxci + 1):
                    if pc == clist[i_idx]:
                        pindex = i_idx
                if pindex <= index:
                    index = index + 1
                col = clist[index]

                # Move to front
                for i_idx in range(index, 0, -1):
                    clist[i_idx] = clist[i_idx - 1]
                clist[0] = col
            else:
                # Predicted
                col = pc

            xset(x, y, col)

            # Adjust predictions
            pn[t] = col
            pn[l] = col
            pn[t + l] = col
            i = i + 1

        span = span + 1


def load_gfx(index, x, y):
    """Load graphics at index"""
    offset = 0x0000  # Screen memory
    for i in range(index):
        offset = offset + peek(offset + 0) + peek(offset + 1) * 256 + 2
    decomp(offset + 2, x, y, pget, pset)


# ============================================================================
# Room Definitions
# ============================================================================

# [Liquidream logo "room"]
obj_liquidream = {
    "x": 1,
    "y": 1,
    "classes": ["class_untouchable"],

    "draw": lambda me: (
        sspr(48, 8, 32, 8, 32, 56, 64, 16)
    )
}

rm_liquidream = {
    "map": [0, 8],

    "objects": [
        obj_liquidream
    ],

    "enter": lambda me: liquidream_enter(me)
}


def liquidream_enter(me):
    """Liquidream room enter handler"""
    global t

    # Switch gfx
    load_gfx_page(0)
    t = 0

    cutscene(
        3,  # no verbs & no follow
        lambda: (
            break_time(150),
            change_room(rm_scumm, 1)  # iris fade
        )
    )


# [Scumm-8 logo "room"]
obj_scumm8_logo = {
    "x": 1,
    "y": 1,
    "classes": ["class_untouchable"],

    "draw": lambda me: (
        # Draw logo
        sspr(48, 8, 32, 8, 32, 56, 64, 16)
    )
}

rm_scumm = {
    "map": [0, 0],

    "objects": [
        obj_scumm8_logo
    ],

    "enter": lambda me: scumm_enter(me)
}


def scumm_enter(me):
    """Scumm room enter handler"""
    global t

    # Switch gfx
    load_gfx_page(1)
    t = 0

    cutscene(
        3,  # no verbs & no follow
        lambda: (
            print_line("powered by", 64, 48, 7, 1, False, 175),
            # Fade out, using "iris" transition
            fades(1, 1),
            # change_room(rm_crash, 1)  # iris fade
            print_line("music by;;chris (gruber) donnelly", 64, 50, 7, 1, False, 100),
            # Load the game cart
            load("_game-pt1")
        )
    )


# [ "The void" (room) ]
# A place to put objects/actors when not in a room

rm_void = {
    "map": [0, 0],

    "objects": []
}


# ============================================================================
# Room Lists
# ============================================================================

rooms = [
    rm_void,
    rm_liquidream,
    rm_scumm
]


# ============================================================================
# Actor Definitions
# ============================================================================

# Initialize the player's actor object
# main_actor = {
#     "name": "humanoid",
#     "w": 1,
#     "h": 4,
#     "idle": [65, 69, 71, 69],
#     "talk": [90, 91, 92, 91],
#     "walk_anim_side": [68, 69, 70, 69],
#     "walk_anim_front": [66, 65, 67, 65],
#     "walk_anim_back": [72, 71, 73, 71],
#     "col": 7,
#     "trans_col": 11,
#     "walk_speed": 0.6,
#     "frame_delay": 5,
#     "classes": ["class_actor"],
#     "face_dir": "face_front",
#
#     # Sprites for directions (front, left, back, right) - note: "right=left-flipped"
#     "inventory": [
#         # obj_switch_tent
#     ],
#
#     "verbs": {
#         "use": lambda me: (
#             selected_actor.__setitem__("value", me),
#             camera_follow(me)
#         )
#     }
# }


# ============================================================================
# Active Actors List
# ============================================================================

actors = [
    # main_actor
]
