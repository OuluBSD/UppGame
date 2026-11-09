#ifndef UPP_RENDERING_H
#define UPP_RENDERING_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <CtrlCore/CtrlCore.h>
#include <Geometry/Geometry.h>
#include <GameLib/GameLib.h>
#include <GameEngine/GameEngine.h>

NAMESPACE_UPP_BEGIN

// Renderable component (will be used when ECS is integrated)
struct Renderable {
    // Basic properties for a renderable object
    Vector<Point3> vertices;
    Vector<Point3> normals;
    Vector<Point2> texCoords;
    Vector<int> indices;  // For indexed rendering
    Color color = White();
    bool visible = true;
    
    Renderable() = default;
};

// Camera component (will be used when ECS is integrated)
struct Camera {
    Matrix4 projection;
    Matrix4 view;
    Point3 position = Point3(0, 0, 0);
    Vector3 direction = Vector3(0, 0, -1);  // Looking down negative Z
    Vector3 up = Vector3(0, 1, 0);
    double fov = 45.0;  // Field of view in degrees
    double aspect = 1.0; // Aspect ratio
    double nearPlane = 0.1;
    double farPlane = 100.0;
    
    Camera() {
        // Initialize with default perspective projection
        UpdateProjection();
    }
    
    void UpdateProjection() {
        projection = Matrix4::Perspective(fov * M_PI / 180.0, aspect, nearPlane, farPlane);
    }
    
    void UpdateView() {
        view = Matrix4::LookAt(position, position + Point3(direction), up);
    }
    
    Matrix4 GetViewProjection() const {
        return projection * view;
    }
    
    // Utility methods for camera control
    void LookAt(const Point3& target) {
        direction = (target - position).Normalize();
        UpdateView();
    }
    
    void SetPerspective(double fieldOfView, double aspectRatio, double nPlane, double fPlane) {
        fov = fieldOfView;
        aspect = aspectRatio;
        nearPlane = nPlane;
        farPlane = fPlane;
        UpdateProjection();
    }
    
    void SetOrthographic(double left, double right, double bottom, double top, double nPlane, double fPlane) {
        projection = Matrix4::Ortho(left, right, bottom, top, nPlane, fPlane);
    }
    
    // Move the camera
    void Move(const Vector3& offset) {
        position = position + offset;
        UpdateView();
    }
    
    void SetPosition(const Point3& newPos) {
        position = newPos;
        UpdateView();
    }
    
    // Rotate the camera
    void Rotate(const Vector3& rotation) {  // rotation in radians
        // Simple rotation - in a real implementation, you'd want proper quaternion-based rotation
        Matrix4 rotX = Matrix4::RotationX(rotation.x);
        Matrix4 rotY = Matrix4::RotationY(rotation.y);
        Matrix4 rotZ = Matrix4::RotationZ(rotation.z);
        
        Matrix4 rotationMatrix = rotZ * rotY * rotX;
        direction = rotationMatrix.Transform(direction).Normalize();
        up = rotationMatrix.Transform(up).Normalize();
        UpdateView();
    }
};

// Camera system for managing the active camera
class CameraSystem {
public:
    CameraSystem();
    
    // Set the active camera
    void SetActiveCamera(const Camera& camera);
    void SetActiveCamera(std::shared_ptr<Camera> camera);
    
    // Get the active camera
    const Camera& GetActiveCamera() const;
    Camera& GetActiveCamera();
    
    // Update camera matrices
    void UpdateMatrices();
    
    // Set viewport for the camera
    void SetViewport(const Rect& viewport);
    const Rect& GetViewport() const { return viewport; }
    
    // Utility methods
    Point3 ScreenToWorld(const Point& screenPos, double depth = 0.0) const;
    Point ScreenToScreen(const Point3& worldPos) const;
    
private:
    std::shared_ptr<Camera> activeCamera;
    Rect viewport;
    bool matrices_dirty = true;
};

// Implementation
inline CameraSystem::CameraSystem() {
    // Create a default camera
    activeCamera = std::make_shared<Camera>();
    viewport = Rect(0, 0, 800, 600); // Default size
}

inline void CameraSystem::SetActiveCamera(const Camera& camera) {
    activeCamera = std::make_shared<Camera>(camera);
    matrices_dirty = true;
}

inline void CameraSystem::SetActiveCamera(std::shared_ptr<Camera> camera) {
    activeCamera = camera;
    matrices_dirty = true;
}

inline const Camera& CameraSystem::GetActiveCamera() const {
    return *activeCamera;
}

inline Camera& CameraSystem::GetActiveCamera() {
    return *activeCamera;
}

inline void CameraSystem::UpdateMatrices() {
    if (activeCamera && matrices_dirty) {
        activeCamera->UpdateView();
        matrices_dirty = false;
    }
}

inline void CameraSystem::SetViewport(const Rect& vp) {
    viewport = vp;
    if (activeCamera && viewport.Height() > 0) {
        activeCamera->aspect = (double)viewport.Width() / (double)viewport.Height();
        activeCamera->UpdateProjection();
    }
}

inline Point3 CameraSystem::ScreenToWorld(const Point& screenPos, double depth) const {
    if (!activeCamera) return Point3(0, 0, 0);
    
    // This is a simplified implementation - a full implementation would require more complex math
    // Convert screen coordinates to normalized device coordinates
    double ndcX = (2.0 * screenPos.x / viewport.Width()) - 1.0;
    double ndcY = 1.0 - (2.0 * screenPos.y / viewport.Height()); // Flip Y axis
    
    // In a real implementation, we'd un-project these coordinates using inverse view-projection matrix
    // For now, just return a position in front of the camera
    return activeCamera->position + activeCamera->direction * depth + 
           Vector3(ndcX, ndcY, 0) * 10.0; // Scaled offset
}

inline Point CameraSystem::ScreenToScreen(const Point3& worldPos) const {
    if (!activeCamera) return Point(0, 0);
    
    // Transform the world position by the view-projection matrix
    Matrix4 viewProjection = activeCamera->GetViewProjection();
    Point4 homogeneous = viewProjection.Transform(Point4(worldPos.x, worldPos.y, worldPos.z, 1.0));
    
    if (homogeneous.w != 0) {
        Point3 ndc = Point3(homogeneous.x / homogeneous.w, 
                           homogeneous.y / homogeneous.w, 
                           homogeneous.z / homogeneous.w);
        
        int screenX = (int)((ndc.x + 1.0) * 0.5 * viewport.Width());
        int screenY = (int)((-ndc.y + 1.0) * 0.5 * viewport.Height()); // Flip Y axis
        
        return Point(screenX, screenY);
    }
    
    // Return a point outside the viewport if the point is behind the camera
    return Point(-1, -1);
}

// Basic renderer for 2D objects
class Basic2DRenderer {
public:
    Basic2DRenderer() = default;
    
    // Render a simple 2D shape using Draw
    void RenderRect(Draw& draw, int x, int y, int width, int height, Color color = White()) {
        draw.DrawRect(x, y, width, height, color);
    }
    
    void RenderLine(Draw& draw, Point p1, Point p2, Color color = White(), int width = 1) {
        draw.DrawLine(p1, p2, width, color);
    }
    
    void RenderText(Draw& draw, int x, int y, const String& text, Font font = StdFont(), Color color = White()) {
        draw.DrawText(x, y, text, font, color);
    }
};

// Basic renderer for 3D objects
class Basic3DRenderer {
public:
    Basic3DRenderer() = default;
    
    // Project a 3D point to 2D screen space
    Point Project3DTo2D(const Point3& point3D, const Matrix4& viewProjection, const Rect& viewport) const {
        // Transform the point by the view-projection matrix
        Point4 homogeneous = viewProjection.Transform(Point4(point3D.x, point3D.y, point3D.z, 1.0));
        
        // Perform perspective division
        if (homogeneous.w != 0) {
            Point3 ndc = Point3(homogeneous.x / homogeneous.w, 
                               homogeneous.y / homogeneous.w, 
                               homogeneous.z / homogeneous.w);
            
            // Transform from NDC to screen coordinates
            int screenX = (int)((ndc.x + 1.0) * 0.5 * viewport.Width());
            int screenY = (int)((-ndc.y + 1.0) * 0.5 * viewport.Height()); // Flip Y axis
            
            return Point(screenX, screenY);
        }
        
        // Return a point outside the viewport if the point is behind the camera
        return Point(-1, -1);
    }
    
    // Render a 3D line (for debugging/simple rendering)
    void RenderLine3D(Draw& draw, const Point3& start, const Point3& end, 
                      const Matrix4& viewProjection, const Rect& viewport, 
                      Color color = White(), int width = 1) {
        Point p1 = Project3DTo2D(start, viewProjection, viewport);
        Point p2 = Project3DTo2D(end, viewProjection, viewport);
        
        // Only draw if both points are in front of camera
        if (p1.x >= 0 && p1.y >= 0 && p2.x >= 0 && p2.y >= 0) {
            draw.DrawLine(p1, p2, width, color);
        }
    }
    
    // Render a 3D triangle (for debugging/simple rendering)
    void RenderTriangle3D(Draw& draw, const Point3& v1, const Point3& v2, const Point3& v3,
                          const Matrix4& viewProjection, const Rect& viewport,
                          Color color = White()) {
        Point p1 = Project3DTo2D(v1, viewProjection, viewport);
        Point p2 = Project3DTo2D(v2, viewProjection, viewport);
        Point p3 = Project3DTo2D(v3, viewProjection, viewport);
        
        // Only draw if all points are in front of camera
        if (p1.x >= 0 && p1.y >= 0 && p2.x >= 0 && p2.y >= 0 && p3.x >= 0 && p3.y >= 0) {
            // Draw filled triangle using DrawPolygon
            Vector<Point> points;
            points.Add(p1);
            points.Add(p2);
            points.Add(p3);
            
            draw.DrawPolygon(points, color);
        }
    }
};

// Main renderer class that manages both 2D and 3D rendering
class Renderer {
public:
    Renderer();
    
    // Initialize renderer with game window context
    bool Initialize(GameWindow& window);
    
    // Main render function
    void Render(Draw& draw);
    
    // Add drawable objects for the next render cycle
    void Add2DObject(std::function<void(Draw&)> renderFunc);
    void Add3DObject(std::function<void(Draw&, const Matrix4&, const Rect&)> renderFunc);
    
    // Camera management
    void SetCamera(const Camera& camera) { cameraSystem.SetActiveCamera(camera); }
    void SetCamera(std::shared_ptr<Camera> camera) { cameraSystem.SetActiveCamera(camera); }
    const Camera& GetCamera() const { return cameraSystem.GetActiveCamera(); }
    Camera& GetCamera() { return cameraSystem.GetActiveCamera(); }
    CameraSystem& GetCameraSystem() { return cameraSystem; }
    const CameraSystem& GetCameraSystem() const { return cameraSystem; }
    
    // Clear the render queue
    void ClearRenderQueue();
    
    // Get viewport rect
    Rect GetViewport() const;
    
private:
    Basic2DRenderer basic2d;
    Basic3DRenderer basic3d;
    CameraSystem cameraSystem;
    Vector<std::function<void(Draw&)>> renderQueue2D;
    Vector<std::function<void(Draw&, const Matrix4&, const Rect&)>> renderQueue3D;
    GameWindow* gameWindow = nullptr;
};

// Implementation
inline Renderer::Renderer() {
    // Initialize with a default camera - the CameraSystem handles initialization
}

inline bool Renderer::Initialize(GameWindow& window) {
    gameWindow = &window;
    
    // Set up the camera's aspect ratio based on window size
    if (gameWindow) {
        Size sz = gameWindow->GetSize();
        if (sz.cy > 0) {
            Rect viewport(0, 0, sz.cx, sz.cy);
            cameraSystem.SetViewport(viewport);
        }
    }
    
    return true;
}

inline void Renderer::Render(Draw& draw) {
    // Update camera system
    cameraSystem.UpdateMatrices();
    
    // Get current viewport
    Rect viewport = GetViewport();
    cameraSystem.SetViewport(viewport);
    
    // Render 3D objects first
    Matrix4 viewProjection = cameraSystem.GetActiveCamera().GetViewProjection();
    for (auto& renderFunc : renderQueue3D) {
        renderFunc(draw, viewProjection, viewport);
    }
    
    // Then render 2D objects on top
    for (auto& renderFunc : renderQueue2D) {
        renderFunc(draw);
    }
    
    // Clear queues for next frame
    ClearRenderQueue();
}

inline void Renderer::Add2DObject(std::function<void(Draw&)> renderFunc) {
    renderQueue2D.Add(renderFunc);
}

inline void Renderer::Add3DObject(std::function<void(Draw&, const Matrix4&, const Rect&)> renderFunc) {
    renderQueue3D.Add(renderFunc);
}

inline void Renderer::ClearRenderQueue() {
    renderQueue2D.Clear();
    renderQueue3D.Clear();
}

inline Rect Renderer::GetViewport() const {
    if (gameWindow) {
        Size sz = gameWindow->GetSize();
        return Rect(0, 0, sz.cx, sz.cy);
    }
    return Rect(0, 0, 800, 600); // Default size
}

NAMESPACE_UPP_END

#endif