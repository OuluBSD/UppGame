#ifndef UPP_SCENEGRAPH_H
#define UPP_SCENEGRAPH_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <GameLib/GameLib.h>
#include <Geometry/Geometry.h>
#include <GameEngine/VFS.h>

NAMESPACE_UPP

// Forward declarations
class SceneNode;
class CameraNode;
class RenderableNode;
class TransformNode;

// Base class for all scene graph nodes
class SceneNode : public Moveable<SceneNode>, public std::enable_shared_from_this<SceneNode> {
public:
    SceneNode();
    virtual ~SceneNode();

    // Set/get node properties
    void SetName(const String& name) { this->name = name; }
    const String& GetName() const { return name; }
    
    void SetVisible(bool visible) { this->visible = visible; }
    bool IsVisible() const { return visible; }
    
    void SetEnabled(bool enabled) { this->enabled = enabled; }
    bool IsEnabled() const { return enabled; }

    // Transform properties
    void SetPosition(const Point3& pos);
    void SetRotation(const Quaternion& rot);
    void SetScale(const Vector3& scale);
    void SetTransform(const Point3& pos, const Quaternion& rot, const Vector3& scale);

    Point3 GetPosition() const;
    Quaternion GetRotation() const;
    Vector3 GetScale() const;
    Matrix4 GetWorldTransform() const;

    // Hierarchy management
    void AddChild(std::shared_ptr<SceneNode> child);
    void RemoveChild(std::shared_ptr<SceneNode> child);
    void RemoveAllChildren();
    int GetChildCount() const { return children.GetCount(); }
    std::shared_ptr<SceneNode> GetChild(int index) const { return children[index]; }
    std::shared_ptr<SceneNode> GetParent() const { return parent.lock(); }
    
    // Find child by name (recursive)
    std::shared_ptr<SceneNode> FindChild(const String& name, bool recursive = true) const;

    // Update and render
    virtual void Update(double dt);
    virtual void Render(Draw& draw);

    // Mark transform as dirty (needs recalculation)
    void MarkTransformDirty();

protected:
    String name;
    bool visible = true;
    bool enabled = true;
    
    // Local transform (relative to parent)
    Point3 local_position = Point3(0, 0, 0);
    Quaternion local_rotation = Quaternion(0, 0, 0, 1);  // Identity quaternion
    Vector3 local_scale = Vector3(1, 1, 1);
    
    // World transform (absolute in world space)
    Matrix4 world_transform = Matrix4::Identity();
    bool transform_dirty = true;
    
    // Hierarchy
    std::weak_ptr<SceneNode> parent;
    Vector<std::shared_ptr<SceneNode>> children;

    // Calculate world transform based on local transform and parent
    void CalculateWorldTransform();
};

// Camera node for view transformations
class CameraNode : public SceneNode {
public:
    CameraNode();
    virtual ~CameraNode() = default;

    // Camera properties
    void SetProjectionType(bool is_ortho) { is_orthographic = is_ortho; }
    bool IsOrthographic() const { return is_orthographic; }
    
    void SetFieldOfView(double fov) { field_of_view = fov; }
    double GetFieldOfView() const { return field_of_view; }
    
    void SetOrthoSize(double size) { ortho_size = size; }
    double GetOrthoSize() const { return ortho_size; }
    
    void SetNearPlane(double near_val) { near_plane = near_val; }
    double GetNearPlane() const { return near_plane; }
    
    void SetFarPlane(double far_val) { far_plane = far_val; }
    double GetFarPlane() const { return far_plane; }

    // Get view and projection matrices
    Matrix4 GetViewMatrix() const;
    Matrix4 GetProjectionMatrix(int viewport_width, int viewport_height) const;

    void Update(double dt) override;

private:
    bool is_orthographic = false;
    double field_of_view = 45.0;  // In degrees
    double ortho_size = 10.0;
    double near_plane = 0.1;
    double far_plane = 1000.0;
};

// Renderable node that can be drawn
class RenderableNode : public SceneNode {
public:
    RenderableNode();
    virtual ~RenderableNode() = default;

    // Set/get renderable properties
    void SetTexture(const Image& texture) { this->texture = texture; }
    const Image& GetTexture() const { return texture; }
    
    void SetColor(Color color) { this->color = color; }
    Color GetColor() const { return color; }
    
    void SetSourceRect(const Rect& rect) { this->source_rect = rect; }
    Rect GetSourceRect() const { return source_rect; }

    void Render(Draw& draw) override;

protected:
    Image texture;
    Color color = White();
    Rect source_rect;  // Source rectangle in the texture
};

// Transform node that just handles transformations (useful as a parent for other nodes)
class TransformNode : public SceneNode {
public:
    TransformNode();
    virtual ~TransformNode() = default;

    // Transform nodes typically don't render anything
    void Render(Draw& draw) override { /* Do nothing */ }
};

// Scene graph manager
class SceneGraph {
public:
    SceneGraph();
    virtual ~SceneGraph();

    // Root node management
    void SetRoot(std::shared_ptr<SceneNode> root) { this->root = root; }
    std::shared_ptr<SceneNode> GetRoot() const { return root; }

    // Update and render the entire scene
    void Update(double dt);
    void Render(Draw& draw);

    // Find a node by name in the entire scene
    std::shared_ptr<SceneNode> FindNode(const String& name) const;

    // Get the main camera
    std::shared_ptr<CameraNode> GetMainCamera() const { return main_camera; }
    void SetMainCamera(std::shared_ptr<CameraNode> camera) { main_camera = camera; }

private:
    std::shared_ptr<SceneNode> root;
    std::shared_ptr<CameraNode> main_camera;
};

END_UPP_NAMESPACE

#endif