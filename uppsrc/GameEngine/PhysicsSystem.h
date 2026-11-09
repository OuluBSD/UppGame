#ifndef UPP_PHYSICSSYSTEM_H
#define UPP_PHYSICSSYSTEM_H

#include <Core/Core.h>
#include <GameLib/GameLib.h>
#include <api/Physics/Physics.h>  // Physics API includes
#include <Geometry/Geometry.h>    // For vector math

NAMESPACE_UPP

// Forward declarations
class PhysicsBody;
class PhysicsShape;

// Physics system class that integrates with the Physics API
class PhysicsSystem {
public:
	PhysicsSystem();
	virtual ~PhysicsSystem();
	
	// Initialize the physics system
	bool Initialize();
	void Uninitialize();
	
	// Update physics simulation
	void Update(double dt);
	
	// Create physics objects
	std::shared_ptr<PhysicsBody> CreateBody(const Point3& position = Point3(0, 0, 0));
	std::shared_ptr<PhysicsShape> CreateBoxShape(double width, double height, double depth);
	std::shared_ptr<PhysicsShape> CreateSphereShape(double radius);
	std::shared_ptr<PhysicsShape> CreateCapsuleShape(double radius, double length);
	
	// Set global physics properties
	void SetGravity(const Vector3& gravity);
	Vector3 GetGravity() const { return gravity; }
	
	// Direct access to physics system
#ifdef flagODE
	OdeSystem* GetOdeSystem() { return &ode_system; }
	const OdeSystem* GetOdeSystem() const { return &ode_system; }
#endif
	
	// Check if physics system is initialized
	bool IsInitialized() const { return initialized; }

private:
	bool initialized = false;
	Vector3 gravity = Vector3(0, -9.81, 0);  // Earth gravity by default
	
	// Physics engine implementation
#ifdef flagODE
	OdeSystem ode_system;
#endif
	
	// Internal update
	void ProcessPhysicsEvents();
};

// Physics shape class
class PhysicsShape : public Moveable<PhysicsShape> {
public:
	PhysicsShape();
	virtual ~PhysicsShape();
	
	// Getters
	double GetMass() const { return mass; }
	void SetMass(double m) { mass = m; }
	
	// Shape data
	const String& GetShapeType() const { return shape_type; }
	
private:
	String shape_type;  // "box", "sphere", "capsule", etc.
	double mass = 1.0;
	
	friend class PhysicsSystem;  // Allow PhysicsSystem to set private members
};

// Physics body class
class PhysicsBody {
public:
	PhysicsBody(const Point3& position = Point3(0, 0, 0));
	virtual ~PhysicsBody();
	
	// Transform operations
	void SetPosition(const Point3& pos);
	Point3 GetPosition() const;
	
	void SetRotation(const Quaternion& rot);
	Quaternion GetRotation() const;
	
	void SetTransform(const Point3& pos, const Quaternion& rot);
	void GetTransform(Point3& pos, Quaternion& rot) const;
	
	// Velocity operations
	void SetLinearVelocity(const Vector3& vel);
	Vector3 GetLinearVelocity() const;
	
	void SetAngularVelocity(const Vector3& vel);
	Vector3 GetAngularVelocity() const;
	
	// Force operations
	void ApplyForce(const Vector3& force, const Point3& pos);
	void ApplyImpulse(const Vector3& impulse, const Point3& pos);
	void ApplyTorque(const Vector3& torque);
	
	// Properties
	void SetMass(double mass);
	double GetMass() const;
	
	void SetKinematic(bool kinematic);
	bool IsKinematic() const;
	
	// Attach a shape to this body
	void AddShape(std::shared_ptr<PhysicsShape> shape, const Point3& offset = Point3(0, 0, 0));
	
	// Access to native physics object if available
#ifdef flagODE
	dBodyID GetOdeBodyId() const { return body_id; }
#endif

private:
#ifdef flagODE
	dBodyID body_id = 0;
#endif
	bool kinematic = false;
	
	Point3 position = Point3(0, 0, 0);
	Quaternion rotation = Quaternion(0, 0, 0, 1);  // Identity quaternion
	
	friend class PhysicsSystem;  // Allow PhysicsSystem to access private members
};

END_UPP_NAMESPACE

#endif