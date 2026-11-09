#ifndef _SweetSandy_Vehicle_h_
#define _SweetSandy_Vehicle_h_

// Driver input, -1..1 (brake is 0..1).
struct VehicleInput {
	double throttle = 0;
	double brake = 0;
	double steer = 0;
};

// Box chassis + 4 cylinder wheels on hinge2 joints with a deliberately
// soft, bouncy suspension, expressed entirely through PhysBackend
// (Physics Abstraction Task 3) rather than ODE directly. Mirrors the
// classic ODE "buggy" wheel-joint trick (steering on axis1, spin on
// axis2, suspension as soft ERP/CFM on the axis1 anchor row) - see
// Task 5 in the plan for the reasoning.
//
// Vehicle does not own world/space: the caller (PlayScreen) creates and
// destroys those, and destroying the physics world/space also destroys
// every body/joint/geom created here.
class Vehicle {
public:
	enum { WHEEL_FL, WHEEL_FR, WHEEL_RL, WHEEL_RR, WHEEL_COUNT };

private:
	PhysWorldId world = 0;
	PhysSpaceId space = 0;

	VehicleDef def;

	PhysBodyId chassis = 0;
	PhysGeomId chassis_geom = 0; // part 1, the structural tub
	PhysGeomId shell_geom = 0;   // part 2, a box approximation of the wedge shell for collision

	PhysBodyId wheel[WHEEL_COUNT] = { 0, 0, 0, 0 };
	PhysGeomId wheel_geom[WHEEL_COUNT] = { 0, 0, 0, 0 };
	PhysJointId hinge2[WHEEL_COUNT] = { 0, 0, 0, 0 };

	// Slew-rate-limited throttle actually applied to the wheel motors (see
	// Step()). Slamming the full drive torque on in one step is what caused
	// the "kaasua painamalla auto flippaa ympäri" incident: all 4 wheels'
	// motor reaction torque lands on the chassis about the same (pitch)
	// axis, and an instant torque step is far more of a shock than the same
	// torque ramped on like a real right foot would.
	double throttle_ramped = 0;

	static bool IsFrontWheel(int i) { return i == WHEEL_FL || i == WHEEL_FR; }

public:
	// hub_pos is the y=wheel-hub-height point the vehicle spawns at (x/z is
	// the vehicle's center); the chassis is placed def.ride_height above it.
	void Create(PhysWorldId world, PhysSpaceId space, const VehicleDef& def, Vec3 hub_pos);

	void Step(const VehicleInput& in, double dt);

	PhysBodyId GetChassisBody() const { return chassis; }
	PhysGeomId GetChassisGeom() const { return chassis_geom; } // part 1's actual collision geom - for scale diagnostics (main.cpp's RunCityScaleDiagnostic) that need to cross-check against the *live* geometry, not just VehicleDef's numbers
	PhysBodyId GetWheelBody(int i) const { return wheel[i]; }

	Vec3 GetChassisPosition() const;
	// Full body footprint: x/z as chassis_size_x/z, y as the combined
	// part1+part2 height (the body origin is part 1's bottom - see
	// Vehicle::Create - not this box's center, unlike a plain single box).
	Vec3 GetChassisSize() const {
		return Vec3(def.chassis_size_x, def.chassis_part1_height + def.chassis_part2_height, def.chassis_size_z);
	}
	double GetWheelRadius() const { return def.wheel_radius; }
	double GetWheelWidth() const { return def.wheel_width; }

	// Full def, e.g. for PlayScreen to draw the two-part body (see
	// chassis_part1_height/chassis_part2_height/chassis_part2_mass) without
	// Vehicle needing a getter for every individual dimension.
	const VehicleDef& GetDef() const { return def; }
};

#endif
