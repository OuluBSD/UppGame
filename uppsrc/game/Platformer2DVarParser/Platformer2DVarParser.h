#ifndef _Platformer2DVarParser_Platformer2DVarParser_h_
#define _Platformer2DVarParser_Platformer2DVarParser_h_

#include <game/Platformer2D/Platformer2D.h>
#include <game/Platformer2D/Resource/Resource.h>
#include <game/Platformer2D/Resource/Episode.h>

NAMESPACE_UPP

// ============================================================================
// Var Parser CLI
// ============================================================================

class VarParserCLI
{
public:
	struct Options
	{
		bool decls = false;
		bool actions = false;
		bool staticData = false;
		bool headers = false;
		bool help = false;
		String inputFile;
	};
	
	static int Run(int argc, char* argv[]);
	
private:
	static Options ParseArgs(int argc, char* argv[]);
	static void PrintHelp();
	static void GenerateDeclarations(const EpisodeVars& vars);
	static void GenerateActions(const EpisodeVars& vars);
	static void GenerateStatic(const EpisodeVars& vars);
	static void GenerateHeaders(const EpisodeVars& vars);
};

END_UPP_NAMESPACE

#endif // _Platformer2DVarParser_Platformer2DVarParser_h_