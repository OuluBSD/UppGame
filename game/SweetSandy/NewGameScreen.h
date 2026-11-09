#ifndef _SweetSandy_NewGameScreen_h_
#define _SweetSandy_NewGameScreen_h_

// Track + vehicle selection before starting a run. Lists are filled from
// Catalog.h; extending the catalog is enough to extend this screen.
class NewGameScreen : public ParentCtrl {
	Label    title;
	Label    track_label, vehicle_label;
	DropList track_list, vehicle_list;
	Button   start_button, back_button;

public:
	typedef NewGameScreen CLASSNAME;

	Event<>             WhenBack;
	Event<String,String> WhenStart; // (track_id, vehicle_id)

	NewGameScreen();
};

#endif
