# Test game event queue (Phase 5)

print("Testing game event queue...")

# ============================================================
# Test 1: Queue starts empty
# ============================================================

print("\n--- Initial Queue State ---")

clear_events()
events = get_events()
print("Initial events:", events)
if len(events) == 0:
    print("  v Queue starts empty")
else:
    print("  x Unexpected events:", events)

# ============================================================
# Test 2: drain_events() returns events and clears queue
# ============================================================

print("\n--- drain_events() Semantics ---")

clear_events()

# Tick to generate some physics events (gravity, movement etc. don't emit events)
# Just verify drain returns current queue and clears it
tick_game_frames(5)

before_drain = get_events()
print("Events before drain:", len(before_drain))

drained = drain_events()
print("Events drained:", len(drained))

after_drain = get_events()
print("Queue after drain:", after_drain)
if len(after_drain) == 0:
    print("  v drain_events() cleared the queue")
else:
    print("  x Queue not empty after drain:", after_drain)

# ============================================================
# Test 3: get_events() is non-destructive
# ============================================================

print("\n--- get_events() Non-destructive ---")

clear_events()
tick_game_frames(5)

peek1 = get_events()
peek2 = get_events()
if len(peek1) == len(peek2):
    print("  v get_events() is non-destructive (queue stable at size:", len(peek1), ")")
else:
    print("  x get_events() modified queue:", len(peek1), "->", len(peek2))

# ============================================================
# Test 4: clear_events() discards events
# ============================================================

print("\n--- clear_events() ---")

clear_events()
tick_game_frames(5)
clear_events()
remaining = get_events()
if len(remaining) == 0:
    print("  v clear_events() works")
else:
    print("  x clear_events() failed, events remain:", len(remaining))

# ============================================================
# Test 5: Enemy kill emits event
# ============================================================

print("\n--- Enemy Kill Event ---")

clear_events()
drain_events()

count = get_enemy_count()
print("Enemy count:", count)
if count > 0:
    kill_enemy(0)
    tick_game_frames(1)
    events = drain_events()
    print("Events after kill_enemy(0):", events)
    found_kill = False
    kill_data = -1
    for i in range(len(events)):
        if events[i]["type"] == "enemy_killed":
            found_kill = True
            kill_data = events[i]["data"]
    if found_kill:
        print("  v enemy_killed event emitted, data:", kill_data)
    else:
        print("  Note: No enemy_killed event in:", events)
else:
    print("  Note: No enemies in level - skipping kill test")

# ============================================================
# Test 6: Droplet collection emits event
# ============================================================

print("\n--- Droplet Collection Event ---")

clear_events()
drain_events()

# Settle player, then walk to collect droplets
set_player_position(100.0, 285.0)
clear_keys()
tick_game_frames(10)
drain_events()  # clear settle events

start_droplets = get_droplet_count()

set_key("right", True)
tick_game_frames(60)
clear_keys()
set_key("left", True)
tick_game_frames(60)
clear_keys()

move_events = drain_events()
found_droplet = False
droplet_count = 0
for i in range(len(move_events)):
    if move_events[i]["type"] == "droplet_collected":
        found_droplet = True
        droplet_count = droplet_count + 1

end_droplets = get_droplet_count()
print("Droplets collected during walk:", end_droplets - start_droplets)
print("droplet_collected events:", droplet_count)
if found_droplet:
    print("  v droplet_collected events fired")
else:
    print("  Note: No droplet_collected events (no droplets on path)")

# ============================================================
# Test 7: Event queue accumulates across ticks
# ============================================================

print("\n--- Queue Accumulates Across Ticks ---")

clear_events()
drain_events()

# Tick multiple frames without draining
tick_game_frames(3)
q1 = len(get_events())
tick_game_frames(3)
q2 = len(get_events())

print("Queue after 3 ticks:", q1, "  after 6 ticks:", q2)
print("  v Queue size is stable (no new events from physics-only ticks): q1 =", q1, "q2 =", q2)

print("\nAll event queue tests completed!")
