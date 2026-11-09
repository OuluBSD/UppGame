#ifndef _SweetSandy_PedestrianModel_h_
#define _SweetSandy_PedestrianModel_h_

// Tesseract integration, Task 3D3: the skinned character the pedestrians are
// drawn with - Tesseract's own ogre (`packages/models/ogro2/ogro.iqm` with its
// `idle.iqm` / `run.iqm` clips), loaded once, skinned on the CPU by Rt3D and
// drawn through SweetSandy's own immediate-mode GL.
//
// The model is *data* here and the emission is deliberately thin: Rt3D's
// `Rt3SkelModel` produces the skinned vertex array, and this class only places
// it in the world and walks the element list. That keeps Rt3D free of GL (see
// uppsrc/Rt3D/AGENTS.md) and keeps one skinning implementation for the game
// and for the headless self-test alike.

// One clip: where it starts in the skeleton's keyframe array, how many frames
// it runs, and how fast the exporter wanted it played. `speed` is
// milliseconds per frame, the same unit `Rt3AnimInfo` uses.
struct PedestrianClip
{
	String label;
	int frame = 0, range = 0;
	double speed = 100.0;

	bool IsValid() const { return range > 0; }
};

class PedestrianModel
{
	Rt3SkelModel model;
	PedestrianClip idle, run;
	// One entry per bone, saying which animation part drives it - upstream's
	// `animpartmask`. Every bone belongs to the same (only) part here.
	Vector<byte> partmask;
	// The pose cache one draw shares. There is a single character model, so
	// there is a single pose: every pedestrian in a frame is skinned in turn
	// through this one entry rather than keeping 185 copies of a pose.
	Rt3SkelCacheEntry cache;
	// The skin texture, created on first use because it needs a GL context.
	GLuint skin_tex = 0;
	// The folder the model files were found in - the skin texture sits next to
	// them, and finding it again per frame would be a filesystem hit per
	// pedestrian per frame.
	String model_folder;
	// The model's own scale into meters and the offset that puts its feet on
	// the ground, both derived from the bind pose at load time.
	double scale = 1, ground_offset = 0;
	double model_height = 0;
	bool loaded = false;

	// Finds `ogro.iqm` (plus a clip file) the way the Rt3D test finds them, and
	// returns the folder both live in - the skin texture sits next to them.
	static String FindModelFile(const String& name);

public:
	// Reads the mesh and both clips and lays the mesh out for drawing. No GL
	// happens here, so this is safe from a self-test; `target_height` is how
	// tall the character should stand, in meters. Returns false with a reason
	// when the fork's model files are not on disk - callers are expected to
	// fall back to the old flat figure rather than fail.
	bool Load(double target_height, String& error);

	// Whether `Load` succeeded, i.e. whether there is a real character to draw.
	bool IsValid() const { return loaded; }

	// The mesh as it stands now, for the headless self-test: evaluate `clip`
	// at `millis` and report the skinned bounding box **in meters** (the same
	// units `GetHeight` uses, so the two can be compared). Returns false when
	// the clip is unknown.
	bool Evaluate(const String& clip, dword millis, Vec3& bbmin, Vec3& bbmax);

	// Draws the character standing at `pos` (its y is ignored - the ground
	// height is passed separately), facing `yaw` radians the same way
	// `AIPathFollower::heading` is, walking at `speed` m/s, with the clip at
	// phase `anim_ms` already advanced by the caller. Does nothing when the
	// model failed to load or the clip is missing, so the caller can fall back.
	void DrawGL(Vec3 pos, double ground_y, double yaw, double speed,
	            dword anim_ms, dword phase_offset);

	// The clip for a walking speed: the run clip above `run_threshold`, the
	// idle clip at or below it. Both may be invalid if only one file loaded.
	PedestrianClip ClipForSpeed(double speed, double run_threshold) const;

	int GetVertexCount() const { return model.vlen; }
	// The length of the i-th skinned normal - for the self-test, because a
	// degenerate normal is a silent "the character draws black" failure.
	double GetNormalLength(int i) const { return model.vdata[i].norm.Length(); }
	int GetTriangleCount() const { return model.elems.GetCount() / 3; }
	// How tall the character stands, in meters - the load-time target, kept so
	// the caller can check the collider against it.
	double GetHeight() const { return model_height; }
	// The number of vertices the last `DrawGL` actually transformed, for the
	// LOD diagnostics in the log.
	int GetLastSkinnedCount() const { return last_skinned; }

private:
	int last_skinned = 0;
};

#endif
