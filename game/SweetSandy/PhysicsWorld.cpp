#include "SweetSandy.h"

NAMESPACE_UPP

// Physics Abstraction Task 3: finishes the migration Task 1 left
// partial - no more raw dContact/dGeomGetBody/dCollide/
// dJointCreateContact/dAreConnectedExcluding/dJointAttach anywhere in
// this file, only PhysBackend's own equivalents.
static void NearCallback(void *data, PhysGeomId o1, PhysGeomId o2)
{
	PhysicsWorld *pw = (PhysicsWorld *)data;
	PhysBodyId b1 = PhysGeomGetBody(o1);
	PhysBodyId b2 = PhysGeomGetBody(o2);
	if(b1 && b2 && PhysBodiesConnectedByNonContactJoint(b1, b2))
		return;

	const int MAX_CONTACTS = 8;
	PhysContactGeom contact[MAX_CONTACTS];
	int n = PhysCollide(o1, o2, MAX_CONTACTS, contact);

	PhysContactSurface surface;
	surface.mu = 3.0;
	surface.slip1 = surface.slip2 = 0.01;
	surface.soft_erp = 0.6;
	surface.soft_cfm = 0.003;

	for(int i = 0; i < n; i++) {
		PhysJointId c = PhysCreateContactJoint(pw->world, pw->contacts, contact[i], surface);
		PhysJointAttach(c, b1, b2);
	}
}

void PhysicsWorld::Create()
{
	world = PhysCreateWorld();
	space = PhysCreateSpace();
	contacts = PhysCreateJointGroup();
	PhysWorldSetGravity(world, Vec3(0, -9.81, 0));
	PhysWorldSetCFM(world, 1e-5);
}

void PhysicsWorld::Destroy()
{
	if(contacts) PhysDestroyJointGroup(contacts);
	if(space)    PhysDestroySpace(space);
	if(world)    PhysDestroyWorld(world);
	contacts = 0;
	space = 0;
	world = 0;
}

void PhysicsWorld::Step(double dt)
{
	PhysJointGroupEmpty(contacts);
	PhysSpaceCollide(space, this, NearCallback); // NearCallback's own body: unchanged, still raw ODE - see its comment above
	PhysWorldQuickStep(world, dt);
}

END_UPP_NAMESPACE
