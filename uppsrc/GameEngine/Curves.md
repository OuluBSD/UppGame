# Curves and Paths Utilities

## Overview

The Curves and Paths utilities in GameEngine provide libgdx-style functionality for defining and traversing paths and curves for animations and motion. These utilities enable complex movement patterns, trajectory calculations, and path-based animations in games.

## Key Features

- **Multiple Curve Types**: Linear, Bezier, and Catmull-Rom spline curves
- **Composite Paths**: Ability to combine multiple curves into complex paths
- **Path Interface**: Standardized interface for path traversal
- **Tangent Calculation**: Compute tangents for orientation along paths
- **Distance-based Location**: Find positions based on distance along paths
- **Nearest Point**: Find the closest point on a path to a given point
- **libgdx Compatible**: Follows libgdx patterns for familiarity

## Basic Usage

### Creating and Using Curves

#### PointCurve - Linear interpolation between points

```cpp
#include <GameEngine/GameEngine.h>

// Create a curve with multiple control points
Vector<Point3> points;
points.Add(Point3(0, 0, 0));
points.Add(Point3(10, 0, 0));
points.Add(Point3(10, 10, 0));
points.Add(Point3(0, 10, 0));

PointCurve curve(points);

// Get a point along the curve at parameter t (0.0 to 1.0)
Point3 position = curve.Apply(0.5);  // Midpoint of the curve
```

#### BezierCurve - Bezier curves with control points

```cpp
// Create a quadratic Bezier curve
BezierCurve quadBezier(Point3(0, 0, 0), Point3(5, 10, 0), Point3(10, 0, 0));

// Create a cubic Bezier curve
BezierCurve cubicBezier(
    Point3(0, 0, 0),    // Start point
    Point3(2, 8, 0),    // First control point
    Point3(8, 8, 0),    // Second control point
    Point3(10, 0, 0)    // End point
);

Point3 position = cubicBezier.Apply(0.3);  // Point at 30% along the curve
```

#### CatmullRomCurve - Smooth spline through points

```cpp
Vector<Point3> points;
points.Add(Point3(0, 0, 0));
points.Add(Point3(10, 5, 0));
points.Add(Point3(15, 10, 0));
points.Add(Point3(20, 0, 0));

// Tension parameter affects curve tightness (0.5 is default)
CatmullRomCurve spline(points, 0.5);

Point3 position = spline.Apply(0.7);  // Point at 70% along the spline
```

### Creating and Using Paths

#### CompositePath - Combining multiple curves

```cpp
// Create a path from multiple segments
auto path = std::make_shared<CompositePath>();

// Add different types of segments
path->AddLine(Point3(0, 0, 0), Point3(10, 0, 0));                           // Line segment
path->AddBezier(Point3(10, 0, 0), Point3(15, 5, 0), Point3(20, 5, 0), Point3(25, 0, 0));  // Bezier curve
path->AddLine(Point3(25, 0, 0), Point3(30, 10, 0));                        // Another line

// Get a point along the entire path
Point3 position = path->GetPoint(0.5);  // Point at 50% along the entire path

// Get the tangent vector at a given point (useful for orientation)
Point3 tangent = path->GetTangent(0.5);
```

#### Using PathFactory for common paths

```cpp
// Create a rectangular path
auto rectPath = PathFactory::CreateRectanglePath(100.0, 50.0);

// Create a circular path (approximated with line segments)
auto circularPath = PathFactory::CreateCircularPath(50.0, 32);  // 32 segments for smoothness

// Create a linear path from a set of points
Vector<Point3> points;
points.Add(Point3(0, 0, 0));
points.Add(Point3(10, 5, 0));
points.Add(Point3(20, 0, 0));
points.Add(Point3(30, 10, 0));

auto linearPath = PathFactory::CreateLinearPath(points);
```

## Advanced Usage

### Path Following with Orientation

```cpp
// Example of making an object follow a path with proper orientation
void FollowPath(std::shared_ptr<Path<Point3>> path, double time, double& x, double& y, double& rotation) {
    // Get position along the path
    Point3 position = path->GetPoint(time);
    x = position.x;
    y = position.y;
    
    // Get the tangent to determine orientation
    Point3 tangent = path->GetTangent(time);
    rotation = atan2(tangent.y, tangent.x);  // Convert to angle
}
```

### Distance-based Path Traversal

```cpp
// Travel a specific distance along a path
auto path = PathFactory::CreateRectanglePath(100.0, 50.0);
double totalDistance = path->GetLength();
double targetDistance = 75.0;  // Travel 75 units along the path

// Find the parameter t that corresponds to this distance
double t = path->Locate(targetDistance);
Point3 position = path->GetPoint(t);
```

### Finding Nearest Point

```cpp
// Find the closest point on a path to a given position
Point3 objectPos(75, 25, 0);
auto path = PathFactory::CreateRectanglePath(100.0, 50.0);

Point3 nearest = path->GetNearest(objectPos);
```

## Curve and Path Types

### PointCurve
- **Purpose**: Linear interpolation between a sequence of points
- **Usage**: Simple paths with straight segments
- **Characteristics**: Piecewise linear, passes through all control points

### BezierCurve
- **Purpose**: Smooth curved paths with control points
- **Types**: 
  - Quadratic (1 control point)
  - Cubic (2 control points)
- **Characteristics**: Smooth curve, does not necessarily pass through control points

### CatmullRomCurve
- **Purpose**: Smooth spline curve that passes through all points
- **Characteristics**: Interpolating spline, adjustable tension parameter

### CompositePath
- **Purpose**: Combines multiple curves into a single traversable path
- **Characteristics**: Can mix different types of curves in one path

## Performance Considerations

1. **Sampling Rate**: For smooth animations, sample the path at an appropriate rate based on object speed
2. **Complexity**: Complex paths with many segments may impact performance; consider caching if necessary
3. **Tangent Calculation**: Computing tangents involves additional calculations; cache if used frequently
4. **Memory Usage**: Composite paths store references to multiple curves; be mindful of memory in large worlds

## Integration with Other Systems

The Curves and Paths utilities integrate well with other GameEngine systems:

- Animation system: For defining trajectory animations
- AI system: For complex movement patterns
- Physics system: For predefined motion paths
- Rendering: For particle effects following paths
- Camera system: For cinematic camera movements

## Common Use Cases

1. **Enemy Movement**: Create complex patrol patterns
2. **Camera Paths**: Define cinematic camera movements
3. **Particle Effects**: Make particles follow complex trajectories
4. **Vehicle Routes**: Define paths for vehicles or moving platforms
5. **Projectile Trajectories**: Create curved projectile paths
6. **UI Animations**: Create complex UI motion paths

## Tips

1. **Start Simple**: Begin with PointCurve for basic linear paths, then move to more complex curves as needed
2. **Visual Debugging**: Render paths during development to verify they match your expectations
3. **Parameter Normalization**: Remember that parameter t is always normalized from 0.0 to 1.0
4. **Tension Tuning**: For Catmull-Rom splines, adjust tension (0.0 to 1.0) to achieve desired smoothness
5. **Performance Testing**: Test path complexity on target hardware, especially for real-time applications