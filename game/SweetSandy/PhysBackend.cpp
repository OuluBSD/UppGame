#include <ode/ode.h>
#include "SweetSandy.h"

// The one file in this project allowed to touch ODE directly (Task 4's
// own completion check makes this literal and mechanical - see
// Manager/2-plan/.../007-physics-abstraction/004-task4-remaining-consumers.md).
// Every function below converts between the opaque Phys* handles
// declared in PhysBackend.h and ODE's own real types
// (dWorldID/dSpaceID/dJointGroupID today) via reinterpret_cast, and
// between double/Vec3 and ODE's own dReal (confirmed single-precision
// float in this build - see 006-ai-traffic-and-pedestrians/REPORT.md's
// Addendum 2) via ordinary narrowing/widening conversion at each call.

NAMESPACE_UPP

void PhysInit()
{
	dInitODE2(0);
}

void PhysClose()
{
	dCloseODE();
}

// --- World ---

PhysWorldId PhysCreateWorld()
{
	return (PhysWorldId)dWorldCreate();
}

void PhysDestroyWorld(PhysWorldId w)
{
	dWorldDestroy((dWorldID)w);
}

void PhysWorldSetGravity(PhysWorldId w, Vec3 gravity)
{
	dWorldSetGravity((dWorldID)w, gravity.x, gravity.y, gravity.z);
}

void PhysWorldSetCFM(PhysWorldId w, double cfm)
{
	dWorldSetCFM((dWorldID)w, (dReal)cfm);
}

void PhysWorldQuickStep(PhysWorldId w, double dt)
{
	dWorldQuickStep((dWorldID)w, (dReal)dt);
}

// --- Space ---

PhysSpaceId PhysCreateSpace()
{
	return (PhysSpaceId)dHashSpaceCreate(0);
}

void PhysDestroySpace(PhysSpaceId s)
{
	dSpaceID space = (dSpaceID)s;
	// A heightfield geom's own height data (PhysCreateHeightfield) is
	// not freed by ODE when its geom is destroyed - normally
	// PhysDestroyGeom handles that pairing, but dSpaceDestroy below
	// destroys every geom still in this space directly, bypassing
	// PhysDestroyGeom entirely. Sweep for any live heightfield geoms
	// first and free their attached data here, so a caller (Terrain,
	// in practice) never needs to touch its own hf_geom once the space
	// that owned it might already be gone - doing so would risk
	// destroying an already-destroyed geom.
	int n = dSpaceGetNumGeoms(space);
	for(int i = 0; i < n; i++) {
		dGeomID g = dSpaceGetGeom(space, i);
		if(dGeomGetClass(g) == dHeightfieldClass) {
			dHeightfieldDataID hf_data = (dHeightfieldDataID)dGeomGetData(g);
			if(hf_data)
				dGeomHeightfieldDataDestroy(hf_data);
		}
	}
	dSpaceDestroy(space);
}

// dSpaceCollide's own dNearCallback signature is fixed
// (void(*)(void*, dGeomID, dGeomID)) - it has no room for also
// carrying the caller's real PhysNearCallback pointer alongside their
// own user_data, so this trampoline packs both into a small struct on
// the stack for the duration of one PhysSpaceCollide call and unpacks
// them again in the thunk ODE actually invokes.
struct PhysSpaceCollideThunkData {
	void *user_data;
	PhysNearCallback callback;
};

static void PhysSpaceCollideThunk(void *data, dGeomID o1, dGeomID o2)
{
	PhysSpaceCollideThunkData *t = (PhysSpaceCollideThunkData *)data;
	t->callback(t->user_data, (PhysGeomId)o1, (PhysGeomId)o2);
}

void PhysSpaceCollide(PhysSpaceId s, void *user_data, PhysNearCallback callback)
{
	PhysSpaceCollideThunkData thunk_data;
	thunk_data.user_data = user_data;
	thunk_data.callback = callback;
	dSpaceCollide((dSpaceID)s, &thunk_data, PhysSpaceCollideThunk);
}

int PhysSpaceGetNumGeoms(PhysSpaceId s)
{
	return dSpaceGetNumGeoms((dSpaceID)s);
}

// --- Joint group ---

PhysJointGroupId PhysCreateJointGroup()
{
	return (PhysJointGroupId)dJointGroupCreate(0);
}

void PhysDestroyJointGroup(PhysJointGroupId g)
{
	dJointGroupDestroy((dJointGroupID)g);
}

void PhysJointGroupEmpty(PhysJointGroupId g)
{
	dJointGroupEmpty((dJointGroupID)g);
}

// --- Geoms ---

PhysGeomId PhysCreateBox(PhysSpaceId s, double sx, double sy, double sz)
{
	return (PhysGeomId)dCreateBox((dSpaceID)s, (dReal)sx, (dReal)sy, (dReal)sz);
}

PhysGeomId PhysCreateSphere(PhysSpaceId s, double radius)
{
	return (PhysGeomId)dCreateSphere((dSpaceID)s, (dReal)radius);
}

PhysGeomId PhysCreateRay(PhysSpaceId s, double length)
{
	dGeomID ray = dCreateRay((dSpaceID)s, (dReal)length);
	// A single closest hit, not every intersection along the ray - the
	// only thing every current/anticipated caller wants (matches
	// PhysCollide's own "find contacts" contract, not a full
	// intersection-list API).
	dGeomRaySetClosestHit(ray, 1);
	return (PhysGeomId)ray;
}

void PhysRaySet(PhysGeomId ray, Vec3 origin, Vec3 dir)
{
	Vec3 d = dir;
	double len = sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
	if(len > 1e-9)
		d = Vec3(d.x / len, d.y / len, d.z / len);
	dGeomRaySet((dGeomID)ray, (dReal)origin.x, (dReal)origin.y, (dReal)origin.z,
	            (dReal)d.x, (dReal)d.y, (dReal)d.z);
}

PhysGeomId PhysCreateCylinder(PhysSpaceId s, double radius, double length)
{
	return (PhysGeomId)dCreateCylinder((dSpaceID)s, (dReal)radius, (dReal)length);
}

PhysGeomId PhysCreateCapsule(PhysSpaceId s, double radius, double length)
{
	return (PhysGeomId)dCreateCapsule((dSpaceID)s, (dReal)radius, (dReal)length);
}

PhysGeomId PhysCreatePlane(PhysSpaceId s, Vec3 normal, double d)
{
	return (PhysGeomId)dCreatePlane((dSpaceID)s, (dReal)normal.x, (dReal)normal.y, (dReal)normal.z, (dReal)d);
}

void PhysDestroyGeom(PhysGeomId g)
{
	dGeomID geom = (dGeomID)g;
	// The same-session rebuild path (Terrain::BuildOdeHeightfield
	// calling this explicitly on its own still-valid previous geom,
	// within a space it knows is still alive) - PhysDestroySpace's own
	// sweep covers the other path (the owning space itself being torn
	// down, which never reaches this function at all).
	if(dGeomGetClass(geom) == dHeightfieldClass) {
		dHeightfieldDataID hf_data = (dHeightfieldDataID)dGeomGetData(geom);
		if(hf_data)
			dGeomHeightfieldDataDestroy(hf_data);
	}
	dGeomDestroy(geom);
}

PhysGeomId PhysCreateHeightfield(PhysSpaceId s, const float *height_row_major,
                                  int grid_w, int grid_d,
                                  double world_size_x, double world_size_z,
                                  double height_scale, double offset)
{
	dHeightfieldDataID hf_data = dGeomHeightfieldDataCreate();
	// bCopyHeightData = 1: ODE keeps its own copy, so the caller's
	// buffer does not need to outlive this call.
	dGeomHeightfieldDataBuildSingle(hf_data, height_row_major, 1,
	                                 (dReal)world_size_x, (dReal)world_size_z, grid_w, grid_d,
	                                 (dReal)height_scale, (dReal)offset, 1.0f, 0);
	dGeomID geom = dCreateHeightfield((dSpaceID)s, hf_data, 1);
	// Pairs the geom with its own height data (see PhysDestroyGeom and
	// PhysDestroySpace, the two places that read this back to free it).
	dGeomSetData(geom, hf_data);
	return (PhysGeomId)geom;
}

void PhysGeomSetPosition(PhysGeomId g, Vec3 pos)
{
	dGeomSetPosition((dGeomID)g, (dReal)pos.x, (dReal)pos.y, (dReal)pos.z);
}

Vec3 PhysGeomGetPosition(PhysGeomId g)
{
	const dReal *p = dGeomGetPosition((dGeomID)g);
	return Vec3((double)p[0], (double)p[1], (double)p[2]);
}

void PhysGeomSetRotationAxisAngle(PhysGeomId g, Vec3 axis, double angle_rad)
{
	dQuaternion q;
	dQFromAxisAndAngle(q, (dReal)axis.x, (dReal)axis.y, (dReal)axis.z, (dReal)angle_rad);
	dGeomSetQuaternion((dGeomID)g, q);
}

void PhysGeomSetBody(PhysGeomId g, PhysBodyId b)
{
	dGeomSetBody((dGeomID)g, (dBodyID)b);
}

void PhysGeomSetOffsetPosition(PhysGeomId g, Vec3 pos)
{
	dGeomSetOffsetPosition((dGeomID)g, (dReal)pos.x, (dReal)pos.y, (dReal)pos.z);
}

PhysBodyId PhysGeomGetBody(PhysGeomId g)
{
	return (PhysBodyId)dGeomGetBody((dGeomID)g);
}

void PhysGeomGetAABB(PhysGeomId g, double aabb_out[6])
{
	dReal aabb[6];
	dGeomGetAABB((dGeomID)g, aabb);
	for(int i = 0; i < 6; i++)
		aabb_out[i] = (double)aabb[i];
}

// --- Mass ---

static void ToDMass(const PhysMass& m, dMass& out)
{
	out.mass = (dReal)m.mass;
	out.c[0] = (dReal)m.cx; out.c[1] = (dReal)m.cy; out.c[2] = (dReal)m.cz; out.c[3] = 0;
	for(int i = 0; i < 9; i++)
		out.I[(i / 3) * 4 + (i % 3)] = (dReal)m.I[i]; // dMatrix3 rows are 4-wide (padded)
}

static PhysMass FromDMass(const dMass& m)
{
	PhysMass out;
	out.mass = (double)m.mass;
	out.cx = (double)m.c[0]; out.cy = (double)m.c[1]; out.cz = (double)m.c[2];
	for(int i = 0; i < 9; i++)
		out.I[i] = (double)m.I[(i / 3) * 4 + (i % 3)];
	return out;
}

PhysMass PhysMakeBoxMass(double total_mass, double sx, double sy, double sz)
{
	dMass m;
	dMassSetBoxTotal(&m, (dReal)total_mass, (dReal)sx, (dReal)sy, (dReal)sz);
	return FromDMass(m);
}

PhysMass PhysMakeCylinderMass(double total_mass, int direction, double radius, double length)
{
	dMass m;
	dMassSetCylinderTotal(&m, (dReal)total_mass, direction, (dReal)radius, (dReal)length);
	return FromDMass(m);
}

void PhysMassTranslate(PhysMass& m, Vec3 t)
{
	dMass dm;
	ToDMass(m, dm);
	dMassTranslate(&dm, (dReal)t.x, (dReal)t.y, (dReal)t.z);
	m = FromDMass(dm);
}

void PhysMassAdd(PhysMass& dst, const PhysMass& src)
{
	dMass ddst, dsrc;
	ToDMass(dst, ddst);
	ToDMass(src, dsrc);
	dMassAdd(&ddst, &dsrc);
	dst = FromDMass(ddst);
}

// --- Bodies ---

PhysBodyId PhysCreateBody(PhysWorldId w)
{
	return (PhysBodyId)dBodyCreate((dWorldID)w);
}

void PhysBodySetPosition(PhysBodyId b, Vec3 pos)
{
	dBodySetPosition((dBodyID)b, (dReal)pos.x, (dReal)pos.y, (dReal)pos.z);
}

Vec3 PhysBodyGetPosition(PhysBodyId b)
{
	const dReal *p = dBodyGetPosition((dBodyID)b);
	return Vec3((double)p[0], (double)p[1], (double)p[2]);
}

void PhysBodySetRotationAxisAngle(PhysBodyId b, Vec3 axis, double angle_rad)
{
	dQuaternion q;
	dQFromAxisAndAngle(q, (dReal)axis.x, (dReal)axis.y, (dReal)axis.z, (dReal)angle_rad);
	dBodySetQuaternion((dBodyID)b, q);
}

Vec3 PhysBodyGetLinearVel(PhysBodyId b)
{
	const dReal *v = dBodyGetLinearVel((dBodyID)b);
	return Vec3((double)v[0], (double)v[1], (double)v[2]);
}

Vec3 PhysBodyGetAngularVel(PhysBodyId b)
{
	const dReal *v = dBodyGetAngularVel((dBodyID)b);
	return Vec3((double)v[0], (double)v[1], (double)v[2]);
}

void PhysBodySetMass(PhysBodyId b, const PhysMass& m)
{
	dMass dm;
	ToDMass(m, dm);
	dBodySetMass((dBodyID)b, &dm);
}

void PhysBodyGetGLMatrix(PhysBodyId b, double m[16])
{
	// ODE's dBodyGetRotation is a row-major 3x3 (padded to 3x4);
	// OpenGL's glMultMatrixd wants column-major 4x4 - the standard
	// conversion (same one ODE's own drawstuff helper uses, and the one
	// PlayScreen.cpp's own BodyGLMatrix used before this wrapper).
	const dReal *r = dBodyGetRotation((dBodyID)b);
	const dReal *p = dBodyGetPosition((dBodyID)b);
	m[0] = r[0]; m[1] = r[4]; m[2]  = r[8];  m[3]  = 0;
	m[4] = r[1]; m[5] = r[5]; m[6]  = r[9];  m[7]  = 0;
	m[8] = r[2]; m[9] = r[6]; m[10] = r[10]; m[11] = 0;
	m[12] = p[0]; m[13] = p[1]; m[14] = p[2]; m[15] = 1;
}

// --- Hinge2 joints ---

PhysJointId PhysCreateHinge2Joint(PhysWorldId w)
{
	return (PhysJointId)dJointCreateHinge2((dWorldID)w, 0);
}

void PhysJointAttach(PhysJointId j, PhysBodyId b1, PhysBodyId b2)
{
	dJointAttach((dJointID)j, (dBodyID)b1, (dBodyID)b2);
}

void PhysHinge2SetAnchor(PhysJointId j, Vec3 p)
{
	dJointSetHinge2Anchor((dJointID)j, (dReal)p.x, (dReal)p.y, (dReal)p.z);
}

void PhysHinge2SetAxis1(PhysJointId j, Vec3 a)
{
	dJointSetHinge2Axis1((dJointID)j, (dReal)a.x, (dReal)a.y, (dReal)a.z);
}

void PhysHinge2SetAxis2(PhysJointId j, Vec3 a)
{
	dJointSetHinge2Axis2((dJointID)j, (dReal)a.x, (dReal)a.y, (dReal)a.z);
}

void PhysHinge2SetSuspension(PhysJointId j, double erp, double cfm)
{
	dJointSetHinge2Param((dJointID)j, dParamSuspensionERP, (dReal)erp);
	dJointSetHinge2Param((dJointID)j, dParamSuspensionCFM, (dReal)cfm);
}

void PhysHinge2SetStops1(PhysJointId j, double lo, double hi)
{
	dJointSetHinge2Param((dJointID)j, dParamLoStop, (dReal)lo);
	dJointSetHinge2Param((dJointID)j, dParamHiStop, (dReal)hi);
}

void PhysHinge2SetMotor1(PhysJointId j, double vel, double f_max)
{
	dJointSetHinge2Param((dJointID)j, dParamVel, (dReal)vel);
	dJointSetHinge2Param((dJointID)j, dParamFMax, (dReal)f_max);
}

void PhysHinge2SetMotor2(PhysJointId j, double vel, double f_max)
{
	dJointSetHinge2Param((dJointID)j, dParamVel2, (dReal)vel);
	dJointSetHinge2Param((dJointID)j, dParamFMax2, (dReal)f_max);
}

double PhysHinge2GetAngle1(PhysJointId j)
{
	return (double)dJointGetHinge2Angle1((dJointID)j);
}

// --- Contacts ---

int PhysCollide(PhysGeomId a, PhysGeomId b, int max_contacts, PhysContactGeom *out)
{
	const int CAP = 64; // comfortably above every current call site's own max_contacts (4-8)
	ASSERT_(max_contacts <= CAP, "PhysCollide: max_contacts exceeds PhysBackend.cpp's own internal buffer");
	dContactGeom buf[CAP];
	int n = dCollide((dGeomID)a, (dGeomID)b, min(max_contacts, CAP), buf, sizeof(dContactGeom));
	for(int i = 0; i < n; i++) {
		out[i].pos = Vec3((double)buf[i].pos[0], (double)buf[i].pos[1], (double)buf[i].pos[2]);
		out[i].normal = Vec3((double)buf[i].normal[0], (double)buf[i].normal[1], (double)buf[i].normal[2]);
		out[i].depth = (double)buf[i].depth;
		out[i].g1 = (PhysGeomId)buf[i].g1;
		out[i].g2 = (PhysGeomId)buf[i].g2;
	}
	return n;
}

PhysJointId PhysCreateContactJoint(PhysWorldId w, PhysJointGroupId g,
                                    const PhysContactGeom& cg, const PhysContactSurface& surf)
{
	dContact contact;
	contact.geom.pos[0] = (dReal)cg.pos.x; contact.geom.pos[1] = (dReal)cg.pos.y; contact.geom.pos[2] = (dReal)cg.pos.z;
	contact.geom.normal[0] = (dReal)cg.normal.x; contact.geom.normal[1] = (dReal)cg.normal.y; contact.geom.normal[2] = (dReal)cg.normal.z;
	contact.geom.depth = (dReal)cg.depth;
	contact.geom.g1 = (dGeomID)cg.g1;
	contact.geom.g2 = (dGeomID)cg.g2;
	contact.surface.mode = dContactSlip1 | dContactSlip2 | dContactSoftERP | dContactSoftCFM | dContactApprox1;
	contact.surface.mu = (dReal)surf.mu;
	contact.surface.slip1 = (dReal)surf.slip1;
	contact.surface.slip2 = (dReal)surf.slip2;
	contact.surface.soft_erp = (dReal)surf.soft_erp;
	contact.surface.soft_cfm = (dReal)surf.soft_cfm;
	return (PhysJointId)dJointCreateContact((dWorldID)w, (dJointGroupID)g, &contact);
}

bool PhysBodiesConnectedByNonContactJoint(PhysBodyId b1, PhysBodyId b2)
{
	return dAreConnectedExcluding((dBodyID)b1, (dBodyID)b2, dJointTypeContact) != 0;
}

END_UPP_NAMESPACE
