#ifndef _Umbrella_ActionSet_h_
#define _Umbrella_ActionSet_h_

// ============================================================================
// ActionSet - flexible KV action map for enemy AI output
//
// Small-buffer optimized: up to INLINE_CAP entries stored inline (no heap).
// Rare overflow (>12 actions) uses a heap-allocated array.
//
// Boolean actions use value = 1.0f.  Analog actions use their float value.
// ============================================================================

enum ActionId {
	ACT_LEFT        = 0,   // Boolean: move left
	ACT_RIGHT       = 1,   // Boolean: move right
	ACT_JUMP        = 2,   // Boolean: press jump
	ACT_ATTACK      = 3,   // Boolean: melee attack
	ACT_SHOOT       = 4,   // Boolean: fire projectile
	ACT_AIM_X       = 5,   // Analog:  shoot direction X (-1..1)
	ACT_AIM_Y       = 6,   // Analog:  shoot direction Y (-1..1)
	ACT_SPAWN_TYPE  = 7,   // Analog:  which enemy type to spawn (cast to EnemyType)
	ACT_SPECIAL_1   = 8,
	ACT_SPECIAL_2   = 9,
	ACT_SPECIAL_3   = 10,
	ACT_SPECIAL_4   = 11,
};

struct ActionSet {
	static constexpr int INLINE_CAP = 12;

	struct KV {
		int   id;
		float value;
	};

	KV  buf[INLINE_CAP];  // 96 bytes inline, zero allocation
	int count    = 0;
	KV* ovf      = nullptr;
	int ovfCount = 0;

	ActionSet()  { count = 0; ovfCount = 0; ovf = nullptr; }
	~ActionSet() { delete[] ovf; }

	// Non-copyable to keep ownership simple; move is fine.
	ActionSet(const ActionSet&)            = delete;
	ActionSet& operator=(const ActionSet&) = delete;
	ActionSet(ActionSet&& o)               { *this = (ActionSet&&)o; }
	ActionSet& operator=(ActionSet&& o) {
		if(this == &o) return *this;
		delete[] ovf;
		for(int i = 0; i < INLINE_CAP; i++) buf[i] = o.buf[i];
		count    = o.count;
		ovf      = o.ovf;
		ovfCount = o.ovfCount;
		o.ovf      = nullptr;
		o.ovfCount = 0;
		o.count    = 0;
		return *this;
	}

	void Set(int id, float v = 1.0f) {
		// Update existing inline entry
		for(int i = 0; i < count; i++) {
			if(buf[i].id == id) { buf[i].value = v; return; }
		}
		// Update existing overflow entry
		for(int i = 0; i < ovfCount; i++) {
			if(ovf[i].id == id) { ovf[i].value = v; return; }
		}
		// Insert new
		if(count < INLINE_CAP) {
			buf[count++] = { id, v };
		} else {
			// Rare overflow: grow by 4
			int newCap = ovfCount + 4;
			KV* n = new KV[newCap];
			for(int i = 0; i < ovfCount; i++) n[i] = ovf[i];
			delete[] ovf;
			ovf = n;
			ovf[ovfCount++] = { id, v };
		}
	}

	float Get(int id, float def = 0.0f) const {
		for(int i = 0; i < count; i++)
			if(buf[i].id == id) return buf[i].value;
		for(int i = 0; i < ovfCount; i++)
			if(ovf[i].id == id) return ovf[i].value;
		return def;
	}

	bool Has(int id) const { return Get(id) != 0.0f; }

	void Clear() {
		count    = 0;
		ovfCount = 0;
		// Keep ovf allocation for reuse; contents stale but ovfCount=0
	}
};

#endif
