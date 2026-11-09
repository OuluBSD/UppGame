#include "GameTable.h"
#include "TexasHoldemLocalGame.h"
#include <EditorCommon/EditorCommon.h>
#include <Poker/LocalEngineFactory.h>
#include <GameRules/Game.h>
#include <GameRules/PlayerData.h>
#include <GameRules/HandInterface.h>
#include <GameRules/BeroInterface.h>
#include <GameRules/BoardInterface.h>
#include <GameRules/PlayerInterface.h>
#include <GameRules/GuiInterface.h>
#include <EditorCommon/ConfigFile.h>
#include <GameRules/EngineLog.h>
#include <ByteVM/ByteVM.h>

using namespace Upp;

NAMESPACE_UPP

void InitLocalGame(GameTable& table, int numPlayers, int startCash, int gameSpeed, const String& provider,
                   class ConfigFile& config, EngineLog& engineLog);

void RunLocalGame(int numPlayers, int startCash, int gameSpeed, const String& provider,
                  class ConfigFile& config, EngineLog& engineLog)
{
	auto table = std::make_shared<GameTable>(provider);
	InitLocalGame(*table, numPlayers, startCash, gameSpeed, provider, config, engineLog);
	table->Run();
}

void InitLocalGame(GameTable& table, int numPlayers, int startCash, int gameSpeed, const String& provider,
                   class ConfigFile& config, EngineLog& engineLog)
{
	TexasHoldemLocalGameOptions options;
	options.num_players = numPlayers;
	options.start_cash = startCash;
	options.game_speed = gameSpeed;
	options.provider = provider;
	options.project_name = "default";
	options.script_automation = false;
	StartTexasHoldemLocalGame(table, options, config, engineLog);
}

namespace {

struct ScriptLoopContext {
	std::shared_ptr<Game> game;
	std::shared_ptr<GameTable> table;
	bool verbose = false;
	bool auto_human_action = false;
	bool no_wait_between_actions = false;
	int64 next_action_due_ms = 0;
};

static ScriptLoopContext* g_loop_ctx = nullptr;

static std::shared_ptr<BeroInterface> GetCurrentBero(const std::shared_ptr<Game>& game) {
	if (!game)
		return nullptr;
	std::shared_ptr<HandInterface> hand = game->getCurrentHand();
	if (!hand)
		return nullptr;
	return hand->getCurrentBeRo();
}

// NOTE: GetHumanOnTurn/ApplyHumanFold/ApplyHumanCheckCall/ApplyHumanRaiseTo/
// ApplyHumanAllIn used to be defined here as file-local statics. They have
// been moved to TexasHoldemLocalGame.cpp (as TexasHoldemGetHumanOnTurn/
// TexasHoldemApplyHuman*) so StepTexasHoldemLocalGameAction's automated
// `--step-actions` driver can share the exact same proven check/call math
// instead of reimplementing it. See task 0106. All call sites below now use
// the TexasHoldem-prefixed shared versions.

static PyValue builtin_log(const Vector<PyValue>& args, void*) {
	if (g_loop_ctx && g_loop_ctx->verbose && args.GetCount() > 0)
		Cout() << "SCRIPT: " << args[0].ToString() << "\n";
	return PyValue::None();
}

static PyValue builtin_auto_human_enabled(const Vector<PyValue>&, void*) {
	if (!g_loop_ctx)
		return PyValue(false);
	return PyValue(g_loop_ctx->auto_human_action);
}

static PyValue builtin_has_hand(const Vector<PyValue>&, void*) {
	if (!g_loop_ctx || !g_loop_ctx->game)
		return PyValue(false);
	return PyValue((bool)g_loop_ctx->game->getCurrentHand());
}

static PyValue builtin_is_game_over(const Vector<PyValue>&, void*) {
	if (!g_loop_ctx || !g_loop_ctx->game)
		return PyValue(true);
	return PyValue(g_loop_ctx->game->isGameOver());
}

static PyValue builtin_get_pot(const Vector<PyValue>&, void*) {
	if (!g_loop_ctx || !g_loop_ctx->game)
		return PyValue((int64)0);
	std::shared_ptr<HandInterface> hand = g_loop_ctx->game->getCurrentHand();
	if (!hand)
		return PyValue((int64)0);
	std::shared_ptr<BoardInterface> board = hand->getBoard();
	if (!board)
		return PyValue((int64)0);
	return PyValue((int64)board->getPot());
}

static PyValue builtin_get_stack(const Vector<PyValue>& args, void*) {
	if (!g_loop_ctx || !g_loop_ctx->game || args.GetCount() < 1)
		return PyValue((int64)0);
	int idx = (int)args[0].AsInt();
	std::shared_ptr<PlayerInterface> p = g_loop_ctx->game->getPlayerByNumber(idx);
	if (!p)
		return PyValue((int64)0);
	return PyValue((int64)p->getMyCash());
}

static PyValue builtin_get_round(const Vector<PyValue>&, void*) {
	if (!g_loop_ctx || !g_loop_ctx->game)
		return PyValue((int64)-1);
	std::shared_ptr<HandInterface> hand = g_loop_ctx->game->getCurrentHand();
	if (!hand)
		return PyValue((int64)-1);
	return PyValue((int64)(int)hand->getCurrentRound());
}

static PyValue builtin_start_new_hand(const Vector<PyValue>&, void*) {
	if (!g_loop_ctx || !g_loop_ctx->game)
		return PyValue::None();
	try {
		g_loop_ctx->game->initHand();
		g_loop_ctx->game->startHand();
	}
	catch (const Exc&) {
		return PyValue::None();
	}
	return PyValue::None();
}

static PyValue builtin_get_turn_id(const Vector<PyValue>&, void*) {
	if (!g_loop_ctx || !g_loop_ctx->game)
		return PyValue((int64)-1);
	std::shared_ptr<BeroInterface> bero = GetCurrentBero(g_loop_ctx->game);
	if (!bero)
		return PyValue((int64)-1);
	return PyValue((int64)(int)bero->getCurrentPlayersTurnId());
}

static PyValue builtin_is_human_turn(const Vector<PyValue>&, void*) {
	if (!g_loop_ctx || !g_loop_ctx->game)
		return PyValue(false);
	std::shared_ptr<BeroInterface> bero = GetCurrentBero(g_loop_ctx->game);
	if (!bero)
		return PyValue(false);
	int turn_id = (int)bero->getCurrentPlayersTurnId();
	if (turn_id < 0)
		return PyValue(false);
	std::shared_ptr<PlayerInterface> p = g_loop_ctx->game->getPlayerByUniqueId((unsigned)turn_id);
	if (!p)
		return PyValue(false);
	return PyValue(p->getMyType() == PLAYER_TYPE_HUMAN);
}

static PyValue builtin_set_human_action(const Vector<PyValue>& args, void*) {
	if (!g_loop_ctx || !g_loop_ctx->game || args.GetCount() < 1)
		return PyValue::None();
	std::shared_ptr<BeroInterface> bero = GetCurrentBero(g_loop_ctx->game);
	if (!bero)
		return PyValue::None();
	int action_id = (int)args[0].AsInt();
	if (action_id == 0) TexasHoldemApplyHumanFold(g_loop_ctx->game, bero);
	else if (action_id == 1) TexasHoldemApplyHumanCheckCall(g_loop_ctx->game, bero);
	else if (action_id == 2) {
		int target_total = bero->getHighestSet() + max(bero->getMinimumRaise(), 1);
		TexasHoldemApplyHumanRaiseTo(g_loop_ctx->game, bero, target_total);
	}
	else if (action_id == 3) TexasHoldemApplyHumanAllIn(g_loop_ctx->game, bero);
	return PyValue::None();
}

static PyValue builtin_set_human_bet(const Vector<PyValue>& args, void*) {
	if (!g_loop_ctx || !g_loop_ctx->game || args.GetCount() < 1)
		return PyValue::None();
	std::shared_ptr<BeroInterface> bero = GetCurrentBero(g_loop_ctx->game);
	if (!bero)
		return PyValue::None();
	int target_total = (int)args[0].AsInt();
	TexasHoldemApplyHumanRaiseTo(g_loop_ctx->game, bero, target_total);
	return PyValue::None();
}

static PyValue builtin_next_player(const Vector<PyValue>&, void*) {
	if (!g_loop_ctx || !g_loop_ctx->game)
		return PyValue::None();
	if (g_loop_ctx->table && g_loop_ctx->table->IsActionFlowPaused())
		return PyValue::None();
	if (!g_loop_ctx->no_wait_between_actions) {
		int delay_ms = g_loop_ctx->table ? g_loop_ctx->table->GetActionDelayMs() : 0;
		if (delay_ms > 0) {
			int64 now = msecs();
			if (now < g_loop_ctx->next_action_due_ms)
				return PyValue::None();
			g_loop_ctx->next_action_due_ms = now + delay_ms;
		}
	}
	std::shared_ptr<BeroInterface> bero = GetCurrentBero(g_loop_ctx->game);
	if (bero)
		bero->nextPlayer();
	return PyValue::None();
}

class LocalLoopScript {
	PyVM vm;
	Vector<PyIR> ir;
	String error;

public:
	LocalLoopScript() {
		vm.GetGlobalsRW().SetItem(PyValue("log"), PyValue::Function("log", builtin_log));
		vm.GetGlobalsRW().SetItem(PyValue("auto_human_enabled"), PyValue::Function("auto_human_enabled", builtin_auto_human_enabled));
		vm.GetGlobalsRW().SetItem(PyValue("has_hand"), PyValue::Function("has_hand", builtin_has_hand));
		vm.GetGlobalsRW().SetItem(PyValue("is_game_over"), PyValue::Function("is_game_over", builtin_is_game_over));
		vm.GetGlobalsRW().SetItem(PyValue("get_pot"), PyValue::Function("get_pot", builtin_get_pot));
		vm.GetGlobalsRW().SetItem(PyValue("get_stack"), PyValue::Function("get_stack", builtin_get_stack));
		vm.GetGlobalsRW().SetItem(PyValue("get_round"), PyValue::Function("get_round", builtin_get_round));
		vm.GetGlobalsRW().SetItem(PyValue("start_new_hand"), PyValue::Function("start_new_hand", builtin_start_new_hand));
		vm.GetGlobalsRW().SetItem(PyValue("get_turn_id"), PyValue::Function("get_turn_id", builtin_get_turn_id));
		vm.GetGlobalsRW().SetItem(PyValue("is_human_turn"), PyValue::Function("is_human_turn", builtin_is_human_turn));
		vm.GetGlobalsRW().SetItem(PyValue("set_human_action"), PyValue::Function("set_human_action", builtin_set_human_action));
		vm.GetGlobalsRW().SetItem(PyValue("set_human_bet"), PyValue::Function("set_human_bet", builtin_set_human_bet));
		vm.GetGlobalsRW().SetItem(PyValue("next_player"), PyValue::Function("next_player", builtin_next_player));
	}

	bool Load(const String& code, ScriptLoopContext& ctx) {
		error.Clear();
		try {
			Tokenizer tokenizer;
			tokenizer.SkipPythonComments(true);
			if (!tokenizer.Process(code, "texas_holdem_loop.py")) {
				error = "tokenization failed";
				return false;
			}
			tokenizer.CombineTokens();
			ir.Clear();
			PyCompiler compiler(tokenizer.GetTokens());
			compiler.Compile(ir);
			vm.SetIR(ir);
			g_loop_ctx = &ctx;
			vm.Run();
			g_loop_ctx = nullptr;
			if (vm.GetGlobals().GetItem(PyValue("game_loop_tick")).IsNone()) {
				error = "missing function: game_loop_tick";
				return false;
			}
			return true;
		}
		catch (const Exc& e) {
			g_loop_ctx = nullptr;
			error = e;
			return false;
		}
	}

	bool Tick(ScriptLoopContext& ctx) {
		error.Clear();
		try {
			PyValue func = vm.GetGlobals().GetItem(PyValue("game_loop_tick"));
			if (func.IsNone()) {
				error = "missing function: game_loop_tick";
				return false;
			}
			g_loop_ctx = &ctx;
			vm.Call(func, Vector<PyValue>());
			g_loop_ctx = nullptr;
			return true;
		}
		catch (const Exc& e) {
			g_loop_ctx = nullptr;
			error = e;
			return false;
		}
	}

	const String& GetError() const { return error; }
};

static String GetDefaultLoopScript() {
	return
		"def game_loop_tick():\n"
		"    if not has_hand():\n"
		"        start_new_hand()\n"
		"        return\n"
		"    turn_id = get_turn_id()\n"
		"    if turn_id >= 0:\n"
		"        if not auto_human_enabled():\n"
		"            return\n"
		"        if is_human_turn():\n"
		"            # 0=fold, 1=check, 2=raise, 3=allin\n"
		"            set_human_action(1)\n"
		"        next_player()\n"
		"    elif not is_game_over():\n"
		"        start_new_hand()\n";
}

static String GetDefaultLoopScriptPath() {
	return AppendFileName(AppendFileName(GetCurrentDirectory(), "gamescreen/default/platforms"),
	                     "texas-holdem.local-loop.py");
}

struct LoopStateRecord : Moveable<LoopStateRecord> {
	int tick = 0;
	int hand_id = 0;
	int round = -1;
	int turn_id = -1;
	int pot = 0;
	int hero_stack = 0;
	int hero_bet = 0;
	bool hero_turn = false;

	void Jsonize(JsonIO& jio) {
		jio("tick", tick)
		   ("hand_id", hand_id)
		   ("round", round)
		   ("turn_id", turn_id)
		   ("pot", pot)
		   ("hero_stack", hero_stack)
		   ("hero_bet", hero_bet)
		   ("hero_turn", hero_turn);
	}
};

struct LoopStateDump : Moveable<LoopStateDump> {
	String project;
	String script_path;
	int seed = -1;
	int max_ticks = 0;
	Vector<LoopStateRecord> states;

	void Jsonize(JsonIO& jio) {
		jio("project", project)
		   ("script_path", script_path)
		   ("seed", seed)
		   ("max_ticks", max_ticks)
		   ("states", states);
	}
};

class ScriptLoopGui : public GuiInterface {
public:
	virtual bool isTestMode() const override { return true; }
	virtual void initGui(int) override {}
	virtual void refreshGameLabels(TexasRound) const override {}
	virtual void nextRoundCleanGui() override {}
	virtual void logNewGameHandMsg(int, int) override {}
	virtual void flushLogAtGame(int) override {}
	virtual void logNewBlindsSetsMsg(int, int, String, String) override {}
	virtual void flushLogAtHand() override {}
	virtual void dealHoleCards() override {}
	virtual void refreshPot() override {}
	virtual void refreshSet() override {}
	virtual void nextPlayerAnimation() override {}
	virtual void flipHolecardsAllIn() override {}
	virtual void logDealBoardCardsMsg(int, int, int, int, int = -1, int = -1) override {}
	virtual void refreshGroupbox(int, int) override {}
	virtual void preflopAnimation1() override {}
	virtual void flopAnimation1() override {}
	virtual void turnAnimation1() override {}
	virtual void riverAnimation1() override {}
	virtual void postRiverAnimation1() override {}
	virtual void logPlayerActionMsg(String, int, int) override {}
	virtual void logFlipHoleCardsMsg(String, int, int, int = -1, String = "shows") override {}
	virtual void logWinningHandMsg(String, String, int) override {}
	virtual void dealBeRoCards(TexasRound) override {}
	virtual void beRoAnimation2(TexasRound) override {}
	virtual void meInAction() override {}
	virtual void postRiverRunAnimation1() override {}
	virtual void refreshCash() override {}
	virtual void refreshAction(int, int) override {}
	virtual void SignalNetClientError(int, int) override {}
};

}

int RunLocalGameScripted(int numPlayers, int startCash, int gameSpeed, class ConfigFile& config, EngineLog& engineLog,
                         const String& project_name, const String& script_path, int max_ticks, int sleep_ms,
                         int seed, bool verbose, const String& dump_loop_state_json, bool headless,
                         bool auto_human_action, bool no_wait_between_actions)
{
	PlayerDataList pdList;
	String humanNick = config.readConfigString("Nick");
	if (humanNick.IsEmpty())
		humanNick = "Player";
	pdList.push_back(std::make_shared<PlayerData>(0, 0, PLAYER_TYPE_HUMAN, PLAYER_RIGHTS_ADMIN, true));
	pdList.back()->SetName(humanNick);
	for (int i = 1; i < numPlayers; i++) {
		pdList.push_back(std::make_shared<PlayerData>(i, i, PLAYER_TYPE_COMPUTER, PLAYER_RIGHTS_NORMAL, false));
		pdList.back()->SetName(Format("Computer %d", i));
	}

	GameData gData;
	gData.maxNumberOfPlayers = numPlayers;
	gData.startMoney = startCash;
	gData.firstSmallBlind = 10;
	gData.guiSpeed = gameSpeed;
	StartData sData;
	sData.numberOfPlayers = numPlayers;
	sData.startDealerPlayerId = 0;

	std::shared_ptr<GameTable> table;
	std::shared_ptr<GuiInterface> gui;
	if (headless)
		gui = std::make_shared<ScriptLoopGui>();
	else {
		table = std::make_shared<GameTable>();
		gui = table;
	}
	auto factory = std::make_shared<LocalEngineFactory>();
	auto game = std::make_shared<Game>(gui.get(), factory, pdList, gData, sData, 1, &engineLog, &config);
	if (seed >= 0)
		game->SetBaseSeed(seed);
	const bool script_controls_turns = headless || auto_human_action || no_wait_between_actions;
	if (table) {
		table->SetGame(game);
		table->SetProjectContext(project_name, "texas-holdem");
		table->SetScriptAutomationEnabled(script_controls_turns);
	}

	String active_script_path = script_path;
	if (active_script_path.IsEmpty()) {
		String platform_loop = GetPlatformLocalLoopScriptPath(project_name, "texas-holdem");
		if (FileExists(platform_loop))
			active_script_path = platform_loop;
	}
	if (active_script_path.IsEmpty())
		active_script_path = ResolveProjectLocalLoopScriptPath(project_name);
	if (active_script_path.IsEmpty()) {
		String default_path = GetDefaultLoopScriptPath();
		if (FileExists(default_path))
			active_script_path = default_path;
	}

	String code;
	bool script_from_file = false;
	Time script_mtime;
	if (!active_script_path.IsEmpty()) {
		if (!FileExists(active_script_path)) {
			Cerr() << "error: loop script not found: " << active_script_path << "\n";
			return 2;
		}
		code = LoadFile(active_script_path);
		script_mtime = FileGetTime(active_script_path);
		script_from_file = true;
	}
	if (code.IsEmpty())
		code = GetDefaultLoopScript();

	ScriptLoopContext ctx;
	ctx.game = game;
	ctx.table = table;
	ctx.verbose = verbose;
	ctx.auto_human_action = auto_human_action;
	ctx.no_wait_between_actions = no_wait_between_actions;
	std::unique_ptr<LocalLoopScript> loop = std::make_unique<LocalLoopScript>();
	if (!loop->Load(code, ctx)) {
		Cerr() << "error: failed to load loop script: " << loop->GetError() << "\n";
		return 3;
	}
	if (verbose && script_from_file)
		Cout() << "loop_script_loaded path=" << active_script_path << "\n";

	LoopStateDump dump;
	dump.project = project_name;
	dump.script_path = active_script_path;
	dump.seed = seed;
	dump.max_ticks = max_ticks;

	game->initHand();
	game->startHand();

	const bool unlimited_ticks = max_ticks < 0;
	int tick = 0;
	auto run_tick = [&]() -> bool {
		if (script_from_file) {
			Time now_mtime = FileGetTime(active_script_path);
			if (now_mtime != script_mtime) {
				String reloaded_code = LoadFile(active_script_path);
				if (!reloaded_code.IsEmpty()) {
					std::unique_ptr<LocalLoopScript> reloaded = std::make_unique<LocalLoopScript>();
					if (reloaded->Load(reloaded_code, ctx)) {
						loop = std::move(reloaded);
						script_mtime = now_mtime;
						if (verbose)
							Cout() << "loop_script_reloaded path=" << active_script_path << " tick=" << tick << "\n";
					}
					else {
						Cerr() << "warning: loop script reload failed at tick " << tick << ": " << reloaded->GetError() << "\n";
					}
				}
			}
		}
		// In GUI default mode we keep native C++ turn flow (manual hero actions, slider-paced bots).
		// Script turn-driving is only enabled for testing codepaths (headless/--auto-human/--no-wait).
		if (script_controls_turns) {
			if (!loop->Tick(ctx)) {
				Cerr() << "error: loop script tick failed at tick " << tick << ": " << loop->GetError() << "\n";
				return false;
			}
		}
		{
			LoopStateRecord& rec = dump.states.Add();
			rec.tick = tick;
			rec.hand_id = game->getCurrentHandID();
			auto hand = game->getCurrentHand();
			if (hand) {
				rec.round = (int)hand->getCurrentRound();
				auto board = hand->getBoard();
				if (board)
					rec.pot = board->getPot();
				auto bero = hand->getCurrentBeRo();
				if (bero)
					rec.turn_id = (int)bero->getCurrentPlayersTurnId();
			}
			auto hero = game->getPlayerByNumber(0);
			if (hero) {
				rec.hero_stack = hero->getMyCash();
				rec.hero_bet = hero->getMySet();
				rec.hero_turn = hero->getMyTurn();
			}
		}
		Ctrl::ProcessEvents();
		if (sleep_ms > 0)
			Sleep(sleep_ms);
		tick++;
		return true;
	};

	if (table) {
		table->Open();
		while (table->IsOpen()) {
			if (!unlimited_ticks && tick >= max(1, max_ticks))
				break;
			if (!run_tick())
				return 4;
			if (game->isGameOver() && unlimited_ticks)
				break;
		}
		if (table->IsOpen())
			table->Close();
	}
	else {
		while (unlimited_ticks || tick < max(1, max_ticks)) {
			if (!run_tick())
				return 4;
			if (game->isGameOver())
				break;
		}
	}

	if (!dump_loop_state_json.IsEmpty()) {
		RealizeDirectory(GetFileFolder(dump_loop_state_json));
		SaveFile(dump_loop_state_json, StoreAsJson(dump));
		if (verbose)
			Cout() << "loop_state_dump=" << dump_loop_state_json << "\n";
	}

	Cout() << Format("script_loop_done hand_id=%d game_over=%d\n", game->getCurrentHandID(), game->isGameOver() ? 1 : 0);
	return 0;
}

END_UPP_NAMESPACE
