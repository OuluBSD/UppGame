#ifndef _Platformer2DDumpPrinter_Platformer2DDumpPrinter_h_
#define _Platformer2DDumpPrinter_Platformer2DDumpPrinter_h_

#include <game/Platformer2D/Platformer2D.h>
#include <game/Platformer2D/Core/Core.h>

NAMESPACE_UPP

// ============================================================================
// Dump Printer (matches OmniSpeak dumpprinter.c exactly)
// ============================================================================

class DumpPrinter
{
public:
	static int Run(int argc, char* argv[]);
	
private:
	static bool ProcessTimeCount(FileIn& fp);
	static bool ProcessGameState(FileIn& fp);
	static bool ProcessObjects(FileIn& fp);
	
	static size_t ReadU16LE(FileIn& fp, void* ptr, size_t count);
	static size_t ReadU32LE(FileIn& fp, void* ptr, size_t count);
	
	static int g_episodeNum;
};

END_UPP_NAMESPACE

#endif // _Platformer2DDumpPrinter_Platformer2DDumpPrinter_h_