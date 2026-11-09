#ifndef _SweetSandy_PhysBackend_h_
#define _SweetSandy_PhysBackend_h_

// Physics engine abstraction phase: this game's own thin wrapper around
// whichever physics engine it actually links (ODE today) - see
// Manager/2-plan/.../007-physics-abstraction/001-planning.md for the
// full reasoning (a real user request: no game code should touch ODE
// directly, and no game code should need to know or care whether the
// engine's own scalar type is 32- or 64-bit).
//
// This header never includes <ode/ode.h> (or any other physics
// engine's own header) - only PhysBackend.cpp does. Every handle here
// is an opaque pointer to an incomplete struct, the same pattern ODE
// itself already uses for dWorldID/dBodyID/etc. - a real, distinct
// pointer type per concept (so passing a PhysBodyId where a
// PhysGeomId is expected is a compile error), not a shared void*.
// PhysBackend.cpp reinterpret_casts between these and the real ODE
// types at every function's own boundary; nothing outside that one
// file ever needs to.
//
// double (and Vec3 for anything vector-shaped) is used directly
// throughout this API - no separate "PhysReal" type alias. This
// project already uses double/Vec3 pervasively; whatever the engine's
// own scalar type actually is (confirmed float in this build's linked
// ODE - see 006-ai-traffic-and-pedestrians/REPORT.md's Addendum 2)
// only matters inside PhysBackend.cpp's own narrow conversions at each
// individual engine call.
struct PhysWorldOpaque;      typedef PhysWorldOpaque*      PhysWorldId;
struct PhysSpaceOpaque;      typedef PhysSpaceOpaque*      PhysSpaceId;
struct PhysJointGroupOpaque; typedef PhysJointGroupOpaque* PhysJointGroupId;
// Geom/Body/Joint (Tasks 2-3) - forward-declared here since
// PhysNearCallback's own signature already needs PhysGeomId.
struct PhysGeomOpaque; typedef PhysGeomOpaque* PhysGeomId;
struct PhysBodyOpaque; typedef PhysBodyOpaque* PhysBodyId;
struct PhysJointOpaque; typedef PhysJointOpaque* PhysJointId;

// Global engine init/close - call once each, at process start/end.
void PhysInit();
void PhysClose();

// --- World ---
PhysWorldId PhysCreateWorld();
void PhysDestroyWorld(PhysWorldId);
void PhysWorldSetGravity(PhysWorldId, Vec3 gravity);
void PhysWorldSetCFM(PhysWorldId, double cfm);
void PhysWorldQuickStep(PhysWorldId, double dt);

// --- Space ---
// No parent-space parameter: this project only ever creates one
// top-level space per PhysicsWorld (matches actual usage exactly, not
// a missing feature - add one if a real nested-space need ever shows
// up, not speculatively now).
PhysSpaceId PhysCreateSpace();
void PhysDestroySpace(PhysSpaceId);

// Broad-phase collision: calls `callback(user_data, o1, o2)` once per
// potentially-colliding geom pair, mirroring dSpaceCollide's own
// dNearCallback contract exactly - narrow-phase collision and contact
// joint creation happen entirely inside the callback body (Task 3's
// own PhysCollide/PhysCreateContactJoint), not here.
typedef void (*PhysNearCallback)(void *user_data, PhysGeomId o1, PhysGeomId o2);
void PhysSpaceCollide(PhysSpaceId, void *user_data, PhysNearCallback callback);

// Task 4: needed by RunTrafficManagerSelfTest's own leak checks (spawned
// car count vs. geoms actually added/removed) once that test moved off
// raw ODE - not part of Tasks 1-3's own narrower scope, added here when
// an actual consumer needed it rather than speculatively earlier.
int PhysSpaceGetNumGeoms(PhysSpaceId);

// --- Joint group ---
PhysJointGroupId PhysCreateJointGroup();
void PhysDestroyJointGroup(PhysJointGroupId);
void PhysJointGroupEmpty(PhysJointGroupId);

// --- Geoms ---
PhysGeomId PhysCreateBox(PhysSpaceId, double sx, double sy, double sz);
PhysGeomId PhysCreateSphere(PhysSpaceId, double radius);
PhysGeomId PhysCreateCylinder(PhysSpaceId, double radius, double length);
PhysGeomId PhysCreateCapsule(PhysSpaceId, double radius, double length);
PhysGeomId PhysCreatePlane(PhysSpaceId, Vec3 normal, double d);
// Map Editor phase, Task 16: a real ODE ray geom - collides via the
// same PhysCollide every other geom pair already uses, so querying
// "what height is the live collider actually at (x,z)" no longer
// needs a probe sphere positioned at a guessed height (see
// --collidertest's own history - Tasks 7-9 repeatedly re-tuned a
// probe-sphere-depth check that turned out to still be capable of
// silently reporting zero contacts if the terrain rose enough to
// bury the whole probe past ODE's own heightfield-sphere contact
// range - a position-independent raycast has no such failure mode).
// `length` bounds how far along `dir` ODE will search for a hit.
PhysGeomId PhysCreateRay(PhysSpaceId, double length);
// dir need not be pre-normalized. For a hit against geom B via
// PhysCollide(ray, B, ...), the resulting PhysContactGeom::pos is the
// real world-space hit point and ::depth is the distance along the
// ray to it - ODE's own standard ray-collision contract, unchanged by
// this wrapper.
void PhysRaySet(PhysGeomId ray, Vec3 origin, Vec3 dir);
// Frees a geom created by any PhysCreate*/PhysCreateHeightfield call
// above. For a heightfield geom, this also frees its own height data
// (see PhysCreateHeightfield's own comment) - callers never need a
// separate data-destroy step, and never destroy a geom whose owning
// space has already been destroyed (see PhysDestroySpace's own note
// on why that space-teardown path frees heightfield data itself,
// without ever touching the geom).
void PhysDestroyGeom(PhysGeomId);

// Builds a heightfield geom from row-major height data, copied once at
// build time (the caller's own buffer does not need to outlive this
// call). If the caller is rebuilding (replacing an earlier heightfield
// built from the same call site), destroy the previous one via
// PhysDestroyGeom first - PhysBackend does not track "the last
// heightfield you built" on the caller's own behalf; Terrain's own
// hf_geom member already does exactly that bookkeeping.
PhysGeomId PhysCreateHeightfield(PhysSpaceId, const float *height_row_major,
                                  int grid_w, int grid_d,
                                  double world_size_x, double world_size_z,
                                  double height_scale, double offset);

void PhysGeomSetPosition(PhysGeomId, Vec3);
Vec3 PhysGeomGetPosition(PhysGeomId);
void PhysGeomSetRotationAxisAngle(PhysGeomId, Vec3 axis, double angle_rad);
void PhysGeomSetBody(PhysGeomId, PhysBodyId);        // Task 3's own type
void PhysGeomSetOffsetPosition(PhysGeomId, Vec3);    // the vehicle's own two-part chassis
PhysBodyId PhysGeomGetBody(PhysGeomId);
void PhysGeomGetAABB(PhysGeomId, double aabb_out[6]); // [minx,maxx,miny,maxy,minz,maxz]

// --- Mass ---
// Plain double fields, not an opaque handle: callers only ever build
// one via the functions below and hand it straight to PhysBodySetMass
// (Task 3), matching how ODE's own dMass is already used today, but
// keeping this header free of dReal - PhysBackend.cpp converts each
// field individually at the one place (PhysBodySetMass) that needs a
// real dMass.
struct PhysMass {
	double mass = 0;
	double cx = 0, cy = 0, cz = 0;   // center of mass
	double I[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 }; // row-major 3x3 inertia tensor
};

PhysMass PhysMakeBoxMass(double total_mass, double sx, double sy, double sz);
PhysMass PhysMakeCylinderMass(double total_mass, int direction, double radius, double length);
void PhysMassTranslate(PhysMass&, Vec3);
void PhysMassAdd(PhysMass& dst, const PhysMass& src); // dst becomes the combined mass

// --- Bodies ---
PhysBodyId PhysCreateBody(PhysWorldId);
// No PhysDestroyBody: nothing in this project ever destroys a body
// individually - ODE destroys every body owned by a world when that
// world itself is destroyed (PhysicsWorld::Destroy already relies on
// this), so this is a scope match to actual usage, not a missing
// feature.
void PhysBodySetPosition(PhysBodyId, Vec3);
Vec3 PhysBodyGetPosition(PhysBodyId);
void PhysBodySetRotationAxisAngle(PhysBodyId, Vec3 axis, double angle_rad);
Vec3 PhysBodyGetLinearVel(PhysBodyId);
Vec3 PhysBodyGetAngularVel(PhysBodyId);
void PhysBodySetMass(PhysBodyId, const PhysMass&);

// Ready-to-use column-major 4x4 for glMultMatrixd.
void PhysBodyGetGLMatrix(PhysBodyId, double m[16]);

// --- Hinge2 joints ---
// Named Motor1/Motor2/Stops1 (not e.g. SetSteeringMotor), matching
// ODE's own axis-numbered hinge2 concept directly - Vehicle.cpp itself
// is where "axis 1 is steering, axis 2 is wheel spin" gets named
// meaningfully; this wrapper stays a faithful, thin hinge2 wrapper,
// not a car-physics abstraction of its own.
PhysJointId PhysCreateHinge2Joint(PhysWorldId);
void PhysJointAttach(PhysJointId, PhysBodyId body1, PhysBodyId body2);
void PhysHinge2SetAnchor(PhysJointId, Vec3);
void PhysHinge2SetAxis1(PhysJointId, Vec3); // steering + suspension axis
void PhysHinge2SetAxis2(PhysJointId, Vec3); // wheel spin axis
void PhysHinge2SetSuspension(PhysJointId, double erp, double cfm);
void PhysHinge2SetStops1(PhysJointId, double lo, double hi); // steering angle limits
void PhysHinge2SetMotor1(PhysJointId, double vel, double f_max); // steering motor
void PhysHinge2SetMotor2(PhysJointId, double vel, double f_max); // wheel spin/drive motor
double PhysHinge2GetAngle1(PhysJointId);

// --- Contacts ---
struct PhysContactGeom {
	Vec3 pos, normal;
	double depth = 0;
	PhysGeomId g1 = 0, g2 = 0;
};

struct PhysContactSurface {
	double mu = 0;
	double slip1 = 0, slip2 = 0;
	double soft_erp = 0, soft_cfm = 0;
};

// Fills out[0..return value) - mirrors dCollide's own "returns the
// number of contacts found, up to max_contacts" contract, but hides
// the dContact/dContactGeom struct-layout-and-stride trick
// PhysicsWorld's own original NearCallback relied on - PhysBackend.cpp
// does the ODE-facing part of this internally, once, so no caller ever
// needs to know about it.
int PhysCollide(PhysGeomId, PhysGeomId, int max_contacts, PhysContactGeom *out);

// Always uses the one contact surface mode this project has ever
// needed (mu + slip1/2 + soft ERP/CFM + approx1) - PhysContactSurface
// intentionally has no separate "mode" field to set, matching every
// current contact-creating call site.
PhysJointId PhysCreateContactJoint(PhysWorldId, PhysJointGroupId,
                                    const PhysContactGeom&, const PhysContactSurface&);

bool PhysBodiesConnectedByNonContactJoint(PhysBodyId, PhysBodyId);

#endif
