#ifndef _Platformer2DTest_Platformer2DTest_h_
#define _Platformer2DTest_Platformer2DTest_h_

#include <game/Platformer2D/Core/Core.h>
#include <game/Platformer2D/Resource/Resource.h>
#include <game/Platformer2D/Resource/Decompress.h>
#include <game/Platformer2D/Render/Render.h>
#include <game/Platformer2D/Audio/Audio.h>
#include <game/Platformer2D/Platform/Platform.h>

NAMESPACE_UPP

// ============================================================================
// Test Framework
// ============================================================================

struct TestResult : TriviallyRelocatable<TestResult>
{
	String name;
	bool passed = false;
	String message;
	double durationMs = 0;
};

class TestSuite
{
public:
	static void RunAll();
	static void AddTest(const String& name, std::function<void()> testFunc);
	static const Vector<TestResult>& GetResults();
	static bool AllPassed();

private:
	static Vector<TestResult> s_results;
};

// Assertion macros
#define TEST_ASSERT(cond, msg) \
	do { if (!(cond)) { throw TestFailure(msg); } } while(0)

#define TEST_ASSERT_EQUAL(expected, actual, msg) \
	do { if ((expected) != (actual)) { throw TestFailure(String(msg) + " Expected: " + AsString(expected) + " Actual: " + AsString(actual)); } } while(0)

struct TestFailure : public Exc
{
	String msg;
	TestFailure(const String& m) : msg(m) {}
	const char* what() const noexcept { return msg; }
};;

// ============================================================================
// Dump Comparator (for differential tests)
// ============================================================================

struct DumpFrame
{
uint32_t timecount;
GameState gameState;
Object objects[100];
};

class DumpComparator
{
public:
	static bool CompareDumps(const String& platformer2DDump, const String& omnispkDump, int episodeNum, Vector<String>* diffs = nullptr);
	static bool ParseDump(const String& filename, DumpFrame& frame, int episodeNum);

private:
	static bool CompareGameState(const GameState& a, const GameState& b, int episodeNum, Vector<String>& diffs);
	static bool CompareObjects(const Object a[100], const Object b[100], Vector<String>& diffs);
};

// ============================================================================
// Framebuffer Comparator
// ============================================================================

class FramebufferComparator
{
public:
	static bool Compare(const IndexedSurface* a, const IndexedSurface* b, Vector<String>* diffs = nullptr);
	static bool CompareFiles(const String& fileA, const String& fileB, Vector<String>* diffs = nullptr);
};

// ============================================================================
// Test Entry Point
// ============================================================================

int RunPlatformer2DTests();

END_UPP_NAMESPACE

#endif // _Platformer2DTest_Platformer2DTest_h_