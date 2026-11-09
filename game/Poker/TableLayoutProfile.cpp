#include <Poker/TableLayoutProfile.h>

namespace Upp {

static String s_profile_name = "texas-holdem-classic";

// Legacy hardcoded-profile approach: this is a manual, per-profile coordinate
// transcription (e.g. the "ps-6p" numbers below were hand-copied and scaled
// from game/TexasHoldem/GameTable_PS_6p.form's Player0 rect), superseded, for
// VisualStateModel/layout-model purposes, by parsing the platform's actual
// `.form` file (see MILESTONE_04_layout_model_ps6p.md and task
// 0112_m04_form_driven_layout_element_model.md in the Manager repo, plus
// uppsrc/VisualStateModel/FormLayout.h). Keep this working for its existing
// renderer/theme callers, but do not add new profile structs for new
// platforms here — give them a `.form` file instead.
struct LayoutBaseCoords {
	int table_l = 100, table_t = 100, table_r = 1820, table_b = 980;
	int pot_l = 840, pot_t = 337, pot_r = 1080, pot_b = 405;
	int pot_tx = 850, pot_ty = 345;
	int round_tx = 840, round_ty = 280;
	int board_x = 700, board_y = 510, board_step = 120, board_w = 20, board_h = 32;
	int hero_l = 800, hero_t = 700, hero_r = 1120, hero_b = 900;
	int hero_c1_dx = 10, hero_c2_dx = 40, hero_c_dy = 130;
	int other_l = 100, other_t = 150, other_r = 300, other_b = 300, other_step = 200;
	bool use_fixed_players = false;
	int player_l[10] = {};
	int player_t[10] = {};
	int player_r[10] = {};
	int player_b[10] = {};
};

static LayoutBaseCoords GetCoords() {
	LayoutBaseCoords c;
	String p = ToLower(TrimBoth(s_profile_name));
	if (p == "pokerth-v13i" || p == "v13i") {
		c.table_l = 120; c.table_t = 96; c.table_r = 1800; c.table_b = 970;
		c.pot_l = 836; c.pot_t = 330; c.pot_r = 1084; c.pot_b = 404;
		c.round_tx = 838; c.round_ty = 270;
		c.board_x = 690; c.board_y = 504; c.board_step = 124;
		c.hero_l = 790; c.hero_t = 694; c.hero_r = 1130; c.hero_b = 900;
		c.other_t = 144; c.other_b = 296;
	}
	else if (p == "ps-6p" || p == "ps_6p" || p == "pokerstars-6p") {
		c.table_l = 0; c.table_t = 0; c.table_r = 1920; c.table_b = 1080;
		c.pot_l = 873; c.pot_t = 318; c.pot_r = 1057; c.pot_b = 347;
		c.pot_tx = 873; c.pot_ty = 318;
		c.round_tx = 875; c.round_ty = 530;
		c.board_x = 641; c.board_y = 349; c.board_step = 131; c.board_w = 122; c.board_h = 143;
		c.hero_l = 704; c.hero_t = 606; c.hero_r = 1162; c.hero_b = 849;
		c.hero_c1_dx = 136; c.hero_c2_dx = 261; c.hero_c_dy = 55;
		c.use_fixed_players = true;
		const int pl[10] = {704, 31, 90, 750, 1141, 1215, 2, 4, 6, 8};
		const int pt[10] = {606, 445, 112, 17, 105, 448, 2, 2, 2, 2};
		const int pr[10] = {1162, 733, 783, 1278, 1842, 1902, 4, 6, 8, 10};
		const int pb[10] = {876, 669, 413, 315, 412, 668, 4, 4, 4, 4};
		for (int i = 0; i < 10; i++) {
			c.player_l[i] = pl[i];
			c.player_t[i] = pt[i];
			c.player_r[i] = pr[i];
			c.player_b[i] = pb[i];
		}
	}
	else if (p == "texas-holdem-legacy-pokertable" || p == "legacy-pokertable") {
		// Matches original PokerTable fixed 10-seat ring geometry (scaled to 1920x1080 base space).
		c.table_l = 0; c.table_t = 0; c.table_r = 1920; c.table_b = 1080;
		c.pot_l = 458; c.pot_t = 310; c.pot_r = 712; c.pot_b = 377;
		c.pot_tx = 458; c.pot_ty = 346;
		c.round_tx = 1293; c.round_ty = 310;
		c.board_x = 680; c.board_y = 293; c.board_step = 113; c.board_w = 94; c.board_h = 133;
		c.hero_l = 795; c.hero_t = 497; c.hero_r = 1125; c.hero_b = 710;
		c.hero_c1_dx = 148; c.hero_c2_dx = 197; c.hero_c_dy = 53;
		c.use_fixed_players = true;
		const int pl[10] = {795, 405, 19, 19, 403, 795, 1181, 1575, 1575, 1181};
		const int pt[10] = {497, 497, 413, 93, 10, 10, 10, 93, 413, 497};
		const int pr[10] = {1125, 735, 349, 349, 733, 1125, 1511, 1905, 1905, 1511};
		const int pb[10] = {710, 710, 626, 306, 223, 223, 223, 306, 626, 710};
		for (int i = 0; i < 10; i++) {
			c.player_l[i] = pl[i];
			c.player_t[i] = pt[i];
			c.player_r[i] = pr[i];
			c.player_b[i] = pb[i];
		}
	}
	return c;
}

// Legacy hardcoded-profile entry point — superseded, for
// VisualStateModel/layout-model purposes, by `.form`-file parsing (see
// MILESTONE_04_layout_model_ps6p.md and task
// 0112_m04_form_driven_layout_element_model.md in the Manager repo, plus
// uppsrc/VisualStateModel/FormLayout.h). Still required for its existing
// renderer/theme callers (GameTable.cpp, TexasRenderer.cpp,
// GameThemeRender.cpp); do not extend with new profile names for new
// platforms — add a `.form` file instead.
void TexasTableLayout::SetProfile(const String& profile_name) {
	String p = TrimBoth(profile_name);
	if (p.IsEmpty())
		p = "texas-holdem-classic";
	s_profile_name = p;
}

String TexasTableLayout::GetProfile() {
	return s_profile_name;
}

Vector<String> TexasTableLayout::GetProfiles() {
	Vector<String> v;
	v.Add("texas-holdem-classic");
	v.Add("ps-6p");
	v.Add("pokerth-v13i");
	v.Add("texas-holdem-legacy-pokertable");
	return v;
}

Size TexasTableLayout::BaseSize() {
	return Size(1920, 1080);
}

static double Sx(Size sz) {
	return (double)sz.cx / (double)TexasTableLayout::BaseSize().cx;
}

static double Sy(Size sz) {
	return (double)sz.cy / (double)TexasTableLayout::BaseSize().cy;
}

Rect TexasTableLayout::TableRect(Size sz) {
	LayoutBaseCoords c = GetCoords();
	return Rect((int)(c.table_l * Sx(sz)), (int)(c.table_t * Sy(sz)), (int)(c.table_r * Sx(sz)), (int)(c.table_b * Sy(sz)));
}

Rect TexasTableLayout::PotRect(Size sz) {
	LayoutBaseCoords c = GetCoords();
	return Rect((int)(c.pot_l * Sx(sz)), (int)(c.pot_t * Sy(sz)), (int)(c.pot_r * Sx(sz)), (int)(c.pot_b * Sy(sz)));
}

Point TexasTableLayout::PotTextPos(Size sz) {
	LayoutBaseCoords c = GetCoords();
	return Point((int)(c.pot_tx * Sx(sz)), (int)(c.pot_ty * Sy(sz)));
}

Point TexasTableLayout::RoundTextPos(Size sz) {
	LayoutBaseCoords c = GetCoords();
	return Point((int)(c.round_tx * Sx(sz)), (int)(c.round_ty * Sy(sz)));
}

Rect TexasTableLayout::BoardCardRect(Size sz, int index) {
	LayoutBaseCoords c = GetCoords();
	int i = minmax(index, 0, 4);
	int cw = max(1, (int)(c.board_w * Sx(sz)));
	int ch = max(1, (int)(c.board_h * Sy(sz)));
	int x = (int)(c.board_x * Sx(sz)) + i * (int)(c.board_step * Sx(sz));
	int y = (int)(c.board_y * Sy(sz));
	return RectC(x, y, cw, ch);
}

Rect TexasTableLayout::PlayerRect(Size sz, int player_index) {
	LayoutBaseCoords c = GetCoords();
	int i = max(0, player_index);
	if (c.use_fixed_players && i < 10) {
		return Rect((int)(c.player_l[i] * Sx(sz)), (int)(c.player_t[i] * Sy(sz)),
		            (int)(c.player_r[i] * Sx(sz)), (int)(c.player_b[i] * Sy(sz)));
	}
	if (i == 0)
		return Rect((int)(c.hero_l * Sx(sz)), (int)(c.hero_t * Sy(sz)), (int)(c.hero_r * Sx(sz)), (int)(c.hero_b * Sy(sz)));
	return Rect((int)((c.other_l + i * c.other_step) * Sx(sz)), (int)(c.other_t * Sy(sz)), (int)((c.other_r + i * c.other_step) * Sx(sz)), (int)(c.other_b * Sy(sz)));
}

Rect TexasTableLayout::HeroCardRect(Size sz, int card_index) {
	LayoutBaseCoords c = GetCoords();
	Rect pr = PlayerRect(sz, 0);
	int idx = (card_index <= 0) ? 0 : 1;
	int x = pr.left + (int)((idx == 0 ? c.hero_c1_dx : c.hero_c2_dx) * Sx(sz));
	int y = pr.top + (int)(c.hero_c_dy * Sy(sz));
	int cw = max(1, (int)(c.board_w * Sx(sz)));
	int ch = max(1, (int)(c.board_h * Sy(sz)));
	return RectC(x, y, cw, ch);
}

}
