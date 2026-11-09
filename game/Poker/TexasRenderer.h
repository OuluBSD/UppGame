#ifndef _GameCommon_TexasRenderer_h_
#define _GameCommon_TexasRenderer_h_

#include <Draw/Draw.h>
#include <GameRules/Game.h>

namespace Upp {

class TexasRenderer {
public:
	static void Render(Draw& w, Game& game, const Vector<String>& gameLog, Size sz);
	static void RenderPlayer(Draw& w, PlayerInterface& p, Rect pr, double sy);
	static Image GetCardImage(int card);
	static void SetCardTheme(const String& theme_name_or_path, const String& back_image = String());
	static void ResetCardTheme();
};

}

#endif
