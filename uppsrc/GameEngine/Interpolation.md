# Interpolation Utilities

## Overview

The Interpolation utilities in GameEngine provide libgdx-style interpolation functions that enable smooth animations, transitions, and various game effects. These utilities allow for easing functions that make motion appear more natural by controlling the rate of change over time.

## Key Features

- **Easing Functions**: Various interpolation curves for different animation effects
- **Range Interpolation**: Ability to interpolate between any two values
- **Inverse Interpolation**: Some interpolations support inverse calculations
- **Performance Optimized**: Implemented for fast calculation during game loops
- **libgdx Compatible**: Follows libgdx patterns for familiarity

## Basic Usage

### Using Predefined Interpolations

The system provides several commonly used interpolation functions through the `Interpolations` namespace:

```cpp
#include <GameEngine/GameEngine.h>

// Apply interpolation from 0.0 to 1.0
double result = Interpolations::sineInOut.Apply(0.5);  // S-curve at midpoint

// Interpolate between two arbitrary values
double rangeResult = Interpolations::sineInOut.Apply(10.0, 50.0, 0.5);  // 30 (midpoint between 10 and 50)

// Common predefined interpolations:
// - Linear: Interpolations::linear
// - Sine: Interpolations::sine, sineIn, sineOut, sineInOut
// - Exponential: Interpolations::expIn, expOut, expInOut
// - Bounce: Interpolations::bounceIn, bounceOut, bounceInOut
// - Circle: Interpolations::circleIn, circleOut, circleInOut
```

### Using Custom Interpolations

For specific needs, you can create interpolations with custom parameters:

```cpp
// Power-based interpolation with custom exponent
PowIn pow2(2.0);    // Quadratic in
PowIn pow3(3.0);    // Cubic in
double powered = pow2.Apply(0.5);  // 0.25 for quadratic curve at midpoint

// The same pattern works for PowOut and PowInOut
PowOut powOut3(3.0);       // Cubic out
PowInOut powInOut2(2.0);   // Quadratic in-out
```

## Available Interpolation Types

### Linear Interpolation
- `Interpolations::linear`: Simple linear interpolation (f(t) = t)
- No acceleration or deceleration, constant speed

### Sine Interpolations
- `Interpolations::sine`: Symmetric sine curve
- `Interpolations::sineIn`: Slow start, fast end (sine curve from 0 to 1)
- `Interpolations::sineOut`: Fast start, slow end (inverted sine curve)
- `Interpolations::sineInOut`: Slow start and end (S-curve using sine)

### Exponential Interpolations
- `Interpolations::expIn`: Exponential curve starting slowly
- `Interpolations::expOut`: Exponential curve ending slowly
- `Interpolations::expInOut`: Exponential curve with slow start and end

### Power Interpolations
- `PowIn(power)`: Power curve with custom exponent starting slowly
- `PowOut(power)`: Power curve with custom exponent ending slowly
- `PowInOut(power)`: Power curve with slow start and end
- Common values: 2.0 (quadratic), 3.0 (cubic), etc.

### Bounce Interpolations
- `Interpolations::bounceIn`: Simulates object being dropped from height
- `Interpolations::bounceOut`: Simulates object bouncing to rest
- `Interpolations::bounceInOut`: Combination of bounceIn and bounceOut

### Circle Interpolations
- `Interpolations::circleIn`: Circular curve starting slowly
- `Interpolations::circleOut`: Circular curve ending slowly
- `Interpolations::circleInOut`: Circular curve with slow start and end

## Common Use Cases

### Position Animation
```cpp
// Smooth movement from start to end position
Point startPos(0, 0);
Point endPos(100, 100);
double t = animationTime / totalTime;  // Normalized time from 0.0 to 1.0

Point currentPos;
currentPos.x = Interpolations::sineInOut.Apply(startPos.x, endPos.x, t);
currentPos.y = Interpolations::sineInOut.Apply(startPos.y, endPos.y, t);
```

### Fade Effects
```cpp
// Smooth opacity transition
double startOpacity = 0.0;  // Transparent
double endOpacity = 1.0;    // Opaque
double t = fadeTime / fadeDuration;

double currentOpacity = Interpolations::linear.Apply(startOpacity, endOpacity, t);
```

### Scale Animations
```cpp
// Smooth scaling effect
double startScale = 0.0;  // Invisible
double endScale = 1.0;    // Normal size
double t = scaleTime / scaleDuration;

double currentScale = Interpolations::expOut.Apply(startScale, endScale, t);
```

## Performance Tips

1. **Precompute**: For frequently used values, consider precomputing interpolation tables
2. **Simple Interpolations**: Linear and quadratic interpolations are generally faster than complex ones
3. **Reuse Instances**: For custom interpolations like `PowIn`, reuse the same instance if parameters don't change
4. **Avoid Complex Inverses**: Inverse interpolations use approximation algorithms and may be slower

## Integration with Game Systems

The interpolation utilities integrate well with other GameEngine systems:

- Animation system: For smooth transitions between keyframes
- UI system: For smooth menu transitions and animations
- Physics system: For smooth parameter transitions
- Audio system: For smooth volume or pitch changes

## Extending Interpolations

To create custom interpolations, inherit from the `Interpolation` base class:

```cpp
class CustomInterpolation : public Interpolation {
public:
    double Apply(double a) const override {
        // Your custom interpolation formula
        return a; // Replace with your formula
    }
};
```

## Testing

The interpolation utilities include comprehensive tests to ensure accuracy. When creating custom interpolations, ensure they satisfy:

1. `Apply(0.0)` returns `0.0` (or close approximation)
2. `Apply(1.0)` returns `1.0` (or close approximation)
3. Values between 0.0 and 1.0 should return results in the same range (for most interpolations)