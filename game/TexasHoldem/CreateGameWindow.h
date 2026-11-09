#ifndef _CardEngine_CreateGameWindow_h_
#define _CardEngine_CreateGameWindow_h_

#include <CtrlLib/CtrlLib.h>
#include <memory>
#include <GameRules/GameData.h>

NAMESPACE_UPP

#define LAYOUTFILE <TexasHoldem/CreateGameWindow.lay>
#include <CtrlCore/lay.h>

class CreateGameWindow : public WithCreateGameWindowLayout<TopWindow> {
public:
	typedef CreateGameWindow CLASSNAME;
	CreateGameWindow();
	
	GameData GetGameData() const;
	String GetGameName() const { return ~name; }

private:
	void OnOK();
	void OnCancel();
};

END_UPP_NAMESPACE

#endif
