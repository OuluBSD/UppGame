#include "AnimationSystem.h"

NAMESPACE_UPP

// AnimationCurve implementation
AnimationCurve::AnimationCurve() {
}

void AnimationCurve::AddKeyframe(double time, double value) {
	KeyframeValue kf;
	kf.time = time;
	kf.value = value;
	
	// Insert in time-sorted order
	int insert_pos = 0;
	for (int i = 0; i < keyframes.GetCount(); i++) {
		if (keyframes[i].time > time) {
			break;
		}
		insert_pos = i + 1;
	}
	keyframes.Insert(insert_pos, kf);
}

double AnimationCurve::Evaluate(double time) const {
	if (keyframes.IsEmpty()) return 0.0;
	
	// Find the keyframes before and after the requested time
	int after_idx = -1;
	for (int i = 0; i < keyframes.GetCount(); i++) {
		if (keyframes[i].time >= time) {
			after_idx = i;
			break;
		}
	}
	
	if (after_idx == -1) {
		// Time is after the last keyframe, return the last value
		return keyframes.Top().value;
	} else if (after_idx == 0) {
		// Time is before the first keyframe, return the first value
		return keyframes[0].value;
	} else {
		// Interpolate between the two keyframes
		const KeyframeValue& k1 = keyframes[after_idx - 1];
		const KeyframeValue& k2 = keyframes[after_idx];
		
		if (k2.time == k1.time) {
			return k1.value;  // Avoid division by zero
		}
		
		double t = (time - k1.time) / (k2.time - k1.time);
		return k1.value + (k2.value - k1.value) * t;  // Linear interpolation
	}
}

double AnimationCurve::GetDuration() const {
	if (keyframes.IsEmpty()) return 0.0;
	return keyframes.Top().time;
}

// AnimationClip implementation
AnimationClip::AnimationClip() {
}

void AnimationClip::AddKeyframe(const Keyframe& keyframe) {
	// Insert in time-sorted order
	int insert_pos = 0;
	for (int i = 0; i < keyframes.GetCount(); i++) {
		if (keyframes[i].time > keyframe.time) {
			break;
		}
		insert_pos = i + 1;
	}
	keyframes.Insert(insert_pos, keyframe);
}

double AnimationClip::GetDuration() const {
	if (keyframes.IsEmpty()) return 0.0;
	return keyframes.Top().time;
}

Keyframe AnimationClip::SampleAtTime(double time) const {
	if (keyframes.IsEmpty()) {
		return Keyframe();
	}
	
	if (keyframes.GetCount() == 1) {
		return keyframes[0];
	}
	
	// Find the keyframes before and after the requested time
	int after_idx = -1;
	for (int i = 0; i < keyframes.GetCount(); i++) {
		if (keyframes[i].time >= time) {
			after_idx = i;
			break;
		}
	}
	
	if (after_idx == -1) {
		// Time is after the last keyframe, return the last keyframe
		return keyframes.Top();
	} else if (after_idx == 0) {
		// Time is before the first keyframe, return the first keyframe
		return keyframes[0];
	} else {
		// Interpolate between the two keyframes
		const Keyframe& k1 = keyframes[after_idx - 1];
		const Keyframe& k2 = keyframes[after_idx];
		
		if (k2.time == k1.time) {
			return k1;  // Avoid division by zero
		}
		
		double t = (time - k1.time) / (k2.time - k1.time);
		
		// Interpolate position
		Point3 pos = k1.position + (k2.position - k1.position) * t;
		
		// Interpolate rotation using spherical linear interpolation (slerp)
		Quaternion rot = Slerp(k1.rotation, k2.rotation, t);
		
		// Interpolate scale
		Vector3 scl = k1.scale + (k2.scale - k1.scale) * t;
		
		return Keyframe(time, pos, rot, scl);
	}
}

Point3 AnimationClip::InterpolatePosition(double time) const {
	// Implementation for position interpolation between keyframes
	return SampleAtTime(time).position;
}

Quaternion AnimationClip::InterpolateRotation(double time) const {
	// Implementation for rotation interpolation between keyframes using SLERP
	return SampleAtTime(time).rotation;
}

Vector3 AnimationClip::InterpolateScale(double time) const {
	// Implementation for scale interpolation between keyframes
	return SampleAtTime(time).scale;
}

// AnimationState implementation
AnimationState::AnimationState() {
}

void AnimationState::SetClip(std::shared_ptr<AnimationClip> clip) {
	this->clip = clip;
}

void AnimationState::Play() {
	if (clip) {
		state = PlayState::PLAYING;
	}
}

void AnimationState::Pause() {
	if (state == PlayState::PLAYING) {
		state = PlayState::PAUSED;
	}
}

void AnimationState::Stop() {
	state = PlayState::STOPPED;
	current_time = 0.0;
}

void AnimationState::Rewind() {
	current_time = 0.0;
}

void AnimationState::Update(double deltaTime) {
	if (state != PlayState::PLAYING || !clip) {
		return;
	}
	
	double duration = clip->GetDuration();
	if (duration <= 0.0) {
		return;
	}
	
	current_time += deltaTime * speed;
	
	if (current_time > duration) {
		if (looping) {
			current_time = fmod(current_time, duration);
		} else {
			current_time = duration;
			state = PlayState::STOPPED;
		}
	} else if (current_time < 0.0) {
		if (looping) {
			current_time = duration + fmod(current_time, duration);
		} else {
			current_time = 0.0;
			state = PlayState::STOPPED;
		}
	}
}

Keyframe AnimationState::GetCurrentTransform() const {
	if (!clip) {
		return Keyframe();
	}
	
	return clip->SampleAtTime(current_time);
}

// AnimationController implementation
AnimationController::AnimationController() {
}

void AnimationController::AddClip(const String& name, std::shared_ptr<AnimationClip> clip) {
	if (clip) {
		clip->SetName(name);
		clips.Add(name, clip);
	}
}

void AnimationController::Play(const String& name) {
	auto clip = GetClip(name);
	if (clip) {
		if (!current_state) {
			current_state = std::make_shared<AnimationState>();
		}
		current_state->SetClip(clip);
		current_state->Rewind();
		current_state->Play();
	}
}

void AnimationController::Crossfade(const String& name, double duration) {
	auto clip = GetClip(name);
	if (clip && current_state) {
		next_state = std::make_shared<AnimationState>();
		next_state->SetClip(clip);
		next_state->Play();
		
		crossfade_duration = duration;
		crossfade_time = 0.0;
	}
}

void AnimationController::Update(double deltaTime) {
	// Update the current animation
	if (current_state) {
		current_state->Update(deltaTime * time_scale);
	}
	
	// Update the next animation if crossfading
	if (next_state) {
		next_state->Update(deltaTime * time_scale);
		
		// Update crossfade time
		crossfade_time += deltaTime * time_scale;
		if (crossfade_time >= crossfade_duration) {
			// Switch to the new animation
			current_state = next_state;
			next_state = nullptr;
			crossfade_time = 0.0;
			crossfade_duration = 0.0;
		}
	}
}

Keyframe AnimationController::GetCurrentTransform() const {
	if (!current_state) {
		return Keyframe();
	}
	
	Keyframe current_transform = current_state->GetCurrentTransform();
	
	// If crossfading, blend between current and next transform
	if (next_state && crossfade_duration > 0.0) {
		double blend_factor = crossfade_time / crossfade_duration;
		Keyframe next_transform = next_state->GetCurrentTransform();
		
		// Blend position
		Point3 blended_pos = current_transform.position + 
		                    (next_transform.position - current_transform.position) * blend_factor;
		
		// Blend rotation using SLERP
		Quaternion blended_rot = Slerp(current_transform.rotation, next_transform.rotation, blend_factor);
		
		// Blend scale
		Vector3 blended_scale = current_transform.scale + 
		                       (next_transform.scale - current_transform.scale) * blend_factor;
		
		return Keyframe(current_state->GetTime(), blended_pos, blended_rot, blended_scale);
	}
	
	return current_transform;
}

void AnimationController::SetSpeed(double speed) {
	if (current_state) {
		current_state->SetSpeed(speed);
	}
	if (next_state) {
		next_state->SetSpeed(speed);
	}
}

void AnimationController::SetTimeScale(double scale) {
	time_scale = scale;
}

void AnimationController::StopAll() {
	if (current_state) {
		current_state->Stop();
	}
	if (next_state) {
		next_state->Stop();
	}
}

void AnimationController::PauseAll() {
	if (current_state) {
		current_state->Pause();
	}
	if (next_state) {
		next_state->Pause();
	}
}

void AnimationController::ResumeAll() {
	if (current_state) {
		current_state->Play();
	}
	if (next_state) {
		next_state->Play();
	}
}

std::shared_ptr<AnimationClip> AnimationController::GetClip(const String& name) const {
	int idx = clips.Find(name);
	if (idx >= 0) {
		return clips[idx];
	}
	return nullptr;
}

// Skeleton implementation
int Skeleton::FindBone(const String& name) const {
	for (int i = 0; i < bones.GetCount(); i++) {
		if (bones[i].name == name) {
			return i;
		}
	}
	return -1;
}

// SkeletalAnimationClip implementation
SkeletalAnimationClip::SkeletalAnimationClip() {
}

void SkeletalAnimationClip::SetBoneTransforms(double time, const Vector<Matrix4>& transforms) {
	// Find or create the bone keyframe for this time
	BoneKeyframe* keyframe = nullptr;
	for (int i = 0; i < bone_keyframes.GetCount(); i++) {
		if (bone_keyframes[i].time == time) {
			keyframe = &bone_keyframes[i];
			break;
		}
	}
	
	if (!keyframe) {
		// Create a new keyframe
		BoneKeyframe new_keyframe;
		new_keyframe.time = time;
		new_keyframe.transforms = transforms;
		bone_keyframes.Add(new_keyframe);
		
		// Sort by time
		Sort(bone_keyframes, [](const BoneKeyframe& a, const BoneKeyframe& b) {
			return a.time < b.time;
		});
	} else {
		keyframe->transforms = transforms;
	}
}

Vector<Matrix4> SkeletalAnimationClip::GetBoneTransformsAtTime(double time) const {
	if (bone_keyframes.IsEmpty()) {
		return Vector<Matrix4>();
	}
	
	if (bone_keyframes.GetCount() == 1) {
		return bone_keyframes[0].transforms;
	}
	
	// Find the keyframes before and after the requested time
	int after_idx = -1;
	for (int i = 0; i < bone_keyframes.GetCount(); i++) {
		if (bone_keyframes[i].time >= time) {
			after_idx = i;
			break;
		}
	}
	
	if (after_idx == -1) {
		// Time is after the last keyframe, return the last transforms
		return bone_keyframes.Top().transforms;
	} else if (after_idx == 0) {
		// Time is before the first keyframe, return the first transforms
		return bone_keyframes[0].transforms;
	} else {
		// Interpolate between the two keyframes
		const BoneKeyframe& k1 = bone_keyframes[after_idx - 1];
		const BoneKeyframe& k2 = bone_keyframes[after_idx];
		
		if (k2.time == k1.time) {
			return k1.transforms;  // Avoid division by zero
		}
		
		double t = (time - k1.time) / (k2.time - k1.time);
		
		// Linearly interpolate each matrix (this is a simplification - in practice,
		// you'd want to interpolate translation, rotation, and scale separately)
		Vector<Matrix4> result;
		result.SetCount(k1.transforms.GetCount());
		
		for (int i = 0; i < k1.transforms.GetCount(); i++) {
			const Matrix4& m1 = k1.transforms[i];
			const Matrix4& m2 = k2.transforms[i];
			
			for (int r = 0; r < 4; r++) {
				for (int c = 0; c < 4; c++) {
					result[i][r][c] = m1[r][c] + (m2[r][c] - m1[r][c]) * t;
				}
			}
		}
		
		return result;
	}
}

END_UPP_NAMESPACE