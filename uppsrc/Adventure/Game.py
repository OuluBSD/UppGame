# Adventure Game - Python version
# Converted from Game.esc (Scumm-8 by Paul Nicholls)
# Translated from PICO-8 to ESC by Seppo Pakonen

# ============================================================================
# Global Variables
# ============================================================================

# Game flags
enable_diag_squeeze = False

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

# Index of the verb to use when clicking items in inventory (e.g. look-at)
verb_default = 4

# Default cursor sprite
ui_cursorspr = 224

# Default up arrow sprite
ui_uparrowspr = 208

# Default down arrow sprite
ui_dnarrowspr = 240

# Default colors to use when animating cursor
ui_cursor_cols = [7, 12, 13, 13, 12, 7]

# UI colors (set in reset_ui)
verb_maincol = 12   # main color (lt blue)
verb_hovcol = 7     # hover color (white)
verb_shadcol = 1    # shadow (dk blue)
verb_defcol = 10    # default action (yellow)

# Selected actor
selected_actor = None

# ============================================================================
# Initialization Functions
# ============================================================================

def reset():
    """Reset game state"""
    global enable_diag_squeeze, verbs, verb_default
    global ui_cursorspr, ui_uparrowspr, ui_dnarrowspr, ui_cursor_cols
    
    # Game flags
    enable_diag_squeeze = False
    
    # Game verbs
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
    
    # Default verb index
    verb_default = 4
    
    # Default cursor sprites
    ui_cursorspr = 224
    ui_uparrowspr = 208
    ui_dnarrowspr = 240
    
    # Default cursor animation colors
    ui_cursor_cols = [7, 12, 13, 13, 12, 7]


def reset_ui():
    """Reset UI colors"""
    global verb_maincol, verb_hovcol, verb_shadcol, verb_defcol
    
    verb_maincol = 12   # main color (lt blue)
    verb_hovcol = 7     # hover color (white)
    verb_shadcol = 1    # shadow (dk blue)
    verb_defcol = 10    # default action (yellow)


def startup_script():
    """Initial game setup"""
    global selected_actor
    
    # Set UI colors
    reset_ui()
    
    # Set which room to start the game in (title screen)
    change_room(rm_title, 1)  # iris fade
    
    # Set which actor the player controls by default
    selected_actor = main_actor


# ============================================================================
# Object Definitions
# ============================================================================

# Switch objects (for debugging/switching characters)
def _switch_tent_use(me):
    use_switch_tent(me)

def _switch_player_use(me):
    use_switch_player(me)

def _duck_pickup(me):
    pickup_obj(me)

obj_switch_tent = {
    "name": "purple tentacle",
    "state": "state_here",
    "state_here": 170,
    "trans_col": 15,
    "x": 1,
    "y": 1,
    "w": 1,
    "h": 1,

    "verbs": {
        "use": _switch_tent_use
    }
}

obj_switch_player = {
    "name": "humanoid",
    "state": "state_here",
    "state_here": 209,
    "trans_col": 11,
    "x": 1,
    "y": 1,
    "w": 1,
    "h": 1,

    "verbs": {
        "use": _switch_player_use
    }
}

obj_duck = {
    "name": "rubber duck",
    "state": "state_here",
    "state_here": 142,
    "trans_col": 12,
    "x": 1,
    "y": 1,
    "w": 1,
    "h": 1,
    "classes": ["class_pickupable"],

    "verbs": {
        "pickup": _duck_pickup
    }
}


# ============================================================================
# Room Callback Functions
# ============================================================================

def rm_title_fn0(me):
    """Title room - function 0 (win game)"""
    if not me["gameover"]:
        change_room(rm_outside, 1)  # iris fade
    else:
        # Win game
        print_line("congratulations!:you've completed the game!", 64, 45, 8, 1, True, 8, False)
        fades(1, 1)  # fade out
        while True:
            break_time(10)


def rm_title_fn1(me):
    """Title room - function 1 (after intro)"""
    if not rm_title["gameover"]:
        rm_outside["done_intro"] = True
        selected_actor = main_actor
        put_at(selected_actor, 30, 55, rm_outside)
        camera_follow(selected_actor)


# ============================================================================
# Room Definitions
# ============================================================================

def _title_enter(me):
    cutscene(
        3,  # no verbs & no follow
        rm_title_fn0,
        rm_title_fn1
    )

def _title_exit():
    pass  # todo: "anything here?"

rm_title = {
    "map": [0, 16],
    "objects": [],
    "enter": _title_enter,
    "exit": _title_exit
}


# ============================================================================
# Outside Room Objects
# ============================================================================

def _outside_stairs_draw(me):
    set_trans_col(8, True)
    map_draw(56, 23, 136, 60, 6, 1)

obj_outside_stairs = {
    "x": 1,
    "y": 1,
    "classes": ["class_untouchable"],

    "draw": _outside_stairs_draw
}

obj_rail_left = {
    "state": "state_here",
    "x": 80,
    "y": 24,
    "w": 1,
    "h": 2,
    "state_here": 47,
    "trans_col": 8,
    "repeat_x": 8,
    "classes": ["class_untouchable"]
}

obj_rail_right = {
    "state": "state_here",
    "x": 176,
    "y": 24,
    "w": 1,
    "h": 2,
    "state_here": 47,
    "trans_col": 8,
    "repeat_x": 8,
    "classes": ["class_untouchable"]
}

def _front_door_init(me):
    me.update({"target_door": obj_front_door_inside})

obj_front_door = {
    "name": "front door",
    "state": "state_closed",
    "x": 152,
    "y": 8,
    "w": 1,
    "h": 3,
    "state_closed": 78,
    "flip_x": True,
    "classes": ["class_openable", "class_door"],
    "use_dir": "face_back",

    "init": _front_door_init
}

def _bucket_lookat(me):
    say_line("it's an old bucket")

def _bucket_pickup(me):
    pickup_obj(me)

def _bucket_use(me, noun2):
    bucket_use(me, noun2)

obj_bucket = {
    "name": "bucket",
    "state": "state_open",
    "x": 208,
    "y": 48,
    "w": 1,
    "h": 1,
    "state_closed": 143,
    "state_open": 159,
    "trans_col": 15,
    "use_with": True,
    "classes": ["class_pickupable"],

    "verbs": {
        "lookat": _bucket_lookat,
        "pickup": _bucket_pickup,
        "use": _bucket_use
    }
}


def bucket_use(me, noun2):
    """Bucket use verb handler"""
    if noun2 == obj_fire and me["state"] == "state_closed":
        put_at(obj_fire, 0, 0, rm_void)
        put_at(obj_key, 88, 32, rm_library)
        me["state"] = "state_open"
        say_line("the fire's out now")
    elif noun2 == obj_pool:
        say_line("let's fill this up...")
        me["state"] = "state_closed"
        me["name"] = "full bucket"
        say_line("that's better!")


def _rm_outside_enter(me):
    rm_outside_enter(me)

def _rm_outside_exit(me):
    pass  # todo: "anything here?"

rm_outside = {
    "map": [0, 24, 31, 31],
    "autodepth_scale": [0.75, 1],

    "objects": [
        obj_outside_stairs,
        obj_rail_left,
        obj_rail_right,
        obj_front_door,
        obj_bucket
    ],

    "enter": _rm_outside_enter,
    "exit": _rm_outside_exit
}


def _rm_outside_intro(me):
    camera_at(144)
    camera_pan_to(selected_actor)
    wait_for_camera()
    say_line("wow! look at that old house:i wonder if anyone's home...")

def rm_outside_enter(me):
    """Outside room enter handler"""
    if not me["done_intro"]:
        # Don't do this again
        me["done_intro"] = True

        # Set which actor the player controls by default
        selected_actor = main_actor

        # Init actor
        put_at(selected_actor, 30, 55, rm_outside)

        # Make camera follow player
        camera_follow(selected_actor)

        # Do cutscene
        cutscene(
            1,  # no verbs
            _rm_outside_intro,
            None
        )


# ============================================================================
# Hall Room Objects
# ============================================================================

def _front_door_inside_init(me):
    me.update({"target_door": obj_front_door})

obj_front_door_inside = {
    "name": "front door",
    "state": "state_closed",
    "x": 8,
    "y": 16,
    "z": 1,
    "w": 1,
    "h": 4,
    "state_closed": 79,
    "classes": ["class_door"],
    "use_pos": "pos_right",
    "use_dir": "face_left",

    "init": _front_door_inside_init
}

def _clock_draw(me):
    set_trans_col(8, True)
    map_draw(56, 16, 32, 16, 2, 6)

def _clock_lookat(me):
    say_line("wow. that is impressive...:must've taken ages to code that!")

obj_clock = {
    "name": "clock",
    "x": 32,
    "y": 0,
    "w": 2,
    "h": 5,
    "z": 2,

    "draw": _clock_draw,

    "verbs": {
        "lookat": _clock_lookat
    }
}

def _pendulum_draw(me):
    rectfill(35, 20, 43, 56, 0)
    line(me["x"], me["y"], obj_pendulum["bobx"], obj_pendulum["boby"], 9)
    circfill(obj_pendulum["bobx"], obj_pendulum["boby"], 2)

obj_pendulum = {
    "x": 40,
    "y": 20,
    "w": 1,
    "z": 1,
    "bobx": 40,
    "boby": 20,

    "draw": _pendulum_draw
}

def _inside_stairs_draw(me):
    set_trans_col(8, True)
    map_draw(56, 23, 68, 52, 6, 1)

obj_inside_stairs = {
    "x": 1,
    "y": 1,
    "w": 1,
    "h": 1,
    "state": "state_here",
    "state_here": 3,
    "classes": ["class_untouchable"],

    "draw": _inside_stairs_draw
}

def _hall_rail_draw(me):
    set_trans_col(8, True)
    map_draw(59, 19, 100, 12, 4, 4)

obj_hall_rail = {
    "state": "state_here",
    "x": 1,
    "y": 1,
    "w": 1,
    "h": 2,
    "z": 30,
    "state_here": 3,
    "classes": ["class_untouchable"],

    "draw": _hall_rail_draw
}

def _hall_exit_landing_walkto(me):
    come_out_door(me, obj_landing_exit_hall)

obj_hall_exit_landing = {
    "name": "upstairs",
    "state": "state_open",
    "x": 106,
    "y": 0,
    "w": 3,
    "h": 2,
    "use_pos": "pos_center",
    "use_dir": "face_back",

    "verbs": {
        "walkto": _hall_exit_landing_walkto
    }
}

def _hall_door_library_walkto(me):
    come_out_door(me, obj_library_door_hall)

obj_hall_door_library = {
    "name": "library",
    "state": "state_open",
    "x": 136,
    "y": 16,
    "w": 1,
    "h": 3,
    "use_dir": "face_back",

    "verbs": {
        "walkto": _hall_door_library_walkto
    }
}

def _hall_door_kitchen_walkto(me):
    come_out_door(me, obj_kitchen_door_hall)

obj_hall_door_kitchen = {
    "name": "kitchen",
    "state": "state_open",
    "x": 176,
    "y": 16,
    "w": 1,
    "h": 4,
    "use_pos": "pos_left",
    "use_dir": "face_right",

    "verbs": {
        "walkto": _hall_door_kitchen_walkto
    }
}


def hall_anim_clock():
    """Hall room - animate clock script"""
    angle = 0.5149  # 3.1415 / 6.101
    avel = 0
    val = -10
    played = False

    while True:
        aacc = -6.81 / 31 * __import__("math").sin(angle)  # -9.81
        avel = avel + aacc * 0.1 * 0.2
        angle = angle + avel * 0.1
        obj_pendulum["bobx"] = obj_pendulum["x"] + __import__("math").sin(angle) * 31
        obj_pendulum["boby"] = obj_pendulum["y"] - __import__("math").cos(angle) * 31
        if angle <= 0.4850 and not played:
            sfx(0)
            played = True
        elif angle >= 0.5140 and not played:
            sfx(1)
            played = True
        elif angle > 0.49 and angle < 0.50:
            played = False
        break_time(None)


def _hall_enter(me):
    start_script(hall_anim_clock, True)

def _hall_exit(me):
    stop_script(hall_anim_clock)

rm_hall = {
    "map": [32, 24, 55, 31],
    "col_replace": [5, 2],
    "autodepth_pos": [40, 50],
    "autodepth_scale": [0.82, 1],

    "objects": [
        obj_clock,
        obj_pendulum,
        obj_front_door_inside,
        obj_inside_stairs,
        obj_hall_rail,
        obj_hall_exit_landing,
        obj_hall_door_library,
        obj_hall_door_kitchen
    ],

    "enter": _hall_enter,
    "exit": _hall_exit,
    "scripts": {
        "anim_clock": hall_anim_clock
    }
}


# ============================================================================
# Library Room Objects
# ============================================================================

def _library_door_hall_init(me):
    me.update({"target_door": obj_hall_door_library})

obj_library_door_hall = {
    "name": "hall",
    "state": "state_open",
    "x": 48,
    "y": 16,
    "w": 1,
    "h": 3,
    "state_open": 128,
    "use_dir": "face_back",
    "classes": ["class_door"],
    "lighting": 1,

    "init": _library_door_hall_init
}

def _lightswitch_use(me):
    lightswitch_use(me)

obj_lightswitch = {
    "name": "light switch",
    "state": "state_here",
    "x": 56,
    "y": 23,
    "w": 1,
    "h": 1,
    "state_here": 125,
    "lighting": 0.6,
    "on": True,

    "verbs": {
        "use": _lightswitch_use
    }
}


def lightswitch_use(me):
    """Light switch use handler"""
    if me["on"]:
        rm_library["lighting"] = 0.25
        obj_library_door_hall["lighting"] = 1
        obj_lightswitch["lighting"] = 0.6
        me["on"] = False
    else:
        rm_library["lighting"] = 1
        obj_library_door_hall["lighting"] = 0
        obj_lightswitch["lighting"] = 1
        me["on"] = True


def _fire_lookat_wrapper():
    fire_lookat()

def _fire_talkto_wrapper():
    fire_talkto()

def _fire_pickup(me):
    pickup_obj(me)

obj_fire = {
    "name": "fire",
    "x": 88,
    "y": 32,
    "w": 1,
    "h": 1,
    "state": 1,
    "states": [81, 82, 83],
    "use_pos": [97, 42],
    "lighting": 1,

    "verbs": {
        "lookat": _fire_lookat_wrapper,
        "talkto": _fire_talkto_wrapper,
        "pickup": _fire_pickup
    }
}


def fire_lookat():
    """Fire look-at handler"""
    say_line("it's a nice, warm fire...")
    break_time(10)
    do_anim(selected_actor, "face_towards", "face_front")
    say_line("ouch! it's hot!:*stupid fire*")


def fire_talkto():
    """Fire talk-to handler"""
    say_line("'hi fire...'")
    break_time(10)
    do_anim(selected_actor, "face_towards", "face_front")
    say_line("the fire didn't say hello back:burn!!")


obj_library_secret_panel = {
    "state": "state_closed",
    "x": 120,
    "y": 16,
    "z": -1,
    "w": 1,
    "h": 3,
    "state_closed": 80,
    "state_open": 80,
    "classes": ["class_untouchable"],
    "use_dir": "face_back",
    "y_offset": 0,
    
    "verbs": {}
}

def _library_secret_walkto_wrapper(me):
    library_secret_walkto(me)

obj_library_door_secret = {
    "name": "secret passage",
    "state": "state_closed",
    "x": 120,
    "y": 16,
    "z": -10,
    "w": 1,
    "h": 3,
    "state_closed": 77,
    "use_dir": "face_back",
    "dependent_on_state": "state_open",
    "dependent_on": obj_library_secret_panel,
    "dependent_on_state": "state_open",

    "verbs": {
        "walkto": _library_secret_walkto_wrapper
    }
}


def library_secret_walkto(me):
    """Secret door walk-to handler"""
    rm_title["gameover"] = True
    change_room(rm_title, 1)


def _book_lookat_wrapper(me):
    book_lookat(me)

def _book_pull_wrapper(me):
    book_pull(me)

def _book_pickup_wrapper(me):
    book_pickup(me)

obj_book = {
    "name": "loose book",
    "state": "state_gone",
    "x": 136,
    "y": 16,
    "w": 1,
    "h": 1,
    "state_gone": 66,
    "state_here": 65,
    "use_pos": [144, 40],
    "classes": ["class_pickupable"],

    "verbs": {
        "lookat": _book_lookat_wrapper,
        "pull": _book_pull_wrapper,
        "pickup": _book_pickup_wrapper
    }
}


def book_lookat(me):
    """Book look-at handler"""
    if me["state"] == "state_gone":
        say_line("this book sticks out")
    else:
        say_line("it's a secret lock that was hidden behind the book")


def book_pull(me):
    """Book pull handler"""
    me["state"] = "state_here"
    me["name"] = "secret lock"


def book_pickup(me):
    """Book pickup handler"""
    book_pull(me)


def _key_lookat(me):
    say_line("it's a gold key")

def _key_pickup(me):
    key_pickup(me)

def _key_use(me, noun2):
    key_use(me, noun2)

obj_key = {
    "name": "gold key",
    "state": "state_gone",
    "x": 1,
    "y": 1,
    "z": 30,
    "w": 1,
    "h": 1,
    "state_gone": 32,
    "state_here": 173,
    "use_with": True,
    "classes": ["class_pickupable"],

    "verbs": {
        "lookat": _key_lookat,
        "pickup": _key_pickup,
        "use": _key_use
    }
}


def key_pickup(me):
    """Key pickup handler"""
    me["state"] = "state_here"
    pickup_obj(me)


def key_use(me, noun2):
    """Key use handler"""
    if noun2 == obj_book:
        put_at(me, 0, 0, rm_void)
        obj_library_secret_panel["state"] = "state_open"
        shake(True)
        while obj_library_secret_panel["y_offset"] > -8:
            obj_library_secret_panel["y_offset"] = obj_library_secret_panel["y_offset"] - 1
            break_time(10)
        shake(False)


def library_anim_fire():
    """Library room - animate fireplace"""
    while True:
        for f in range(1, 4):
            obj_fire["state"] = f
            break_time(8)


def _library_enter(me):
    start_script(library_anim_fire, True)

def _library_exit(me):
    stop_script(library_anim_fire)

rm_library = {
    "map": [56, 24, 79, 31],
    "trans_col": 10,
    "col_replace": [7, 4],
    "lighting": 0.25,

    "objects": [
        obj_library_door_hall,
        obj_lightswitch,
        obj_fire,
        obj_library_door_secret,
        obj_library_secret_panel,
        obj_book
    ],

    "enter": _library_enter,
    "exit": _library_exit,
    "scripts": {
        "anim_fire": library_anim_fire
    }
}


# ============================================================================
# Kitchen Room Objects
# ============================================================================

def _spinning_top_use(me):
    spinning_top_use(me)

obj_spinning_top = {
    "name": "spinning top",
    "x": 148,
    "y": 50,
    "w": 1,
    "h": 1,
    "state": 1,
    "states": [158, 174, 190],
    "col_replace": [12, 7],
    "trans_col": 15,

    "scripts": {
        "spin_top": None  # Will be set below
    },

    "verbs": {
        "use": _spinning_top_use
    }
}


def kitchen_spin_top():
    """Spinning top script"""
    dir = -1
    while True:
        for x in range(1, 4):
            for f in range(1, 4):
                obj_spinning_top["state"] = f
                break_time(4)
            # Move top
            obj_spinning_top["x"] = obj_spinning_top["x"] - dir
        dir = dir * -1


obj_spinning_top["scripts"]["spin_top"] = kitchen_spin_top


def spinning_top_use(me):
    """Spinning top use handler"""
    if script_running(me["scripts"]["spin_top"]):
        stop_script(me["scripts"]["spin_top"])
        me["state"] = 1
    else:
        start_script(me["scripts"]["spin_top"])


def _kitchen_door_hall_init(me):
    me.update({"target_door": obj_hall_door_kitchen})

obj_kitchen_door_hall = {
    "name": "hall",
    "state": "state_open",
    "x": 8,
    "y": 16,
    "w": 1,
    "h": 4,
    "use_pos": "pos_right",
    "use_dir": "face_left",
    "classes": ["class_door"],

    "init": _kitchen_door_hall_init
}

def _back_door_init(me):
    me.update({"target_door": obj_garden_door_kitchen})

obj_back_door = {
    "name": "back door",
    "state": "state_closed",
    "x": 176,
    "y": 16,
    "z": 1,
    "w": 1,
    "h": 4,
    "state_closed": 79,
    "flip_x": True,
    "classes": ["class_openable", "class_door"],
    "use_pos": "pos_left",
    "use_dir": "face_right",

    "init": _back_door_init
}


def _kitchen_tentacle_stop(me):
    stop_actor(selected_actor)
    say_line_actor(purp_tentacle, "stop!:come back here!", True, 0)
    walk_to(selected_actor, purp_tentacle["x"] - 8, purp_tentacle["y"])
    do_anim(selected_actor, "face_towards", purp_tentacle)
    purp_tentacle.update({"alerting": False})

def kitchen_tentacle_guard():
    """Kitchen room - tentacle guard script"""
    while True:
        if (proximity(main_actor, obj_back_door) < 40
            and not purp_tentacle["alerting"]
            and purp_tentacle["in_room"] == selected_actor["in_room"]):
            purp_tentacle["alerting"] = True
            purp_tentacle["stopped_player"] = True
            cutscene(
                2,
                _kitchen_tentacle_stop,
                None
            )
        break_time(10)


def rm_kitchen_enter(me):
    start_script(kitchen_tentacle_guard, True)

def rm_kitchen_exit(me):
    stop_script(kitchen_tentacle_guard)

rm_kitchen = {
    "map": [80, 24, 103, 31],
    
    "objects": [
        obj_spinning_top,
        obj_kitchen_door_hall,
        obj_back_door
    ],
    
    "enter": rm_kitchen_enter,
    "exit": rm_kitchen_exit,
    "scripts": {
        "tentacle_guard": kitchen_tentacle_guard
    }
}


# ============================================================================
# Garden Room Objects
# ============================================================================

def _garden_door_kitchen_init(me):
    me.update({"target_door": obj_back_door})

obj_garden_door_kitchen = {
    "name": "kitchen",
    "state": "state_closed",
    "x": 104,
    "y": 8,
    "w": 1,
    "h": 3,
    "state_closed": 78,
    "classes": ["class_openable", "class_door"],
    "use_dir": "face_back",

    "init": _garden_door_kitchen_init
}

def _pool_walkto(me):
    say_line("i can't swim!")

def _pool_lookat(me):
    say_line("it's filled with water")

obj_pool = {
    "name": "swimming pool",
    "x": 96,
    "y": 48,
    "w": 3,
    "h": 2,
    "use_pos": "pos_above",
    "use_dir": "face_front",

    "verbs": {
        "walkto": _pool_walkto,
        "lookat": _pool_lookat
    }
}

def _garden_enter():
    pass  # todo: "anything here?"

def _garden_exit():
    pass  # todo: "anything here?"

rm_garden = {
    "map": [104, 24, 127, 31],
    "autodepth_scale": [0.75, 1],

    "objects": [
        obj_garden_door_kitchen,
        obj_pool
    ],

    "enter": _garden_enter,
    "exit": _garden_exit
}


# ============================================================================
# Landing Room Objects
# ============================================================================

obj_landing_rail_left = {
    "state": "state_here",
    "x": 0,
    "y": 47,
    "w": 1,
    "h": 2,
    "state_here": 47,
    "trans_col": 8,
    "repeat_x": 16,
    "classes": ["class_untouchable"]
}

obj_landing_rail_right = {
    "state": "state_here",
    "x": 144,
    "y": 47,
    "w": 1,
    "h": 2,
    "state_here": 47,
    "trans_col": 8,
    "repeat_x": 6,
    "classes": ["class_untouchable"]
}

def _landing_exit_hall_init(me):
    me.update({"target_door": obj_hall_exit_landing})

obj_landing_exit_hall = {
    "name": "hall",
    "state": "state_open",
    "x": 124,
    "y": 56,
    "w": 3,
    "h": 2,
    "use_pos": "pos_center",
    "use_dir": "face_front",
    "classes": ["class_door"],

    "init": _landing_exit_hall_init
}

def _landing_door_computer_init(me):
    me.update({"target_door": obj_computer_door_landing})

obj_landing_door_computer = {
    "name": "computer room",
    "state": "state_open",
    "x": 176,
    "y": 16,
    "w": 1,
    "h": 4,
    "use_pos": "pos_left",
    "use_dir": "face_right",
    "classes": ["class_door"],

    "init": _landing_door_computer_init
}

def _landing_door_room1_open(me):
    landing_door_teleport(me, obj_landing_door_room3)

obj_landing_door_room1 = {
    "name": "door #1",
    "state": "state_closed",
    "x": 8,
    "y": 16,
    "z": 1,
    "w": 1,
    "h": 4,
    "state_closed": 79,
    "state_open": 0,
    "classes": ["class_openable"],
    "use_pos": "pos_right",
    "use_dir": "face_left",

    "verbs": {
        "open": _landing_door_room1_open
    }
}

def _landing_door_room2_open(me):
    landing_door_teleport(me, obj_landing_door_room3)

obj_landing_door_room2 = {
    "name": "door #2",
    "state": "state_closed",
    "x": 48,
    "y": 16,
    "z": 1,
    "w": 1,
    "h": 3,
    "state_closed": 78,
    "classes": ["class_openable"],
    "use_pos": "pos_infront",
    "use_dir": "face_back",

    "verbs": {
        "open": _landing_door_room2_open
    }
}

def _landing_door_room3_open(me):
    landing_door_teleport(me, obj_landing_door_room1)

obj_landing_door_room3 = {
    "name": "door #3",
    "state": "state_closed",
    "x": 136,
    "y": 16,
    "z": 1,
    "w": 1,
    "h": 3,
    "state_closed": 78,
    "state_open": 0,
    "classes": ["class_openable"],
    "use_pos": "pos_infront",
    "use_dir": "face_back",

    "verbs": {
        "open": _landing_door_room3_open
    }
}


def _landing_door_teleport_scene(me, door1, door2):
    open_door(door1, None)
    break_time(10)
    put_at(selected_actor, 0, 0, rm_void)
    close_door(door1)
    camera_pan_to(door2)
    wait_for_camera()
    open_door(door1, door2)
    break_time(10)
    come_out_door(door1, door2)
    close_door(door1, door2)
    camera_follow(selected_actor)

# Store door params for teleport scene
_landing_door_teleport_params = {"door1": None, "door2": None}

def _landing_door_teleport_fn(me):
    _landing_door_teleport_scene(me, _landing_door_teleport_params["door1"], _landing_door_teleport_params["door2"])

def landing_door_teleport(door1, door2):
    """Landing room - door teleport cutscene"""
    _landing_door_teleport_params["door1"] = door1
    _landing_door_teleport_params["door2"] = door2
    cutscene(
        2,  # quick-cut
        _landing_door_teleport_fn,
        None
    )


def _landing_enter(me):
    pass

def _landing_exit(me):
    pass

rm_landing = {
    "map": [32, 16, 55, 31],

    "objects": [
        obj_landing_rail_left,
        obj_landing_rail_right,
        obj_landing_exit_hall,
        obj_landing_door_room1,
        obj_landing_door_room2,
        obj_landing_door_room3,
        obj_landing_door_computer
    ],

    "enter": _landing_enter,
    "exit": _landing_exit,
    "scripts": {
        "door_teleport": landing_door_teleport
    }
}


# ============================================================================
# Computer Room Objects
# ============================================================================

def _computer_door_landing_init(me):
    me.update({"target_door": obj_landing_door_computer})

obj_computer_door_landing = {
    "name": "first floor",
    "state": "state_open",
    "x": 8,
    "y": 16,
    "w": 1,
    "h": 4,
    "use_pos": "pos_right",
    "use_dir": "face_left",
    "classes": ["class_door"],

    "init": _computer_door_landing_init
}

def _computer_draw(me):
    set_trans_col(8, True)
    map_draw(58, 16, 40, 28, 6, 4, 0x80)

def _computer_lookat(me):
    say_line("it's old \"286\" pc-compatible")

def _computer_use(me):
    computer_use(me)

obj_computer = {
    "name": "computer",
    "state": "state_here",
    "state_here": 1,
    "x": 56,
    "y": 16,
    "z": 1,
    "w": 2,
    "h": 2,
    "use_pos": [63, 44],
    "use_dir": "face_back",
    "played": False,

    "draw": _computer_draw,

    "verbs": {
        "lookat": _computer_lookat,
        "use": _computer_use
    }
}


def computer_use(me):
    """Computer use handler"""
    me["played"] = True
    change_room(rm_mi_title, 1)


obj_cursor = {
    "x": 56,
    "y": 12,
    "w": 1,
    "h": 1,
    "trans_col": 8,
    "state": 1,
    "states": [74, 76],
    "classes": ["class_untouchable"],
    
    "verbs": {}
}

def _floppy_lookat(me):
    floppy_lookat(me)

def _floppy_pickup(me):
    pickup_obj(me)

def _floppy_use(me, noun2):
    floppy_use(me, noun2)

def _floppy_give(me, noun2):
    floppy_give(me, noun2)

obj_floppy_disk = {
    "name": "pico-8",
    "x": 60,
    "y": 44,
    "w": 1,
    "h": 1,
    "state": "state_here",
    "state_here": 189,
    "trans_col": 11,
    "use_with": True,
    "classes": ["class_pickupable"],

    "verbs": {
        "lookat": _floppy_lookat,
        "pickup": _floppy_pickup,
        "use": _floppy_use,
        "give": _floppy_give
    }
}


def floppy_lookat(me):
    """Floppy disk look-at handler"""
    say_line("it's a licenced copy of pico-8:\"a fantasy console for (making, sharing and playing tiny games and other computer programs\":sounds like fun!")


def floppy_use(me, noun2):
    """Floppy disk use handler"""
    if noun2 == obj_computer:
        say_line("there's already a disk inserted")


def floppy_give(me, noun2):
    """Floppy disk give handler"""
    if noun2 == purp_tentacle:
        say_line("do you like programming?")
        say_line_actor(purp_tentacle, "yes, why?", 0, 0)
        say_line("give pico-8 a go,;see what you can make")
        me["owner"] = purp_tentacle
        say_line_actor(purp_tentacle, "this is perfect!", True, 0)
        say_line_actor(purp_tentacle, "thank you!:i shall start making a game right now...", 0, 0)
        stop_script(rm_kitchen["scripts"]["tentacle_guard"])
        walk_to(purp_tentacle, obj_kitchen_door_hall["x"] + 4, obj_kitchen_door_hall["y"] + 30)
        put_at(purp_tentacle, 0, 0, rm_void)
    else:
        say_line("i might need this")


def computer_anim_cursor():
    """Computer room - animate cursor"""
    while True:
        for f in range(1, 3):
            obj_cursor["state"] = f
            break_time(15)


def _computer_enter(me):
    computer_enter(me)

def _computer_exit(me):
    stop_script(me["scripts"]["anim_cursor"])

rm_computer = {
    "map": [64, 16],
    "autodepth_scale": [0.75, 1],

    "objects": [
        obj_computer_door_landing,
        obj_computer,
        obj_cursor,
        obj_floppy_disk
    ],

    "enter": _computer_enter,
    "exit": _computer_exit,
    "scripts": {
        "anim_cursor": computer_anim_cursor
    }
}


def _computer_enter_scene(me):
    reload()
    reset_ui()
    selected_actor.__setitem__("selected_actor", main_actor)
    camera_follow(main_actor)
    do_anim(main_actor, "face_towards", "face_front")
    say_line("well, that was short!:developers are so lazy...")

def computer_enter(me):
    """Computer room enter handler"""
    # Just exited the game?
    start_script(me["scripts"]["anim_cursor"], True)
    if obj_computer["played"]:
        obj_computer["played"] = False
        cutscene(
            3,  # no verbs & no follow
            _computer_enter_scene,
            None
        )


# ============================================================================
# Monkey Island Mini-Game - Title Room
# ============================================================================

def _mi_title_enter(me):
    mi_title_enter(me)

def _mi_title_exit():
    pass  # todo: "anything here?"

rm_mi_title = {
    "map": [72, 0],
    "objects": {},

    "enter": _mi_title_enter,
    "exit": _mi_title_exit
}


def _mi_title_intro(me):
    break_time(50)
    print_line("deep in the; caribbean:on the isle of;...thimbleweed!", 64, 40, 8, 1, True, None, True)
    change_room(rm_mi_dock, 1)

def mi_title_enter(me):
    """Monkey Island title room enter handler"""
    # Load embedded gfx (from sfx area)
    reload(0, 0x3b00, 0x800)

    # Load embedded gfx flags (from sfx area)
    reload(0x3000, 0x3a00, 0x100)

    # Demo intro
    cutscene(
        3,  # no verbs & no follow
        _mi_title_intro,
        None
    )


# ============================================================================
# Monkey Island Mini-Game - Dock Room
# ============================================================================

def _mi_bg_draw(me):
    map_draw(88, 0, 0, 16, 40, 7)

obj_mi_bg = {
    "x": 0,
    "y": 0,
    "w": 1,
    "h": 1,
    "z": -10,
    "classes": ["class_untouchable"],
    "state": "state_here",
    "state_here": 1,

    "draw": _mi_bg_draw
}

def _mi_poster_lookat(me):
    say_line("\"re-elect governor marly\"")

obj_mi_poster = {
    "name": "poster",
    "x": 32,
    "y": 40,
    "w": 1,
    "h": 1,

    "verbs": {
        "lookat": _mi_poster_lookat
    }
}

def _mi_scummdoor_walkto(me):
    mi_scummdoor_walkto(me)

def _mi_scummdoor_open(me):
    open_door(me, obj_front_door_inside)

def _mi_scummdoor_close(me):
    close_door(me, obj_front_door_inside)

obj_mi_scummdoor = {
    "name": "door",
    "state": "state_closed",
    "x": 240,
    "y": 40,
    "w": 1,
    "h": 2,
    "state_closed": 43,
    "state_open": 12,
    "classes": ["class_openable"],
    "use_dir": "face_back",

    "verbs": {
        "walkto": _mi_scummdoor_walkto,
        "open": _mi_scummdoor_open,
        "close": _mi_scummdoor_close
    }
}


def mi_scummdoor_walkto(me):
    """Monkey Island door walk-to handler"""
    if me["state"] == "state_open":
        # Outro
        change_room(rm_computer, 1)
    else:
        say_line("the door is closed")


def _mi_dock_enter(me):
    mi_dock_enter(me)

def _mi_dock_exit(me):
    pass  # todo: "anything here?"

rm_mi_dock = {
    "map": [88, 8, 127, 15],
    "trans_col": 11,

    "objects": [
        obj_mi_bg,
        obj_mi_poster,
        obj_mi_scummdoor
    ],

    "enter": _mi_dock_enter,
    "exit": _mi_dock_exit
}


def mi_dock_enter(me):
    """Monkey Island dock room enter handler"""
    global verb_maincol, verb_hovcol, verb_shadcol, verb_defcol
    global selected_actor
    
    # Set UI colors for MI
    verb_maincol = 11
    verb_hovcol = 10
    verb_shadcol = 1
    verb_defcol = 10
    
    # Set which actor the player controls by default
    selected_actor = mi_actor
    
    # Init actor
    put_at(selected_actor, 212, 60, rm_mi_dock)
    camera_at(0)
    break_time(30)
    camera_pan_to_coord(212, 60)
    wait_for_camera()
    camera_follow(selected_actor)
    say_line("this all seems very famililar...")


# ============================================================================
# The Void Room
# ============================================================================

rm_void = {
    "map": [0, 0],
    
    "objects": [
        obj_key
    ]
}


# ============================================================================
# Actor Definitions
# ============================================================================

# Initialize the player's actor object
def _main_actor_use(me):
    actor_use(me)

main_actor = {
    "name": "humanoid",
    "w": 1,
    "h": 4,
    "idle": [193, 197, 199, 197],
    "talk": [218, 219, 220, 219],
    "walk_anim_side": [196, 197, 198, 197],
    "walk_anim_front": [194, 193, 195, 193],
    "walk_anim_back": [200, 199, 201, 199],
    "col": 12,
    "trans_col": 11,
    "walk_speed": 0.6,
    "frame_delay": 5,
    "classes": ["class_actor"],
    "face_dir": "face_front",

    # Sprites for directions (front, left, back, right)
    "inventory": [
        obj_switch_tent
    ],
    "verbs": {
        "use": _main_actor_use
    },
    "in_room": None,
    "alerting": False,
    "stopped_player": False,
    "asked_where": False,
    "asked_woodchuck": False,
    "asked_why_stop": False
}


def actor_use(me):
    """Actor use handler"""
    global selected_actor
    selected_actor = me
    camera_follow(me)


def _purp_tentacle_lookat():
    say_line("it's a weird looking tentacle, thing!")

def _purp_tentacle_talkto(me):
    purp_tentacle_talkto(me)

def _purp_tentacle_use(me):
    actor_use(me)

purp_tentacle = {
    "name": "purple tentacle",
    "x": 140,
    "y": 52,
    "w": 1,
    "h": 3,
    "idle": [154, 154, 154, 154],
    "talk": [171, 171, 171, 171],
    "col": 11,
    "trans_col": 15,
    "walk_speed": 0.4,
    "frame_delay": 5,
    "classes": ["class_actor", "class_talkable"],
    "face_dir": "face_front",
    "use_pos": "pos_left",

    "in_room": rm_kitchen,
    "inventory": [
        obj_switch_player
    ],
    "verbs": {
        "lookat": _purp_tentacle_lookat,
        "talkto": _purp_tentacle_talkto,
        "use": _purp_tentacle_use
    },
    "alerting": False,
    "stopped_player": False,
    "asked_where": False,
    "asked_woodchuck": False,
    "asked_why_stop": False
}


def _purp_tentacle_talkto_scene(me):
    say_line_actor(me, "what do you want?", 0, 0)

def purp_tentacle_talkto(me):
    """Purple tentacle talk-to handler"""
    cutscene(
        1,  # no verbs
        _purp_tentacle_talkto_scene,
        None
    )
    
    # Dialog loop
    while True:
        # Build dialog options
        dialog_set([
            "" if me["stopped_player"] else "why did you stop me?",
            "" if me["asked_where"] else "where am i?",
            "" if me["asked_woodchuck"] else "how much wood would a wood-chuck chuck, if a wood-chuck could chuck wood?",
            "nevermind"
        ])
        dialog_start(selected_actor["col"], 7)
        
        # Wait for selection
        # Note: selected_sentence would need to be provided by the engine
        while True:
            break_time(None)
        
        # Chosen options
        dialog_hide()
        # Dialog handling would continue here...
        
        dialog_clear()


mi_actor = {
    "name": "guybrush",
    "w": 1,
    "h": 2,
    "idle": [47, 47, 15, 47],
    "walk_anim_side": [44, 45, 44, 46],
    "col": 7,
    "trans_col": 8,
    "walk_speed": 0.5,
    "frame_delay": 8,
    "classes": ["class_actor"],
    "face_dir": "face_front",
    
    # Sprites for directions (front, left, back, right)
    "inventory": {},
    "verbs": {},
    "in_room": None
}


# ============================================================================
# Active Rooms List
# ============================================================================

rooms = [
    rm_void,
    rm_title,
    rm_outside,
    rm_hall,
    rm_kitchen,
    rm_garden,
    rm_library,
    rm_landing,
    rm_computer,
    rm_mi_title,
    rm_mi_dock
]


# ============================================================================
# Active Actors List
# ============================================================================

actors = [
    main_actor,
    purp_tentacle,
    mi_actor
]


# ============================================================================
# Graphics Data
# ============================================================================

__label__ = "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000\n" + \
    "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000\n" * 100  # Truncated for brevity

__gff__ = "0001010101010100000000010000000000010101010101000000000101000000000101010101010101010101000000000001010101010100000000000000000000000000000000000000808000000000000000000000000000008080000000000000000000008080000000008000000000000000000000000000010180000000\n" + \
    "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000\n" + \
    "__map__\n"  # Truncated for brevity


# ============================================================================
# Helper Functions for Switch Objects
# ============================================================================

def use_switch_tent(me):
    """Switch to purple tentacle"""
    global selected_actor
    selected_actor = purp_tentacle
    camera_follow(purp_tentacle)


def use_switch_player(me):
    """Switch to main actor"""
    global selected_actor
    selected_actor = main_actor
    camera_follow(main_actor)


# ============================================================================
# Game Initialization
# ============================================================================

# Run startup when loaded
