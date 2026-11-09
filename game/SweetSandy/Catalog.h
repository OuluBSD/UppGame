#ifndef _SweetSandy_Catalog_h_
#define _SweetSandy_Catalog_h_

// Track and vehicle definitions for the "New game" screen. Deliberately a
// flat hardcoded list for now (one entry each) - room for many more later
// without changing how the screen or the catalog accessors work.

// Selects which procedural pattern Terrain generates when no heightmap
// asset file exists yet (see Terrain::Load).
enum TerrainKind {
	TERRAIN_ROLLING,          // gentle bumpy hills (the original demo terrain)
	TERRAIN_OBSTACLE_COURSE,  // ramps, bumps and a jump laid out along +z
	// Island Terrain phase, Task 4: Terrain::Load builds a stateless
	// IslandTerrain instead of a baked grid for this kind - see
	// Terrain.h's own `island` member comment. heightmap_path is
	// ignored; world_size sets IslandTerrainParams::island_radius (half
	// of it) and height_scale sets IslandTerrainParams::height_scale
	// directly (same meaning as for the baked-grid kinds - amplitude in
	// meters), unlike those it's not clamped to a minimum of 1.
	TERRAIN_ISLAND,
};

struct TrackDef : Moveable<TrackDef> {
	String id, name;
	String heightmap_path; // share/sweetsandy/heightmaps/<id>.png, may not exist yet
	double world_size = 200;    // meters, full width/depth of the terrain
	double height_scale = 12;   // meters, black(0) to white(1); TERRAIN_OBSTACLE_COURSE
	                             // ignores this and uses its own meter-scale profile
	TerrainKind kind = TERRAIN_ROLLING;
};

struct VehicleDef : Moveable<VehicleDef> {
	String id, name;
	double chassis_size_x = 1.6;  // meters, across (left-right)
	double chassis_size_z = 3.12; // meters, along (front-back) - +20% per tuning request
	double wheel_radius = 0.45;   // was 0.55, -18%: "renkaat voisi olla vähän pienemmät"
	double wheel_width = 0.35;
	double ride_height = 0.26;   // meters, wheel-hub-to-body-origin height. The body
	                              // origin used to be the (single-box) chassis's
	                              // center; now that the body is two parts with
	                              // local y=0 at part 1's *bottom* (see
	                              // Vehicle::Create), this is directly the car's
	                              // ground clearance. Was 1.4 -> 1.1 -> 0.85 -> 0.68
	                              // (all center-referenced) -> 0.26 (bottom-referenced,
	                              // ~0.38 equivalent minus more per "vieläkin paljon
	                              // matalammalla") - lower center of gravity, less
	                              // tippy, and buys back steering authority (see below)
	double suspension_travel = 0.6;

	// Two-part body: a heavy, low/thin structural "tub" (part 1, the
	// chassis body's local y=0 is its bottom - see Vehicle::Create) and a
	// light shell above it (part 2, mirrors/roof/doors - "lähes olematon"
	// mass), colored differently and wedge-shaped. See Vehicle::Create for
	// how the two are combined into one rigid body (dMassAdd/dMassTranslate)
	// and PlayScreen::GLPaint for how they're drawn. Both get collision
	// geoms now - part 2's lets the shell actually hit the ground on a
	// rollover instead of passing through it ("kuoriosa oli kuin ilmaa").
	// "Painava osa vieläkin paljon matalammalla, kuoriosa ainakin 2x
	// nykyiseen korkeuteen": part1 thinner, part2 much taller, independent
	// of each other now (no longer required to sum to some fixed total).
	double chassis_part1_height = 0.07; // meters - the heavy structural tub, very thin/low
	double chassis_part2_height = 1.0;  // meters - the light shell above it, tall/wedge-shaped
	double chassis_part2_mass = 8;      // kg - "lähes olematon" next to chassis_mass below

	double chassis_mass = 180;   // kg - part 1 (the structural tub); part 2 is separate, above
	double wheel_mass = 18;      // kg, each

	// Hinge2 axis1 anchor softness (see Vehicle.cpp / AGENTS.md for the
	// spring/damper derivation). CFM this close to 1.0 turned out to be
	// numerically unstable - see the "car explodes" incident notes.
	double suspension_erp = 0.3;
	double suspension_cfm = 0.002;

	double max_steer_angle = 0.5; // radians, ~29 degrees - the low-speed/level ceiling;
	                               // see Vehicle::Step for how it's trimmed at speed/tilt
	double max_wheel_spin = 75;   // rad/s; at wheel_radius 0.45 that's ~34 m/s (75 mph) top
	                               // wheel-surface speed - must clear the 60mph (26.8 m/s)
	                               // acceleration target with room to spare, or the car can
	                               // never reach it no matter how much torque is available

	// Speed-sensitive steering (Vehicle::Step): full max_steer_angle is only
	// available below low_speed_full_steer_mps; it's smoothly cut down to
	// steer_high_speed_fraction of max_steer_angle by
	// high_speed_reduced_steer_mps and stays there above that. Real cars do
	// this (speed-proportional steering) because full lock held at speed is
	// what turns a hard corner into a rollover - "renkaat voi kääntyä
	// vähemmän kovassa vauhdissa kuin hitaassa" tuning request. Not yet
	// tuned to a specific target - the 40km/h/full-lock/8s turning test is
	// the thing to re-run after changing any of these.
	double low_speed_full_steer_mps = 4.0;      // ~14 km/h
	double high_speed_reduced_steer_mps = 11.0; // ~40 km/h
	double steer_high_speed_fraction = 0.35;    // loosened from 0.18 - "nopea kääntyminen on
	                                             // liian jäykkä"; the lower ride_height above
	                                             // buys back the stability margin to afford it

	// Stability-based steering cut (Vehicle::Step): once the chassis tilts
	// past stability_steer_upright_full (1.0 = level), steering authority
	// is further reduced, reaching a floor (never fully locked out) at
	// stability_steer_upright_zero - lets an already-leaning car recover
	// instead of a full steering input driving it the rest of the way into
	// a rollover. "kääntyvyyttä voidaan vähentää dynaamisesti kiikkeryyden
	// perusteella" tuning request.
	double stability_steer_upright_full = 0.97; // react to even a small lean
	double stability_steer_upright_zero = 0.8;  // and cut hard well before a real rollover angle

	// dParamFMax2 ceiling (drive torque per wheel). Each wheel's motor
	// torque reacts equally and oppositely onto the chassis (Newton's
	// third law) about the wheels' shared spin axis (world X = pitch) -
	// with all 4 wheels reacting the same way and the motor holding near-
	// max torque throughout the whole spin-up to max_wheel_spin (not just
	// an instant kick), too much torque here doesn't just accelerate the
	// car, it pitches it over backwards like a wheelie taken too far
	// ("kaasua painamalla auto flippaa ympäri" incident) - see
	// Vehicle::throttle_ramped, which also helps by easing the torque on
	// instead of stepping it. 80 hits the "0-60mph in 10-15s" tuning
	// target (--geomtest's RunAccelerationTest: 13.7s) and keeps
	// RunVehicleTerrainSelfTest/RunObstacleCourseSelfTest's upright checks
	// green with the current (longer, lower-riding) chassis. Higher values
	// (tried: 180, 3500) measurably pitch the chassis; re-run --geomtest
	// if you raise it again, and note chassis_size_z/ride_height changes
	// shift how much torque the car can safely take too.
	double drive_force = 160; // 2x, per tuning request ("kiihtyvyys pitäisi olla 2x")
	double steer_force = 400; // dParamFMax ceiling; steering needs far less torque than driving
	double brake_force = drive_force * 1.33; // dParamFMax2 ceiling while braking - "jarrut
	                                          // voisi olla 33% tehokkaammat myös"; declared
	                                          // after drive_force so this default can read it
	                                          // (member initializers run in declaration order)
};

const Vector<TrackDef>&   GetTrackCatalog();
const Vector<VehicleDef>& GetVehicleCatalog();

const TrackDef*   FindTrack(const String& id);
const VehicleDef* FindVehicle(const String& id);

#endif
