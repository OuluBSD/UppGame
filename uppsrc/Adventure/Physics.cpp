#include "Adventure.h"
#include "AdventureBindings.h"

namespace Adventure {



// collision detection
void Program::CheckCollisions() {
	auto& global = ctx.global;

	// check for (current room
	if (room_curr.IsNone())
		return;

	// reset hover collisions
	hover_curr_verb			= PyValue();
	hover_curr_default_verb	= PyValue();
	hover_curr_object		= PyValue();
	//hover_curr_sentence	= EscValue();
	hover_curr_arrow		= PyValue();

	// are we in dialog mode?
	if (dialog_curr && dialog_curr.visible) {
		for (Sentence& s : dialog_curr.sentences) {
			if (IsCursorColliding(s))
				hover_curr_sentence = &s;
		}
		// skip remaining collisions
		return;
	}

	// reset zplane info
	ResetZPlanes();

	// check room/object collisions
	PyValue objects = Program::GetProp(room_curr, "objects");
	const Vector<PyValue>& room_arr = objects.GetArray();
	for(PyValue& obj : const_cast<Vector<PyValue>&>(room_arr)) {
		// capture bounds (even for ("invisible", but not untouchable/dependent, objects)
		PyValue c = Program::ClassesPy(obj);
		PyValue dep_on_key = Program::GetProp(obj, "dependent_on");
		PyValue dep_on;
		if(dep_on_key.GetType() == PY_STR) {
			String key_str = dep_on_key.GetStr().ToString();
			dep_on = EscToPyValue(global.Get(key_str, EscValue()));
		}
		if ((c.IsNone() || (!c.IsNone() && !HasFlag(c, "class_untouchable")))
			&& (dep_on.IsNone() // object has a valid dependent state?
			 || Program::GetProp(dep_on, "state") == Program::GetProp(obj, "dependent_on_state"))) {
			int w = Program::PyInt(Program::GetProp(obj, "w"));
			int h = Program::PyInt(Program::GetProp(obj, "h"));
			RecalculateBoundsPy(obj, w*8, h*8, cam.x, cam.y);
		}
		else {
			// reset bounds
			Program::SetProp(obj, "bounds", PyValue());
		}

		if (IsCursorCollidingPy(obj)) {
			// if (highest (or first) object in hover "stack"
			int obj_z = Program::PyInt(Program::GetProp(obj, "z"));
			int hover_curr_object_z = hover_curr_object.GetType() != PY_NONE ? Program::PyInt(Program::GetProp(hover_curr_object, "z")) : 0;
			int max_z = max(obj_z, hover_curr_object_z);
			if (hover_curr_object.IsNone() || max_z == obj_z) {
				hover_curr_object = obj;
			}
		}
		// recalc z-plane
		RecalcZPlanePy(obj);
	}

	PyValue selected_actor = GetSelectedActorPy();

	// check actor collisions
	EscValue actors_esc = global.Get("actors", EscValue());
	PyValue actors = EscToPyValue(actors_esc);
	if (actors.GetType() == PY_LIST) {
		const Vector<PyValue>& actor_arr = actors.GetArray();
		for(const PyValue& actor : actor_arr) {
			PyValue in_room = Program::GetProp(actor, "in_room");
			if (in_room.GetPtr() == room_curr.GetPtr()) {
				RecalculateBoundsPy(actor, (int)Program::PyInt(Program::GetProp(actor, "w"))*8, (int)Program::PyInt(Program::GetProp(actor, "h"))*8, cam.x, cam.y);

				// recalc z-plane
				RecalcZPlanePy(actor);

				// are we colliding (ignore self!)
				if (IsCursorCollidingPy(actor) && actor.GetPtr() != selected_actor.GetPtr())
					hover_curr_object = actor;
			}
		}
	}

	if (selected_actor.GetType() != PY_NONE) {
		// check ui/inventory collisions
		const Vector<PyValue>& verbs_arr = verbs.GetArray();
		for (const PyValue& v : verbs_arr) {
			if (IsCursorCollidingPy(v))
				hover_curr_verb = v;
		}
		const Vector<PyValue>& arrows_arr = ui_arrows.GetArray();
		for (const PyValue& a : arrows_arr) {
			if (IsCursorCollidingPy(a))
				hover_curr_arrow = a;
		}

		// check room/object collisions
		PyValue inventory = Program::GetProp(selected_actor, "inventory");
		if(inventory.GetType() == PY_LIST) {
			const Vector<PyValue>& inv_arr = inventory.GetArray();
			for (const PyValue& obj : inv_arr) {
				if (IsCursorCollidingPy(obj)) {
					hover_curr_object = obj;
					// pickup override for (inventory objects
					if (verb_curr.GetPtr() == V_PICKUP.GetPtr() && Program::GetProp(hover_curr_object, "owner").GetType() != PY_NONE)
						verb_curr = PyValue();
				}
				// check for (disowned objects!
				PyValue owner = Program::GetProp(obj, "owner");
				if (owner.GetPtr() != selected_actor.GetPtr()) {
					PyValue new_inv_list = PyValue::List();
					Vector<PyValue>& new_inv_arr = const_cast<Vector<PyValue>&>(new_inv_list.GetArray());
					for(const PyValue& item : inv_arr) {
						if(item.GetPtr() != obj.GetPtr())
							new_inv_arr.Add(item);
					}
					Program::SetProp(selected_actor, "inventory", new_inv_list);
				}
			}
		}

		// default to walkto (if (nothing set)
		if (verb_curr.IsNone())
			verb_curr = GetVerb(Program::PyInt(verb_default));

		// update "default" verb for (hovered object (if (any)
		hover_curr_default_verb =
			hover_curr_object.GetType() != PY_NONE ? FindDefaultVerb(PyToEscValue(hover_curr_object)) : hover_curr_default_verb;
	}
}



void Program::ResetZPlanes() {
	draw_zplanes.Clear();
	draw_zplanes.SetCount(64*2+1); // -64 to +64
}

void Program::RecalcZPlane(SObj& obj) {
	// calculate the correct z-plane
	// based on obj || x,y pos + elevation
	EscValue y = obj.MapGet("y");
	EscValue h = obj.MapGet("h");
	EscValue z = obj.MapGet("z");
	EscValue offset_y = obj.MapGet("offset_y");
	int idx;
	if (z.IsInt())
		idx = z.GetInt();
	else {
		int hi = h.IsInt() ? h.GetInt() : 0;
		int yi = y.IsInt() ? y.GetInt() : 0;
		int offset_yi = offset_y.IsInt() ? offset_y.GetInt() : 0;
		idx = (int)(yi + (offset_yi ? 0 : hi * 8));
	}
	draw_zplanes[idx].objs.Add(obj);
}

void Program::RecalcZPlanePy(const PyValue& obj) {
	// calculate the correct z-plane (PyValue version)
	// based on obj || x,y pos + elevation
	PyValue y = Program::GetProp(obj, "y");
	PyValue h = Program::GetProp(obj, "h");
	PyValue z = Program::GetProp(obj, "z");
	PyValue offset_y = Program::GetProp(obj, "offset_y");
	int idx;
	if (z.GetType() == PY_INT)
		idx = Program::PyInt(z);
	else {
		int hi = Program::PyInt(h);
		int yi = Program::PyInt(y);
		int offset_yi = Program::PyInt(offset_y);
		idx = (int)(yi + (offset_yi ? 0 : hi * 8));
	}
	draw_zplanes[idx].objs_py.Add(obj);
}

bool Program::IsCellWalkable(int celx, int cely) {
	const uint16* m = map.Begin();
	int img_w = map_sz.cx;
	int img_h = map_sz.cy;
	ASSERT(celx >= 0 && celx < img_w);
	ASSERT(cely >= 0 && cely < img_h);
	int i = (cely * img_w) + celx;
	uint16 tile = *(m + i);

	ASSERT(tile >= 0 && tile < gff.GetCount());
	uint16 flag = *(gff + tile);

	bool r = flag & 1;
	return r;
}


void Program::RecalculateBounds(SObj o, int w, int h, int cam_off_x, int cam_off_y) {
	int x = o("x");
	int y = o("y");

	// offset for (actors?
	if (HasFlag(Classes(o), "class_actor")) {
		int w = o("w");
		int h = o("h");
		int offset_x = x - (w * 8) / 2;
		int offset_y = y - (h * 8) + 1;
		x = offset_x;
		y = offset_y;
		o.MapSet("offset_x", offset_x);
		o.MapSet("offset_y", offset_y);
	}

	EscValue bounds;
	bounds.SetEmptyMap();
	bounds.MapSet("x", x);
	bounds.MapSet("y", y + stage_top);
	bounds.MapSet("x1", x + w - 1);
	bounds.MapSet("y1", y + h + stage_top - 1);
	bounds.MapSet("cam_off_x", cam_off_x);
	bounds.MapSet("cam_off_y", cam_off_y);
	o.MapSet("bounds", bounds);
}

void Program::RecalculateBoundsPy(PyValue o, int w, int h, int cam_off_x, int cam_off_y) {
	int x = PyInt(GetProp(o, "x"));
	int y = PyInt(GetProp(o, "y"));

	// offset for (actors?
	if (HasFlag(ClassesPy(o), "class_actor")) {
		int w = PyInt(GetProp(o, "w"));
		int h = PyInt(GetProp(o, "h"));
		int offset_x = x - (w * 8) / 2;
		int offset_y = y - (h * 8) + 1;
		x = offset_x;
		y = offset_y;
		SetProp(o, "offset_x", PyValue(offset_x));
		SetProp(o, "offset_y", PyValue(offset_y));
	}

	PyValue bounds = PyValue::Dict();
	bounds.SetItem(PyValue("x"), PyValue(x));
	bounds.SetItem(PyValue("y"), PyValue(y + stage_top));
	bounds.SetItem(PyValue("x1"), PyValue(x + w - 1));
	bounds.SetItem(PyValue("y1"), PyValue(y + h + stage_top - 1));
	bounds.SetItem(PyValue("cam_off_x"), PyValue(cam_off_x));
	bounds.SetItem(PyValue("cam_off_y"), PyValue(cam_off_y));
	SetProp(o, "bounds", bounds);
}



//
// a* pathfinding functions
//

double Program::GetHeuristic(Point chk, Point goal) {
	double h = max(abs(goal.x - chk.x), abs(goal.y - chk.y)) + min(abs(goal.x - chk.x), abs(goal.y - chk.y)) * .414;
	return h;
}

void Program::FindPath(Point start, Point goal, Vector<Point>& path) {
	path.SetCount(0);
	
	struct Node {
		Point pt;
		Node* came_from;
		double cost;
		double heuristic;
		
		void Set(Point p, Node* came, double c, double h) {pt = p; came = came_from; cost = c; heuristic = h;}
	};
	ArrayMap<Point, Node> frontier, visited;
	frontier.Add(start).Set(start, 0, 0, GetDistance(start, goal));

	const int frontier_limit = 1000;

	PyValue map_py = Program::GetProp(room_curr, "map");
	const Vector<PyValue>& map_arr = map_py.GetArray();
	int rc_map_x = Program::PyInt(map_arr[0]);
	int rc_map_y = Program::PyInt(map_arr[1]);
	int rc_map_w = Program::PyInt(map_arr[2]);
	int rc_map_h = Program::PyInt(map_arr[3]);
	Node* lowest_dist_node = 0;
	while (!frontier.IsEmpty() && frontier.GetCount() < frontier_limit) {
		double lowest_dist = DBL_MAX;
		lowest_dist_node = 0;
		int lowest_dist_i = -1, i = 0;
		for (Node& n : frontier.GetValues()) {
			bool fast_exit = n.pt == goal;
			if (fast_exit || n.heuristic < lowest_dist) {
				lowest_dist = n.heuristic;
				lowest_dist_node = &n;
				lowest_dist_i = i;
				if (fast_exit)
					break;
			}
			i++;
		}
		
		// pop the last element off a table
		Node& current = *lowest_dist_node;
		visited.Add(current.pt, frontier.Detach(lowest_dist_i));
		
		if (current.pt == goal)
			break;
		
		for (int x = -1; x <= 1; x++) {
			for (int y = -1; y <= 1; y++) {
				if (x == 0 && y == 0) continue;
				
				Point next_pt(current.pt.x + x, current.pt.y + y);
				
				if (   next_pt.x >= rc_map_x && next_pt.x <= rc_map_x + rc_map_w
					&& next_pt.y >= rc_map_y && next_pt.y <= rc_map_y + rc_map_h
					&& IsCellWalkable(next_pt.x, next_pt.y)
					
					// squeeze check for (corners
					&&((abs(x) != abs(y))
						|| IsCellWalkable(next_pt.x, current.pt.y)
						|| IsCellWalkable(next_pt.x - x, next_pt.y)
						|| enable_diag_squeeze)) {
					// process a valid neighbor
					double new_cost = current.cost + (x * y == 0 ? 1.0 : 1.414); // diagonals cost more
					
					int next_i = visited.Find(next_pt);
					Node* next = next_i >= 0 ? &visited[next_i] : 0;
					if (!next || new_cost < next->cost) {
						if (next)
							frontier.Add(next_pt, visited.Detach(next_i));
						else
							next = &frontier.Add(next_pt);
						next->pt = next_pt;
						next->cost = new_cost;
						next->came_from = &current;
						next->heuristic = new_cost + GetHeuristic(next_pt, goal);
					}
				}
			}
		}
	}
	if (!lowest_dist_node)
		return;
	
	int i = 0;
	Node* n = lowest_dist_node;
	while (n) {
		path.Add(n->pt);
		n = n->came_from;
		if (++i >= 10000) {
			ASSERT(0);
			path.SetCount(0);
			return;
		}
	}
	int c = path.GetCount();
	int c_2 = c / 2;
	for(int i = 0; i < c_2; i++)
		Swap(path[i], path[c-1-i]);
	
}


// collision check
bool Program::IsCursorColliding(const Sentence& obj) {
	// check params / not in cutscene
	if (cutscene_curr) return false;
	const Bounds& b = obj.bounds;
	return !((cursor_x + b.cam_off_x > b.x1 || cursor_x + b.cam_off_x < b.x) || (cursor_y > b.y1 || cursor_y < b.y));
}

bool Program::IsCursorColliding(const SObj& obj) {
	// check params / not in cutscene
	EscValue bounds = obj("bounds");
	if (!bounds || cutscene_curr) return false;

	int cam_off_x = bounds("cam_off_x");
	int x1 = bounds("x1");
	int y1 = bounds("y1");
	int x = bounds("x");
	int y = bounds("y");

	return !((cursor_x + cam_off_x > x1 || cursor_x + cam_off_x < x) || (cursor_y > y1 || cursor_y < y));
}

bool Program::IsCursorCollidingPy(const PyValue& obj) {
	// check params / not in cutscene
	PyValue bounds = Program::GetProp(obj, "bounds");
	if (bounds.IsNone() || cutscene_curr) return false;

	int cam_off_x = Program::PyInt(Program::GetProp(bounds, "cam_off_x"));
	int x1 = Program::PyInt(Program::GetProp(bounds, "x1"));
	int y1 = Program::PyInt(Program::GetProp(bounds, "y1"));
	int x = Program::PyInt(Program::GetProp(bounds, "x"));
	int y = Program::PyInt(Program::GetProp(bounds, "y"));

	return !((cursor_x + cam_off_x > x1 || cursor_x + cam_off_x < x) || (cursor_y > y1 || cursor_y < y));
}


}
