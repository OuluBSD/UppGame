#ifndef UPP_3D_SCENEGRAPH_H
#define UPP_3D_SCENEGRAPH_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <GameLib/GameLib.h>
#include <Geometry/Geometry.h>
#include <GameEngine/GameEngine.h>
#include <GameEngine/SpriteMesh.h>
#include <GameEngine/Material.h>
#include <GameEngine/Shader.h>

NAMESPACE_UPP

// 3D Renderable node that can render complex 3D meshes
class ModelNode : public SceneNode {
public:
    ModelNode();
    virtual ~ModelNode() = default;

    // Set/get mesh
    void SetMesh(const Mesh& mesh) { this->mesh = mesh; }
    const Mesh& GetMesh() const { return mesh; }
    
    // Set/get material
    void SetMaterial(const Material& material) { this->material = material; }
    const Material& GetMaterial() const { return material; }
    
    // Set/get model
    void SetModel(std::shared_ptr<Mesh> model) { this->model = model; }
    std::shared_ptr<Mesh> GetModel() const { return model; }

    // Render with full 3D pipeline
    void Render(Draw& draw) override;
    void Render3D(Draw& draw, const Matrix4& viewProjection, const Rect& viewport);

protected:
    Mesh mesh;
    Material material;
    std::shared_ptr<Mesh> model;  // Optional shared model reference
};

// Enhanced 3D scene graph manager
class Scene3DGraph {
public:
    Scene3DGraph();
    virtual ~Scene3DGraph();

    // Root node management
    void SetRoot(std::shared_ptr<SceneNode> root) { this->root = root; }
    std::shared_ptr<SceneNode> GetRoot() const { return root; }

    // Update and render the entire scene
    void Update(double dt);
    void Render(Draw& draw, const Matrix4& viewProjection, const Rect& viewport);

    // Find a node by name in the entire scene
    std::shared_ptr<SceneNode> FindNode(const String& name) const;

    // Get/Set main camera
    std::shared_ptr<CameraNode> GetMainCamera() const { return main_camera; }
    void SetMainCamera(std::shared_ptr<CameraNode> camera) { main_camera = camera; }

    // Lighting system
    void AddLight(std::shared_ptr<SceneNode> light);
    const Vector<std::shared_ptr<SceneNode>>& GetLights() const { return lights; }

    // Scene bounds calculation
    Rect3 GetBounds() const;

protected:
    std::shared_ptr<SceneNode> root;
    std::shared_ptr<CameraNode> main_camera;
    Vector<std::shared_ptr<SceneNode>> lights;  // Light nodes in the scene
    bool bounds_dirty = true;
    Rect3 cached_bounds;
    void CalculateBounds();
};

END_UPP_NAMESPACE

#endif