#ifndef _Umbrella_GameEntity_h_
#define _Umbrella_GameEntity_h_

#include <Vfs/Core/Core.h>

using namespace Upp;

// ============================================================================
// GameEntity - VfsValueExt-based base class for all game world objects.
//
// Every entity in the game (Enemy, Treat, Droplet, Pickup, Projectile,
// GrimReaper, Player) derives from this.  An entity is always backed by
// a VfsValue node inside GameScreen::entityRoot, which enables:
//   - Serialisation / deserialisation for free (via Visit)
//   - Introspection / tooling integration
//   - Consistent lifecycle (alive / active flags)
//
// Construction is two-step because VfsValue::Add<T>() only calls T(VfsValue&):
//   T& e = entityRoot.Add<T>("name");
//   e.Init(x, y, ...);
//
// Owner (GameScreen) clears entity arrays BEFORE resetting entityRoot so raw
// pointers are never dangling when the VfsValue tree is destroyed.
// ============================================================================

struct GameEntity : VfsValueExt {
	CLASSTYPE(GameEntity)

	// Common world state – public following U++ plain-struct conventions.
	Rectf   bounds;
	Pointf  velocity;
	int     facing  = 1;    // -1 left, +1 right
	bool    alive   = true;
	bool    active  = true;

	GameEntity(VfsValue& v) : VfsValueExt(v) {}
	virtual ~GameEntity() {}

	// VfsValueExt pure-virtual – no-op until serialisation is wired up.
	void Visit(Vis& s) override {}

	// Accessors
	virtual Rectf  GetBounds()   const { return bounds; }
	Pointf  GetVelocity()        const { return velocity; }
	int     GetFacing()          const { return facing; }
	bool    IsAlive()            const { return alive; }
	bool    IsActive()           const { return active; }

	// Mutators
	void    SetActive(bool a)             { active = a; }
	void    SetBounds(const Rectf& b)     { bounds = b; }
	void    SetVelocity(const Pointf& v_) { velocity = v_; }
	void    Kill()                        { alive = false; active = false; }
};

#endif
