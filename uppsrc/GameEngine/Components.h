#ifndef UPP_GAMEENGINE_COMPONENTS_H
#define UPP_GAMEENGINE_COMPONENTS_H

#include <Core/Core.h>
#include <Vfs/Ecs/Ecs.h>
#include <Geometry/Geometry.h>
#include <GameLib/Curves.h>
#include <GameLib/Interpolation.h>

NAMESPACE_UPP

// Transform component for game objects
class TransformComponent : public Component {
public:
    ECS_COMPONENT_CTOR(TransformComponent)
    
    // Set position in 3D space
    void SetPosition(const Point3& pos) { position = pos; }
    Point3 GetPosition() const { return position; }
    
    // Set rotation as quaternion
    void SetRotation(const Quaternion& rot) { rotation = rot; }
    Quaternion GetRotation() const { return rotation; }
    
    // Set scale in 3D space
    void SetScale(const Point3& scale) { scale3d = scale; }
    Point3 GetScale() const { return scale3d; }
    
    // Get the transformation matrix
    Matrix4 GetTransformationMatrix() const {
        Matrix4 translation = Matrix4::Translation(position);
        Matrix4 rotation_matrix = Matrix4::FromQuaternion(rotation);
        Matrix4 scaling = Matrix4::Scaling(scale3d);
        
        return translation * rotation_matrix * scaling;
    }
    
    // Visit method for serialization and debugging
    void Visit(Vis& v) override {
        v("position", position)("rotation", rotation)("scale", scale3d);
    }

private:
    Point3 position = Point3(0, 0, 0);
    Quaternion rotation = Quaternion(0, 0, 0, 1);  // Identity quaternion
    Point3 scale3d = Point3(1, 1, 1);
};

// Component for rendering game objects
class RenderComponent : public Component {
public:
    ECS_COMPONENT_CTOR(RenderComponent)
    
    void SetModelPath(const String& path) { model_path = path; }
    String GetModelPath() const { return model_path; }
    
    void SetMaterialPath(const String& path) { material_path = path; }
    String GetMaterialPath() const { return material_path; }
    
    void SetTexturePath(const String& path) { texture_path = path; }
    String GetTexturePath() const { return texture_path; }
    
    void SetVisible(bool visible) { is_visible = visible; }
    bool IsVisible() const { return is_visible; }
    
    void Visit(Vis& v) override {
        v("model_path", model_path)
         ("material_path", material_path)
         ("texture_path", texture_path)
         ("is_visible", is_visible);
    }

private:
    String model_path;
    String material_path;
    String texture_path;
    bool is_visible = true;
};

// Component for physics simulation
class PhysicsComponent : public Component {
public:
    ECS_COMPONENT_CTOR(PhysicsComponent)
    
    void SetMass(double mass) { this->mass = mass; }
    double GetMass() const { return mass; }
    
    void SetVelocity(const Vector3& vel) { velocity = vel; }
    Vector3 GetVelocity() const { return velocity; }
    
    void SetAcceleration(const Vector3& accel) { acceleration = accel; }
    Vector3 GetAcceleration() const { return acceleration; }
    
    void SetKinematic(bool kinematic) { this->kinematic = kinematic; }
    bool IsKinematic() const { return kinematic; }
    
    void SetStatic(bool is_static) { this->is_static = is_static; }
    bool IsStatic() const { return is_static; }
    
    void Visit(Vis& v) override {
        v("mass", mass)
         ("velocity", velocity)
         ("acceleration", acceleration)
         ("kinematic", kinematic)
         ("is_static", is_static);
    }

private:
    double mass = 1.0;
    Vector3 velocity = Vector3(0, 0, 0);
    Vector3 acceleration = Vector3(0, 0, 0);
    bool kinematic = false;
    bool is_static = false;
};

// Component for audio in game objects
class AudioComponent : public Component {
public:
    ECS_COMPONENT_CTOR(AudioComponent)
    
    void SetAudioPath(const String& path) { audio_path = path; }
    String GetAudioPath() const { return audio_path; }
    
    void SetVolume(double vol) { volume = vol; }
    double GetVolume() const { return volume; }
    
    void SetLooping(bool loop) { should_loop = loop; }
    bool IsLooping() const { return should_loop; }
    
    void SetPlaying(bool playing) { is_playing = playing; }
    bool IsPlaying() const { return is_playing; }
    
    void Visit(Vis& v) override {
        v("audio_path", audio_path)
         ("volume", volume)
         ("should_loop", should_loop)
         ("is_playing", is_playing);
    }

private:
    String audio_path;
    double volume = 1.0;
    bool should_loop = false;
    bool is_playing = false;
};

// Component for animation
class AnimationComponent : public Component {
public:
    ECS_COMPONENT_CTOR(AnimationComponent)
    
    void SetAnimationPath(const String& path) { animation_path = path; }
    String GetAnimationPath() const { return animation_path; }
    
    void SetCurrentAnimation(const String& name) { current_animation = name; }
    String GetCurrentAnimation() const { return current_animation; }
    
    void SetPlaybackSpeed(double speed) { playback_speed = speed; }
    double GetPlaybackSpeed() const { return playback_speed; }
    
    void SetTime(double time) { current_time = time; }
    double GetTime() const { return current_time; }
    
    void SetPlaying(bool playing) { is_playing = playing; }
    bool IsPlaying() const { return is_playing; }
    
    void Visit(Vis& v) override {
        v("animation_path", animation_path)
         ("current_animation", current_animation)
         ("playback_speed", playback_speed)
         ("current_time", current_time)
         ("is_playing", is_playing);
    }

private:
    String animation_path;
    String current_animation;
    double playback_speed = 1.0;
    double current_time = 0.0;
    bool is_playing = false;
};

// Component for tagging entities
class TagComponent : public Component {
public:
    ECS_COMPONENT_CTOR(TagComponent)
    
    void SetTag(const String& tag) { tag_value = tag; }
    String GetTag() const { return tag_value; }
    
    void SetGroup(const String& group) { group_value = group; }
    String GetGroup() const { return group_value; }
    
    void Visit(Vis& v) override {
        v("tag", tag_value)("group", group_value);
    }

private:
    String tag_value;
    String group_value;
};

// Component for input handling
class InputComponent : public Component {
public:
    ECS_COMPONENT_CTOR(InputComponent)
    
    void SetInputMode(const String& mode) { input_mode = mode; }
    String GetInputMode() const { return input_mode; }
    
    void SetEnabled(bool enabled) { is_enabled = enabled; }
    bool IsEnabled() const { return is_enabled; }
    
    void Visit(Vis& v) override {
        v("input_mode", input_mode)("is_enabled", is_enabled);
    }

private:
    String input_mode = "default";
    bool is_enabled = true;
};

// Component for UI elements
class UIComponent : public Component {
public:
    ECS_COMPONENT_CTOR(UIComponent)
    
    void SetUIType(const String& type) { ui_type = type; }
    String GetUIType() const { return ui_type; }
    
    void SetPosition(const Point& pos) { position = pos; }
    Point GetPosition() const { return position; }
    
    void SetSize(const Size& size) { this->size = size; }
    Size GetSize() const { return size; }
    
    void SetVisible(bool visible) { is_visible = visible; }
    bool IsVisible() const { return is_visible; }
    
    void Visit(Vis& v) override {
        v("ui_type", ui_type)
         ("position", position)
         ("size", size)
         ("is_visible", is_visible);
    }

private:
    String ui_type = "panel";
    Point position = Point(0, 0);
    Size size = Size(100, 100);
    bool is_visible = true;
};

END_UPP_NAMESPACE

#endif