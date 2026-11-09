#include "SweetSandy.h"

NAMESPACE_UPP

void Vehicle::Create(PhysWorldId w, PhysSpaceId s, const VehicleDef& vdef, Vec3 hub_pos)
{
	world = w;
	space = s;
	def = vdef;

	double half_x = def.chassis_size_x * 0.5 + def.wheel_width * 0.5;
	double half_z = def.chassis_size_z * 0.35;

	Vec3 chassis_pos = hub_pos + Vec3(0, def.ride_height, 0);

	chassis = PhysCreateBody(world);
	PhysBodySetPosition(chassis, chassis_pos);

	// Two-part body, both parts positioned relative to the body origin
	// above, which is part 1's *bottom* (not a box center): part 1 is the
	// heavy, thin structural tub; part 2 is the light shell stacked
	// directly on top of it. Combining their masses with
	// PhysMassTranslate+PhysMassAdd (rather than, say, a second body on a
	// fixed joint) is exact for a rigid, non-moving-relative-to-each-other
	// pair, and much simpler/safer than adding another joint.
	double part1_height = def.chassis_part1_height;
	double part2_height = def.chassis_part2_height;
	double part1_center_y = part1_height * 0.5;
	double part2_center_y = part1_height + part2_height * 0.5;

	PhysMass part1_mass = PhysMakeBoxMass(def.chassis_mass, def.chassis_size_x, part1_height, def.chassis_size_z);
	PhysMassTranslate(part1_mass, Vec3(0, part1_center_y, 0));
	PhysMass part2_mass = PhysMakeBoxMass(def.chassis_part2_mass, def.chassis_size_x, part2_height, def.chassis_size_z);
	PhysMassTranslate(part2_mass, Vec3(0, part2_center_y, 0));
	PhysMassAdd(part1_mass, part2_mass); // part1_mass becomes the combined mass
	PhysBodySetMass(chassis, part1_mass);

	// Both parts get a collision geom now: part 2 (the shell) used to be
	// decorative-only and would pass straight through the terrain on a
	// rollover ("kuoriosa oli kuin ilmaa, ettei se törmännyt maahan") - a
	// box approximation of the wedge shape (PlayScreen::GLPaint draws the
	// actual wedge) is enough to make it collide realistically.
	chassis_geom = PhysCreateBox(space, def.chassis_size_x, part1_height, def.chassis_size_z);
	PhysGeomSetBody(chassis_geom, chassis);
	PhysGeomSetOffsetPosition(chassis_geom, Vec3(0, part1_center_y, 0));

	shell_geom = PhysCreateBox(space, def.chassis_size_x, part2_height, def.chassis_size_z);
	PhysGeomSetBody(shell_geom, chassis);
	PhysGeomSetOffsetPosition(shell_geom, Vec3(0, part2_center_y, 0));

	// Wheel hub offsets relative to hub_pos: FL/FR at the front (+z), RL/RR
	// at the back (-z); left is -x, right is +x.
	Vec3 hub_offset[WHEEL_COUNT] = {
		Vec3(-half_x, 0,  half_z), // FL
		Vec3( half_x, 0,  half_z), // FR
		Vec3(-half_x, 0, -half_z), // RL
		Vec3( half_x, 0, -half_z), // RR
	};

	// direction 3 = cylinder axis along body-local Z, matching
	// PhysCreateCylinder's default orientation before the 90-degree
	// rotation below is applied to the whole body (geom + inertia rotate
	// together).
	PhysMass wheel_mass = PhysMakeCylinderMass(def.wheel_mass, 3, def.wheel_radius, def.wheel_width);

	for(int i = 0; i < WHEEL_COUNT; i++) {
		Vec3 p = hub_pos + hub_offset[i];
		wheel[i] = PhysCreateBody(world);
		PhysBodySetPosition(wheel[i], p);
		PhysBodySetRotationAxisAngle(wheel[i], Vec3(0, 1, 0), M_PI / 2); // local Z -> world X
		PhysBodySetMass(wheel[i], wheel_mass);

		wheel_geom[i] = PhysCreateCylinder(space, def.wheel_radius, def.wheel_width);
		PhysGeomSetBody(wheel_geom[i], wheel[i]);

		PhysJointId j = PhysCreateHinge2Joint(world);
		hinge2[i] = j;
		PhysJointAttach(j, chassis, wheel[i]);
		PhysHinge2SetAnchor(j, p);
		PhysHinge2SetAxis1(j, Vec3(0, 1, 0)); // steering + suspension axis (vertical)
		PhysHinge2SetAxis2(j, Vec3(1, 0, 0)); // wheel spin axis (horizontal)

		PhysHinge2SetSuspension(j, def.suspension_erp, def.suspension_cfm);

		if(IsFrontWheel(i))
			PhysHinge2SetStops1(j, -def.max_steer_angle, def.max_steer_angle);
		else
			// Rear wheels do not steer; lock axis1 rotation. Suspension
			// softness is independent (it lives on the anchor constraint,
			// not on this angular limit).
			PhysHinge2SetStops1(j, 0, 0);
	}
}

void Vehicle::Step(const VehicleInput& in, double dt)
{
	// Speed-sensitive + stability-sensitive steering: full lock is only
	// available slow and level. Full lock held at speed is what turns a
	// hard turn into a rollover (see the "40km/h full-left turn" tuning
	// test); this trims the *authority*, not just relies on the driver -
	// same idea as real speed-proportional steering, applied as a hard
	// limit instead of just an assist weighting.
	Vec3 chassis_vel = PhysBodyGetLinearVel(chassis);
	double speed = sqrt(chassis_vel.x * chassis_vel.x + chassis_vel.z * chassis_vel.z);

	double gl[16];
	PhysBodyGetGLMatrix(chassis, gl);
	double upright = gl[5]; // 1 = level, less = tilted - see AGENTS.md

	double speed_span = max(0.01, def.high_speed_reduced_steer_mps - def.low_speed_full_steer_mps);
	double speed_t = Smooth01((speed - def.low_speed_full_steer_mps) / speed_span);
	double speed_factor = 1.0 - speed_t * (1.0 - def.steer_high_speed_fraction);

	double stability_span = max(0.01, def.stability_steer_upright_full - def.stability_steer_upright_zero);
	double stability_t = Smooth01((upright - def.stability_steer_upright_zero) / stability_span);
	double stability_factor = max(0.03, stability_t); // never fully lock the driver out

	double effective_max_steer = def.max_steer_angle * speed_factor * stability_factor;

	double desired_steer = Clampd(in.steer, -1, 1) * effective_max_steer;
	for(int i = 0; i < WHEEL_COUNT; i++) {
		if(!IsFrontWheel(i))
			continue;
		PhysJointId j = hinge2[i];
		PhysHinge2SetStops1(j, -effective_max_steer, effective_max_steer);
		double angle = PhysHinge2GetAngle1(j);
		double vel = Clampd((desired_steer - angle) * 8.0, -4.0, 4.0);
		PhysHinge2SetMotor1(j, vel, def.steer_force);
	}

	double throttle_target = Clampd(in.throttle, -1, 1);
	double brake = Clampd(in.brake, 0, 1);
	// Slew-rate limit: reaches full throttle in ~0.4s instead of one step.
	double max_step = 1.2 * dt;
	throttle_ramped += Clampd(throttle_target - throttle_ramped, -max_step, max_step);
	double throttle = throttle_ramped;

	for(int i = 0; i < WHEEL_COUNT; i++) {
		PhysJointId j = hinge2[i];
		if(brake > 0.01)
			PhysHinge2SetMotor2(j, 0, def.brake_force * brake);
		else
			PhysHinge2SetMotor2(j, -throttle * def.max_wheel_spin, def.drive_force * fabs(throttle));
	}
}

Vec3 Vehicle::GetChassisPosition() const
{
	return PhysBodyGetPosition(chassis);
}

END_UPP_NAMESPACE
