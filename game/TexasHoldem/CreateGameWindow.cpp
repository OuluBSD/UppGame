#include "CreateGameWindow.h"

NAMESPACE_UPP

CreateGameWindow::CreateGameWindow()
{
	CtrlLayout(*this);
	Title("Create Game");
	
	maxPlayers <<= 10;
	startMoney <<= 10000;
	
	btnOK << [this] { OnOK(); };
	btnCancel << [this] { OnCancel(); };
}

GameData CreateGameWindow::GetGameData() const
{
	GameData d;
	d.maxNumberOfPlayers = ~maxPlayers;
	d.startMoney = ~startMoney;
	return d;
}

void CreateGameWindow::OnOK()
{
	Break(IDOK);
}

void CreateGameWindow::OnCancel()
{
	Break(IDCANCEL);
}

END_UPP_NAMESPACE
