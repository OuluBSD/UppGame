#ifndef _CardEngine_TexasHoldemLocalGame_h_
#define _CardEngine_TexasHoldemLocalGame_h_

#include <Core/Core.h>
#include <memory>

NAMESPACE_UPP

class EngineLog;
class Game;
class GameTable;
class BeroInterface;
class PlayerInterface;

bool TexasHoldemIsPs6pProvider(const String& provider);
String TexasHoldemProviderLayoutProfile(const String& provider);

// Shared human auto-act helpers. Originally implemented as file-local statics
// in RunLocalGame.cpp for the `--local-game-script --auto-human` path; moved
// here (per AGENTS.md's Headless/GUI Dual-Purpose Rule for this module) so
// StepTexasHoldemLocalGameAction (the `--step-actions`/`--record-session`
// automated driver) can reuse the exact same proven check/call math instead
// of reimplementing it. RunLocalGame.cpp now calls these instead of its own
// copies. See task 0106.
std::shared_ptr<PlayerInterface> TexasHoldemGetHumanOnTurn(const std::shared_ptr<Game>& game,
                                                           const std::shared_ptr<BeroInterface>& bero);
void TexasHoldemApplyHumanFold(const std::shared_ptr<Game>& game, const std::shared_ptr<BeroInterface>& bero);
void TexasHoldemApplyHumanCheckCall(const std::shared_ptr<Game>& game, const std::shared_ptr<BeroInterface>& bero);
void TexasHoldemApplyHumanRaiseTo(const std::shared_ptr<Game>& game, const std::shared_ptr<BeroInterface>& bero,
                                  int target_total);
void TexasHoldemApplyHumanAllIn(const std::shared_ptr<Game>& game, const std::shared_ptr<BeroInterface>& bero);

// True iff the current turn belongs to a human player who still needs to act
// (mirrors LocalBero::nextPlayer()'s human-wait predicate at LocalBero.cpp:267-268,
// computed here via the same public PlayerInterface/BeroInterface accessors,
// without touching LocalBero itself).
bool TexasHoldemHumanNeedsAutoAct(const std::shared_ptr<Game>& game, const std::shared_ptr<BeroInterface>& bero);

// ---------------------------------------------------------------------------
// External (ungated) action injection -- task 0270.
//
// The TexasHoldemApplyHuman* helpers above always act on "the human player
// whose turn it currently is" (via TexasHoldemGetHumanOnTurn's human/turn
// gate). The helpers below apply the SAME check/call/raise/fold/all-in math to
// an EXPLICITLY supplied target player, with no human-type and no on-turn
// gate. They exist so an offline replay/validation harness (see
// reference/VideoGameEngineReplayValidator) can feed a pre-recorded,
// already-ordered real action sequence into a real headless Game for an
// arbitrary seat.
//
// These are ADDITIVE: they do not call TexasHoldemGetHumanOnTurn and are not
// called by any existing GUI or scripted gameplay path, so the four gated
// TexasHoldemApplyHuman* functions and every normal human-played game code
// path behave exactly as before. Callers are responsible for driving the
// engine's turn flow (BeroInterface::nextPlayer) themselves.
enum TexasHoldemExternalActionKind {
	TEXAS_EXT_ACTION_FOLD,
	TEXAS_EXT_ACTION_CHECK_CALL,
	TEXAS_EXT_ACTION_RAISE_TO,   // uses target_total (the seat's total set for this street)
	TEXAS_EXT_ACTION_ALLIN,
};

void TexasHoldemApplyActionFoldFor(const std::shared_ptr<Game>& game,
                                   const std::shared_ptr<PlayerInterface>& p);
void TexasHoldemApplyActionCheckCallFor(const std::shared_ptr<Game>& game,
                                        const std::shared_ptr<BeroInterface>& bero,
                                        const std::shared_ptr<PlayerInterface>& p);
void TexasHoldemApplyActionRaiseToFor(const std::shared_ptr<Game>& game,
                                      const std::shared_ptr<BeroInterface>& bero,
                                      const std::shared_ptr<PlayerInterface>& p, int target_total);
void TexasHoldemApplyActionAllInFor(const std::shared_ptr<Game>& game,
                                    const std::shared_ptr<BeroInterface>& bero,
                                    const std::shared_ptr<PlayerInterface>& p);
bool TexasHoldemApplyExternalAction(const std::shared_ptr<Game>& game,
                                    const std::shared_ptr<BeroInterface>& bero,
                                    const std::shared_ptr<PlayerInterface>& p,
                                    TexasHoldemExternalActionKind kind, int target_total = 0);

struct TexasHoldemLocalGameOptions : Moveable<TexasHoldemLocalGameOptions> {
	int num_players = 10;
	int start_cash = 2000;
	int game_speed = 4;
	int seed = -1;
	String provider;
	String project_name = "default";
	bool script_automation = false;
};

std::shared_ptr<Game> StartTexasHoldemLocalGame(GameTable& table,
                                                const TexasHoldemLocalGameOptions& options,
                                                class ConfigFile& config, EngineLog& engineLog);
int TexasHoldemCurrentTurnUid(const std::shared_ptr<Game>& game);
bool StepTexasHoldemLocalGameAction(const std::shared_ptr<Game>& game);

END_UPP_NAMESPACE

#endif
