#include "GameEngine.h"
#include "Scene3DGraph.h"

NAMESPACE_UPP

ModelNode::ModelNode() {
    name = "Model";
}

void ModelNode::Render(Draw& draw) {
    if (!visible || !enabled) return;

    // Use the parent SceneNode's Render which will call this for children
    SceneNode::Render(draw);
}

void ModelNode::Render3D(Draw& draw, const Matrix4& viewProjection, const Rect& viewport) {
    if (!visible || !enabled) return;

    // Get the world transform for this node
    Matrix4 worldTransform = GetWorldTransform();
    
    // Create the world-view-projection matrix
    Matrix4 wvp = viewProjection * worldTransform;

    // Use the MeshRenderer to actually render the mesh
    MeshRenderer renderer;
    
    // If we have a specific model assigned, use it; otherwise use local mesh
    const Mesh& renderMesh = model ? *model : mesh;
    
    // Apply material properties when rendering
    renderer.Render(draw, renderMesh, worldTransform, viewProjection, viewport, material.GetColor());
    
    // Render children in 3D space
    for (auto& child : children) {
        // Cast child to ModelNode if possible to call Render3D
        // Otherwise use standard Render
        ModelNode* modelChild = dynamic_cast<ModelNode*>(child.get());
        if (modelChild) {
            modelChild->Render3D(draw, viewProjection, viewport);
        } else {
            // For other node types, we might just call their regular render
            child->Render(draw);
        }
    }
}

Scene3DGraph::Scene3DGraph() {
    // Initialize with default values
}

Scene3DGraph::~Scene3DGraph() {
}

void Scene3DGraph::Update(double dt) {
    if (root) {
        root->Update(dt);
    }
}

void Scene3DGraph::Render(Draw& draw, const Matrix4& viewProjection, const Rect& viewport) {
    if (root) {
        // Cast root to ModelNode or other 3D-capable node if needed
        ModelNode* modelRoot = dynamic_cast<ModelNode*>(root.get());
        if (modelRoot) {
            modelRoot->Render3D(draw, viewProjection, viewport);
        } else {
            // For a regular SceneNode, we might need to traverse and render differently
            // This is where we would implement the proper 3D rendering traversal
            // For now, we'll just call the regular render as a fallback
            root->Render(draw);
        }
    }
}

std::shared_ptr<SceneNode> Scene3DGraph::FindNode(const String& name) const {
    if (!root) return nullptr;

    if (root->GetName() == name) {
        return root;
    }

    return root->FindChild(name, true);
}

void Scene3DGraph::AddLight(std::shared_ptr<SceneNode> light) {
    if (light) {
        lights.Add(light);
    }
}

Rect3 Scene3DGraph::GetBounds() const {
    if (bounds_dirty || cached_bounds.IsEmpty()) {
        const_cast<Scene3DGraph*>(this)->CalculateBounds();
    }
    return cached_bounds;
}

void Scene3DGraph::CalculateBounds() {
    if (!root) {
        cached_bounds = Rect3(Point3(0, 0, 0), Point3(0, 0, 0));
        bounds_dirty = false;
        return;
    }

    // This is a simplified bounds calculation
    // In a more complete implementation, we'd traverse all nodes with meshes
    // and calculate the combined bounding box
    cached_bounds = Rect3(root->GetPosition(), root->GetPosition());
    bounds_dirty = false;
}

END_UPP_NAMESPACE