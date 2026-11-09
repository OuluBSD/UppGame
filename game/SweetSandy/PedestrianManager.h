#ifndef _SweetSandy_PedestrianManager_h_
#define _SweetSandy_PedestrianManager_h_

// AI traffic and pedestrians phase, Task 3: spawns kinematically-moving
// pedestrians near a newly-loaded chunk's own blocks, walks each forever
// around a block's own inset polygon perimeter with Task 1's
// AIPathFollower (closed_loop=true), gives each a static ODE capsule
// collider, and draws a simplified flat-colored figure. See Manager/2-plan/
// .../006-ai-traffic-and-pedestrians/003-task3-ai-pedestrians.md.
//
// Tesseract integration, Task 3D3: the flat figure this used to draw is now
// the *fallback*. A pedestrian within `kPedMeshDistance` of the camera is
// drawn as the real skinned ogre (see PedestrianModel.h), further away as the
// old flat figure - the level of detail is a distance, because there are
// hundreds of pedestrians and the CPU skinning of 539 vertices each is not
// something to do for the ones behind you.
//
// Notably simpler than TrafficManager: no graph, no routing choices, no
// junctions - a block perimeter is already a single closed loop, and
// AIPathFollower's own closed_loop=true mode handles wrapping forever
// with no extra logic needed here at all.

struct Pedestrian : Moveable<Pedestrian> {
	AIPathFollower follower;
	PhysGeomId geom = 0;  // static capsule collider, repositioned every tick
	double speed = 0;     // meters/second, walking pace
	WorldChunkCoord owner_chunk;
	// The character's own animation clock, in milliseconds, advanced by Tick -
	// so pedestrians on the same block are not in lockstep even though they
	// share one model. `anim_phase` is a fixed per-pedestrian offset for the
	// same reason.
	dword anim_ms = 0;
	dword anim_phase = 0;
};

class PedestrianManager {
	PhysSpaceId space = 0;  // not owned - PhysicsWorld's own space
	Terrain *terrain = NULL; // not owned - PlayScreen's own Terrain
	String world_seed;
	Vector<Pedestrian> people;
	// The shared skinned character. Loaded once, in `Create` - which is safe
	// from a self-test because the model itself needs no GL context (only its
	// texture does, and that is created on the first real draw). Invalid when
	// the fork's model files are not on disk, which is exactly when every
	// pedestrian falls back to the flat figure.
	// `mutable` because the draw is a `const` method that still has to write the
	// character's pose cache and create its texture on first use.
	mutable PedestrianModel model;
	// Last frame's LOD split. `mutable` because `DrawGL` is const - it
	// draws and does not move anybody, and these are only diagnostics.
	mutable int drawn_meshed = 0, drawn_flat = 0;
	// One line in the log the first time a real character is actually drawn -
	// the only way to tell from a headless run that the whole path (model
	// loaded, clip sampled, GL emitted) reached a frame.
	mutable bool logged_first_character = false;
	// See SetDrawCharacter. Starts on; `Create` is what actually decides
	// whether a character exists.
	bool draw_character = true;

public:
	void Create(PhysSpaceId new_space, Terrain& new_terrain, const String& new_world_seed);

	// For each block polygon in content.blocks, builds its own inset ring
	// (reusing DrawCityGL's own CityPolygonAverage + inset_fraction, so a
	// pedestrian visibly walks along the *same* line already drawn on the
	// ground) and spawns some number of pedestrians walking it - each
	// starting at a random point along the ring, walking in a random
	// direction (CW or CCW). No-ops if the chunk has no blocks.
	void SpawnForChunk(const WorldChunkCoord& coord, const WorldChunkContent& content);

	// Destroys every pedestrian whose owner_chunk == coord (PhysDestroyGeom
	// each one's collider).
	void DespawnForChunk(const WorldChunkCoord& coord);

	// Draw the flat figure even when the character is loaded. Only for
	// `--pedpic`'s in-frame A/B capture, which needs the same camera, the same
	// frame and the same everything else with the character on and off.
	void SetDrawCharacter(bool on) { draw_character = on; }

	void Tick(double dt);
	// `eye` is the camera position, which is what the distance-based level of
	// detail is measured from.
	void DrawGL(Vec3 eye) const;

	// How the last `DrawGL` split the pedestrians between the real character
	// and the flat fallback, and whether the character is available at all.
	bool HasModel() const { return model.IsValid(); }
	// How tall a pedestrian stands, in meters. With the real character this is
	// the model's own loaded height; without it, the flat figure's own height -
	// so a caller measuring a pedestrian on screen is measuring the thing that
	// was actually drawn.
	double GetPedHeightMeters() const { return model.IsValid() ? model.GetHeight() : 1.7; }
	int GetMeshedDrawn() const { return drawn_meshed; }
	int GetFlatDrawn() const { return drawn_flat; }

	int GetPedestrianCount() const { return people.GetCount(); }
	const Pedestrian& GetPedestrian(int i) const { return people[i]; }
};

#endif
