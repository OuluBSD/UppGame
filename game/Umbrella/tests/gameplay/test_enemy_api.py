# Test enemy API functions

print("Testing enemy API...")

# Query enemy count
count = get_enemy_count()
print("Enemy count:", count)

# Get all enemies
if count > 0:
    print("\nQuerying all enemies...")
    enemies = get_all_enemies()
    print("Got", len(enemies), "enemies")

    # Show first enemy details
    if len(enemies) > 0:
        print("\nFirst enemy details:")
        enemy = enemies[0]
        print("  Position:", enemy["position"])
        print("  Type:", enemy["type"])
        print("  Alive:", enemy["alive"])
        print("  Active:", enemy["active"])
        print("  Captured:", enemy["captured"])
        print("  Thrown:", enemy["thrown"])
        print("  Facing:", enemy["facing"])

    # Query individual enemy
    print("\nQuerying enemy 0 directly...")
    enemy0 = get_enemy(0)
    print("  Type:", enemy0["type"])
    print("  Position:", enemy0["position"])

    # Test manipulation - kill first enemy
    if count > 0:
        print("\nKilling enemy 0...")
        kill_enemy(0)
        enemy0_after = get_enemy(0)
        print("  Alive after kill:", enemy0_after["alive"])
        if not enemy0_after["alive"]:
            print("  ✓ Enemy killed successfully")
        else:
            print("  ✗ Enemy not killed")

    # Test clear_enemies
    print("\nClearing all enemies...")
    clear_enemies()
    new_count = get_enemy_count()
    print("Enemy count after clear:", new_count)
    if new_count == 0:
        print("✓ All enemies cleared successfully")
    else:
        print("✗ Enemies not cleared -", new_count, "remaining")
else:
    print("No enemies spawned in level - skipping detailed tests")

print("\nAll enemy API tests completed!")
