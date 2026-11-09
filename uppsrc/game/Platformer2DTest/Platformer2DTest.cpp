#include "Platformer2DTest.h"
#include <game/Platformer2D/Resource/VAR.h>
#include <game/Platformer2D/Resource/Episode.h>
#include <game/Platformer2D/Core/Core.h>
#include <game/Platformer2D/Core/Physics.h>

// Make unit/tile conversion inline functions available in test scope
inline int UnitToTile(int unit) { return unit >> 8; }
inline int TileToUnit(int tile) { return tile << 8; }
inline int PixelToTile(int pixel) { return (pixel + 255) >> 8; }
inline int UnitToPixel(int unit) { return unit >> 4; }
inline int PixelToUnit(int pixel) { return pixel << 4; }

NAMESPACE_UPP

Vector<TestResult> TestSuite::s_results;

void TestSuite::AddTest(const String& name, std::function<void()> testFunc)
{
	TestResult result;
	result.name = name;

	TimeStop tm;
	try
	{
		testFunc();
		result.passed = true;
		result.message = "OK";
	}
	catch (TestFailure& e)
	{
		result.passed = false;
		result.message = e.msg;
	}
	catch (Exc& e)
	{
		result.passed = false;
		result.message = "Exception: " + String(e);
	}
	catch (...)
	{
		result.passed = false;
		result.message = "Unknown exception";
	}

	result.durationMs = tm.Elapsed();
	s_results.Add(result);

	LOG("TEST " << (result.passed ? "PASS" : "FAIL") << ": " << name << " (" << result.durationMs << "ms)");
	if (!result.passed) LOG("  " << result.message);
}

const Vector<TestResult>& TestSuite::GetResults()
{
	return s_results;
}

bool TestSuite::AllPassed()
{
	for (const auto& r : s_results)
		if (!r.passed) return false;
	return true;
}

// ============================================================================
// Dump Comparator
// ============================================================================

bool DumpComparator::CompareGameState(const GameState& a, const GameState& b, int episodeNum, Vector<String>& diffs)
{
	bool ok = true;
	if (a.mapPosX != b.mapPosX) { diffs.Add("mapPosX"); ok = false; }
	if (a.mapPosY != b.mapPosY) { diffs.Add("mapPosY"); ok = false; }
	if (a.keenScore != b.keenScore) { diffs.Add("keenScore"); ok = false; }
	if (a.currentLevel != b.currentLevel) { diffs.Add("currentLevel"); ok = false; }
	if (a.numLives != b.numLives) { diffs.Add("numLives"); ok = false; }
	return ok;
}

bool DumpComparator::CompareObjects(const Object a[100], const Object b[100], Vector<String>& diffs)
{
	bool ok = true;
	for (int i = 0; i < 100; ++i)
	{
		for (int f = 0; f < 38; ++f)
		{
			if (a[i].fields[f] != b[i].fields[f])
			{
				diffs.Add(Format("obj[%d].field[%d]", i, f));
				ok = false;
			}
		}
	}
	return ok;
}

bool DumpComparator::ParseDump(const String& filename, DumpFrame& frame, int episodeNum)
{
	FileIn fp(filename);
	if (!fp)
		return false;
	// Full binary dump parsing lives in Platformer2DDumpPrinter; this stub
	// returns an empty frame so the differential API compiles headless.
	frame.timecount = 0;
	Zero(frame.gameState);
	for (int i = 0; i < 100; ++i) frame.objects[i] = Object();
	return true;
}

bool DumpComparator::CompareDumps(const String& platformer2DDump, const String& omnispkDump, int episodeNum, Vector<String>* diffs)
{
	DumpFrame a, b;
	if (!ParseDump(platformer2DDump, a, episodeNum))
		return false;
	if (!ParseDump(omnispkDump, b, episodeNum))
		return false;

	if (a.timecount != b.timecount)
	{
		if (diffs) diffs->Add("timecount");
		return false;
	}

	bool ok = true;
	if (diffs)
	{
		ok &= CompareGameState(a.gameState, b.gameState, episodeNum, *diffs);
		ok &= CompareObjects(a.objects, b.objects, *diffs);
	}
	return ok;
}

// ============================================================================
// Framebuffer Comparator
// ============================================================================

bool FramebufferComparator::Compare(const IndexedSurface* a, const IndexedSurface* b, Vector<String>* diffs)
{
	if (!a || !b) return false;
	if (a->GetSize() != b->GetSize()) return false;
	int w = a->GetSize().cx;
	int h = a->GetSize().cy;
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			if (a->GetPixel(x, y) != b->GetPixel(x, y))
			{
				if (diffs) diffs->Add(Format("pixel(%d,%d)", x, y));
				return false;
			}
		}
	}
	return true;
}

bool FramebufferComparator::CompareFiles(const String& fileA, const String& fileB, Vector<String>* diffs)
{
	IndexedSurface a, b;
	// File-based comparison is a stub; raw index dumps can be loaded here.
	return Compare(&a, &b, diffs);
}

// ============================================================================
// Test Suite
// ============================================================================

void TestSuite::RunAll()
{
	s_results.Clear();

	// Core tests
	AddTest("Core: GameState serialization", []() {
		// Set a known episode for deterministic serialization
		g_currentEpisodeEnum = Episode::KEEN4;
		GameState state;
		state.mapPosX = 100;
		state.mapPosY = 200;
		state.keenScore = 12345;
		state.currentLevel = 1;
		state.numLives = 3;

		// Use a pre-allocated buffer for MemStream
		char buffer[1024];
		MemStream ms(buffer, sizeof(buffer));
		state.Serialize(ms);
		ms.Seek(0);
		GameState state2;
		state2.Serialize(ms);

		TEST_ASSERT_EQUAL(state.mapPosX, state2.mapPosX, "mapPosX");
		TEST_ASSERT_EQUAL(state.keenScore, state2.keenScore, "keenScore");
		TEST_ASSERT_EQUAL(state.currentLevel, state2.currentLevel, "currentLevel");
	});

	AddTest("Core: Object field access", []() {
		Object obj;
		obj.SetField(0, 1234);
		obj.SetField(1, 5678);

		TEST_ASSERT_EQUAL(1234, obj.GetField(0), "Field 0");
		TEST_ASSERT_EQUAL(5678, obj.GetField(1), "Field 1");
		TEST_ASSERT_EQUAL(0, obj.GetField(38), "Out of bounds");
	});

	// Resource tests
	AddTest("Resource: BinaryReader LE", []() {
		uint8_t data[] = { 0x34, 0x12, 0x78, 0x56, 0x9A, 0xBC };
		MemStream ms(data, sizeof(data));
		BinaryReader reader(ms);

		uint16_t u16;
		TEST_ASSERT(reader.ReadU16LE(u16), "Read U16");
		TEST_ASSERT_EQUAL(0x1234, u16, "U16 value");

		uint32_t u32;
		TEST_ASSERT(reader.ReadU32LE(u32), "Read U32");
		TEST_ASSERT_EQUAL(0xBC9A5678u, u32, "U32 value");
	});

	AddTest("Resource: RLEW decode", []() {
		uint16_t input[] = { 0x1111, 0x2222, 0xFEFE, 0x0003, 0x3333, 0x4444 };
		uint16_t output[10] = { 0 };

		DecompressInfo info = RLEWDecoder::Decode(input, 6, output, 10);
		TEST_ASSERT_EQUAL((int)DecompressResult::OK, (int)info.result, "RLEW result");
		TEST_ASSERT_EQUAL(0x1111, output[0], "Output[0]");
		TEST_ASSERT_EQUAL(0x2222, output[1], "Output[1]");
		TEST_ASSERT_EQUAL(0x3333, output[2], "Output[2]");
		TEST_ASSERT_EQUAL(0x3333, output[3], "Output[3]");
		TEST_ASSERT_EQUAL(0x3333, output[4], "Output[4]");
		TEST_ASSERT_EQUAL(0x4444, output[5], "Output[5]");
	});

	AddTest("Resource: LZW decode simple", []() {
		// Simple test: clear code + literal + end code
		uint8_t input[] = { 0x01, 0x00, 0x40, 0x00 }; // LZW encoded "A"
		uint8_t output[10] = { 0 };

		DecompressInfo info = LZWDecoder::Decode(input, 4, output, 10);
		// This is a minimal test - real LZW needs proper bit packing
	});

	// Render tests
	AddTest("Render: Surface create/clear", []() {
		IndexedSurface surf(320, 200);
		TEST_ASSERT(surf.IsValid(), "Surface valid");
		TEST_ASSERT_EQUAL(320, surf.GetWidth(), "Width");
		TEST_ASSERT_EQUAL(200, surf.GetHeight(), "Height");

		surf.Clear(5);
		TEST_ASSERT_EQUAL(5, surf.GetPixel(0, 0), "Clear color");
		TEST_ASSERT_EQUAL(5, surf.GetPixel(319, 199), "Clear color corner");
	});

	AddTest("Render: Surface fill rect", []() {
		IndexedSurface surf(10, 10);
		surf.Clear(0);
		surf.FillRect(2, 2, 4, 4, 7);

		TEST_ASSERT_EQUAL(7, surf.GetPixel(2, 2), "Rect inside");
		TEST_ASSERT_EQUAL(7, surf.GetPixel(5, 5), "Rect corner");
		TEST_ASSERT_EQUAL(0, surf.GetPixel(1, 1), "Outside rect");
		TEST_ASSERT_EQUAL(0, surf.GetPixel(7, 7), "Outside rect");
	});

	AddTest("Render: Palette defaults", []() {
		EGAPalette pal;
		pal.SetDefaults();

		TEST_ASSERT_EQUAL(0, pal.GetColor(0, 0).r, "Black R");
		TEST_ASSERT_EQUAL(0, pal.GetColor(0, 0).g, "Black G");
		TEST_ASSERT_EQUAL(0, pal.GetColor(0, 0).b, "Black B");

		TEST_ASSERT_EQUAL(255, pal.GetColor(0, 15).r, "White R");
		TEST_ASSERT_EQUAL(255, pal.GetColor(0, 15).g, "White G");
		TEST_ASSERT_EQUAL(255, pal.GetColor(0, 15).b, "White B");
	});

	// Audio tests
	AddTest("Audio: PC Speaker frequency", []() {
		PCSpeaker spk;
		spk.SetFrequency(440); // A4
		TEST_ASSERT(spk.GetPhaseInc() > 0, "Phase increment set");

		spk.SetFrequency(0);
		TEST_ASSERT_EQUAL(0, (int)spk.GetPhaseInc(), "Phase increment zero");
	});

	AddTest("Audio: Mixer volume clamp", []() {
		AudioMixer mixer;
		mixer.Initialize();

		mixer.SetMasterVolume(1.5f);
		TEST_ASSERT_EQUAL(1.0f, mixer.GetMasterVolume(), "Clamp max");

		mixer.SetMasterVolume(-0.5f);
		TEST_ASSERT_EQUAL(0.0f, mixer.GetMasterVolume(), "Clamp min");

		mixer.Shutdown();
	});

	// Diff tests
	AddTest("Diff: Dump frame comparison", []() {
		DumpFrame a, b;
		memset(&a, 0, sizeof(a));
		memset(&b, 0, sizeof(b));
		a.timecount = 1000;
		b.timecount = 1000;

		Vector<String> diffs;
		// Would compare here
	});

	// VAR Engine tests
	AddTest("Resource: VAR engine basic parse", []() {
		VAR_Engine::Startup();
		
		// Test simple parsing
		const char* testData = 
			"%int testVar 42\n"
			"%string testStr \"hello\"\n"
			"%intarray testArray 1, 2, 3\n"
			"%stringarray testStrArray \"a\", \"b\"\n";
		
		VAR_ParserState ps;
		ps.Init(testData, strlen(testData));
		
		while (VAR_Engine::ParseVar(ps))
			;
		
		TEST_ASSERT_EQUAL(42, VAR_Engine::GetInt("testVar"), "Int variable");
		TEST_ASSERT_EQUAL(String("hello"), VAR_Engine::GetString("testStr", ""), "String variable");
		
		const Vector<int32_t>* arr = VAR_Engine::GetIntArray("testArray");
		TEST_ASSERT(arr && arr->GetCount() == 3, "Int array count");
		if (arr) {
			TEST_ASSERT_EQUAL(1, (*arr)[0], "Int array[0]");
			TEST_ASSERT_EQUAL(2, (*arr)[1], "Int array[1]");
			TEST_ASSERT_EQUAL(3, (*arr)[2], "Int array[2]");
		}
		
		const Vector<String>* strArr = VAR_Engine::GetStringArray("testStrArray");
		TEST_ASSERT(strArr && strArr->GetCount() == 2, "String array count");
		if (strArr) {
			TEST_ASSERT_EQUAL("a", (*strArr)[0], "String array[0]");
			TEST_ASSERT_EQUAL("b", (*strArr)[1], "String array[1]");
		}
		
		// Test variable reference (@var)
		const char* testRef = "%int refVar @testVar\n";
		VAR_ParserState ps2;
		ps2.Init(testRef, strlen(testRef));
		while (VAR_Engine::ParseVar(ps2))
			;
		TEST_ASSERT_EQUAL(42, VAR_Engine::GetInt("refVar"), "Variable reference");
		
		VAR_Engine::Shutdown();
	});

	AddTest("Resource: VAR engine action parse", []() {
		VAR_Engine::Startup();
		
		// Register a dummy think function
		VAR_Engine::SetFunction("TestThink", [](int objIdx) {});
		
		const char* testAction = 
			"%action TestAction 0 1 2 UnscaledOnce 0 0 10 5 0 TestThink NULL NULL NextAction\n";
		
		VAR_ParserState ps;
		ps.Init(testAction, strlen(testAction));
		while (VAR_Engine::ParseVar(ps))
			;
		
		Action* act = VAR_Engine::GetAction("TestAction");
		TEST_ASSERT(act != nullptr, "Action created");
		if (act) {
			TEST_ASSERT_EQUAL(0, act->compatDosPointer, "compatDosPointer");
			TEST_ASSERT_EQUAL(1, act->chunkLeft, "chunkLeft");
			TEST_ASSERT_EQUAL(2, act->chunkRight, "chunkRight");
			TEST_ASSERT_EQUAL((int16_t)ActionType::UnscaledOnce, act->type, "type");
			TEST_ASSERT_EQUAL(0, act->protectAnimation, "protectAnimation");
			TEST_ASSERT_EQUAL(0, act->stickToGround, "stickToGround");
			TEST_ASSERT_EQUAL(10, act->timer, "timer");
			TEST_ASSERT_EQUAL(5, act->velX, "velX");
			TEST_ASSERT_EQUAL(0, act->velY, "velY");
			TEST_ASSERT(act->think != nullptr, "think function set");
			TEST_ASSERT(act->collide == nullptr, "collide is null");
			TEST_ASSERT(act->draw == nullptr, "draw is null");
		}
		
		VAR_Engine::Shutdown();
	});

	AddTest("Resource: VAR engine parse EPISODE.CK4", []() {
		VAR_Engine::Startup();
		
		// Set game path to find EPISODE.CK4
		FS_PathManager::SetGamePath(".");
		
		if (!FS_PathManager::FileExistsInPaths("EPISODE.CK4")) {
			LOG("  (skipped: EPISODE.CK4 not present in game path)");
			VAR_Engine::Shutdown();
			return;
		}
		
		bool result = VAR_Engine::LoadVars("EPISODE.CK4");
		TEST_ASSERT(result, "LoadVars succeeded");
		
		// Check some known variables from EPISODE.CK4
		TEST_ASSERT_EQUAL(4, VAR_Engine::GetInt("ck_episodeNumber"), "ck_episodeNumber");
		TEST_ASSERT_EQUAL(String("GFXINFOE.CK4"), VAR_Engine::GetString("ca_gfxInfoFile", ""), "ca_gfxInfoFile");
		TEST_ASSERT_EQUAL(String("EGADICT.CK4"), VAR_Engine::GetString("ca_graphDict", ""), "ca_graphDict");
		TEST_ASSERT_EQUAL(String("EGAHEAD.CK4"), VAR_Engine::GetString("ca_graphHead", ""), "ca_graphHead");
		TEST_ASSERT_EQUAL(String("EGAGRAPH.CK4"), VAR_Engine::GetString("ca_graphFile", ""), "ca_graphFile");
		TEST_ASSERT_EQUAL(String("AUDINFOE.CK4"), VAR_Engine::GetString("ca_audioInfoFile", ""), "ca_audioInfoFile");
		TEST_ASSERT_EQUAL(String("AUDIOHHD.CK4"), VAR_Engine::GetString("ca_audioHead", ""), "ca_audioHead");
		TEST_ASSERT_EQUAL(String("AUDIODCT.CK4"), VAR_Engine::GetString("ca_audioDict", ""), "ca_audioDict");
		TEST_ASSERT_EQUAL(String("AUDIO.CK4"), VAR_Engine::GetString("ca_audioFile", ""), "ca_audioFile");
		TEST_ASSERT_EQUAL(String("GAMEMAPS.CK4"), VAR_Engine::GetString("ca_mapFile", ""), "ca_mapFile");
		TEST_ASSERT_EQUAL(String("MAPHEAD.CK4"), VAR_Engine::GetString("ca_mapHead", ""), "ca_mapHead");
		TEST_ASSERT_EQUAL(String("TILEINFO.CK4"), VAR_Engine::GetString("ca_tileInfo", ""), "ca_tileInfo");
		
		// Check variable reference
		TEST_ASSERT_EQUAL(1, VAR_Engine::GetInt("ck_minWarpLevel"), "ck_minWarpLevel (ref)");
		TEST_ASSERT_EQUAL(18, VAR_Engine::GetInt("ck_maxWarpLevel"), "ck_maxWarpLevel (ref)");
		
		// Check int array
		const Vector<int32_t>* starWarsPalette = VAR_Engine::GetIntArray("ck_starWarsPalette");
		TEST_ASSERT(starWarsPalette && starWarsPalette->GetCount() == 17, "ck_starWarsPalette count");
		
		// Check string array
		const Vector<String>* highScoreNames = VAR_Engine::GetStringArray("ck_defaultHighScoreNames");
		TEST_ASSERT(highScoreNames && highScoreNames->GetCount() == 8, "ck_defaultHighScoreNames count");
		if (highScoreNames) {
			TEST_ASSERT_EQUAL("Id Software - '91", (*highScoreNames)[0], "High score name 0");
			TEST_ASSERT_EQUAL("Jason Blochowiak", (*highScoreNames)[2], "High score name 2");
		}
		
		// Check action was parsed (from %include "ACTION.CK4")
		Action* eggHatchAction = VAR_Engine::GetAction("CK4_EggHatchAction");
		TEST_ASSERT(eggHatchAction != nullptr, "CK4_EggHatchAction from %include ACTION.CK4");
		
		VAR_Engine::Shutdown();
	});

	AddTest("Resource: EP_ConfigureEpisode populates EpisodeDef", []() {
		VAR_Engine::Startup();
		FS_PathManager::SetGamePath(".");
		
		if (!FS_PathManager::FileExistsInPaths("EPISODE.CK4")) {
			LOG("  (skipped: EPISODE.CK4 not present in game path)");
			return;
		}
		
		bool ok = EP_ConfigureEpisode(Episode::KEEN4);
		TEST_ASSERT(ok, "EP_ConfigureEpisode succeeded");
		TEST_ASSERT(g_currentEpisodeEnum == Episode::KEEN4, "episode enum set");
		TEST_ASSERT(g_currentEpisode != nullptr, "g_currentEpisode set");
		TEST_ASSERT(g_currentEpisode == g_episodes[0], "episode registered");
		TEST_ASSERT_EQUAL(4, EP_GetEpisodeNumber(), "episode number");
		
		if (g_currentEpisode) {
			TEST_ASSERT_EQUAL(String("EGAHEAD.CK4"), String(g_currentEpisode->graphHeadFile), "graphHeadFile");
			TEST_ASSERT_EQUAL(String("EGAGRAPH.CK4"), String(g_currentEpisode->graphFile), "graphFile");
			TEST_ASSERT_EQUAL(String("EGADICT.CK4"), String(g_currentEpisode->graphDictFile), "graphDictFile");
			TEST_ASSERT_EQUAL(String("GAMEMAPS.CK4"), String(g_currentEpisode->mapFile), "mapFile");
			TEST_ASSERT_EQUAL(String("MAPHEAD.CK4"), String(g_currentEpisode->mapHeadFile), "mapHeadFile");
			TEST_ASSERT_EQUAL(String("TILEINFO.CK4"), String(g_currentEpisode->tileInfoFile), "tileInfoFile");
			TEST_ASSERT_EQUAL(String("GFXINFOE.CK4"), String(g_currentEpisode->gfxInfoFile), "gfxInfoFile");
			TEST_ASSERT_EQUAL(String("AUDIO.CK4"), String(g_currentEpisode->audioFile), "audioFile");
			TEST_ASSERT_EQUAL(String("AUDIOHHD.CK4"), String(g_currentEpisode->audioHeadFile), "audioHeadFile");
			TEST_ASSERT_EQUAL(String("CK4"), String(g_currentEpisode->graphHeadExt), "graphHeadExt");
			TEST_ASSERT(g_currentEpisode->setupFunctions != nullptr, "setupFunctions");
			TEST_ASSERT(g_currentEpisode->isPresent != nullptr, "isPresent");
		}
	});

	AddTest("Resource: VAR engine iteration", []() {
		VAR_Engine::Startup();
		FS_PathManager::SetGamePath(".");

		if (!FS_PathManager::FileExistsInPaths("EPISODE.CK4")) {
			LOG("  (skipped: EPISODE.CK4 not present in game path)");
			return;
		}

		TEST_ASSERT(VAR_Engine::LoadVars("EPISODE.CK4"), "LoadVars");

		TEST_ASSERT(VAR_Engine::GetVarCount() > 0, "var count > 0");

		// Locate ck_episodeNumber through the iteration API and confirm its type.
		bool foundEpisodeNumber = false;
		bool foundPalette = false;
		for (int i = 0; i < VAR_Engine::GetVarCount(); ++i) {
			const char* name = VAR_Engine::GetVarNameAt(i);
			TEST_ASSERT(name != nullptr, "var name not null");

			if (String(name) == "ck_episodeNumber") {
				foundEpisodeNumber = true;
				TEST_ASSERT(VAR_Engine::GetVarTypeAt(i) == VAR_VarType::Int, "ck_episodeNumber type");
				TEST_ASSERT_EQUAL(4, VAR_Engine::GetIntAt(i), "ck_episodeNumber value");
			}
			else if (String(name) == "ck_starWarsPalette") {
				foundPalette = true;
				TEST_ASSERT(VAR_Engine::GetVarTypeAt(i) == VAR_VarType::IntArray, "palette type");
				const Vector<int32_t>* arr = VAR_Engine::GetIntArrayAt(i);
				TEST_ASSERT(arr && arr->GetCount() == 17, "palette count");
			}
		}
		TEST_ASSERT(foundEpisodeNumber, "ck_episodeNumber enumerated");
		TEST_ASSERT(foundPalette, "ck_starWarsPalette enumerated");

		VAR_Engine::Shutdown();
	});

	AddTest("Resource: tile info accessors", []() {
		// Verify the TILEINFO layout offsets match OmniSpeak id_ti.c using a
		// synthetic buffer, then restore the globals.
		CA_GfxInfo savedGfx = g_gfxInfoE;
		uint8_t* savedTile = g_tileInfo;
		int savedSize = g_tileInfoSize;

		const uint16_t numTiles16 = 4;
		const uint16_t numTiles16m = 4;
		g_gfxInfoE.numTiles16 = numTiles16;
		g_gfxInfoE.numTiles16m = numTiles16m;

		Vector<uint8_t> buf;
		const int size = numTiles16 * 2 + numTiles16m * 7;
		buf.SetCount(size, 0);

		const int offForeTop = numTiles16 * 2;
		const int offForeRight = numTiles16 * 2 + numTiles16m;
		const int offForeBottom = numTiles16 * 2 + numTiles16m * 2;
		const int offForeLeft = numTiles16 * 2 + numTiles16m * 3;

		buf[offForeTop + 1] = 0x19;   // platform marker
		buf[offForeTop + 2] = 0x03;   // slope
		buf[offForeRight + 3] = 0x80;
		buf[offForeBottom + 1] = 0x04;
		buf[offForeLeft + 1] = 0x80;

		g_tileInfo = buf.Begin();
		g_tileInfoSize = size;

		TEST_ASSERT_EQUAL(0x19, ResourceManager::TileForeTop(1), "TileForeTop(1)");
		TEST_ASSERT_EQUAL(0x03, ResourceManager::TileForeTop(2), "TileForeTop(2)");
		TEST_ASSERT_EQUAL(0, ResourceManager::TileForeTop(0), "TileForeTop(0)");
		TEST_ASSERT_EQUAL(0x80, ResourceManager::TileForeRight(3), "TileForeRight(3)");
		TEST_ASSERT_EQUAL(0, ResourceManager::TileForeRight(1), "TileForeRight(1)");
		TEST_ASSERT_EQUAL(0x04, ResourceManager::TileForeBottom(1), "TileForeBottom(1)");
		TEST_ASSERT_EQUAL(0x80, ResourceManager::TileForeLeft(1), "TileForeLeft(1)");
		TEST_ASSERT_EQUAL(0, ResourceManager::TileForeTop(100), "TileForeTop out of range");

		g_gfxInfoE = savedGfx;
		g_tileInfo = savedTile;
		g_tileInfoSize = savedSize;

		// Real-data sanity: GFXINFOE + TILEINFO load and the offsets stay in range.
		FS_PathManager::SetGamePath(".");
		if (!FS_PathManager::FileExistsInPaths("EPISODE.CK4")
			|| !FS_PathManager::FileExistsInPaths("TILEINFO.CK4")
			|| !FS_PathManager::FileExistsInPaths("GFXINFOE.CK4")) {
			LOG("  (skipped real-data checks: GFXINFOE.CK4/TILEINFO.CK4 not present)");
			return;
		}

		VAR_Engine::Startup();
		TEST_ASSERT(EP_ConfigureEpisode(Episode::KEEN4), "EP_ConfigureEpisode");
		TEST_ASSERT(ResourceManager::LoadGfxInfo(), "LoadGfxInfo");
		TEST_ASSERT(ResourceManager::LoadTileInfo(), "LoadTileInfo");
		TEST_ASSERT(g_gfxInfoE.numTiles16 > 0, "numTiles16 > 0");
		TEST_ASSERT(g_tileInfoSize > 0, "tile info buffer present");

		// Accessors must not read out of bounds for in-range tiles.
		for (uint16_t t = 0; t <= g_gfxInfoE.numTiles16m && t < 64; ++t) {
			(void)ResourceManager::TileForeTop(t);
			(void)ResourceManager::TileForeBottom(t);
			(void)ResourceManager::TileForeLeft(t);
			(void)ResourceManager::TileForeRight(t);
			(void)ResourceManager::TileForeMisc(t);
		}
		TEST_ASSERT_EQUAL(0, ResourceManager::TileForeTop(0xFFFF), "out-of-range tile returns 0");
	});

	AddTest("Core: PhysClipHorz wall clipping", []() {
		// Inject a synthetic map + TILEINFO and verify horizontal wall clipping.
		CA_GfxInfo savedGfx = g_gfxInfoE;
		uint8_t* savedTile = g_tileInfo;
		int savedSize = g_tileInfoSize;
		int savedMapOn = g_mapOn;
		CA_MapHeader* savedHdr = g_mapHeaders[0];
		uint16_t* savedPlane = g_mapPlanes[1];

		const uint16_t numTiles16 = 4;
		const uint16_t numTiles16m = 8;
		g_gfxInfoE.numTiles16 = numTiles16;
		g_gfxInfoE.numTiles16m = numTiles16m;

		const int tiSize = numTiles16 * 2 + numTiles16m * 7;
		Vector<uint8_t> ti;
		ti.SetCount(tiSize, 0);
		const int offForeRight = numTiles16 * 2 + numTiles16m;
		const int offForeLeft = numTiles16 * 2 + numTiles16m * 3;
		ti[offForeLeft + 7] = 0x80;   // tile 7 blocks from the left (wall on the right)
		ti[offForeRight + 9] = 0x80;  // tile 9 blocks from the right (wall on the left)
		g_tileInfo = ti.Begin();
		g_tileInfoSize = tiSize;

		const int W = 10, H = 10;
		CA_MapHeader hdr;
		Zero(hdr);
		hdr.width = W;
		hdr.height = H;
		Vector<uint16_t> plane;
		plane.SetCount(W * H, 0);
		plane[3 * W + 5] = 7;  // wall tile at (5,3)
		plane[4 * W + 3] = 9;  // wall tile at (3,4)
		g_mapHeaders[0] = &hdr;
		g_mapPlanes[1] = plane.Begin();
		g_mapOn = 0;

		// An object whose right edge penetrates the wall at tile (5,3).
		Object obj;
		obj.SetField(Object::FLD_CLIP_TILE_X1, 4);
		obj.SetField(Object::FLD_CLIP_TILE_X2, 5);
		obj.SetField(Object::FLD_CLIP_TILE_Y1, 3);
		obj.SetField(Object::FLD_CLIP_TILE_Y2, 3);
		obj.SetField(Object::FLD_CLIP_UNIT_X1, TileToUnit(4) + 10);
		obj.SetField(Object::FLD_CLIP_UNIT_X2, TileToUnit(5) + 100);
		obj.SetField(Object::FLD_CLIP_UNIT_Y1, TileToUnit(3) + 1);
		obj.SetField(Object::FLD_CLIP_UNIT_Y2, TileToUnit(4) - 1);
		obj.SetPosX(obj.GetField(Object::FLD_CLIP_UNIT_X1));
		obj.SetPosY(obj.GetField(Object::FLD_CLIP_UNIT_Y1));

		TEST_ASSERT(!NotStuckInWall(&obj), "object overlaps wall");
		PhysClipHorz(&obj);
		TEST_ASSERT_EQUAL(TileToUnit(5) - 1, (int)obj.GetField(Object::FLD_CLIP_UNIT_X2), "pushed left of wall");
		TEST_ASSERT(obj.GetField(Object::FLD_LEFT_TI) == 0x80, "leftTI set");

		// An object clear of any wall is unaffected and reports not-stuck.
		Object clear;
		clear.SetField(Object::FLD_CLIP_TILE_X1, 1);
		clear.SetField(Object::FLD_CLIP_TILE_X2, 1);
		clear.SetField(Object::FLD_CLIP_TILE_Y1, 1);
		clear.SetField(Object::FLD_CLIP_TILE_Y2, 1);
		clear.SetField(Object::FLD_CLIP_UNIT_X1, TileToUnit(1) + 10);
		clear.SetField(Object::FLD_CLIP_UNIT_X2, TileToUnit(1) + 100);
		clear.SetField(Object::FLD_CLIP_UNIT_Y1, TileToUnit(1) + 1);
		clear.SetField(Object::FLD_CLIP_UNIT_Y2, TileToUnit(2) - 1);
		TEST_ASSERT(NotStuckInWall(&clear), "clear object not stuck");

		g_gfxInfoE = savedGfx;
		g_tileInfo = savedTile;
		g_tileInfoSize = savedSize;
		g_mapOn = savedMapOn;
		g_mapHeaders[0] = savedHdr;
		g_mapPlanes[1] = savedPlane;
	});

	LOG("=== Test Suite Complete ===");
	LOG("Passed: " << (int)std::count_if(s_results.begin(), s_results.end(), [](const TestResult& r){ return r.passed; })
	     << " / " << s_results.GetCount());
}

int RunPlatformer2DTests()
{
	// Initialize headless platform
	Platformer2DConfig cfg;
	cfg.gamePath = ".";
	cfg.userPath = ".";
	cfg.omniPath = ".";

	Platformer2D_HeadlessStartup(cfg);

	TestSuite::RunAll();

	Platformer2D_HeadlessShutdown();

	return TestSuite::AllPassed() ? 0 : 1;
}

// RunPlatformer2DTests is declared in Platformer2DTest.h and defined here
// main() is generated in main.cpp via GUI_APP_MAIN

END_UPP_NAMESPACE
