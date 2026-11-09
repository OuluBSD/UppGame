# C8_Intro - Python version (REFACTORED - no nested functions)
# Converted from C8_Intro.esc (Scumm-8 by Paul Nicholls)
# PICO-8 intro sequence - SIMPLIFIED VERSION

# ============================================================================
# Global Variables
# ============================================================================

# Debug flags
show_debuginfo = False

# Game verbs
verbs = [
    {"name": "lookat", "text": "look-at"},
    {"name": "use", "text": "use"}
]

# Default verb index (look-at)
verb_default = 0

# Graphics loading state
req_gfx_num = 0
gfx_page = 0
pn = {}

# Decompression state (avoids nested functions with nonlocal)
_decomp = {}

# ============================================================================
# Initialization Functions
# ============================================================================

def reset():
    """Reset game state"""
    global verbs, verb_default
    verbs = [
        {"name": "lookat", "text": "look-at"},
        {"name": "use", "text": "use"}
    ]
    verb_default = 0


def reset_ui():
    """Reset UI colors"""
    global verb_maincol, verb_hovcol, verb_shadcol, verb_defcol
    verb_maincol = 12
    verb_hovcol = 7
    verb_shadcol = 1
    verb_defcol = 10


def startup_script():
    """Initial game setup"""
    reset_ui()
    say_line("C8 Intro - Refactored (no nested functions)")
    say_line("Python callbacks working!")


# ============================================================================
# Graphics Functions (refactored - no nested functions)
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


def decomp_init(src, px, py):
    """Initialize decompression state"""
    global _decomp, pn
    pn = {}
    _decomp = {
        'src': src - 1,
        'bit': 256,
        'b': 0,
        'byte': 0,
        'px': px,
        'py': py
    }


def decomp_getval(bits):
    """Get value from bitstream (replaces nested getval)"""
    global _decomp
    val = 0
    for i in range(bits):
        if _decomp['bit'] == 256:
            _decomp['bit'] = 1
            _decomp['src'] = _decomp['src'] + 1
            _decomp['byte'] = peek(_decomp['src'])
        if band(_decomp['byte'], _decomp['bit']) > 0:
            val = val + shl(1, i)
        _decomp['bit'] = _decomp['bit'] * 2
    return val


def decomp(src, px, py, xget, xset):
    """Decompress graphics data (refactored - no nested functions)"""
    global pn, _decomp
    decomp_init(src, px, py)
    
    # Read header
    w = decomp_getval(8)
    h = decomp_getval(8)
    cbits = decomp_getval(3)
    rmp = decomp_getval(1)
    maxci = decomp_getval(8)
    bpp = decomp_getval(3) + 1
    clist = []
    for i in range(maxci + 1):
        clist.append(decomp_getval(bpp))

    # Spans
    i = 0
    span = 0
    while i < w * h:
        bl = 1
        while decomp_getval(1) == 0:
            bl = bl + 1
        minv = shl(1, bl - 1)
        if bl == 1:
            minv = 0
        length = decomp_getval(max(1, bl - 1)) + minv + 1

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
                index = 0
                while True:
                    v = decomp_getval(cbits)
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
            else:
                delta = decomp_getval(cbits)
                if band(delta, 1):
                    delta = -shr(delta, 1)
                else:
                    delta = shr(delta, 1)
                col = band(pc + delta, 255)

            xset(x, y, col)
            pn[col] = col
            i = i + 1
            span = span + 1


# ============================================================================
# Room Definitions (simplified)
# ============================================================================

rm_test = {
    "map": [0, 0, 16, 8],
    "objects": [],
    "enter": lambda me: test_enter(me)
}


def test_enter(me):
    """Test room enter handler"""
    say_line("Room enter callback works!")
    say_line("No nested functions needed!")


rm_void = {
    "map": [0, 0],
    "objects": []
}


# ============================================================================
# Room Lists
# ============================================================================

rooms = [
    rm_void,
    rm_test
]

actors = []
