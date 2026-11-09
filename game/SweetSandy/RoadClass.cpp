#include "SweetSandy.h"

NAMESPACE_UPP

bool ParseRoadClass(const String& name, RoadClassParams& out)
{
	RoadClassParams p;
	if(name == "30")       { p.speed_kmh = 30;  p.max_grade_pct = 12; p.min_curve_radius = 15;  p.width = 6;  p.needs_fence = false; }
	else if(name == "40")  { p.speed_kmh = 40;  p.max_grade_pct = 10; p.min_curve_radius = 25;  p.width = 7;  p.needs_fence = false; }
	else if(name == "50")  { p.speed_kmh = 50;  p.max_grade_pct = 8;  p.min_curve_radius = 40;  p.width = 8;  p.needs_fence = false; }
	else if(name == "60")  { p.speed_kmh = 60;  p.max_grade_pct = 7;  p.min_curve_radius = 70;  p.width = 9;  p.needs_fence = true;  }
	else if(name == "80")  { p.speed_kmh = 80;  p.max_grade_pct = 6;  p.min_curve_radius = 130; p.width = 10; p.needs_fence = true;  }
	else if(name == "100") { p.speed_kmh = 100; p.max_grade_pct = 5;  p.min_curve_radius = 300; p.width = 12; p.needs_fence = true;  }
	else if(name == "120") { p.speed_kmh = 120; p.max_grade_pct = 4;  p.min_curve_radius = 500; p.width = 14; p.needs_fence = true;  }
	else return false;
	out = p;
	return true;
}

END_UPP_NAMESPACE
