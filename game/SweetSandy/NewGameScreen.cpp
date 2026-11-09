#include "SweetSandy.h"

NAMESPACE_UPP

NewGameScreen::NewGameScreen()
{
	Add(title.HSizePos(20, 20).TopPos(20, 40));
	title.SetText("Uusi peli").SetFont(StdFont().Bold().Height(32));

	Add(track_label.LeftPos(20, 120).TopPos(80, 24));
	track_label.SetText("Kentta:");
	Add(track_list.LeftPos(150, 260).TopPos(80, 24));

	Add(vehicle_label.LeftPos(20, 120).TopPos(120, 24));
	vehicle_label.SetText("Auto:");
	Add(vehicle_list.LeftPos(150, 260).TopPos(120, 24));

	const Vector<TrackDef>& tracks = GetTrackCatalog();
	for(int i = 0; i < tracks.GetCount(); i++)
		track_list.Add(i, tracks[i].name);
	if(tracks.GetCount())
		track_list.SetIndex(0);

	const Vector<VehicleDef>& vehicles = GetVehicleCatalog();
	for(int i = 0; i < vehicles.GetCount(); i++)
		vehicle_list.Add(i, vehicles[i].name);
	if(vehicles.GetCount())
		vehicle_list.SetIndex(0);

	Add(start_button.LeftPos(20, 160).TopPos(170, 32));
	start_button.SetLabel("Aloita");
	start_button.WhenAction = [=] {
		int ti = track_list.GetIndex();
		int vi = vehicle_list.GetIndex();
		if(ti < 0 || vi < 0)
			return;
		WhenStart(GetTrackCatalog()[ti].id, GetVehicleCatalog()[vi].id);
	};

	Add(back_button.LeftPos(200, 160).TopPos(170, 32));
	back_button.SetLabel("Takaisin");
	back_button.WhenAction = [=] { WhenBack(); };
}

END_UPP_NAMESPACE
