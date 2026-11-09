#ifdef flagMAIN
#include "StartWindow.h"
#include "ClientThread.h"
#include "AvatarManager.h"
#include <GameRules/EngineLog.h>
#include <EditorCommon/ConfigFile.h>
#include <EditorCommon/Tools.h>
#include "GameTable.h"
#include "TexasHoldemLocalGame.h"
#include "TexasHoldemSessionContract.h"
#include <TexasHoldemProviderCatalog/TexasHoldemProviderCatalog.h>
#include <Poker/LocalEngineFactory.h>
#include <GameRules/Game.h>
#include <GameRules/PlayerData.h>
#include <GameRules/HandInterface.h>
#include <GameRules/BeroInterface.h>
#include <GameRules/BoardInterface.h>
#include <GameRules/PlayerInterface.h>
#include "RunLocalGame.h"
#include "RemoteClient.h"
#include <cstdlib>

using namespace Upp;

NAMESPACE_UPP

namespace {
constexpr int LOCAL_GAME_DEFAULT_NUM_PLAYERS = 10;
constexpr int LOCAL_GAME_DEFAULT_START_CASH = 2000;
constexpr int LOCAL_GAME_DEFAULT_SPEED = 4;
constexpr int M01_DEFAULT_WIDTH = 1024;
constexpr int M01_DEFAULT_HEIGHT = 648;

static const TexasHoldemProviderInfo *RequireTexasHoldemCliProvider(const String& provider)
{
	const TexasHoldemProviderInfo *info = TexasHoldemFindProvider(provider);
	if(!info)
		Cerr() << "ERROR: unknown provider '" << provider << "'\n";
	return info;
}

static String CanonicalTexasHoldemCliProvider(const String& provider)
{
	const TexasHoldemProviderInfo *info = RequireTexasHoldemCliProvider(provider);
	return info ? info->id : String();
}

static int TexasHoldemProviderDefaultPlayersOrFallback(const String& provider)
{
	const TexasHoldemProviderInfo *info = TexasHoldemFindProvider(provider);
	return info ? info->default_players : LOCAL_GAME_DEFAULT_NUM_PLAYERS;
}

static void CaptureM01GroundTruth(TexasHoldemGroundTruthRecord& record, const std::shared_ptr<Game>& game)
{
	if (!game)
		return;
	record.game_id = game->getMyGameID();
	record.hand_id = game->getCurrentHandID();
	std::shared_ptr<HandInterface> hand = game->getCurrentHand();
	std::shared_ptr<BeroInterface> bero = hand ? hand->getCurrentBeRo() : nullptr;
	if (hand) {
		record.street = (int)hand->getCurrentRound();
		std::shared_ptr<BoardInterface> board = hand->getBoard();
		if (board) {
			const int *cards = board->getMyCards();
			for (int i = 0; i < 5; i++)
				record.board_cards.Add(cards ? cards[i] : -1);
			record.pot = board->getPot();
		}
	}
	if (bero)
		record.turn_uid = (int)bero->getCurrentPlayersTurnId();
	int player_count = min(10, game->getStartQuantityPlayers());
	for (int i = 0; i < player_count; i++) {
		std::shared_ptr<PlayerInterface> player = game->getPlayerByNumber(i);
		if (!player)
			continue;
		TexasHoldemPlayerSnapshot& ps = record.players.Add();
		ps.seat = i;
		ps.uid = (int)player->getMyUniqueID();
		ps.name = player->getMyName();
		ps.hero = i == 0;
		ps.active = player->getMyActiveStatus();
		ps.stack = player->getMyCash();
		ps.bet = player->getMySet();
		ps.action = (int)player->getMyAction();
		ps.button = player->getMyButton();
		ps.hole_cards <<= player->getMyCards();
	}
}

static int DumpM01SourceContractSample(const String& out_dir, const String& provider,
                                       Size table_size, int num_players, int start_cash,
                                       int game_speed, int seed, const String& session_id, int frame_count,
                                       bool step_actions, bool capture_transient_states,
                                       class ConfigFile& config, EngineLog& engineLog)
{
	String sample_provider = provider.IsEmpty() ? TexasHoldemDefaultProviderId() : provider;
	String sample_session = session_id.IsEmpty() ? "texas-m01-ps6p-sample" : session_id;
	String root = out_dir.IsEmpty() ? AppendFileName(GetCurrentDirectory(), "tmp/texas_m01_sample") : out_dir;
	String frames_dir = AppendFileName(root, "frames");
	RealizeDirectory(frames_dir);
	frame_count = max(1, frame_count);

	// task 0137 (M07): write metadata.json UP FRONT with status="recording" and the
	// already-known target frame_count, so a live/tailing reader can discover the
	// session and its expected frame count as soon as recording begins (not only
	// after the whole process exits). Rewritten once at the very end with
	// status="complete". Old-format sessions have no "status" field at all; readers
	// treat its absence as "complete" (see VsmLiveM01M02SessionSource), so this is a
	// purely additive change — a finalized new session is byte-for-byte identical to
	// what the old code produced except for this one added marker field.
	String meta_path = AppendFileName(root, "metadata.json");
	TexasHoldemSessionMetadata meta;
	meta.session_id = sample_session;
	meta.provider = sample_provider;
	meta.table_width = table_size.cx;
	meta.table_height = table_size.cy;
	meta.seed = seed;
	meta.frame_count = frame_count;
	meta.status = "recording";
	SaveFile(meta_path, StoreAsJson(meta));

	// task 0137 (M07): append one groundtruth.jsonl line per frame as it is captured
	// (flushed each iteration so a tailing reader observes per-frame ground truth
	// appear incrementally), instead of one SaveFile after the whole loop. Opened with
	// FileOut (CREATE|NOWRITESHARE) which truncates any stale file and still permits a
	// concurrent reader to open it FILE_SHARE_READ. The resulting file is byte-for-byte
	// identical to the old single-SaveFile output (same lines, same order).
	String gt_path = AppendFileName(root, "groundtruth.jsonl");
	FileOut gt_out(gt_path);
	if (!gt_out.IsOpen()) {
		Cerr() << "ERROR: cannot open groundtruth for writing: " << gt_path << "\n";
		return 2;
	}

	GameTable table(sample_provider);
	table.SetRect(0, 0, table_size.cx, table_size.cy);
	table.Layout();
	table.RefreshLayoutDeep();
	TexasHoldemLocalGameOptions options;
	options.num_players = num_players;
	options.start_cash = start_cash;
	options.game_speed = game_speed;
	options.seed = seed;
	options.provider = sample_provider;
	options.project_name = "default";
	options.script_automation = true;
	// task 0130: recorder-only opt-in. When --capture-transient-states is set
	// (alongside --step-actions), defer the next-hand deal so a frame showing
	// the showdown reveal can be captured before the next hand overwrites it.
	// Default off -> existing recordings/fixtures behave byte-for-byte as before.
	table.SetDeferNextHandTransition(capture_transient_states && step_actions);
	auto game = StartTexasHoldemLocalGame(table, options, config, engineLog);
	Ctrl::ProcessEvents();
	table.RefreshLayoutDeep();
	table.Layout();

	for (int frame_id = 0; frame_id < frame_count; frame_id++) {
		if (step_actions && frame_id > 0) {
			if (capture_transient_states && table.HasPendingNextHand()) {
				// task 0130: a prior step reached a showdown and deferred the
				// next-hand deal. The reveal frame was already captured on that
				// prior iteration; resolve the deferred deal now as this step's
				// action instead of calling bero->nextPlayer() on the finished
				// hand (which would re-run the post-river resolution).
				int before_turn_uid = TexasHoldemCurrentTurnUid(game);
				table.ResolvePendingNextHand();
				int after_turn_uid = TexasHoldemCurrentTurnUid(game);
				Cout() << "record_step=" << frame_id
				       << " action=deferred_next_hand_deal"
				       << " stepped=1"
				       << " turn_uid_before=" << before_turn_uid
				       << " turn_uid_after=" << after_turn_uid << "\n";
			}
			else {
				int before_turn_uid = TexasHoldemCurrentTurnUid(game);
				bool stepped = StepTexasHoldemLocalGameAction(game);
				int after_turn_uid = TexasHoldemCurrentTurnUid(game);
				Cout() << "record_step=" << frame_id
				       << " action=next_player"
				       << " stepped=" << (stepped ? 1 : 0)
				       << " turn_uid_before=" << before_turn_uid
				       << " turn_uid_after=" << after_turn_uid << "\n";
			}
		}
		Ctrl::ProcessEvents();
		table.RefreshLayoutDeep();
		table.Layout();
		String frame_path = AppendFileName(frames_dir, TexasHoldemFrameName(frame_id));
		if (!table.DumpSnapshot(frame_path, true)) {
			Cerr() << "ERROR: failed to write frame: " << frame_path << "\n";
			return 2;
		}
		Cout() << "record_frame=" << frame_id << " path=" << frame_path << "\n";

		TexasHoldemGroundTruthRecord gt;
		gt.session_id = sample_session;
		gt.frame_id = frame_id;
		gt.render_step = frame_id;
		gt.timestamp_ms = (int64)frame_id * 100;
		gt.provider = sample_provider;
		gt.table_width = table_size.cx;
		gt.table_height = table_size.cy;
		gt.seed = seed;
		CaptureM01GroundTruth(gt, game);
		gt_out << StoreAsJson(gt) << "\n";
		gt_out.Flush();
	}
	gt_out.Close();

	// task 0137 (M07): finalize — rewrite metadata.json with status="complete" as the
	// last write, after every frame PNG and every groundtruth.jsonl line are on disk.
	// This is the signal that flips a tailing reader from "poll/retry" to "done".
	meta.status = "complete";
	SaveFile(meta_path, StoreAsJson(meta));

	Cout() << "source_contract_sample=" << root << "\n";
	Cout() << "metadata=" << AppendFileName(root, "metadata.json") << "\n";
	Cout() << "groundtruth=" << AppendFileName(root, "groundtruth.jsonl") << "\n";
	Cout() << "frames=" << frames_dir << " count=" << frame_count << "\n";
	Cout() << "first_frame=" << AppendFileName(frames_dir, TexasHoldemFrameName(0)) << "\n";
	Cout() << "last_frame=" << AppendFileName(frames_dir, TexasHoldemFrameName(frame_count - 1)) << "\n";
	Cout() << "frame_id=0.." << (frame_count - 1) << " render_step=0.." << (frame_count - 1)
	       << " provider=" << sample_provider << " size=" << table_size
	       << " num_players=" << num_players << " seed=" << seed << "\n";
	Cout().Flush();
	return 0;
}
}

END_UPP_NAMESPACE

#ifdef flagMAIN
GUI_APP_MAIN
{
	const Vector<String>& args = CommandLine();
	
	for (int i = 0; i < args.GetCount(); i++) {
		if (args[i] == "--fastcrash") {
			Upp::InstallPanicMessageBox([](const char* title, const char* text) {
				UPP::Cerr() << "PANIC: " << title << ": " << text << "\n";
				std::_Exit(1);
			});
		}
	}
	
	if (args.GetCount() > 0 && (args[0] == "--help" || args[0] == "-h" || args[0] == "-help")) {
		Cout() << "Usage: CardEngine [options]\n"
		       << "Options:\n"
		       << "  --test-assets          Verify critical assets\n"
		       << "  --dump-render-image    Dump a snapshot of the testing project to tmp/texas_holdem_screenshot.png\n"
		       << "  --dump-layout-rects    Print GameTable layout rectangles and exit\n"
		       << "  --dump-new-game-defaults Print embedded setup defaults and exit\n"
		       << "  --dump-embedded-start  Start embedded local game, print layout/game state, and exit\n"
		       << "  --dump-source-contract-sample Write M01 frame + groundtruth sample session\n"
		       << "  --validate-source-contract-sample <dir> Validate M01 sample session\n"
		       << "  --record-session       Write M02 record session using the TexasHoldem source contract\n"
		       << "  --step-actions         Advance one shared game action before each recorded frame after frame 0\n"
		       << "  --capture-transient-states Defer the next-hand deal after a showdown so the reveal frame is captured (with --step-actions)\n"
		       << "  --replay-session <dir> Validate and print deterministic M02 replay summary\n"
		       << "  --expect-provider <name> Expected provider for --replay-session\n"
		       << "  --expect-size <WxH>    Expected table size for --replay-session\n"
		       << "  --expect-frame-ms <n>  Expected timestamp delta for --replay-session\n"
		       << "  --provider <name>      Select provider/theme such as PS_6p\n"
		       << "  --project <name>       Project name for --dump-render-image (default: testing)\n"
		       << "  --out <path>           Output path for --dump-render-image\n"
		       << "  --test-gameplay        Run a short gameplay simulation test\n"
		       << "  --local-game           Run a standard local game\n"
		       << "  --local-game-script    Run a scripted local game (use --help with this for more sub-options)\n"
		       << "  --connect <addr>       Connect to a remote server\n"
		       << "  --fastcrash            Crash immediately on fatal errors/assertions (no message box)\n";
		return;
	}
	class ConfigFile config(nullptr, false);
	AvatarManager avatarManager;
	auto engineLog = std::make_shared<EngineLog>(&config);
	String cli_provider;
	for (int i = 0; i + 1 < args.GetCount(); ++i)
		if (args[i] == "--provider") {
			cli_provider = args[i + 1];
			break;
		}

	if (args.GetCount() > 0 && args[0] == "--test-assets") {
		String dataDir = Tools::GetDataDir();
		if (DirectoryExists(dataDir)) {
			String testFile = AppendFileName(dataDir, "gfx/cards/default/back.png");
			if (FileExists(testFile)) {
				UPP::Cout() << "SUCCESS: Assets found and accessible.\n";
				Exit(0);
			} else {
				UPP::Cerr() << "ERROR: Critical asset missing: " << testFile << "\n";
				Exit(1);
			}
		} else {
			UPP::Cerr() << "ERROR: Data directory does not exist: " << dataDir << "\n";
			Exit(1);
		}
	}
	
	if (args.GetCount() > 0 &&
	    (args[0] == "--dump-render" || args[0] == "--dump-render-image" || args[0] == "--dump-render-normal")) {
		bool image_mode = args[0] != "--dump-render-normal";
		String out_path = AppendFileName(GetCurrentDirectory(), "tmp/texas_holdem_screenshot.png");
		String project_name = "testing";
		for (int i = 1; i < args.GetCount(); i++) {
			if (args[i] == "--out" && i + 1 < args.GetCount()) {
				out_path = args[++i];
			}
			else if (args[i] == "--project" && i + 1 < args.GetCount()) {
				project_name = args[++i];
			}
		}
		class ConfigFile config(nullptr, false);
		auto engineLog = std::make_shared<EngineLog>(&config);
		GameTable table;
		table.SetRect(0, 0, 1024, 648);
		table.Layout(); // Force layout without opening
		table.SetProjectContext(project_name, "texas-holdem");

		PlayerDataList pdList;
		pdList.push_back(std::make_shared<PlayerData>(0, 0, PLAYER_TYPE_HUMAN, PLAYER_RIGHTS_ADMIN, true));
		pdList.back()->SetName("Player");
		for (int i = 1; i < 6; i++) {
			pdList.push_back(std::make_shared<PlayerData>(i, i, PLAYER_TYPE_COMPUTER, PLAYER_RIGHTS_NORMAL, false));
			pdList.back()->SetName(Format("Computer %d", i));
		}
		GameData gData; gData.maxNumberOfPlayers = 6; gData.startMoney = 1000; gData.firstSmallBlind = 10; gData.guiSpeed = 10;
		StartData sData; sData.numberOfPlayers = 6; sData.startDealerPlayerId = 0;
		auto factory = std::make_shared<LocalEngineFactory>();
		auto game = std::make_shared<Game>(&table, factory, pdList, gData, sData, 1, engineLog.get(), &config);
		table.SetGame(game);
		game->initHand();
		game->startHand();

		// Just a small sleep to let internal state settle, no ProcessEvents
		Sleep(500);

		if (!table.DumpSnapshot(out_path, image_mode)) {
			Cerr() << "ERROR: failed to dump snapshot to " << out_path << "\n";
			std::_Exit(2);
		}
		Cout() << "SUCCESS: Snapshot dumped to " << out_path << " mode=" << (image_mode ? "image" : "normal") << "\n";
		Cout().Flush();
		// Dump-render is a snapshot utility path; force immediate exit to avoid
		// GUI teardown instability in X11 debug runs.
		std::_Exit(0);
	}

	if (args.GetCount() > 0 && args[0] == "--dump-layout-rects") {
		Size size(1024, 648);
		String provider = cli_provider;
		for(int i = 1; i < args.GetCount(); i++) {
			if(args[i] == "--size" && i + 1 < args.GetCount()) {
				Vector<String> part = Split(args[++i], 'x');
				if(part.GetCount() == 2)
					size = Size(max(1, StrInt(part[0])), max(1, StrInt(part[1])));
			}
			else if(args[i] == "--provider" && i + 1 < args.GetCount())
				provider = args[++i];
		}
		provider = CanonicalTexasHoldemCliProvider(provider);
		if(provider.IsEmpty())
			std::_Exit(2);
		GameTable table(provider);
		table.SetRect(0, 0, size.cx, size.cy);
		table.RefreshLayoutDeep();
		table.Layout();
		table.RefreshLayoutDeep();
		table.DumpLayoutRects(Cout());
		Cout().Flush();
		std::_Exit(0);
	}

	if (args.GetCount() > 0 && args[0] == "--dump-new-game-defaults") {
		StartWindow start;
		start.Init(config, engineLog);
		if (!cli_provider.IsEmpty())
			start.SelectProviderForTest(cli_provider);
		start.ShowSetupForTest();
		start.DumpSetupState(Cout());
		Cout().Flush();
		std::_Exit(0);
	}

	if (args.GetCount() > 0 && args[0] == "--dump-embedded-start") {
		Size size(1024, 720);
		String provider = cli_provider;
		int num_players = TexasHoldemProviderDefaultPlayersOrFallback(provider);
		int start_cash = LOCAL_GAME_DEFAULT_START_CASH;
		int game_speed = LOCAL_GAME_DEFAULT_SPEED;
		bool explicit_num_players = false;
		for(int i = 1; i < args.GetCount(); i++) {
			if(args[i] == "--size" && i + 1 < args.GetCount()) {
				Vector<String> part = Split(args[++i], 'x');
				if(part.GetCount() == 2)
					size = Size(max(1, StrInt(part[0])), max(1, StrInt(part[1])));
			}
			else if(args[i] == "--num-players" && i + 1 < args.GetCount()) {
				num_players = StrInt(args[++i]);
				explicit_num_players = true;
			}
			else if(args[i] == "--start-cash" && i + 1 < args.GetCount())
				start_cash = StrInt(args[++i]);
			else if(args[i] == "--game-speed" && i + 1 < args.GetCount())
				game_speed = StrInt(args[++i]);
			else if(args[i] == "--provider" && i + 1 < args.GetCount())
				provider = args[++i];
		}
		provider = CanonicalTexasHoldemCliProvider(provider);
		if(provider.IsEmpty())
			std::_Exit(2);
		if(!explicit_num_players)
			num_players = TexasHoldemProviderDefaultPlayersOrFallback(provider);
		StartWindow start;
		start.Init(config, engineLog);
		start.SetRect(0, 0, size.cx, size.cy);
		start.Layout();
		if (!provider.IsEmpty())
			start.SelectProviderForTest(provider);
		Cout() << "[TexasHoldem] dump-embedded-start provider='" << provider << "' num_players=" << num_players << "\n";
		start.StartLocalGameForTest(num_players, start_cash, game_speed);
		start.Layout();
		Ctrl::ProcessEvents();
		start.DumpEmbeddedGameState(Cout());
		Cout().Flush();
		std::_Exit(0);
	}

	if (args.GetCount() > 0 && args[0] == "--validate-source-contract-sample") {
		String sample_dir = args.GetCount() > 1 ? args[1] : AppendFileName(GetCurrentDirectory(), "tmp/texas_m01_sample");
		int rc = ValidateTexasHoldemSourceSession(sample_dir);
		std::_Exit(rc);
	}

	if (args.GetCount() > 0 && args[0] == "--replay-session") {
		String session_dir = args.GetCount() > 1 ? args[1] : AppendFileName(GetCurrentDirectory(), "tmp/texas_m02_session");
		String expected_provider;
		Size expected_size(0, 0);
		int expected_frame_ms = 0;
		for (int i = 2; i < args.GetCount(); i++) {
			if (args[i] == "--expect-provider" && i + 1 < args.GetCount())
				expected_provider = args[++i];
			else if (args[i] == "--expect-size" && i + 1 < args.GetCount()) {
				Vector<String> part = Split(args[++i], 'x');
				if (part.GetCount() == 2)
					expected_size = Size(max(1, StrInt(part[0])), max(1, StrInt(part[1])));
			}
			else if (args[i] == "--expect-frame-ms" && i + 1 < args.GetCount())
				expected_frame_ms = max(1, StrInt(args[++i]));
		}
		int rc = ReplayTexasHoldemSession(session_dir, expected_provider, expected_size, expected_frame_ms);
		std::_Exit(rc);
	}

	if (args.GetCount() > 0 && (args[0] == "--dump-source-contract-sample" || args[0] == "--record-session")) {
		bool m02_record = args[0] == "--record-session";
		String out_dir = AppendFileName(GetCurrentDirectory(), m02_record ? "tmp/texas_m02_session" : "tmp/texas_m01_sample");
		String provider = cli_provider.IsEmpty() ? TexasHoldemDefaultProviderId() : cli_provider;
		String session_id = m02_record ? "texas-m02-ps6p-recording" : "texas-m01-ps6p-sample";
		Size table_size(M01_DEFAULT_WIDTH, M01_DEFAULT_HEIGHT);
		int num_players = TexasHoldemProviderDefaultPlayersOrFallback(provider);
		int start_cash = LOCAL_GAME_DEFAULT_START_CASH;
		int game_speed = LOCAL_GAME_DEFAULT_SPEED;
		int seed = 1;
		int frame_count = 8;
		bool step_actions = false;
		bool capture_transient_states = false;
		bool explicit_num_players = false;
		for (int i = 1; i < args.GetCount(); i++) {
			if (args[i] == "--out" && i + 1 < args.GetCount())
				out_dir = args[++i];
			else if (args[i] == "--provider" && i + 1 < args.GetCount()) {
				provider = args[++i];
				const TexasHoldemProviderInfo *info = TexasHoldemFindProvider(provider);
				if(info && !explicit_num_players)
					num_players = info->default_players;
			}
			else if (args[i] == "--session-id" && i + 1 < args.GetCount())
				session_id = args[++i];
			else if (args[i] == "--size" && i + 1 < args.GetCount()) {
				Vector<String> part = Split(args[++i], 'x');
				if (part.GetCount() == 2)
					table_size = Size(max(1, StrInt(part[0])), max(1, StrInt(part[1])));
			}
			else if (args[i] == "--num-players" && i + 1 < args.GetCount()) {
				num_players = max(2, StrInt(args[++i]));
				explicit_num_players = true;
			}
			else if (args[i] == "--start-cash" && i + 1 < args.GetCount())
				start_cash = max(100, StrInt(args[++i]));
			else if (args[i] == "--game-speed" && i + 1 < args.GetCount())
				game_speed = max(1, StrInt(args[++i]));
			else if (args[i] == "--seed" && i + 1 < args.GetCount())
				seed = max(0, StrInt(args[++i]));
			else if (args[i] == "--frames" && i + 1 < args.GetCount())
				frame_count = max(1, StrInt(args[++i]));
			else if (args[i] == "--step-actions")
				step_actions = true;
			else if (args[i] == "--capture-transient-states")
				capture_transient_states = true;
		}
		provider = CanonicalTexasHoldemCliProvider(provider);
		if(provider.IsEmpty())
			std::_Exit(2);
		if(!explicit_num_players)
			num_players = TexasHoldemProviderDefaultPlayersOrFallback(provider);
		int rc = DumpM01SourceContractSample(out_dir, provider, table_size, num_players,
		                                     start_cash, game_speed, seed, session_id, frame_count, step_actions,
		                                     capture_transient_states, config, *engineLog);
		std::_Exit(rc);
	}

	if (args.GetCount() > 0 && args[0] == "--test-gameplay") {
		FileAppend testLog("test_gameplay.txt");
		struct StdoutGui : public GuiInterface {
			FileAppend& out;
			std::shared_ptr<Game> m_game;
			StdoutGui(FileAppend& o) : out(o) {}
			virtual bool isTestMode() const override { return true; }
			virtual void SetGame(std::shared_ptr<Game> game) override { m_game = game; }
			virtual void initGui(int speed) override {}
			virtual void refreshGameLabels(TexasRound state) const override {}
			virtual void nextRoundCleanGui() override {}
			virtual void logNewGameHandMsg(int gameID, int HandID) override { 
				out << "HAND START: " << HandID << "\n"; out.Flush();
			}
			virtual void flushLogAtGame(int gameID) override {}
			virtual void logNewBlindsSetsMsg(int sbSet, int bbSet, String sbName, String bbName) override {}
			virtual void flushLogAtHand() override {}
			virtual void dealHoleCards() override {}
			virtual void refreshPot() override {}
			virtual void refreshSet() override {}
			virtual void nextPlayerAnimation() override {
				PostCallback([this] {
					Sleep(100);
					if (m_game && m_game->getCurrentHand()) m_game->getCurrentHand()->getCurrentBeRo()->nextPlayer();
				});
			}
			virtual void flipHolecardsAllIn() override {}
			virtual void logDealBoardCardsMsg(int roundID, int card1, int card2, int card3, int card4, int card5) override {
				out << "BOARD: round=" << roundID << " cards=" << card1 << "," << card2 << "," << card3 << "," << card4 << "," << card5 << "\n";
				out.Flush();
			}
			virtual void refreshGroupbox(int playerID, int type) override {}
			virtual void preflopAnimation1() override { nextPlayerAnimation(); }
			virtual void flopAnimation1() override { nextPlayerAnimation(); }
			virtual void turnAnimation1() override { nextPlayerAnimation(); }
			virtual void riverAnimation1() override { nextPlayerAnimation(); }
			virtual void postRiverAnimation1() override { nextPlayerAnimation(); }
			virtual void logPlayerActionMsg(String playName, int action, int setValue) override {
				out << "ACTION: " << playName << " action=" << action << " value=" << setValue << "\n";
				out.Flush();
			}
			virtual void logFlipHoleCardsMsg(String playerName, int card1, int card2, int cardsValueInt, String showHas) override {}
			virtual void logWinningHandMsg(String playerName, String handName, int amount) override {
				out << "WINNER: " << playerName << " wins " << amount << " with " << handName << "\n"; out.Flush();
			}
			virtual void dealBeRoCards(TexasRound state) override {}
			virtual void beRoAnimation2(TexasRound state) override {}
			virtual void meInAction() override {}
			virtual void postRiverRunAnimation1() override {}
			virtual void refreshCash() override {}
			virtual void refreshAction(int playerID, int action) override {}
			virtual void SignalNetClientError(int errorId, int osErrorCode) override {}
		};
		
		StdoutGui gui(testLog);
		auto factory = std::make_shared<LocalEngineFactory>();
		PlayerDataList pdList;
		for (int i = 0; i < 10; i++) {
			pdList.push_back(std::make_shared<PlayerData>(i, i, i == 0 ? PLAYER_TYPE_HUMAN : PLAYER_TYPE_COMPUTER, PLAYER_RIGHTS_NORMAL, i == 0));
			pdList.back()->SetName(i == 0 ? "Human" : Format("Bot%d", i));
		}
		
		GameData gData; gData.maxNumberOfPlayers = 10; gData.startMoney = 2000; gData.firstSmallBlind = 10;
		StartData sData; sData.numberOfPlayers = 10; sData.startDealerPlayerId = 0;
		
		auto game = std::make_shared<Game>(&gui, factory, pdList, gData, sData, 1, engineLog.get(), &config);
		gui.SetGame(game);
		game->initHand();
		game->startHand();
		
		TimeStop ts;
		while(ts.Elapsed() < 5000) {
			UPP::Ctrl::Ctrl::ProcessEvents();
			Sleep(10);
			if (game->isGameOver()) break;
		}
		UPP::Cout() << "Test complete.\n";
		UPP::Cout().Flush();
		return;
	}

	if (args.GetCount() > 0 && args[0] == "--local-game") {
		String provider = cli_provider;
		int num_players = TexasHoldemProviderDefaultPlayersOrFallback(provider);
		int start_cash = LOCAL_GAME_DEFAULT_START_CASH;
		int game_speed = LOCAL_GAME_DEFAULT_SPEED;
		bool explicit_num_players = false;
		for (int i = 1; i < args.GetCount(); ++i) {
			if (args[i] == "--num-players" && i + 1 < args.GetCount()) {
				num_players = max(2, StrInt(args[++i]));
				explicit_num_players = true;
			}
			else if (args[i] == "--start-cash" && i + 1 < args.GetCount())
				start_cash = max(100, StrInt(args[++i]));
			else if (args[i] == "--game-speed" && i + 1 < args.GetCount())
				game_speed = max(1, StrInt(args[++i]));
			else if (args[i] == "--provider" && i + 1 < args.GetCount())
				provider = args[++i];
		}
		provider = CanonicalTexasHoldemCliProvider(provider);
		if(provider.IsEmpty())
			std::_Exit(2);
		if(!explicit_num_players)
			num_players = TexasHoldemProviderDefaultPlayersOrFallback(provider);
		Cout() << "[TexasHoldem] local-game provider='" << provider << "' num_players=" << num_players
		       << " start_cash=" << start_cash << " game_speed=" << game_speed << "\n";
		RunLocalGame(num_players, start_cash, game_speed, provider, config, *engineLog);
		return;
	}
	
	if (args.GetCount() > 0 && args[0] == "--local-game-script") {
		int num_players = LOCAL_GAME_DEFAULT_NUM_PLAYERS;
		int start_cash = LOCAL_GAME_DEFAULT_START_CASH;
		int game_speed = LOCAL_GAME_DEFAULT_SPEED;
		int max_ticks = 500;
		int sleep_ms = 5;
		int seed = -1;
		bool verbose = false;
		bool headless = false;
		bool has_ticks = false;
		bool auto_human_action = false;
		bool no_wait_between_actions = false;
		bool has_auto_human_arg = false;
		bool has_no_wait_arg = false;
		String project_name = "default";
		String script_path;
		String dump_loop_state_json;
		for (int i = 1; i < args.GetCount(); i++) {
			String a = args[i];
			auto need = [&](const char* name)->String {
				if (i + 1 >= args.GetCount()) {
					Cerr() << "Missing value for " << name << "\n";
					return String();
				}
				return args[++i];
			};
			if (a == "--script") script_path = need("--script");
			else if (a == "--project") project_name = need("--project");
			else if (a == "--num-players") num_players = max(2, StrInt(need("--num-players")));
			else if (a == "--start-cash") start_cash = max(100, StrInt(need("--start-cash")));
			else if (a == "--game-speed") game_speed = max(1, StrInt(need("--game-speed")));
			else if (a == "--ticks") { max_ticks = max(1, StrInt(need("--ticks"))); has_ticks = true; }
			else if (a == "--sleep-ms") sleep_ms = max(0, StrInt(need("--sleep-ms")));
			else if (a == "--seed") seed = max(0, StrInt(need("--seed")));
			else if (a == "--dump-loop-state-json") dump_loop_state_json = need("--dump-loop-state-json");
			else if (a == "--verbose") verbose = true;
			else if (a == "--headless" || a == "--cli") headless = true;
			else if (a == "--auto-human") { auto_human_action = true; has_auto_human_arg = true; }
			else if (a == "--no-wait") { no_wait_between_actions = true; has_no_wait_arg = true; }
			else if (a == "--fastcrash") { /* handled globally */ }
			else {				Cerr() << "Unknown arg: " << a << "\n";
				Cerr() << "Usage: --local-game-script [--project name] [--script path] [--num-players N] [--start-cash N] [--game-speed N] [--ticks N] [--sleep-ms N] [--seed N] [--dump-loop-state-json path] [--verbose] [--headless|--cli] [--auto-human] [--no-wait]\n";
				Exit(1);
				return;
			}
		}
		if (headless) {
			if (!has_auto_human_arg) auto_human_action = true;
			if (!has_no_wait_arg) no_wait_between_actions = true;
		}
		if (!has_ticks && !headless)
			max_ticks = -1; // GUI mode runs until window is closed by user.
		int rc = RunLocalGameScripted(num_players, start_cash, game_speed, config, *engineLog,
		                              project_name, script_path, max_ticks, sleep_ms, seed, verbose, dump_loop_state_json, headless,
		                              auto_human_action, no_wait_between_actions);
		Exit(rc);
		return;
	}

	struct MainGui : public GuiInterface {
		std::shared_ptr<Game> m_game;
		StartWindow startWindow;
		GameTable table;

		MainGui(const String& provider = String()) : table(provider) {}

		virtual void SetGame(std::shared_ptr<Game> game) override { m_game = game; table.SetGame(game); }
		virtual bool isTestMode() const override { return false; }

		virtual void initGui(int speed) override {}
		virtual void refreshGameLabels(TexasRound state) const override {}
		virtual void nextRoundCleanGui() override {}
		virtual void logNewGameHandMsg(int gameID, int HandID) override {}
		virtual void flushLogAtGame(int gameID) override {}
		virtual void logNewBlindsSetsMsg(int sbSet, int bbSet, String sbName, String bbName) override {}
		virtual void flushLogAtHand() override {}
		virtual void dealHoleCards() override { table.dealHoleCards(); }
		virtual void refreshPot() override { table.refreshPot(); }
		virtual void refreshSet() override { table.refreshSet(); }
		virtual void nextPlayerAnimation() override { table.nextPlayerAnimation(); }
		virtual void flipHolecardsAllIn() override { table.flipHolecardsAllIn(); }
		virtual void logDealBoardCardsMsg(int roundID, int card1, int card2, int card3, int card4, int card5) override {
			table.logDealBoardCardsMsg(roundID, card1, card2, card3, card4, card5);
		}
		virtual void refreshGroupbox(int playerID, int type) override { table.refreshGroupbox(playerID, type); }
		virtual void preflopAnimation1() override { table.preflopAnimation1(); }
		virtual void flopAnimation1() override { table.flopAnimation1(); }
		virtual void turnAnimation1() override { table.turnAnimation1(); }
		virtual void riverAnimation1() override { table.riverAnimation1(); }
		virtual void postRiverAnimation1() override { table.postRiverAnimation1(); }
		virtual void logPlayerActionMsg(String playName, int action, int setValue) override {
			table.logPlayerActionMsg(playName, action, setValue);
		}
		virtual void logFlipHoleCardsMsg(String playerName, int card1, int card2, int cardsValueInt, String showHas) override {
			table.logFlipHoleCardsMsg(playerName, card1, card2, cardsValueInt, showHas);
		}
		virtual void logWinningHandMsg(String playerName, String handName, int amount) override {
			table.logWinningHandMsg(playerName, handName, amount);
		}
		virtual void dealBeRoCards(TexasRound state) override { table.dealBeRoCards(state); }
		virtual void beRoAnimation2(TexasRound state) override { table.beRoAnimation2(state); }
		virtual void meInAction() override { table.meInAction(); }
		virtual void postRiverRunAnimation1() override { table.postRiverRunAnimation1(); }
		virtual void refreshCash() override { table.refreshCash(); }
		virtual void refreshAction(int playerID, int action) override { table.refreshAction(playerID, action); }
		virtual void SignalNetClientError(int errorId, int osErrorCode) override { table.SignalNetClientError(errorId, osErrorCode); }
	};

	MainGui mainGui(cli_provider);
	if (!cli_provider.IsEmpty())
		mainGui.startWindow.SelectProviderForTest(cli_provider);
	RemoteClient remote;

	for (int i = 0; i < args.GetCount(); i++) {
		if (args[i] == "--connect" && i + 1 < args.GetCount()) {
			remote.WhenUpdate = [&mainGui](const ScreenGameState& s) {
				mainGui.table.SetRemoteState(s);
			};
			remote.Start(args[i + 1]);
			mainGui.startWindow.Hide();
			mainGui.table.OpenMain();
			break;
		}
	}

	if (!mainGui.table.IsOpen()) {
		mainGui.startWindow.Init(config, engineLog);
		mainGui.startWindow.Run();
	} else {
		Ctrl::EventLoop();
	}
}
#endif

#endif
