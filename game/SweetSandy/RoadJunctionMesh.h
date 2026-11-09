#ifndef _SweetSandy_RoadJunctionMesh_h_
#define _SweetSandy_RoadJunctionMesh_h_

// Road Network phase, Task 3: fills the gap at a junction where two or
// more roads meet - see Manager/2-plan/.../004-road-network/
// 003-task3-junction-mesh.md. Deliberately NOT a port of RoadCreator's
// Intersection.CreateMainMesh (its curve-point-blended pad is deeply
// bound to RoadCreator's own interactive per-connection "curve point",
// which a CityGraph node generated from streamlines has no equivalent
// of - see 001-planning.md's "What gets simplified, and why"). A
// simpler, purely-geometric fan instead: "good enough to drive over
// without a gap", not a claim of matching real-world intersection curb
// geometry.
struct RoadJunctionApproach : Moveable<RoadJunctionApproach> {
	Vec2 left;  // the connected road's own ribbon (RoadMeshBuilder::BuildRibbon)
	Vec2 right; // edge vertex pair nearest this junction
};

class RoadJunctionMesh {
public:
	// centre is the junction's own position (a CityGraphNode's pos).
	// Sorts approaches by angle internally - caller order doesn't
	// matter. A single approach (a dead end) gets a simple closed end
	// cap instead of a fan (nothing to fan around); zero approaches
	// returns an empty mesh (shouldn't happen for a well-formed graph -
	// every node has at least one edge by construction - but guarded
	// rather than asserted, since this is pure geometry with no graph
	// invariant of its own to enforce).
	static RoadMesh Build(Vec2 centre, const Vector<RoadJunctionApproach>& approaches);
};

#endif
