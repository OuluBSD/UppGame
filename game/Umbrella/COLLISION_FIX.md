# Collision System Fixes

## Problems Fixed

### 1. Wall-to-Wall Edge Treated as Floor
**Problem**: Player was landing on edges between wall blocks even when there was no air space above.

**Solution**: Added `IsValidFloor()` helper function in Player.cpp:
```cpp
static bool IsValidFloor(int col, int row, CollisionHandler& collision) {
    if(!collision.IsFloorTile(col, row)) {
        return false;
    }

    // Check if there's air above this floor tile
    int rowAbove = row + 1;
    if(!collision.IsFullBlockTile(col, rowAbove) && !collision.IsWallTile(col, rowAbove)) {
        return true;
    }

    return false; // Wall-to-wall edge
}
```

This ensures a floor tile is only valid for landing if there's air or non-solid space above it.

### 2. Vertical Jitter/Shaking
**Problem**: Player position oscillated vertically when standing on floor because:
1. Movement was applied first (player falls below floor)
2. Then collision detected player is below floor
3. Player corrected back up
4. Next frame, repeat

**Solution**: Refactored `ResolveCollisionY()` to:
1. Check collision BEFORE applying movement
2. If collision would occur, snap exactly to tile boundary
3. Never apply movement that penetrates a surface

Key changes:
- Calculate test position: `testFeetY = currentFeetY + step`
- Check if test position would cross floor boundary
- If yes, snap to boundary: `snapDelta = tileTopY - currentFeetY`
- Apply snap delta instead of original step

This eliminates the overshoot-correct cycle.

## Build Status

**Current Issue**: Upstream CtrlLib has compilation errors unrelated to these fixes. The Player.cpp changes compile successfully when checked in isolation.

Errors in CtrlLib/Bar.h, CtrlLib/DropChoice.h prevent full build.

## Testing Needed

Once upstream CtrlLib is fixed:
1. Player should land smoothly on floors without vertical jitter
2. Player should NOT be able to land on wall-to-wall edges
3. Player should fall through wall sections that have no floor
4. Jumping alongside walls should work without getting stuck
