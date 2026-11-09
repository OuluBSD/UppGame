#include "SweetSandy.h"

NAMESPACE_UPP

Vector<FenceSegment> ComputeFences(const Vector<RoadLinkEntry>& roads, const Vector<ZoneEntry>& zones, const Terrain& terrain)
{
	Vector<FenceSegment> out;
	// A verge/shoulder beyond the road's own edge, not flush against
	// traffic - the road's own "protected corridor" half-width a zone
	// boundary has to fall inside to count as colliding.
	const double kFenceSetback = 3.0;
	const int kZoneSamples = 72; // every 5 degrees - fine grain enough to trace a real arc, not a claim about needing more

	for(const RoadLinkEntry& r : roads) {
		RoadClassParams cls;
		if(!ParseRoadClass(r.road_class, cls) || !cls.needs_fence)
			continue;
		RoadPathResult path = FindRoadPath(terrain, Vec2(r.x1, r.z1), Vec2(r.x2, r.z2), cls);
		if(!path.ok)
			continue;
		double corridor_half = cls.width * 0.5 + kFenceSetback;

		// "Can have varying width" - scaled by the road's own speed
		// class, same idea RoadClassParams already applies to grade/
		// curve: a slower fenced road (60 km/h, the lowest class with
		// needs_fence at all) gets a thin wall; a fast one gets a wide,
		// low dirt-pile berm instead - a real, more substantial barrier
		// for a more dangerous road, not a uniform fence everywhere.
		String kind = cls.speed_kmh < 80 ? "wall" : "dirtpile";
		double width = cls.speed_kmh < 80 ? 0.4 : (cls.speed_kmh < 100 ? 2.0 : 3.0);
		double height = kind == "wall" ? 1.8 : 1.0;

		for(const ZoneEntry& z : zones) {
			if(!ZoneNeedsProtection(z.type))
				continue;

			// Placed a small distance OUTSIDE the zone's own radius, not
			// exactly on it - found by actually rendering a fence built
			// exactly on MaterializeZoneBoundary's own ring: the two
			// coincided so closely they were hard to tell apart. A real
			// fence sits along the property line, not drawn on top of
			// the line itself.
			const double kFenceOutset = 1.5;
			Vector<Vec2> boundary;
			Vector<bool> flagged;
			boundary.SetCount(kZoneSamples);
			flagged.SetCount(kZoneSamples, false);
			for(int i = 0; i < kZoneSamples; i++) {
				double ang = 2 * M_PI * i / kZoneSamples;
				boundary[i] = Vec2(z.cx + cos(ang) * (z.radius + kFenceOutset), z.cz + sin(ang) * (z.radius + kFenceOutset));
			}
			// "The collision line between road and zone": a boundary
			// sample counts as fenced if it falls within the road's own
			// corridor at ANY non-bridge point along the path - not a
			// claim the road passes exactly there, just that the two
			// are close enough to actually collide.
			for(int i = 0; i < kZoneSamples; i++) {
				for(const RoadPathPoint& pp : path.points) {
					if(pp.bridge)
						continue; // no fence floats out over open water
					if((boundary[i] - pp.pos).Length() <= corridor_half) {
						flagged[i] = true;
						break;
					}
				}
			}

			bool any = false, all = true;
			for(int i = 0; i < kZoneSamples; i++) {
				any = any || flagged[i];
				all = all && flagged[i];
			}
			if(!any)
				continue; // this road never actually gets close to this zone - no fence needed

			if(all) {
				// The road wraps/skirts the whole zone (a tight loop, or
				// a small zone entirely inside the corridor) - one
				// closed-ring fence, same shape as MaterializeZoneBoundary's
				// own ring.
				FenceSegment seg;
				seg.kind = kind; seg.width = width; seg.height = height;
				for(int i = 0; i <= kZoneSamples; i++)
					seg.points.Add(boundary[i % kZoneSamples]);
				out.Add(pick(seg));
				continue;
			}

			// Rotate the logical start to a false->true transition so a
			// real contiguous run is never split across the array's own
			// arbitrary index 0 - guaranteed to exist since any && !all.
			int start = -1;
			for(int i = 0; i < kZoneSamples; i++) {
				if(flagged[i] && !flagged[(i - 1 + kZoneSamples) % kZoneSamples]) {
					start = i;
					break;
				}
			}
			int i = 0;
			while(i < kZoneSamples) {
				int idx = (start + i) % kZoneSamples;
				if(!flagged[idx]) {
					i++;
					continue;
				}
				FenceSegment seg;
				seg.kind = kind; seg.width = width; seg.height = height;
				while(i < kZoneSamples && flagged[(start + i) % kZoneSamples]) {
					seg.points.Add(boundary[(start + i) % kZoneSamples]);
					i++;
				}
				out.Add(pick(seg));
			}
		}
	}
	return out;
}

void MaterializeFences(GeomScene& scene, const Terrain& terrain, const Vector<FenceSegment>& fences)
{
	const Color kWallTint(90, 90, 95);      // dark stone-gray
	const Color kDirtpileTint(120, 85, 50);  // earthy brown
	for(int i = 0; i < fences.GetCount(); i++) {
		const FenceSegment& f = fences[i];
		Color tint = f.kind == "wall" ? kWallTint : kDirtpileTint;
		AddFenceObject(scene, "fence_" + AsString(i), f.points, f.width, f.height, terrain, tint);
	}
}

END_UPP_NAMESPACE
