# Demo Game Content - Python version
# Converted from Demo.esc (Scumm-8 by Paul Nicholls)

# ============================================================================
# Global Variables
# ============================================================================

# Game flags
enable_diag_squeeze = False

# Game verbs (used in room definitions and UI)
verbs = []

# Index of the verb to use when clicking items in inventory (e.g. look-at)
verb_default = 4

# Default cursor sprites
ui_cursorspr = 224
ui_uparrowspr = 208
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

    # Set initial inventory (if applicable)
    # pickup_obj(obj_bucket, main_actor)

    # Set which room to start the game in
    # (e.g. could be a "pseudo" room for title screen!)
    change_room(rm_hall, 1)  # iris fade

    # Set actor to start in different room
    # (by default, this is being done in room's startup script)
    # selected_actor = main_actor
    # put_at(selected_actor, 51, 41, rm_library)
    # camera_follow(selected_actor)
    # change_room(rm_library, 1)  # iris fade


# ============================================================================
# Hall Room Objects
# ============================================================================

obj_front_door_inside = {
    "name": "front door",
    "state": "state_closed",
    "x": 8,
    "y": 16,
    "z": 1,
    "w": 1,
    "h": 4,
    "state_closed": 79,
    "classes": ["class_door", "class_openable"],
    "use_pos": "pos_right",
    "use_dir": "face_left",

    "init": lambda me: me.update({"target_door": obj_front_door})
}

obj_hall_door_library = {
    "name": "library",
    "state": "state_open",
    "x": 56,
    "y": 16,
    "w": 1,
    "h": 3,
    "flip_x": True,
    "state_closed": 78,
    "use_dir": "face_back",
    "classes": ["class_door", "class_openable"],

    "init": lambda me: me.update({"target_door": obj_library_door_hall})
}

obj_hall_door_kitchen = {
    "name": "kitchen",
    "state": "state_open",
    "x": 176,
    "y": 16,
    "w": 1,
    "h": 4,
    "use_pos": "pos_left",
    "use_dir": "face_right",
    "classes": ["class_door"],

    "init": lambda me: me.update({"target_door": obj_kitchen_door_hall})
}

obj_bucket = {
    "name": "full bucket",
    "state": "state_closed",
    "x": 142,
    "y": 48,
    "w": 1,
    "h": 1,
    "state_closed": 143,
    "state_open": 159,
    "trans_col": 15,
    "use_with": True,
    "classes": ["class_pickupable"],

    "verbs": {
        "lookat": lambda me: say_line("it's an old bucket"),
        "pickup": lambda me: pickup_obj(me),
        "use": lambda me, noun2: bucket_use(me, noun2)
    }
}


def bucket_use(me, noun2):
    """Bucket use verb handler"""
    if noun2 == "obj_fire" and me["state"] == "state_closed":
        put_at("obj_fire", 0, 0, rm_void)
        # put_at(obj_key, 88, 32, rm_library)
        me["state"] = "state_open"
        say_line("the fire's out now")


obj_spinning_top = {
    "name": "spinning top",
    "x": 36,
    "y": 37,
    "w": 1,
    "h": 1,
    "state": "state_idle",
    "state_idle": 158,
    "anim_spin": [158, 174, 190],
    "frame_delay": 4,
    "col_replace": [12, 7],
    "trans_col": 15,
    "use_dir": "face_front",

    "verbs": {
        "lookat": lambda me: spinning_top_lookat(me),
        "use": lambda me: spinning_top_use(me)
    }
}


def spinning_top_lookat(me):
    """Spinning top look-at handler - shows example dialog"""
    # Do cutscene
    cutscene(
        1,  # no verbs
        # Cutscene code (hides ui, etc.)
        lambda: (
            say_line("this is some example dialog"),
            break_time(20),
            say_line("with some pauses..."),
            break_time(20),
            say_line("you can try skipping next time!")
        ),
        # Override for cutscene
        lambda: stop_talking()
    )


def spinning_top_use(me):
    """Spinning top use handler - starts/stops spin script"""
    if script_running(me["scripts"]["spin_top"]):
        stop_script(me["scripts"]["spin_top"])
        me["curr_anim"] = None      # Stop cycle anim
        me["state"] = "state_idle"  # Go to initial state/frame
    else:
        start_script(me["scripts"]["spin_top"], True)  # BG script, continues executing on room change


def hall_spin_top():
    """Hall room - spinning top script"""
    dir = -1
    while True:
        for x in range(3):
            # Move top
            obj_spinning_top["x"] = obj_spinning_top["x"] - dir
            break_time(12)
        dir = dir * -1


rm_hall = {
    "map": [32, 24, 55, 31],

    "objects": [
        obj_front_door_inside,
        obj_hall_door_library,
        obj_hall_door_kitchen,
        obj_spinning_top,
        obj_bucket
    ],

    "enter": lambda me: hall_enter(me),
    "exit": lambda me: None,  # todo: anything here?

    "scripts": {  # Scripts that are at room-level
        "spin_top": hall_spin_top
    }
}


def hall_enter(me):
    """Hall room enter handler"""
    global selected_actor
    # Note: this will work for first enter, but when using doors
    #       to enter room, door position will override put_at()
    selected_actor = "main_actor"
    put_at(selected_actor, 30, 55, rm_hall)
    camera_follow(selected_actor)


# ============================================================================
# Library Room Objects
# ============================================================================

obj_library_door_hall = {
    "name": "hall",
    "state": "state_open",
    "x": 136,
    "y": 16,
    "w": 1,
    "h": 3,
    "state_closed": 78,
    "use_dir": "face_back",
    "classes": ["class_door", "class_openable"],

    "init": lambda me: me.update({"target_door": obj_hall_door_library})
}

obj_fire = {
    "name": "fire",
    "x": 88,
    "y": 32,
    "w": 1,
    "h": 1,
    "state": "state_here",
    "state_here": 81,
    "anim_fire": [81, 82, 83],
    "frame_delay": 8,
    "use_pos": [97, 42],
    "lighting": 1,

    "init": lambda me: do_anim(me, me["anim_fire"]),

    "verbs": {
        "lookat": lambda: fire_lookat(),
        "talkto": lambda: fire_talkto(),
        "pickup": lambda me: pickup_obj(me)
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


def library_anim_fire():
    """Library room - animate fireplace"""
    while True:
        for f in obj_fire["anim_fire"]:
            obj_fire["state"] = f
            break_time(obj_fire["frame_delay"])


rm_library = {
    "map": [56, 24, 79, 31],
    "col_replace": [7, 4],

    "objects": [
        obj_library_door_hall,
        obj_fire
    ],

    "enter": lambda me: start_script(library_anim_fire, True),
    "exit": lambda me: stop_script(library_anim_fire),
    # Note: we don't need to pause fireplace as not using a script
    #       (it's an anim, so if not visible, will not animate)

    "scripts": {
        "anim_fire": library_anim_fire
    }
}


# ============================================================================
# Void Room
# ============================================================================

# "The void" (room)
# A place to put objects/actors when not in any visible room

rm_void = {
    "map": [0, 0],

    "objects": []
}


# ============================================================================
# Actor Definitions
# ============================================================================

# Initialize the player's actor object
main_actor = {
    # Sprite/anim order for directions = front, left, back, right)
    # (note: right = left value...flipped!)
    "name": "humanoid",
    "w": 1,
    "h": 4,
    "idle": [193, 197, 199, 197],
    "talk": [218, 219, 220, 219, 0, 8, 1, 1],
    "walk_anim_side": [196, 197, 198, 197],
    "walk_anim_front": [194, 193, 195, 193],
    "walk_anim_back": [200, 199, 201, 199],
    "col": 12,
    "trans_col": 11,
    "walk_speed": 0.5,
    "frame_delay": 5,
    "classes": ["class_actor"],
    "face_dir": "face_front"
}

purp_tentacle = {
    "name": "purple tentacle",
    "x": 88,
    "y": 51,
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

    "in_room": rm_hall,

    "verbs": {
        "lookat": lambda: say_line("it's a weird looking tentacle, thing!"),
        "talkto": lambda me: purp_tentacle_talkto(me)
    }
}


def purp_tentacle_talkto(me):
    """Purple tentacle talk-to handler - dialog loop"""
    cutscene(
        1,  # no verbs
        lambda: (
            # do_anim(purp_tentacle, "face_towards", selected_actor)
            say_line_actor(me, "what do you want?")
        )
    )

    # Dialog loop start
    while True:
        # Build dialog options
        dialog_set([
            "where am i?" if not me.get("asked_where") else "",
            # "who are you?",
            "how much wood would a wood-chuck chuck, if a wood-chuck could chuck wood?" if not me.get("asked_woodchuck") else "",
            "nevermind"
        ])
        dialog_start(selected_actor["col"], 7)

        # Wait for selection
        while not selected_sentence:
            break_time()

        # Chosen options
        dialog_hide()

        cutscene(
            1,  # no verbs
            lambda: dialog_handler(me)
        )

        dialog_clear()


def dialog_handler(me):
    """Handle dialog selection"""
    say_line(selected_sentence["msg"])

    if selected_sentence["num"] == 1:
        say_line_actor(me, "you are in a demo scumm-8 game, i think!")
        me["asked_where"] = True
    elif selected_sentence["num"] == 2:
        say_line_actor(me, "a wood-chuck would chuck no amount of wood, coz a wood-chuck can't chuck wood!")
        me["asked_woodchuck"] = True
    elif selected_sentence["num"] == 3:
        say_line_actor(me, "ok bye!")
        dialog_end()
        return


# ============================================================================
# Room Lists
# ============================================================================

rooms = [
    rm_void,
    rm_hall,
    rm_library
]

actors = [
    main_actor,
    purp_tentacle
]
