#include "Umbrella.h"
#include "Pickup.h"

using namespace Upp;

void Pickup::Init(float x, float y, PickupType t)
{
	type = t;
	active = true;
	bobTimer = 0.0f;
	bounds = Rectf(x, y, x + SIZE, y + SIZE);
}

void Pickup::Update(float delta)
{
	if(!active) return;
	bobTimer += delta * BOB_SPEED;
}

void Pickup::Render(Draw& w, Player::CoordinateConverter& coords)
{
	if(!active) return;

	float bobOffset = (float)sin(bobTimer) * BOB_AMP;

	Rectf displayed = Rectf(
		bounds.left,
		bounds.top  + bobOffset,
		bounds.right,
		bounds.bottom + bobOffset
	);

	Point tl = coords.WorldToScreen(Point((int)displayed.left,  (int)min(displayed.top, displayed.bottom)));
	Point br = coords.WorldToScreen(Point((int)displayed.right, (int)max(displayed.top, displayed.bottom)));

	if(br.x <= tl.x) br.x = tl.x + 1;
	if(br.y <= tl.y) br.y = tl.y + 1;

	Color col;
	switch(type) {
		case PU_HEART:      col = Color(220, 30,  30);  break;  // red
		case PU_GEM:        col = Color(0,   200, 220); break;  // cyan
		case PU_LIGHTNING:  col = Color(255, 220, 0);   break;  // yellow
		case PU_SPEED:      col = Color(255, 140, 0);   break;  // orange
		default:            col = White();               break;
	}

	w.DrawRect(tl.x, tl.y, br.x - tl.x, br.y - tl.y, col);

	// Small white highlight in top-left corner
	int hw = max(1, (br.x - tl.x) / 3);
	int hh = max(1, (br.y - tl.y) / 3);
	w.DrawRect(tl.x, tl.y, hw, hh, White());
}
