#ifndef UPP_ANIMATIONSYSTEM_H
#define UPP_ANIMATIONSYSTEM_H

#include <Core/Core.h>
#include <Draw/Draw.h>
#include <GameLib/GameLib.h>
#include <Geometry/Geometry.h>
#include <GameEngine/VFS.h>  // For asset loading

NAMESPACE_UPP

// Forward declarations
class AnimationClip;
class AnimationState;
class AnimationController;

// Keyframe for animation
struct Keyframe {
	double time = 0.0;
	Point3 position = Point3(0, 0, 0);
	Quaternion rotation = Quaternion(0, 0, 0, 1);  // Identity rotation
	Vector3 scale = Vector3(1, 1, 1);
	
	Keyframe() = default;
	Keyframe(double t, const Point3& pos, const Quaternion& rot = Quaternion(0, 0, 0, 1), const Vector3& scl = Vector3(1, 1, 1))
		: time(t), position(pos), rotation(rot), scale(scl) {}
};

// Animation curve for interpolating values
class AnimationCurve {
public:
	AnimationCurve();
	virtual ~AnimationCurve() = default;
	
	// Add a keyframe
	void AddKeyframe(double time, double value);
	
	// Evaluate the curve at a specific time
	double Evaluate(double time) const;
	
	// Get the duration of the animation
	double GetDuration() const;
	
private:
	struct KeyframeValue {
		double time;
		double value;
	};
	
	Vector<KeyframeValue> keyframes;
};

// Animation clip containing a sequence of keyframes
class AnimationClip : public Moveable<AnimationClip> {
public:
	AnimationClip();
	virtual ~AnimationClip() = default;
	
	// Add a keyframe to the animation
	void AddKeyframe(const Keyframe& keyframe);
	
	// Get the duration of the animation
	double GetDuration() const;
	
	// Sample the animation at a specific time
	Keyframe SampleAtTime(double time) const;
	
	// Getters
	const Vector<Keyframe>& GetKeyframes() const { return keyframes; }
	const String& GetName() const { return name; }
	void SetName(const String& n) { name = n; }
	
private:
	Vector<Keyframe> keyframes;
	String name;
	
	// Interpolation helper
	Point3 InterpolatePosition(double time) const;
	Quaternion InterpolateRotation(double time) const;
	Vector3 InterpolateScale(double time) const;
};

// Animation state for playing an animation clip
class AnimationState {
public:
	AnimationState();
	virtual ~AnimationState() = default;
	
	// Set the animation clip to play
	void SetClip(std::shared_ptr<AnimationClip> clip);
	
	// Play control
	void Play();
	void Pause();
	void Stop();
	void Rewind();
	
	// Update the animation state
	void Update(double deltaTime);
	
	// Getters
	std::shared_ptr<AnimationClip> GetClip() const { return clip; }
	double GetTime() const { return current_time; }
	double GetSpeed() const { return speed; }
	bool IsPlaying() const { return state == PlayState::PLAYING; }
	bool IsPaused() const { return state == PlayState::PAUSED; }
	bool IsStopped() const { return state == PlayState::STOPPED; }
	
	// Setters
	void SetSpeed(double s) { speed = s; }
	void SetLooping(bool loop) { looping = loop; }
	void SetTime(double time) { current_time = time; }
	
	// Get the current transform from the animation
	Keyframe GetCurrentTransform() const;
	
private:
	enum class PlayState { STOPPED, PLAYING, PAUSED };
	
	std::shared_ptr<AnimationClip> clip;
	PlayState state = PlayState::STOPPED;
	double current_time = 0.0;
	double speed = 1.0;
	bool looping = true;
};

// Animation controller for managing multiple animation states
class AnimationController {
public:
	AnimationController();
	virtual ~AnimationController() = default;
	
	// Add an animation clip to the controller
	void AddClip(const String& name, std::shared_ptr<AnimationClip> clip);
	
	// Play an animation by name
	void Play(const String& name);
	
	// Crossfade between animations
	void Crossfade(const String& name, double duration);
	
	// Update all active animations
	void Update(double deltaTime);
	
	// Get the current transform from all active animations
	// This returns the blended result of all active animations
	Keyframe GetCurrentTransform() const;
	
	// Animation control
	void SetSpeed(double speed);
	void SetTimeScale(double scale);
	void StopAll();
	void PauseAll();
	void ResumeAll();
	
	// Getters
	std::shared_ptr<AnimationState> GetCurrentState() const { return current_state; }
	std::shared_ptr<AnimationClip> GetClip(const String& name) const;
	
private:
	// Map of animation clips by name
	VectorMap<String, std::shared_ptr<AnimationClip>> clips;
	
	// Current playing state
	std::shared_ptr<AnimationState> current_state;
	
	// Crossfade state
	std::shared_ptr<AnimationState> next_state;
	double crossfade_duration = 0.0;
	double crossfade_time = 0.0;
	
	// Blending
	double time_scale = 1.0;
};

// Skeletal animation classes
struct Bone {
	String name;
	int parent_index = -1;  // -1 for root bone
	Matrix4 local_transform = Matrix4::Identity();
	Matrix4 global_transform = Matrix4::Identity();
	Matrix4 offset_matrix = Matrix4::Identity();
};

struct Skeleton {
	Vector<Bone> bones;
	
	// Find bone by name
	int FindBone(const String& name) const;
};

// Skeletal animation clip
class SkeletalAnimationClip : public AnimationClip {
public:
	SkeletalAnimationClip();
	virtual ~SkeletalAnimationClip() = default;
	
	// Set bone transforms for a specific time
	void SetBoneTransforms(double time, const Vector<Matrix4>& transforms);
	
	// Get bone transforms at a specific time
	Vector<Matrix4> GetBoneTransformsAtTime(double time) const;
	
private:
	struct BoneKeyframe {
		double time;
		Vector<Matrix4> transforms;
	};
	
	Vector<BoneKeyframe> bone_keyframes;
};

END_UPP_NAMESPACE

#endif