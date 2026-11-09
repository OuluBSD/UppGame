#ifndef UPP_CURVES_H
#define UPP_CURVES_H

#include <Core/Core.h>
#include <Geometry/Geometry.h>
#include <GameLib/Interpolation.h>
#include <Vector/Vector.h>

NAMESPACE_UPP

// Forward declaration
template<typename T>
class Path;

// Base Curve interface
template<typename T>
class Curve {
public:
    virtual ~Curve() = default;
    
    // Get the value at time t (0.0 to 1.0)
    virtual T Apply(double t) const = 0;
    
    // Get the value along the curve with optional interpolation
    virtual T Apply(double t, const Interpolation& interpolation) const {
        return Apply(interpolation.Apply(t));
    }
    
    // Get the length (for applicable curves)
    virtual double GetLength() const { return 0.0; }
};

// Point3 curve implementation
class PointCurve : public Curve<Point3> {
private:
    Vector<Point3> points;
    std::shared_ptr<Interpolation> interpolation;

public:
    PointCurve() : interpolation(std::make_shared<Linear>()) {}
    
    PointCurve(const Vector<Point3>& pts) : points(pts), interpolation(std::make_shared<Linear>()) {}
    
    PointCurve(const Vector<Point3>& pts, std::shared_ptr<Interpolation> interp) 
        : points(pts), interpolation(interp) {}
    
    // Set the interpolation method
    void SetInterpolation(std::shared_ptr<Interpolation> interp) {
        interpolation = interp;
    }
    
    // Add a control point
    void AddPoint(const Point3& pt) {
        points.Add(pt);
    }
    
    // Get the value at time t (0.0 to 1.0)
    Point3 Apply(double t) const override {
        if (points.IsEmpty()) return Point3(0, 0, 0);
        if (points.GetCount() == 1) return points[0];
        
        // Use interpolation between points
        t = interpolation->Apply(t);
        
        // Calculate which segment and local t value
        double totalSegments = static_cast<double>(points.GetCount() - 1);
        if (totalSegments <= 0) return points[0];
        
        double scaledT = t * totalSegments;
        int segmentIndex = static_cast<int>(floor(scaledT));
        double localT = scaledT - segmentIndex;
        
        // Clamp to valid range
        segmentIndex = min(segmentIndex, points.GetCount() - 2);
        
        // Linear interpolation between the two points
        const Point3& p0 = points[segmentIndex];
        const Point3& p1 = points[segmentIndex + 1];
        
        return Point3(
            p0.x + localT * (p1.x - p0.x),
            p0.y + localT * (p1.y - p0.y),
            p0.z + localT * (p1.z - p0.z)
        );
    }
    
    // Get the number of points
    int GetPointCount() const { return points.GetCount(); }
    
    // Get a specific point
    Point3 GetPoint(int index) const { return points[index]; }
    
    // Clear all points
    void Clear() { points.Clear(); }
};

// Bezier curve implementation
class BezierCurve : public Curve<Point3> {
private:
    Point3 start, control1, control2, end;
    
public:
    // Quadratic Bezier (one control point)
    BezierCurve(const Point3& start, const Point3& control, const Point3& end) 
        : start(start), control1(control), control2(control), end(end) {}
    
    // Cubic Bezier (two control points)
    BezierCurve(const Point3& start, const Point3& ctrl1, const Point3& ctrl2, const Point3& end) 
        : start(start), control1(ctrl1), control2(ctrl2), end(end) {}
    
    Point3 Apply(double t) const override {
        // Cubic Bezier curve formula: B(t) = (1-t)³P0 + 3(1-t)²tP1 + 3(1-t)t²P2 + t³P3
        double u = 1.0 - t;
        double tt = t * t;
        double uu = u * u;
        double uuu = uu * u;
        double ttt = tt * t;
        
        return Point3(
            uuu * start.x + 3 * uu * t * control1.x + 3 * u * tt * control2.x + ttt * end.x,
            uuu * start.y + 3 * uu * t * control1.y + 3 * u * tt * control2.y + ttt * end.y,
            uuu * start.z + 3 * uu * t * control1.z + 3 * u * tt * control2.z + ttt * end.z
        );
    }
};

// Catmull-Rom spline curve implementation
class CatmullRomCurve : public Curve<Point3> {
private:
    Vector<Point3> points;
    double tension;
    
public:
    CatmullRomCurve(const Vector<Point3>& pts, double tension = 0.5) 
        : points(pts), tension(tension) {}
    
    Point3 Apply(double t) const override {
        if (points.IsEmpty()) return Point3(0, 0, 0);
        if (points.GetCount() == 1) return points[0];
        if (points.GetCount() == 2) {
            // Linear interpolation between two points
            Point3 p0 = points[0];
            Point3 p1 = points[1];
            return Point3(
                p0.x + t * (p1.x - p0.x),
                p0.y + t * (p1.y - p0.y),
                p0.z + t * (p1.z - p0.z)
            );
        }
        
        // Scale t to the number of segments
        t = max(0.0, min(1.0, t));  // Clamp t to [0, 1]
        int numSegments = points.GetCount() - 1;
        double scaledT = t * numSegments;
        int segment = static_cast<int>(scaledT);
        double localT = scaledT - segment;
        
        // Clamp segment index
        segment = min(segment, numSegments - 1);
        
        // Determine the four points to use for the Catmull-Rom calculation
        int i0 = max(0, segment - 1);
        int i1 = segment;
        int i2 = segment + 1;
        int i3 = min(segment + 2, points.GetCount() - 1);
        
        Point3 p0 = points[i0];
        Point3 p1 = points[i1];
        Point3 p2 = points[i2];
        Point3 p3 = points[i3];
        
        // Catmull-Rom coefficients with tension
        double t2 = localT * localT;
        double t3 = t2 * localT;
        
        double c0 = -tension * t3 + 2 * tension * t2 - tension * localT;
        double c1 = (2 - tension) * t3 + (tension - 3) * t2 + 1;
        double c2 = (tension - 2) * t3 + (3 - 2 * tension) * t2 + tension * localT;
        double c3 = tension * t3 - tension * t2;
        
        return Point3(
            c0 * p0.x + c1 * p1.x + c2 * p2.x + c3 * p3.x,
            c0 * p0.y + c1 * p1.y + c2 * p2.y + c3 * p3.y,
            c0 * p0.z + c1 * p1.z + c2 * p2.z + c3 * p3.z
        );
    }
};

// Path interface - a sequence of connected curves
template<typename T>
class Path {
public:
    virtual ~Path() = default;
    
    // Get a point along the path at parameter t (0.0 to 1.0)
    virtual T GetPoint(double t) const = 0;
    
    // Get the length of the path
    virtual double GetLength() const = 0;
    
    // Get a tangent vector at parameter t
    virtual T GetTangent(double t) const = 0;
    
    // Find the parameter t value for a given distance along the path
    virtual double Locate(double distance) const = 0;
    
    // Find the closest point on the path to a given point
    virtual T GetNearest(const T& point) const = 0;
};

// Implementation of a path composed of multiple curves
class CompositePath : public Path<Point3> {
private:
    Vector<std::shared_ptr<Curve<Point3>>> curves;
    Vector<double> lengths;
    double totalLength;
    
public:
    CompositePath() : totalLength(0.0) {}
    
    void AddCurve(std::shared_ptr<Curve<Point3>> curve) {
        curves.Add(curve);
        double length = curve->GetLength();
        lengths.Add(length);
        totalLength += length;
    }
    
    // Add a simple line segment
    void AddLine(const Point3& start, const Point3& end) {
        Vector<Point3> points;
        points.Add(start);
        points.Add(end);
        auto curve = std::make_shared<PointCurve>(points);
        AddCurve(curve);
    }
    
    // Add a Bezier curve segment
    void AddBezier(const Point3& start, const Point3& ctrl1, const Point3& ctrl2, const Point3& end) {
        auto curve = std::make_shared<BezierCurve>(start, ctrl1, ctrl2, end);
        AddCurve(curve);
    }
    
    Point3 GetPoint(double t) const override {
        if (curves.IsEmpty()) return Point3(0, 0, 0);
        if (curves.GetCount() == 1) return curves[0]->Apply(t);
        
        // Find which curve segment to use
        double targetDistance = t * totalLength;
        double currentDistance = 0.0;
        
        for (int i = 0; i < curves.GetCount(); i++) {
            if (targetDistance <= currentDistance + lengths[i]) {
                // Calculate local t for this curve
                double localT = (targetDistance - currentDistance) / lengths[i];
                return curves[i]->Apply(localT);
            }
            currentDistance += lengths[i];
        }
        
        // Return the end of the last curve if t=1.0
        return curves[curves.GetCount() - 1]->Apply(1.0);
    }
    
    double GetLength() const override {
        return totalLength;
    }
    
    Point3 GetTangent(double t) const override {
        if (curves.IsEmpty()) return Point3(0, 0, 0);
        if (curves.GetCount() == 1) {
            // Approximate tangent using small delta
            double dt = 0.01;
            Point3 p1 = curves[0]->Apply(max(0.0, t - dt));
            Point3 p2 = curves[0]->Apply(min(1.0, t + dt));
            return (p2 - p1).Normalized();
        }
        
        // Similar approach for composite paths
        double dt = 0.01;
        Point3 p1 = GetPoint(max(0.0, t - dt));
        Point3 p2 = GetPoint(min(1.0, t + dt));
        return (p2 - p1).Normalized();
    }
    
    double Locate(double distance) const override {
        if (totalLength <= 0) return 0.0;
        distance = max(0.0, min(totalLength, distance));  // Clamp to valid range
        
        double currentDistance = 0.0;
        for (int i = 0; i < curves.GetCount(); i++) {
            if (distance <= currentDistance + lengths[i]) {
                // Calculate local t for this curve
                double localDistance = distance - currentDistance;
                double localT = localDistance / lengths[i];
                return (static_cast<double>(i) + localT) / curves.GetCount();
            }
            currentDistance += lengths[i];
        }
        
        return 1.0; // At the end of the path
    }
    
    Point3 GetNearest(const Point3& point) const override {
        if (curves.IsEmpty()) return Point3(0, 0, 0);
        
        // Simple approach: sample each curve and find the closest point
        Point3 nearestPoint = curves[0]->Apply(0.0);
        double minDistance = (nearestPoint - point).GetLength();
        
        // Sample each curve at multiple points
        for (int c = 0; c < curves.GetCount(); c++) {
            for (int i = 0; i <= 20; i++) {  // 20 samples per curve
                double t = static_cast<double>(i) / 20.0;
                Point3 p = curves[c]->Apply(t);
                double dist = (p - point).GetLength();
                
                if (dist < minDistance) {
                    minDistance = dist;
                    nearestPoint = p;
                }
            }
        }
        
        return nearestPoint;
    }
};

// Helper class for creating common paths
class PathFactory {
public:
    // Create a simple linear path from a set of points
    static std::shared_ptr<CompositePath> CreateLinearPath(const Vector<Point3>& points) {
        auto path = std::make_shared<CompositePath>();
        
        if (points.GetCount() < 2) return path;
        
        for (int i = 0; i < points.GetCount() - 1; i++) {
            path->AddLine(points[i], points[i + 1]);
        }
        
        return path;
    }
    
    // Create a rectangular path
    static std::shared_ptr<CompositePath> CreateRectanglePath(double width, double height) {
        Vector<Point3> points;
        points.Add(Point3(0, 0, 0));           // Bottom-left
        points.Add(Point3(width, 0, 0));       // Bottom-right
        points.Add(Point3(width, height, 0));  // Top-right
        points.Add(Point3(0, height, 0));      // Top-left
        points.Add(Point3(0, 0, 0));           // Close the path
        
        return CreateLinearPath(points);
    }
    
    // Create a circular path (approximated with line segments)
    static std::shared_ptr<CompositePath> CreateCircularPath(double radius, int segments = 16) {
        Vector<Point3> points;
        
        for (int i = 0; i <= segments; i++) {
            double angle = 2.0 * M_PI * i / segments;
            points.Add(Point3(
                radius * cos(angle),
                radius * sin(angle),
                0
            ));
        }
        
        return CreateLinearPath(points);
    }
};

END_UPP_NAMESPACE

#endif