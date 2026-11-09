#include "SweetSandy.h"

NAMESPACE_UPP

namespace {

// Person-sized capsule collider - real dimensions decided per the phase
// planning doc (003-task3-ai-pedestrians.md): ~0.3m radius, ~1.7m tall.
// A capsule stands upright along local Z by default (PhysCreateCapsule's
// own convention), so rotate it to local Y the same way Vegetation's own
// tree-trunk cylinders already do (PhysGeomSetRotationAxisAngle around X
// by -pi/2). No body - kinematic, repositioned every tick, matches
// TrafficManager's own car colliders and Terrain's own static heightfield.
const double kPedRadius = 0.3;
const double kPedHeight = 1.7;

// Tesseract integration, Task 3D3: inside this distance of the camera a
// pedestrian is the real skinned ogre, outside it the flat figure. 60 m is
// roughly the point where a 1.7 m figure is a handful of pixels tall, and it
// is also where the 539-vertex CPU skinning stops being worth paying for -
// the whole city's worth of pedestrians is a few hundred per frame, so the
// split is what keeps this from being the most expensive thing in the frame.
const double kPedMeshDistance = 60.0;

const double kMinPedSpeed = 0.8;  // m/s - a leisurely walking pace
const double kMaxPedSpeed = 1.5;

const double kInsetFraction = 0.12; // matches DrawCityGL's own inset, so
                                    // pedestrians walk the same line already drawn

// Distinctly salted deterministic per (world_seed, chunk coord), same idea
// as TrafficManager's DeterministicTrafficSeed - "ped" salt so this
// generator's own RNG stream doesn't start at the same point either of
// those already reseeded to for the same chunk.
dword DeterministicPedSeed(const String& world_seed, const WorldChunkCoord& coord)
{
	hash_t h = CombineHash(world_seed.GetHashValue(), coord.lod, coord.x, coord.y);
	h = CombineHash(h, coord.z);
	h = CombineHash(h, 'p', 'e', 'd'); // "ped", trimmed to 3 chars - CombineHash tops out at 4 total args
	return (dword)h;
}

// Builds the inset ring for one block polygon, reusing DrawCityGL's own
// exact construction (CityPolygonAverage + inset_fraction) so a pedestrian's
// own follower.path bit-identically matches what DrawCityGL draws. See
// PlayScreen.cpp's DrawCityGL for the original.
Vector<Vec2> BuildInsetRing(const Vector<Vec2>& block_poly)
{
	Vector<Vec2> ring;
	if(block_poly.GetCount() < 3)
		return ring;
	Vec2 avg = CityPolygonAverage(block_poly);
	for(Vec2 v : block_poly) {
		Vec2 p = v + (avg - v) * kInsetFraction;
		ring.Add(p);
	}
	return ring;
}

} // anonymous namespace

void PedestrianManager::Create(PhysSpaceId new_space, Terrain& new_terrain, const String& new_world_seed)
{
	space = new_space;
	terrain = &new_terrain;
	world_seed = new_world_seed;
	people.Clear();

	// The real character, loaded once for the whole city. A missing model is
	// not an error: every pedestrian then keeps drawing as the flat figure,
	// which is what this code did before the model existed at all.
	// `RT3_PED_OFF` forces the fallback, so a picture of "what it looked like
	// before" is one env var away - which is how the character draw was checked
	// against the flat figure it replaces (two captures, then a diff).
	if(GetEnv("RT3_PED_OFF") == "1") {
		LOG("PedestrianManager: RT3_PED_OFF=1 - not loading the skinned character");
	} else {
		String error;
		if(!model.Load(kPedHeight, error))
			LOG("PedestrianManager: no skinned character (" << error
			    << ") - drawing the flat figure for every pedestrian");
	}
}

void PedestrianManager::SpawnForChunk(const WorldChunkCoord& coord, const WorldChunkContent& content)
{
	if(content.blocks.IsEmpty())
		return;

	SeedRandom(DeterministicPedSeed(world_seed, coord));

	// Pick a random starting point and direction per pedestrian on each
	// block's own inset ring, so several pedestrians on the same block
	// don't all walk in lockstep. Walk every block, 1-2 pedestrians each.
	for(const Vector<Vec2>& block : content.blocks) {
		Vector<Vec2> ring = BuildInsetRing(block);
		if(ring.GetCount() < 3)
			continue; // degenerate block - can't form a closed loop

		int ped_count = 1 + (int)(Randomf() * 2); // 1 or 2 per block
		for(int i = 0; i < ped_count; i++) {
			// Random starting point along the ring and random direction
			// (CW or CCW), so several pedestrians on the same block don't
			// all walk in lockstep.
			int start_idx = (int)(Randomf() * ring.GetCount());
			bool reverse_dir = Randomf() < 0.5;

			// Build the walking path: rotate so the chosen start point is
			// at index 0, and optionally reverse the traversal direction.
			// For a closed loop (AIPathFollower::closed_loop=true), the
			// implicit closing segment wraps from the last point back to
			// the first via PointAt()'s modulo - so we rotate the ring to
			// put the start at index 0 and the loop naturally wraps.
			Vector<Vec2> path;
			for(int j = 0; j < ring.GetCount(); j++) {
				int idx = start_idx + j;
				if(reverse_dir)
					idx = (start_idx + ring.GetCount() - j) % ring.GetCount();
				else
					idx = (start_idx + j) % ring.GetCount();
				path.Add(ring[idx]);
			}

			Pedestrian ped;
			ped.follower.SetPath(path, true); // closed_loop=true - wraps forever
			ped.speed = kMinPedSpeed + Randomf() * (kMaxPedSpeed - kMinPedSpeed);
			ped.owner_chunk = coord;
			// Each pedestrian's own place in the clip, so a crowd does not move
			// as one body. Derived by **hash, not by the spawn RNG above**:
			// two extra Randomf() calls per pedestrian shift every later random
			// choice in the chunk and silently change the deterministic world -
			// and the traffic self-test's pedestrian count is one of the numbers
			// that must not move. (185 became 182 the moment this was a
			// Randomf() pair, which is how it was caught.)
			hash_t phase_hash = CombineHash(DeterministicPedSeed(world_seed, coord), (dword)i, 'p', 'h');
			ped.anim_phase = (dword)(phase_hash % 4000);
			ped.anim_ms = (dword)((phase_hash >> 12) % 1000);

			ped.geom = PhysCreateCapsule(space, kPedRadius, kPedHeight);
			double ground_y = terrain->HeightAt(ped.follower.position.x, ped.follower.position.y);
			PhysGeomSetRotationAxisAngle(ped.geom, Vec3(1, 0, 0), -M_PI / 2);
			PhysGeomSetPosition(ped.geom, Vec3(ped.follower.position.x, ground_y + kPedHeight * 0.5, ped.follower.position.y));

			people.Add(pick(ped));
		}
	}
}

void PedestrianManager::DespawnForChunk(const WorldChunkCoord& coord)
{
	for(int i = people.GetCount() - 1; i >= 0; i--)
		if(people[i].owner_chunk == coord) {
			PhysDestroyGeom(people[i].geom);
			people.Remove(i);
		}
}

void PedestrianManager::Tick(double dt)
{
	for(Pedestrian& ped : people) {
		double distance = ped.speed * dt;
		ped.follower.Advance(distance);
		// The character's own clock. Advanced by the same wall time the
		// pedestrian moved in, so the walk cycle stays in step with the walk:
		// the clip's playback rate is scaled by the pedestrian's speed in
		// PedestrianModel::DrawGL, so this stays real time.
		ped.anim_ms += (dword)(dt * 1000.0);

		double ground_y = terrain->HeightAt(ped.follower.position.x, ped.follower.position.y);
		PhysGeomSetPosition(ped.geom, Vec3(ped.follower.position.x, ground_y + kPedHeight * 0.5, ped.follower.position.y));

		// Heading drives visual rotation - same convention as TrafficManager:
		// local X is the "forward" axis, -heading rotates it onto world direction.
		// AIPathFollower's heading is atan2(dir.y, dir.x), same as TrafficManager.
		PhysGeomSetRotationAxisAngle(ped.geom, Vec3(0, 1, 0), -ped.follower.heading);
	}
}

void PedestrianManager::DrawGL(Vec3 eye) const
{
	drawn_meshed = drawn_flat = 0;
	glColor3ub(220, 220, 250); // a pale blue-white "pedestrian" color, distinct from roads/buildings/vehicle
	glDisable(GL_LIGHTING);    // unlit, like DrawCityGL's own wireframe
	for(const Pedestrian& ped : people) {
		double ground_y = terrain->HeightAt(ped.follower.position.x, ped.follower.position.y);

		// The real character, when there is one and it is close enough to be
		// worth 539 transformed vertices. `DrawGL` is const (it draws, it does
		// not move anybody), and the LOD counters are only diagnostics, so
		// they are the one thing written through it.
		Vec3 ped_pos(ped.follower.position.x, 0, ped.follower.position.y);
		if(draw_character && model.IsValid() && (ped_pos - eye).Length() <= kPedMeshDistance) {
			// Lighting is off for the flat figure below, so put it back for
			// the character and leave it as DrawCityGL left it afterwards.
			glEnable(GL_LIGHTING);
			model.DrawGL(ped_pos, ground_y, ped.follower.heading, ped.speed, ped.anim_ms, ped.anim_phase);
			drawn_meshed++;
			glDisable(GL_LIGHTING);
			continue;
		}
		drawn_flat++;

		glPushMatrix();
		glTranslated(ped.follower.position.x, ground_y + kPedHeight * 0.5, ped.follower.position.y);
		glRotated(-ped.follower.heading * 180.0 / M_PI, 0, 1, 0);
		// Simple capsule figure: a vertical line for the body + small circles at top/bottom
		const double body_h = kPedHeight * 0.7;
		glBegin(GL_LINES);
		glVertex3d(0, -body_h/2, 0);
		glVertex3d(0, body_h/2, 0);
		glEnd();
		// Head (small circle at top)
		const int segs = 8;
		const double head_r = 0.1;
		glBegin(GL_LINE_LOOP);
		for(int i = 0; i < segs; i++) {
			double a = 2 * M_PI * i / segs;
			glVertex3d(cos(a) * head_r, body_h/2, sin(a) * head_r);
		}
		glEnd();
		// Legs (short lines from body center down)
		glBegin(GL_LINES);
		glVertex3d(0, -body_h/2, 0); glVertex3d(0, -body_h, 0.1);
		glVertex3d(0, -body_h/2, 0); glVertex3d(0, -body_h, -0.1);
		glEnd();
		glPopMatrix();
	}
	glEnable(GL_LIGHTING);
	if(drawn_meshed && !logged_first_character) {
		logged_first_character = true;
		LOG("PedestrianManager: first skinned character drawn ("
		    << model.GetVertexCount() << " verts, " << model.GetTriangleCount()
		    << " tris each, " << drawn_flat << " flat figures beyond the "
		    << Format("%.0f", kPedMeshDistance) << "m LOD distance)");
	}
}

END_UPP_NAMESPACE
