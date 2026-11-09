#include "SceneGraph.h"

NAMESPACE_UPP

// SceneNode implementation
SceneNode::SceneNode() {
}

SceneNode::~SceneNode() {
    RemoveAllChildren();
}

void SceneNode::SetPosition(const Point3& pos) {
    local_position = pos;
    MarkTransformDirty();
}

void SceneNode::SetRotation(const Quaternion& rot) {
    local_rotation = rot.Normalize();
    MarkTransformDirty();
}

void SceneNode::SetScale(const Vector3& scale) {
    local_scale = scale;
    MarkTransformDirty();
}

void SceneNode::SetTransform(const Point3& pos, const Quaternion& rot, const Vector3& scale) {
    local_position = pos;
    local_rotation = rot.Normalize();
    local_scale = scale;
    MarkTransformDirty();
}

Point3 SceneNode::GetPosition() const {
    if (auto p = parent.lock()) {
        // Transform local position to world space using parent's world transform
        Matrix4 parent_world = p->GetWorldTransform();
        Point3 world_pos = parent_world.TransformPoint(local_position);
        return world_pos;
    }
    return local_position;
}

Quaternion SceneNode::GetRotation() const {
    if (auto p = parent.lock()) {
        // Combine parent's rotation with local rotation
        Quaternion parent_rot = p->GetRotation();
        return parent_rot * local_rotation;  // Order matters
    }
    return local_rotation;
}

Vector3 SceneNode::GetScale() const {
    if (auto p = parent.lock()) {
        // Combine parent's scale with local scale
        Vector3 parent_scale = p->GetScale();
        return Vector3(
            parent_scale.x * local_scale.x,
            parent_scale.y * local_scale.y,
            parent_scale.z * local_scale.z
        );
    }
    return local_scale;
}

Matrix4 SceneNode::GetWorldTransform() const {
    if (transform_dirty) {
        const_cast<SceneNode*>(this)->CalculateWorldTransform();
    }
    return world_transform;
}

void SceneNode::AddChild(std::shared_ptr<SceneNode> child) {
    if (!child) return;
    
    // Remove from current parent if any
    if (auto current_parent = child->GetParent()) {
        current_parent->RemoveChild(child);
    }
    
    // Set this as the parent
    child->parent = shared_from_this();
    children.Add(child);
}

void SceneNode::RemoveChild(std::shared_ptr<SceneNode> child) {
    if (!child) return;
    
    for (int i = 0; i < children.GetCount(); i++) {
        if (children[i] == child) {
            child->parent.reset();
            children.Remove(i);
            break;
        }
    }
}

void SceneNode::RemoveAllChildren() {
    for (auto& child : children) {
        child->parent.reset();
    }
    children.Clear();
}

std::shared_ptr<SceneNode> SceneNode::FindChild(const String& name, bool recursive) const {
    for (auto& child : children) {
        if (child->GetName() == name) {
            return child;
        }
        
        if (recursive) {
            auto found = child->FindChild(name, recursive);
            if (found) return found;
        }
    }
    return nullptr;
}

void SceneNode::Update(double dt) {
    if (!enabled) return;
    
    // Update this node (default: do nothing)
    
    // Update children
    for (auto& child : children) {
        child->Update(dt);
    }
}

void SceneNode::Render(Draw& draw) {
    if (!visible) return;
    
    // Render this node (default: do nothing)
    
    // Render children
    for (auto& child : children) {
        child->Render(draw);
    }
}

void SceneNode::MarkTransformDirty() {
    transform_dirty = true;
    // Mark all children as dirty too since they depend on this transform
    for (auto& child : children) {
        child->MarkTransformDirty();
    }
}

void SceneNode::CalculateWorldTransform() {
    // Create local transform matrix
    Matrix4 translation = Matrix4::Translation(local_position);
    Matrix4 rotation = Matrix4::Rotation(local_rotation);
    Matrix4 scale = Matrix4::Scaling(local_scale);
    
    Matrix4 local_transform = translation * rotation * scale;
    
    // Combine with parent transform if exists
    if (auto p = parent.lock()) {
        world_transform = p->GetWorldTransform() * local_transform;
    } else {
        world_transform = local_transform;
    }
    
    transform_dirty = false;
}

// CameraNode implementation
CameraNode::CameraNode() {
    name = "Camera";
}

Matrix4 CameraNode::GetViewMatrix() const {
    // Get the camera's world position and orientation
    Point3 pos = GetPosition();
    Quaternion rot = GetRotation();
    
    // Create view matrix (inverse of camera's world transform)
    Matrix4 rotation_matrix = Matrix4::Rotation(rot.Inverse());  // Inverse rotation
    Matrix4 translation_matrix = Matrix4::Translation(-pos);     // Inverse translation
    
    return rotation_matrix * translation_matrix;
}

Matrix4 CameraNode::GetProjectionMatrix(int viewport_width, int viewport_height) const {
    if (is_orthographic) {
        double aspect_ratio = (double)viewport_width / viewport_height;
        double half_height = ortho_size;
        double half_width = ortho_size * aspect_ratio;
        
        return Matrix4::Orthographic(-half_width, half_width, 
                                    -half_height, half_height, 
                                    near_plane, far_plane);
    } else {
        // Perspective projection
        return Matrix4::Perspective(field_of_view, (double)viewport_width / viewport_height, 
                                   near_plane, far_plane);
    }
}

void CameraNode::Update(double dt) {
    SceneNode::Update(dt);
}

// RenderableNode implementation
RenderableNode::RenderableNode() {
    name = "Renderable";
}

void RenderableNode::Render(Draw& draw) {
    if (!visible || !enabled) return;
    
    // Get world transform for this node
    Matrix4 world = GetWorldTransform();
    
    // For now, just draw a simple rectangle with the texture
    if (texture) {
        // Get position from transform
        Point3 pos = GetPosition();
        
        // Calculate size based on scale
        Vector3 scale = GetScale();
        
        // This is a simplified rendering - in a real implementation, 
        // we would apply the full transformation matrix
        int width = (int)(texture.GetWidth() * scale.x);
        int height = (int)(texture.GetHeight() * scale.y);
        
        if (source_rect.IsEmpty()) {
            // Draw full texture
            draw.DrawImage((int)pos.x - width/2, (int)pos.y - height/2, width, height, texture);
        } else {
            // Draw sub-texture
            draw.DrawImage((int)pos.x - width/2, (int)pos.y - height/2, width, height, 
                          texture, source_rect);
        }
    } else {
        // Draw a placeholder rectangle if no texture
        Vector3 scale = GetScale();
        int width = (int)(10 * scale.x);
        int height = (int)(10 * scale.y);
        draw.DrawRect(Rect((int)pos.x - width/2, (int)pos.y - height/2, 
                          (int)pos.x + width/2, (int)pos.y + height/2), color);
    }
    
    // Render children
    SceneNode::Render(draw);
}

// TransformNode implementation
TransformNode::TransformNode() {
    name = "Transform";
}

// SceneGraph implementation
SceneGraph::SceneGraph() {
}

SceneGraph::~SceneGraph() {
}

void SceneGraph::Update(double dt) {
    if (root) {
        root->Update(dt);
    }
}

void SceneGraph::Render(Draw& draw) {
    if (root) {
        root->Render(draw);
    }
}

std::shared_ptr<SceneNode> SceneGraph::FindNode(const String& name) const {
    if (!root) return nullptr;
    
    if (root->GetName() == name) {
        return root;
    }
    
    return root->FindChild(name, true);
}

END_UPP_NAMESPACE