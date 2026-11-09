# C8 Part 2 - Python version
# Converted from C8_Part2.esc (Scumm-8 by Paul Nicholls)
# Chapter 8 - Part 2 game content

# ============================================================================
# Global Variables
# ============================================================================

# Debug flags
show_debuginfo = False
show_collision = False
show_pathfinding = True
show_perfinfo = False
enable_mouse = True
enable_diag_squeeze = True  # Allow squeeze through diag gap?

# ============================================================================
# Object Definitions
# ============================================================================

obj_alien_hand = {
    "name": "alien hand",
    "state": "state_here",
    "x": 46,
    "y": 52,
    "z": 1,
    "w": 1,
    "h": 1,
    "state_here": 62,
    "use_pos": "pos_infront",
    "use_dir": "face_back",
    "use_with": True,

    "verbs": {
        "lookat": lambda me: say_line("it looks like a severed alien hand...:ugh!"),
        "pickup": lambda me: (pickup_obj(me), dset(27, 1)),
        "use": lambda me, noun2: alien_hand_use(me, noun2)
    }
}


def alien_hand_use(me, noun2):
    """Alien hand use handler - opens cell door"""
    if noun2 == obj_lock:
        music(1)
        cutscene(
            1,  # no verbs
            # Cutscene code (hides ui, etc.)
            lambda: (
                put_at(me, 0, 0, rm_void),
                put_at(obj_alieninside_celldoor, 0, 0, rm_void),
                main_actor.update({"released_ben": True}),
                dset(28, 1),
                # walk_to(ben_actor, 196, 50)
                # wait_for_actor(ben_actor)
                say_line_actor(ben_actor, "thank you for rescuing me:now...:let's get off this rock!"),
                fades(1, 1),  # Fade out
                print_line("congratulations!:you've completed the game:thanks for playing", 210, 50, 7, 1, True),
                # End game loop
                while_loop_end()
                # load("_game-intro")
            )
        )


def while_loop_end():
    """Infinite loop to end game"""
    while True:
        break_time(10)


obj_boulder = {
    "name": "boulder",
    "state": "state_here",
    "x": 51,
    "y": 26,
    "z": 10,
    "w": 1,
    "h": 1,
    "state_here": 46,
    "trans_col": 3,
    "use_pos": "pos_infront",
    "use_dir": "face_back",
    "use_with": True,

    "verbs": {
        "lookat": lambda me: say_line("it looks heavy...;and indestructible"),
        "pickup": lambda me: (pickup_obj(me), dset(29, 1)),
        "use": lambda me, noun2: boulder_use(me, noun2)
    }
}


def boulder_use(me, noun2):
    """Boulder use handler - blocks signal"""
    if noun2 == obj_signal:
        cutscene(
            1,  # no verbs
            # Cutscene code (hides ui, etc.)
            lambda: boulder_use_inner(me)
        )


def boulder_use_inner(me):
    """Boulder use - inner function"""
    say_line("*hmmpf*")
    put_at(obj_boulder, 92, 31, rm_signal)
    say_line("yes! it's blocking the alien energy beam")
    break_time(150)
    shake(True)
    say_line("uh-oh...:the blockage is building up energy:i think it's gonna blow!")
    walk_to(selected_actor, 90, 62)
    # come_out_door(obj_signal_door_map, obj_map_signal, 1)
    alien_actor["flip_x"] = True
    camera_follow(ben_actor)
    shake(False)
    break_time(1)
    say_line_actor(alien_actor, "oh no!:quick everyone, get to the energy beam!")
    main_actor["disabled_signal"] = True
    dset(22, 1)


# [ Map "Room" (Disk 2) ]
obj_map_door_crash = {
    "name": "my spaceship",
    "state": "state_open",
    "x": 50,
    "y": 40,
    "z": 1,
    "w": 1,
    "h": 1,
    "classes": ["class_door"],
    "use_pos": "pos_left",
    "use_dir": "face_right",

    "verbs": {
        "walkto": lambda me: (
            # printh("load disk 1!!!")
            # Set flag for entered/exited door
            dset(10, 2),  # Crash
            load("_game-pt1")
        )
    }
}

obj_map_door_graves = {
    "name": "graves",
    "state": "state_open",
    "x": 26,
    "y": -2,
    "z": 1,
    "w": 2,
    "h": 2,
    "classes": ["class_door"],
    "use_pos": [29, 12],
    "use_dir": "face_back",

    "verbs": {
        "walkto": lambda me: (
            # printh("load disk 1!!!")
            # Set flag for entered/exited door
            dset(10, 3),  # Graves
            load("_game-pt1")
        )
    }
}

obj_map_door_cave = {
    "name": "crystal cave",
    "state": "state_open",
    "x": 3,
    "y": 52,
    "z": 1,
    "w": 2,
    "h": 1,
    "classes": ["class_door"],
    "use_pos": [15, 60],
    "use_dir": "face_back",

    "verbs": {
        "walkto": lambda me: (
            # printh("load disk 1!!!")
            # Set flag for entered/exited door
            dset(10, 7),  # Cave
            load("_game-pt1")
        )
    }
}

obj_map_signal = {
    "name": "signal generator",
    "state": "state_open",
    "x": 104,
    "y": 9,
    "z": 1,
    "w": 2,
    "h": 1,
    "classes": ["class_door"],
    "use_pos": [111, 18],
    "use_dir": "face_back",

    "draw": lambda me: (
        line(112, 9 + 16, 112, 0 + 16, 8 if flr(rnd(2)) == 0 else 9)
        if not main_actor.get("disabled_signal") else None
    ),

    "init": lambda me: me.update({"target_door": obj_signal_door_map}),

    "verbs": {
        "walkto": lambda me: signal_walkto(me)
    }
}


def signal_walkto(me):
    """Signal generator walk-to handler"""
    if not main_actor.get("disabled_signal"):
        come_out_door(me, obj_signal_door_map)
    else:
        say_line("i'd better not, it's crawling with aliens!")


obj_map_alienbase = {
    "name": "alien base",
    "state": "state_open",
    "x": 106,
    "y": 26,
    "z": 1,
    "w": 3,
    "h": 3,
    "classes": ["class_door"],
    "use_pos": [108, 34],
    "use_dir": "face_back",

    "init": lambda me: me.update({"target_door": obj_alienbase_door_map})
}

rm_map = {
    "map": [32, 8, 47, 15],

    "objects": [
        obj_map_door_crash,
        obj_map_door_graves,
        obj_map_door_cave,
        # obj_map_bridge,
        obj_map_signal,
        obj_map_alienbase,
    ],

    "enter": lambda me: map_enter(me),
    "exit": lambda me: map_exit(me),
}


def map_enter(me):
    """Map room enter handler"""
    # Switch gfx
    load_gfx_page(5)
    selected_actor["scale"] = 0.2
    selected_actor["walk_speed"] = 2


def map_exit(me):
    """Map room exit handler"""
    selected_actor["scale"] = None
    selected_actor["walk_speed"] = 0.6


# [ Signal Generator ]
obj_signal_door_map = {
    "name": "path",
    "state": "state_open",
    "x": 60,
    "y": 58,
    "z": 1,
    "w": 8,
    "h": 1,
    "classes": ["class_door"],
    "use_pos": "pos_center",
    "use_dir": "face_front",

    "init": lambda me: me.update({"target_door": obj_map_signal})
}

obj_signal = {
    "name": "energy beam",
    "x": 91,
    "y": 21,
    "w": 1,
    "h": 3,
    "use_dir": "face_back",

    "draw": lambda me: signal_draw(me),

    "verbs": {
        "lookat": lambda me: say_line("the alien energy beam:it's firing into the atmosphere from below:this is what caused our ships to crash...:i must destroy it!")
    }
}


def signal_draw(me):
    """Signal draw handler"""
    if obj_boulder.get("in_room") != rm_signal:
        rectfill(94, 36 + 16, 96, 0 + 16, 8 if flr(rnd(2)) == 0 else 9)
        line(95, 36 + 16, 95, 0 + 16, 8 if flr(rnd(2)) == 0 else 9)


rm_signal = {
    "map": [32, 8, 47, 15],
    "autoscale_zoom": 0.75,

    "objects": [
        obj_signal_door_map,
        obj_signal,
    ],

    "enter": lambda me: load_gfx_page(2),
    "exit": lambda me: None,  # todo: "anything here?"
}


# [ Outside Alien Base ]
obj_alienbase_door_map = {
    "name": "path",
    "state": "state_open",
    "x": 135,
    "y": 8,
    "z": 1,
    "w": 1,
    "h": 2,
    "classes": ["class_door"],
    "use_pos": "pos_right",
    "use_dir": "face_left",

    "init": lambda me: me.update({"target_door": obj_map_alienbase})
}

obj_alienbase_door_inside = {
    "name": "heavy door",
    "state": "state_closed",
    "x": 8,
    "y": 16,
    "z": 1,
    "w": 2,
    "h": 2,
    "state_closed": 206,
    "use_pos": [18, 32],
    "use_dir": "face_back",

    "verbs": {
        "walkto": lambda me: alienbase_door_walkto(me)
    }
}


def alienbase_door_walkto(me):
    """Alien base door walk-to handler"""
    if me["state"] == "state_open":
        come_out_door(me, obj_alieninside_door_alienbase)
    else:
        say_line("the door is closed")


rm_alien_base_outside = {
    "map": [0, 24, 17, 31],
    "autoscale_zoom": 0.75,

    "objects": [
        obj_alienbase_door_map,
        obj_alienbase_door_inside,
    ],

    "enter": lambda me: alien_base_outside_enter(me),
    "exit": lambda me: None,  # todo: "anything here?"
}


def alien_base_outside_enter(me):
    """Alien base outside enter handler"""
    # Switch gfx
    load_gfx_page(3)

    # Auto-open door?
    if main_actor.get("disabled_signal"):
        obj_alienbase_door_inside["state"] = "state_open"
        obj_alienbase_door_inside["name"] = "alien base"

    # music(6)


# [ Inside Alien Base ]
obj_alieninside_door_alienbase = {
    "name": "outside",
    "state": "state_open",
    "x": 16,
    "y": 24,
    "z": 1,
    "w": 3,
    "h": 3,
    "classes": ["class_door"],
    "use_pos": "pos_infront",
    "use_dir": "face_back",

    "init": lambda me: me.update({"target_door": obj_alienbase_door_inside})
}

obj_alieninside_celldoor = {
    "name": "cell door",
    "state": "state_closed",
    "x": 200,
    "y": 16,
    "z": 1,
    "w": 2,
    "h": 4,
    "state_closed": 206,
    "use_pos": "pos_right",
    "use_dir": "face_back",

    "init": lambda me: me.update({"target_door": obj_alienbase_door_inside})
}

obj_alien_draw = {
    "name": "",
    "state": "state_open",
    "x": 176,
    "y": 36,
    "z": 1,
    "w": 2,
    "h": 4,
    "use_with": True,
    "classes": ["class_pickupable"],
    "use_pos": "pos_infront",
    "use_dir": "face_back",

    "draw": lambda me: alien_draw_fn(me)
}


def alien_draw_fn(me):
    """Alien draw handler"""
    set_trans_col(3, True)
    xoff = 0
    if alien_actor.get("flip_x"):
        xoff = 12

    if flr(time()) % 2 == 0:
        spr(6, me["x"] + xoff, me["y"], 2, 4, alien_actor.get("flip_x"))
    else:
        spr(8, me["x"] + xoff, me["y"], 2, 4, alien_actor.get("flip_x"))


obj_lock = {
    "name": "alien lock",
    "state": "state_here",
    "x": 252,
    "y": 30,
    "w": 1,
    "h": 1,
    "state_here": 162,
    "use_pos": "pos_infront",
    "use_dir": "face_back",
    "trans_col": 1,

    "verbs": {
        "lookat": lambda me: say_line("it looks biometric:only an alien can open it"),
    }
}

rm_alien_base_inside = {
    "map": [24, 24, 57, 31],
    "min_autoscale": 1,

    "objects": [
        obj_alieninside_door_alienbase,
        obj_alieninside_celldoor,
        obj_alien_draw,
        obj_lock,
    ],

    "enter": lambda me: alien_base_inside_enter(me),
    "exit": lambda me: None,
    # Pause fireplace while not in room
    # stop_script(me["scripts"]["anim_alien"])

    "scripts": {}
}


def alien_base_inside_enter(me):
    """Alien base inside enter handler"""
    # Switch gfx
    # printh("in rm_alien_base_inside...")
    load_gfx_page(4)

    if selected_actor.get("disabled_signal"):
        put_at(obj_alien_draw, 0, 0, rm_void)
        put_at(alien_actor, 0, 0, rm_void)

    music(6)
    # Make alien color-effect
    # start_script(me["scripts"]["anim_alien"], True)  # BG script


# ============================================================================
# Void Room
# ============================================================================

obj_lasertool = {
    "name": "laser tool",
    "state": "state_open",
    "x": 0,
    "y": 0,
    "w": 1,
    "h": 1,
    "state_open": 47,
    "trans_col": 0,
    "use_with": True,
    "classes": ["class_pickupable"],

    "verbs": {
        "lookat": lambda me: say_line("it's my trusty laser tool:cuts through anything!"),
        # "pickup": lambda me: pickup_obj(me),
        "use": lambda me, noun2: None  # Disabled in part 2
        # if noun2 == obj_signal:
        #     main_actor["disabled_signal"] = True
        #     dset(22, 1)
    }
}

rm_void = {
    "map": [0, 0],

    "objects": [
        obj_lasertool,
        obj_alien_hand,
        obj_boulder,
    ]
}


# ============================================================================
# Room Lists
# ============================================================================

rooms = [
    rm_void,
    # rm_bridge,
    rm_map,
    rm_signal,
    rm_alien_base_outside,
    rm_alien_base_inside,
]


# ============================================================================
# Actor Definitions
# ============================================================================

# Initialize the player's actor object
main_actor = {
    "name": "humanoid",
    "w": 1,
    "h": 4,
    "idle": [65, 69, 71, 69],
    "talk": [90, 91, 92, 91],
    "walk_anim_side": [68, 69, 70, 69],
    "walk_anim_front": [66, 65, 67, 65],
    "walk_anim_back": [72, 71, 73, 71],
    "col": 7,
    "trans_col": 11,
    "walk_speed": 0.6,
    "frame_delay": 5,
    "classes": ["class_actor"],
    "face_dir": "face_front",

    # Sprites for directions (front, left, back, right) - note: "right=left-flipped"
    "inventory": [
        # obj_switch_tent
    ],

    "verbs": {
        "use": lambda me: (
            selected_actor.__setitem__("value", me),
            camera_follow(me)
        )
    }
}

alien_actor = {
    "name": "alien",
    "x": 192,
    "y": 52,
    "w": 2,
    "h": 4,
    "idle": [3, 3, 3, 3],
    "talk": [28, 28, 28, 28],
    "col": 8,
    "trans_col": 3,
    "walk_speed": 0.4,
    "frame_delay": 5,
    "classes": ["class_actor", "class_talkable"],
    "face_dir": "face_front",
    "use_pos": "pos_left",

    "in_room": rm_alien_base_inside,

    "inventory": [],
}

ben_actor = {
    "name": "ben",
    "x": 224,
    "y": 47,
    "z": -2,
    "w": 1,
    "h": 4,
    "idle": [1, 1, 1, 1],
    "talk": [18, 18, 18, 18],
    "col": 12,
    "trans_col": 3,
    "walk_speed": 0.4,
    "frame_delay": 5,
    "classes": ["class_actor", "class_talkable"],
    "face_dir": "face_front",
    "use_pos": "pos_left",

    "in_room": rm_alien_base_inside,

    "inventory": [],

    "verbs": {
        "talkto": lambda me: ben_talkto(me),
        "lookat": lambda me: say_line("it's captain ben octavi:he's alive!")
    }  # verbs
}


def ben_talkto(me):
    """Ben talk-to handler - dialog loop"""
    # Dialog loop start
    while True:
        # Build dialog options
        dialog_set([
            "are you captain ben octavi?" if me.get("asked_who") != 0 else "",
            "i am a space trader:i received your distress message" if me.get("asked_who") != 1 else "",
            "i'm luke skywalker;i'm here to rescue you" if me.get("asked_who") != 1 else "",
            "i am guybrush threepwood, mighty pirate" if me.get("asked_who") != 1 else "",
            "do you know the how to unlock this cell?" if (me.get("asked_escape") or me.get("asked_who") == 1) else "",
            "i'll be right back"
        ])
        dialog_start(selected_actor["col"], 7)

        # Wait for selection
        while not selected_sentence:
            break_time()

        # Chosen options
        dialog_hide()

        cutscene(
            1,  # no verbs
            lambda: ben_dialog_handler(me)
        )

        dialog_clear()


def ben_dialog_handler(me):
    """Handle ben dialog selection"""
    say_line(selected_sentence["msg"])

    if selected_sentence["num"] == 1:
        say_line_actor(me, "yes i am, who are you?")
        me["asked_who"] = 1
        # Who
    elif selected_sentence["num"] == 2:
        say_line_actor(me, "well, i'm so glad to see you!")
        me["asked_who"] = 2
    elif selected_sentence["num"] == 3:
        say_line_actor(me, "you're who?!?")
        me["asked_who"] = 2
    elif selected_sentence["num"] == 4:
        say_line_actor(me, "guybrush threepwood?:that's the most ridiculous name i've ever heard!")
        me["asked_who"] = 2
    elif selected_sentence["num"] == 5:
        say_line_actor(me, "i think the alien pressed its hand against the sensor other there")
        me["asked_who"] = 2
    elif selected_sentence["num"] == 6:
        say_line_actor(me, "please hurry")
        dialog_end()
        return


# ============================================================================
# Active Actors List
# ============================================================================

actors = [
    main_actor,
    ben_actor,
    alien_actor,
]


# ============================================================================
# Startup Script
# ============================================================================

def startup_script():
    """Initial game setup - Chapter 8 Part 2"""
    global selected_actor

    cartdata("pn_code8")

    # for d in range(10, 30):
    #     printh("dget("..d..")="..dget(d))

    # Permanent inventory?
    pickup_obj(obj_lasertool, main_actor)

    music(6)

    # Game states
    if dget(20) == 1:
        main_actor["fixed_ship"] = True

    main_actor["ben_cutscene"] = dget(21)

    if dget(22) == 1:
        main_actor["disabled_signal"] = True

    if dget(23) == 1:
        main_actor["engine_cover_replaced"] = True  # Not same as fixed ship, just cover!

    if dget(24) == 1:
        main_actor["crystal_replaced"] = True

    # Ben released + alien hand states
    if dget(28) == 1:
        main_actor["released_ben"] = True
        put_at(obj_alien_hand, 0, 0, rm_void)

    if dget(27) == 1 and not main_actor.get("released_ben"):
        pickup_obj(obj_alien_hand, main_actor)

    if dget(29) == 1 and not main_actor.get("disabled_signal"):
        pickup_obj(obj_boulder, main_actor)

    # Still controlling player
    selected_actor = main_actor
    camera_follow(selected_actor)

    # Reset talking state
    ben_actor["asked_who"] = 0

    # Check starting room/door (if came via map)
    sourcedoor = None
    targetdoor = None

    if dget(10) == 2:
        sourcedoor = obj_map_signal
        targetdoor = obj_signal_door_map
    elif dget(10) == 3:
        sourcedoor = obj_map_alienbase
        targetdoor = obj_alienbase_door_map
    else:
        sourcedoor = obj_map_alienbase
        targetdoor = obj_map_alienbase

    # Check for ben cutscenes
    ##############################-
    # First cutscene
    ##############################-
    if main_actor.get("ben_cutscene") == 0.5:
        # printh("doing cutscene 1!")
        # Don't do this again
        main_actor["ben_cutscene"] = 1
        dset(21, 1)
        change_room(rm_alien_base_inside, 1)

        # Do cutscene
        cutscene(
            1,  # no verbs
            # Cutscene code (hides ui, etc.)
            lambda: (
                # camera_at(0)
                # camera_pan_to(ben_actor)
                # wait_for_camera()
                camera_at(ben_actor),
                # say_line(ben_actor, "who are you?:what do you want from me?")
                # say_line(alien_actor, "captain ben,:give us the location of the human base:...or it will hurt!")
                say_line_actor(alien_actor, "give us the location of the human base:or we'll just crash more ships here...:until we find someone who will!"),
                say_line_actor(ben_actor, "wait? you are the reason my ship crashed?:i lost two good men in that crash!"),
                say_line_actor(alien_actor, "we care not:our energy beam is getting even stronger:now, tell us what we want to know!"),
                fades(1, 1),  # Fade out
                dset(10, 1),
                load("_game-pt1")
            )
        )
    else:
        come_out_door(sourcedoor, targetdoor)
    # if not cutscene
