#ifndef _CardEngine_NewGameWindow_h_
#define _CardEngine_NewGameWindow_h_

#include <CtrlLib/CtrlLib.h>
#include <memory>

NAMESPACE_UPP

#define LAYOUTFILE <TexasHoldem/NewGameWindow.lay>
#include <CtrlCore/lay.h>

class NewGameWindow : public WithNewGameWindowLayout<TopWindow> {
public:
	typedef NewGameWindow CLASSNAME;
	NewGameWindow();
	
	void Init(class ConfigFile& config);
	void OnOK();
	void OnCancel();

	int GetNumPlayers() const { return ~numPlayers; }
	int GetStartCash() const { return ~startCash; }
	int GetGameSpeed() const { return ~gameSpeed; }
};

END_UPP_NAMESPACE

#endif
