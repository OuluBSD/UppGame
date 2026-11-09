#ifndef _CardEngine_LobbyWindow_h_
#define _CardEngine_LobbyWindow_h_

#include <CtrlLib/CtrlLib.h>
#include <memory>

NAMESPACE_UPP

#define LAYOUTFILE <TexasHoldem/LobbyWindow.lay>
#include <CtrlCore/lay.h>

class LobbyWindow : public WithLobbyWindowLayout<TopWindow> {
public:
	typedef LobbyWindow CLASSNAME;
	LobbyWindow();
	
	void AddGame(unsigned id, const String& name, int players, int maxPlayers);
	void AddPlayer(unsigned id, const String& name);
	void AddChatMessage(const String& name, const String& text);

private:
	void OnSend();
	void OnJoin();
	void OnCreate();
};

END_UPP_NAMESPACE

#endif
