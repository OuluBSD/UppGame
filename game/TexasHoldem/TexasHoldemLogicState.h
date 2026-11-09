#ifndef _CardEngine_TexasHoldemLogicState_h_
#define _CardEngine_TexasHoldemLogicState_h_

#include <Core/Core.h>
#include <CtrlLib/CtrlLib.h>

NAMESPACE_UPP

// ---------------------------------------------------------------------------
// M05-01 (task 0119): the vision pipeline's parsed logic-state schema.
//
// Mirrors TexasHoldemGroundTruthRecord / TexasHoldemPlayerSnapshot
// (TexasHoldemSessionContract.h:8-42) FIELD FOR FIELD, so a later task only
// has to fill values in, never restructure this record. The key difference:
// every field here is paired with an explicit `<field>_known` bool (default
// false), so "not yet parsed" can never be confused with "parsed as a real
// value of 0/-1/""/empty".
//
// Why a parallel bool per field rather than a single sentinel value (e.g.
// -1 or Null): every ground-truth field's legitimate value range already
// includes at least one of 0/-1/""/empty for a REAL parsed reading (seat 0 is
// a valid seat, street 0 is preflop, pot 0 is a real pot value, an empty
// hole_cards vector happens pre-deal, etc.) - there is no single sentinel
// that is simultaneously invalid for every field. A parallel `bool` is
// simple, uniform across every field type (int/int64/String/Vector), and
// trivially greppable/JSON-visible (`"stack_known":false` is unambiguous in
// the emitted JSONL, unlike inferring "unknown" from a magic number).
//
// Only ONE field is actually vision-derived by this task: `dealer_seat`
// (see reference/VisualStateLogicCompare). EVERY other field's `_known` flag
// is permanently false in this task's output - per the task's guardrail,
// nothing here may be guessed or silently defaulted. `frame_id` itself is
// not a "parsed" field at all (it's the record's own identity/index, always
// present), so it has no `_known` flag, matching how
// TexasHoldemGroundTruthRecord::frame_id has none either.

struct TexasHoldemLogicPlayerState : Moveable<TexasHoldemLogicPlayerState> {
	int seat = -1;   // identity (array position), not a "parsed" field - always meaningful once the record exists

	bool uid_known = false;
	int  uid = 0;

	bool name_known = false;
	String name;

	bool hero_known = false;
	bool hero = false;

	bool active_known = false;
	bool active = false;

	bool stack_known = false;
	int  stack = 0;

	bool bet_known = false;
	int  bet = 0;

	bool action_known = false;
	int  action = 0;

	// Mirrors TexasHoldemPlayerSnapshot::button (1 == GBUTTON_DEALER, see
	// game/GameRules/GameDefs.h:280 and game/TexasHoldem/GameTable.cpp:1570-
	// 1581). This is the ONE per-player field this task's CLI actually sets
	// button_known=true/button=1 for - and only for the single seat it has
	// derived as the current dealer (see VisualStateLogicCompare/main.cpp's
	// dealer-seat derivation). It deliberately does NOT set
	// button_known=true/button=0 for every other seat: the vision signal
	// only tells us "seat N's dealer_button sub-slot changed", never
	// "seat M definitely has no puck" (that would be an inference beyond
	// what was actually observed), so every non-derived seat's button field
	// stays honestly unknown.
	bool button_known = false;
	int  button = 0;

	bool hole_cards_known = false;
	Vector<int> hole_cards;

	void Jsonize(JsonIO& jio);
};

struct TexasHoldemLogicState : Moveable<TexasHoldemLogicState> {
	int schema = 1;

	bool session_id_known = false;
	String session_id;

	int frame_id = -1;   // identity, not a "parsed" field - always present

	bool render_step_known = false;
	int  render_step = 0;

	bool timestamp_ms_known = false;
	int64 timestamp_ms = 0;

	bool provider_known = false;
	String provider;

	bool table_size_known = false;
	int  table_width = 0;
	int  table_height = 0;

	bool seed_known = false;
	int  seed = -1;

	bool game_id_known = false;
	int  game_id = 0;

	bool hand_id_known = false;
	int  hand_id = 0;

	bool street_known = false;
	int  street = -1;

	bool turn_uid_known = false;
	int  turn_uid = -1;

	bool pot_known = false;
	int  pot = 0;

	bool board_cards_known = false;
	Vector<int> board_cards;

	// One entry per seat this record is tracking (seat count/identity comes
	// from the session being observed, NOT parsed from vision either - see
	// VisualStateLogicCompare/main.cpp for how `players` is populated).
	// `players_known` reflects whether ANY per-player field beyond bare
	// seat identity has been set for at least one player in this record.
	bool players_known = false;
	Vector<TexasHoldemLogicPlayerState> players;

	// --------------------------------------------------------------------
	// The one real vision-derived fact this task establishes end to end:
	// which seat currently has the dealer button, derived from the layout
	// model's "dealer_button"-role sub-slot (see LayoutProfile.cpp:165,
	// FormLayout.cpp's "button_puck" sub-slot row) changing between frames.
	bool dealer_seat_known = false;
	int  dealer_seat = -1;   // seat index, meaningful only if dealer_seat_known

	void Jsonize(JsonIO& jio);
};

// ---------------------------------------------------------------------------
// M05-03 (task 0121): the "what image currently represents puck role N under
// theme T" logic, extracted out of GameTable::GetPuckImage (game/TexasHoldem/
// GameTable.cpp, task 0120) into a free function that needs no live GameTable
// instance, so reference/VisualStateLogicCompare can compute the exact same
// reference images GameTable would render for a puck without constructing a
// full GameTable/Ctrl tree.
//
// role: 0 = dealer, 1 = small blind, 2 = big blind (matches
// GBUTTON_DEALER/SMALL_BLIND/BIG_BLIND ordering, game/GameRules/GameDefs.h).
// theme: table theme name, e.g. "default" (GameTable.cpp:538's constructor-
// time `LoadTheme("default")` call is the only theme recorded sessions ever
// use today - no CLI flag for choosing a theme exists yet, so callers should
// pass a clearly-named "default" constant/literal rather than inferring it).
//
// Resolution order mirrors GameTable's existing three tiers exactly (see
// GameTable::LoadTheme + GameTable::refreshGroupbox's inline fallback chain,
// both game/TexasHoldem/GameTable.cpp), so that if a real themed puck PNG is
// ever dropped in under gfx/gui/table/<theme>/{dealerPuck,smallblindPuck,
// bigblindPuck}.png, this function (and any comparator built on it) picks it
// up automatically without further changes:
//   1. themed dir:  <dataDir>/gfx/gui/table/<theme>/{dealerPuck,
//      smallblindPuck,bigblindPuck}.png
//   2. legacy dir:  <dataDir>/gfx/{dealer,small_blind,big_blind}.png
//   3. procedural fallback: a filled/outlined disc with a "D"/"SB"/"BB"
//      label, fixed colors (dealer=cream, SB=light-blue, BB=dark-red) - the
//      same drawing GameTable::GetPuckImage has produced since task 0120.
Image TexasHoldemGetPuckReferenceImage(int role, const String& theme);

// ---------------------------------------------------------------------------
// M05-08 (task 0126): board (community) card template-match reference
// helpers, mirroring TexasHoldemGetPuckReferenceImage's theme-aware pattern
// above but for the card vocabulary instead of the 3 pucks. See
// GameTable::GetCardImage (game/TexasHoldem/GameTable.cpp:246) and
// GameTable::PaintBoard (GameTable.cpp:1270-1289), which this pair of
// functions was extracted from the same way TexasHoldemGetPuckReferenceImage
// was extracted from GetPuckImage/refreshGroupbox/LoadTheme.
//
// TWO separate functions, not one with a card<0 special case, because the
// "not yet dealt" placeholder is a DIFFERENT image per board_card sub-slot
// INDEX (flop/turn/river cardholder art differ), not a single universal
// "blank" image the way `card<0` maps to one "back9.png" for face-down
// pockets/opponents elsewhere in this file's caller GetCardImage - see
// TexasHoldemGetBoardHolderReferenceImage below.
//
// TexasHoldemGetCardReferenceImage(card, target_size, theme): for `card` in
// 0..51, returns exactly what GameTable::GetCardImage(card) would draw for a
// REAL (already dealt, face-up) card - same suit/rank filename encoding
// (card = suit_index*13 + rank_index, suits clubs/diamonds/hearts/spades,
// ranks 2..10/jack/queen/king/ace) and the SAME LoadCardArt (game/CardRender/
// CardRender.h) call GetCardImage makes, reused verbatim (path resolution is
// NOT reimplemented here). Unlike GetCardImage (which always requests a fixed
// Size(48,76)), this takes `target_size` explicitly so a caller can ask
// LoadCardArt to hand back a reference already rescaled to a detected
// region's own on-screen size in one call - LoadCardArt already caches by
// (theme,filename,target_size), so this doesn't cost an extra Rescale() per
// comparison the way the puck reference above requires of its callers.
// `card` must be in 0..51: an out-of-range value (in particular any `card<0`
// "not yet dealt" sentinel) returns an empty Image - callers must route that
// case to TexasHoldemGetBoardHolderReferenceImage instead.
Image TexasHoldemGetCardReferenceImage(int card, Size target_size, const String& theme);

// TexasHoldemGetBoardHolderReferenceImage(card_index, target_size, theme):
// for one board_card sub-slot INDEX (0..4, matching
// VsmFormSubSlot/VsmLayoutCandidate::card_index for role=="board_card"),
// returns the "not yet dealt this street" placeholder GameTable::PaintBoard
// itself would draw for that slot (GameTable.cpp:1280-1283: card_index<3 =>
// cardholder_flop.png, ==3 => cardholder_turn.png, ==4 (or greater) =>
// cardholder_river.png). Two-tier resolution, mirroring PaintBoard's own
// choice exactly (there is no "legacy dir" middle tier here, unlike the
// puck's 3-tier resolution above - PaintBoard itself only ever chooses
// between a themed PNG or a solid color, never a second file path):
//   1. themed dir: <dataDir>/gfx/gui/table/<theme>/cardholder_{flop,turn,
//      river}.png (GameTable::LoadTheme, GameTable.cpp:1211-1213), rescaled
//      to target_size if given and different from the loaded image's size.
//   2. procedural fallback: a plain Color(0,80,0)-filled rect at
//      target_size, byte-for-byte the same color PaintBoard's own fallback
//      draws (GameTable.cpp:1287's `w.DrawRect(x, y, w1, h1, Color(0, 80,
//      0))`) when the themed asset is missing - same reasoning task 0121
//      used for its own procedural puck fallback tier.
Image TexasHoldemGetBoardHolderReferenceImage(int card_index, Size target_size, const String& theme);

// ---------------------------------------------------------------------------
// M05-10 (task 0128): the card-BACK reference - the "hidden/face-down" member
// of the per-player hole-card vocabulary, mirroring the two helpers above's
// theme-aware pattern but resolving the SAME single "back9.png" file
// GameTable::GetCardImage(card<0) resolves regardless of WHICH sub-slot
// index it's being drawn into (GameTable.cpp:257-258's `filename =
// "back9.png"` branch) - unlike TexasHoldemGetBoardHolderReferenceImage
// (task 0126), there is only ONE "not a real card" image here, not a
// per-slot-index family of three, so this is a single function taking no
// index parameter, closer in shape to TexasHoldemGetCardReferenceImage
// itself than to the board-holder helper.
//
// `target_size`/`theme`: same convention as TexasHoldemGetCardReferenceImage
// - LoadCardArt (game/CardRender/CardRender.h) already caches by
// (theme,filename,target_size) and already falls back from a missing themed
// dir to the "default" theme dir (CardRender.cpp's ResolveCardArtPath) - both
// reused verbatim, not reimplemented here, exactly like the sibling helper.
//
// Deliberately does NOT replicate GetCardImage's OWN third-tier procedural
// fallback (GameTable.cpp:271-284: a blue-filled, black-framed placeholder,
// non-trivial unlike the board-holder's flat-color fallback) - this task
// confirmed empirically (see its Manager task file's Evidence section) that
// `share/imgs/cards/default/back9.png` is present and resolves in this
// checkout, so that fallback tier is unreachable in practice here. This
// mirrors the EXACT precedent TexasHoldemGetCardReferenceImage itself
// already set immediately above (task 0126 also did not replicate
// GetCardImage's procedural fallback for real card faces, for the same
// "unreachable given confirmed asset availability" reasoning) - kept
// consistent rather than inventing a new policy for this one sibling value.
// If `back9.png` were ever removed from this checkout, this helper would
// return an empty Image() where the live game would instead render the
// procedural placeholder - a documented, known divergence, not an oversight.
Image TexasHoldemGetCardBackReferenceImage(Size target_size, const String& theme);

// ---------------------------------------------------------------------------
// M05-09 (task 0127): per-player action-icon template-match reference helper,
// mirroring TexasHoldemGetPuckReferenceImage/TexasHoldemGetCardReferenceImage's
// theme-aware pattern above, extracted from GameTable::LoadTheme's actionPics[]
// load (GameTable.cpp:1216-1223) and GameTable::refreshGroupbox's selection of
// one of those 7 images per seat (GameTable.cpp:1713-1716).
//
// Vocabulary convention: reuses `PlayerAction`'s (game/GameRules/GameDefs.h)
// OWN integer values directly for the six real icons - 1=CHECK, 2=CALL,
// 3=BET, 4=RAISE, 5=FOLD, 6=ALLIN - the SAME encoding ground truth's
// `TexasHoldemPlayerSnapshot::action` already uses (game/TexasHoldem/
// Main.cpp:66's `ps.action = (int)player->getMyAction()`), so a caller never
// needs a translation table between "vocabulary index" and "ground truth
// value" the way 0126's card vocabulary needed one extra index (52) for its
// holder tier. The 7th vocabulary member - the `isWinner` override
// (`actionPics[9]`, GameTable.cpp:1714) - is NOT a `PlayerAction` enumerator
// at all (no such value exists in that enum); `kActionIconVocabWinner` below
// names it explicitly as 9, matching `actionPics[9]`'s own array index for
// direct readability against that code, not because 9 means anything as a
// `PlayerAction`. Callers must treat this 7th member as its own distinct,
// non-ground-truth-comparable recognized state (see this task's Manager task
// file's "Ground truth semantics" section - `TexasHoldemPlayerSnapshot` has
// no "is winner" field to compare it against).
//
// IMPORTANT theme-directory finding (verified by re-reading GameTable.cpp:
// 1216 before assuming this parallels 0121/0126): unlike the puck
// (`gfx/gui/table/<theme>/...`) and card-holder (`gfx/gui/table/<theme>/
// cardholder_*.png`) assets, action-icon art loads from a SINGLE SHARED
// location regardless of table theme - `String actionDir =
// AppendFileName(dataDir, "gfx/gui/misc/actionpics");` has no `<theme>`
// path component at all. So, unlike the two helpers above, this function
// takes NO `theme` parameter - adding one that's silently ignored would
// misleadingly imply theme-awareness this asset tier genuinely does not
// have.
//
// IMPORTANT fallback-tier finding (also load-bearing for task 0127's design,
// see its evidence section): `GameTable::refreshGroupbox`'s `aPic` selection
// (GameTable.cpp:1713-1716) has NO procedural-fallback tier the way the puck
// (task 0120) and board holder (task 0126) both do - if
// `StreamRaster::LoadFileAny` fails to find the PNG, `aPic` simply stays a
// default-constructed empty `Image()`, and `ScaledImageCtrl::Paint`/
// `GameTable::RenderToImage`'s own `DrawChild` lambda (GameTable.cpp:1343-
// 1345's `if (!si.img.IsEmpty())` guard) draw literally nothing for it - so
// this helper deliberately does NOT invent a procedural fallback of its own:
// doing so would make its return value NOT correspond to what GameTable
// itself actually renders (this checkout ships no `gfx/gui/misc/actionpics/
// *.png` assets at all - confirmed empirically, see task 0127's evidence
// section - so every one of the 7 vocabulary members legitimately resolves
// to an empty Image() in THIS environment; this function mirrors that
// honestly rather than papering over it, exactly the way the two helpers
// above stay faithful to their own source's real fallback tiers).
static const int kActionIconVocabWinner = 9;

Image TexasHoldemGetActionIconReferenceImage(int action_or_winner, Size target_size);

// ---------------------------------------------------------------------------
// M05-09 (task 0127): the "no icon currently shown" positive reference - the
// task's own framing (see this task's Manager task file's "Key difference
// from board cards" section) correctly points out that, unlike 0126's board
// holder, there is no PNG ASSET for this state. But re-reading the ACTUAL
// off-screen renderer `--record-session`/`--dump-render-image`/`DumpSnapshot`
// use - `GameTable::RenderToImage` (GameTable.cpp:1292-1423), NOT
// `PlayerBgCtrl::Paint`/`ScaledImageCtrl::Paint` (those only run for the
// live interactive window) - shows this state is NOT a photographic/scene-
// dependent "whatever happens to be behind it": `RenderToImage`'s own per-
// player loop (GameTable.cpp:1317-1329) manually re-implements
// `PlayerBgCtrl::Paint`'s striped background DIRECTLY, unconditionally,
// BEFORE drawing any of that seat's children (name/stack/cards/puck/action
// icon) - `w.DrawRect(pr, Black())` then `for(y=0;y<pr.Height();y+=2)
// w.DrawRect(pr.left,pr.top+y,pr.Width(),1,bc)` where `bc` is
// `Color(0,255,0)` if that seat is a hand winner, else `Color(0,120,0)`.
// `ScaledImageCtrl::Paint`'s off-screen replica (the `DrawChild` lambda,
// GameTable.cpp:1343-1345) draws NOTHING at all when `actionPic`'s own image
// is empty (`if (!si.img.IsEmpty())` guard) - no backdrop fill of its own,
// unlike `PaintBoard`'s `Color(0,80,0)` felt fallback (task 0126). So
// whatever this striped pattern already put there is EXACTLY, deterministically,
// what a real captured frame's action_icon rect shows whenever no icon is
// set - a genuine, computable positive reference, just not a file-backed one.
// Confirmed by direct pixel inspection of a real `--record-session` frame
// (see this task's evidence section): alternating (0,0,0)/(0,120,0) rows,
// exact phase match.
//
// `target_size`: the candidate rect's own on-screen (w,h), same convention
// as the other reference helpers.
// `row_parity_offset`: (action_icon_rect.top - owning_player_rect.top) in
// real frame pixels - the phase of the alternating rows depends ONLY on this
// value's parity (the stripe loop above always starts a colored row at
// y=0 relative to the PLAYER rect's own top, not the action_icon sub-rect's
// own top) - not on any other absolute frame coordinate. Callers derive this
// from the SAME layout-candidate list that gives them `target_size`/
// `candidate_rect` in the first place (the owning seat's own "seat"-role
// element candidate's `rect.top`), so no extra GameTable instance/rendering
// is needed to compute it.
// `is_winner`: selects the winner-highlight stripe color (0,255,0)/border
// vs. the normal (0,120,0) - see this helper's caller for why this task
// deliberately does NOT attempt to recognize the winner-highlighted variant
// of this pattern as a distinct vocabulary member (out of scope, no ground
// truth to validate it against either way - see evidence section).
Image TexasHoldemGetActionIconEmptyReferenceImage(Size target_size, int row_parity_offset, bool is_winner);

END_UPP_NAMESPACE

#endif
