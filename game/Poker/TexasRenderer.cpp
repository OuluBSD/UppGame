#include <Painter/Painter.h>
#include <plugin/png/png.h>
#include <Poker/TexasRenderer.h>
#include <EditorCommon/Tools.h>
#include <Poker/CardsValue.h>
#include <GameRules/HandInterface.h>
#include <GameRules/BoardInterface.h>
#include <GameRules/PlayerInterface.h>
#include <GameRules/GameDefs.h>
#include <Poker/TableLayoutProfile.h>

namespace Upp {

static ArrayMap<int, Image> card_cache;
static String s_card_theme = "default_800x480";
static String s_card_back_image = "flipside.png";
static String GetActionName(PlayerAction a);

Image TexasRenderer::GetCardImage(int card)
{
	int q = card_cache.Find(card);
	if (q >= 0) return card_cache[q];
	
	String themeDir = IsFullPath(s_card_theme) ? s_card_theme : AppendFileName(AppendFileName(Tools::GetDataDir(), "gfx/cards"), s_card_theme);
	String fileName = AppendFileName(themeDir, (card < 0) ? s_card_back_image : AsString(card) + ".png");
	
	Image img = StreamRaster::LoadFileAny(fileName);
	if (img.IsEmpty()) {
		Cout() << "WARNING: Failed to load card image: " << fileName << "\n";
		ImageBuffer bp(48, 76);
		RGBA c = (card < 0) ? Color(0, 0, 150) : White();
		for(int i = 0; i < 48*76; i++) bp[0][i] = c;
		img = bp;
	}
	card_cache.Add(card, img);
	return img;
}

void TexasRenderer::SetCardTheme(const String& theme_name_or_path, const String& back_image)
{
	String next_theme = TrimBoth(theme_name_or_path);
	if (next_theme.IsEmpty())
		next_theme = "default_800x480";
	String next_back = TrimBoth(back_image);
	if (next_back.IsEmpty())
		next_back = "flipside.png";
	if (next_theme == s_card_theme && next_back == s_card_back_image)
		return;
	s_card_theme = next_theme;
	s_card_back_image = next_back;
	card_cache.Clear();
}

void TexasRenderer::ResetCardTheme()
{
	SetCardTheme("default_800x480", "flipside.png");
}

void TexasRenderer::Render(Draw& w, Game& game, const Vector<String>& gameLog, Size sz)
{
	w.DrawRect(sz, Black());
	
	double sx = (double)sz.cx / 1920.0;
	double sy = (double)sz.cy / 1080.0;
	
	w.DrawRect(TexasTableLayout::TableRect(sz), Color(0, 60, 0));
	
	std::shared_ptr<HandInterface> hand = game.getCurrentHand();
	if (hand) {
		std::shared_ptr<BoardInterface> board = hand->getBoard();
		if (board) {
			const int* cards = board->getMyCards();
			for (int i = 0; i < 5; i++) {
				Rect cr = TexasTableLayout::BoardCardRect(sz, i);
				if (cards[i] >= 0) {
					w.DrawRect(cr, White());
					w.DrawImage(cr.left, cr.top, GetCardImage(cards[i]));
				}
			}
			
			// POT
			Rect pr = TexasTableLayout::PotRect(sz);
			Point pt = TexasTableLayout::PotTextPos(sz);
			w.DrawRect(pr, Gray());
			w.DrawText(pt.x, pt.y, Format("%d", board->getPot()), StdFont((int)(30*sy)).Bold(), Yellow());
		}
		
		String roundName;
		switch(hand->getCurrentRound()) {
			case GAME_STATE_PREFLOP: roundName = "Pre-flop"; break;
			case GAME_STATE_FLOP: roundName = "Flop"; break;
			case GAME_STATE_TURN: roundName = "Turn"; break;
			case GAME_STATE_RIVER: roundName = "River"; break;
			default: roundName = "None"; break;
		}
		Point rp = TexasTableLayout::RoundTextPos(sz);
		w.DrawText(rp.x, rp.y, roundName, StdFont((int)(20*sy)).Bold(), White());
	}
	
	for (int i = 0; i < game.getStartQuantityPlayers(); i++) {
		auto p = game.getPlayerByNumber(i);
		if (p) {
			Rect pr = TexasTableLayout::PlayerRect(sz, i);
			
			w.DrawRect(pr, Gray());
			RenderPlayer(w, *p, pr, sy);

			int c1, c2;
			p->getMyCards(c1, c2);
			if (c1 >= 0) {
				Rect r = (i == 0) ? TexasTableLayout::HeroCardRect(sz, 0) : RectC(pr.left + 10, pr.top + 130, (int)(20 * sx), (int)(32 * sy));
				w.DrawRect(r, White());
				w.DrawImage(r.left, r.top, GetCardImage(c1));
			}
			if (c2 >= 0) {
				Rect r = (i == 0) ? TexasTableLayout::HeroCardRect(sz, 1) : RectC(pr.left + 40, pr.top + 130, (int)(20 * sx), (int)(32 * sy));
				w.DrawRect(r, White());
				w.DrawImage(r.left, r.top, GetCardImage(c2));
			}

			if (i == 0) {
				if (p->getMyTurn()) {
					w.DrawRect((int)(720*sx), (int)(1012*sy), (int)(480*sx), (int)(56*sy), Color(200, 200, 0));
				}
				
				// Hero OCR Areas (Stack, Bet, Action)
				Rect r_stack((int)(840*sx), (int)(956*sy), (int)(1080*sx), (int)(1012*sy));
				w.DrawRect(r_stack, Gray());
				w.DrawText(r_stack.left + 10, r_stack.top + 10, Format("%d", p->getMyCash()), StdFont((int)(20*sy)).Bold(), White());
				
				Rect r_bet((int)(840*sx), (int)(787*sy), (int)(1080*sx), (int)(843*sy));
				w.DrawRect(r_bet, Gray());
				w.DrawText(r_bet.left + 10, r_bet.top + 10, Format("%d", p->getMySet()), StdFont((int)(20*sy)).Bold(), White());
				
				Rect r_act((int)(840*sx), (int)(843*sy), (int)(1080*sx), (int)(899*sy));
				w.DrawRect(r_act, Gray());
				w.DrawText(r_act.left + 10, r_act.top + 10, GetActionName(p->getMyAction()), StdFont((int)(20*sy)).Bold(), Cyan());
			}			
			if (p->getMyButton() == GBUTTON_DEALER) {
				// Simple dealer button near player area
				int dx, dy;
				if (i == 0) { dx = 770; dy = 730; }
				else { dx = 270 + (i-1) * 200; dy = 180; }
				w.DrawEllipse((int)(dx*sx), (int)(dy*sy), (int)(20*sx), (int)(20*sy), White(), 1, Black());
				w.DrawText((int)((dx+5)*sx), (int)((dy+2)*sy), "D", StdFont((int)(12*sy)).Bold(), Black());
			}
		}
	}
}

static String GetActionName(PlayerAction a) {
	switch (a) {
		case PLAYER_ACTION_CHECK: return "CHECK";
		case PLAYER_ACTION_CALL: return "CALL";
		case PLAYER_ACTION_BET: return "BET";
		case PLAYER_ACTION_RAISE: return "RAISE";
		case PLAYER_ACTION_FOLD: return "FOLD";
		case PLAYER_ACTION_ALLIN: return "ALLIN";
		case PLAYER_ACTION_SMALL_BLIND: return "SB";
		case PLAYER_ACTION_BIG_BLIND: return "BB";
		default: return "";
	}
}

void TexasRenderer::RenderPlayer(Draw& w, PlayerInterface& p, Rect pr, double sy)
{
	w.DrawRect(pr, Color(0, 120, 0));
	w.DrawText(pr.left + 10, pr.top + 10, p.getMyName(), StdFont((int)(16*sy)).Bold(), White());
	w.DrawText(pr.left + 10, pr.top + 40, Format("Stack: %d", p.getMyCash()), StdFont((int)(14*sy)), White());
	w.DrawText(pr.left + 10, pr.top + 70, Format("Bet: %d", p.getMySet()), StdFont((int)(14*sy)), White());
	w.DrawText(pr.left + 10, pr.top + 100, Format("Action: %s", GetActionName(p.getMyAction())), StdFont((int)(14*sy)), Cyan());
}

}
