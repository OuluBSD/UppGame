#ifndef _SweetSandy_PhysicsWorld_h_
#define _SweetSandy_PhysicsWorld_h_

// Thin owner of one ODE world/space/contact-group plus the standard
// near-callback -> dContact -> dWorldQuickStep loop. Shared by the headless
// selftest (main.cpp) and PlayScreen (Task 6) so both step physics the
// same way.
struct PhysicsWorld {
	PhysWorldId      world = 0;
	PhysSpaceId      space = 0;
	PhysJointGroupId contacts = 0;

	void Create();
	void Destroy();

	// dSpaceCollide + contact joints + dWorldQuickStep(dt) + dJointGroupEmpty.
	void Step(double dt);
};

#endif
