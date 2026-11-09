#ifndef _GameCommon_TableLayoutProfile_h_
#define _GameCommon_TableLayoutProfile_h_

#include <Core/Core.h>

namespace Upp {

// Legacy hardcoded-profile approach: coordinates below are manually
// transcribed per platform/profile and were superseded, for
// VisualStateModel/layout-model purposes, by parsing the platform's actual
// `.form` file (see MILESTONE_04_layout_model_ps6p.md and task
// 0112_m04_form_driven_layout_element_model.md in the Manager repo, plus
// uppsrc/VisualStateModel/FormLayout.h). This struct/API must keep working
// for its existing renderer/theme callers, but new platforms should get a
// `.form` file instead of a new profile struct here.
struct TexasTableLayout {
	static void SetProfile(const String& profile_name);
	static String GetProfile();
	static Vector<String> GetProfiles();
	static Size BaseSize();
	static Rect TableRect(Size sz);
	static Rect PotRect(Size sz);
	static Point PotTextPos(Size sz);
	static Point RoundTextPos(Size sz);
	static Rect BoardCardRect(Size sz, int index);
	static Rect PlayerRect(Size sz, int player_index);
	static Rect HeroCardRect(Size sz, int card_index);
};

}

#endif
