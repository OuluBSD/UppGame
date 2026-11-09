#include "Platformer2DVarParser.h"
#include <game/Platformer2D/Resource/VAR.h>

NAMESPACE_UPP

VarParserCLI::Options VarParserCLI::ParseArgs(int argc, char* argv[])
{
	Options opts;
	
	for (int i = 1; i < argc; ++i)
	{
		String arg = argv[i];
		if (arg == "-decls") opts.decls = true;
		else if (arg == "-actions") opts.actions = true;
		else if (arg == "-static") opts.staticData = true;
		else if (arg == "-headers") opts.headers = true;
		else if (arg == "-help" || arg == "--help") opts.help = true;
		else if (arg[0] != '-') opts.inputFile = arg;
		else
		{
			Cerr() << "Unknown option: " << arg << "\n";
			opts.help = true;
		}
	}
	
	return opts;
}

void VarParserCLI::PrintHelp()
{
	Cout() << "Platformer2DVarParser - Episode variable parser\n";
	Cout() << "Usage: Platformer2DVarParser [options] <episode_file>\n";
	Cout() << "Options:\n";
	Cout() << "  -decls     Generate C declarations\n";
	Cout() << "  -actions   Generate action definitions\n";
	Cout() << "  -static    Generate static data\n";
	Cout() << "  -headers   Generate header includes\n";
	Cout() << "  -help      Show this help\n";
}

void VarParserCLI::GenerateDeclarations(const EpisodeVars& vars)
{
	Cout() << "// Generated declarations for episode " << vars.GetEpisodeNumber() << "\n";
	Cout() << "// Episode: " << vars.GetEpisodeName() << "\n\n";
	
	for (int i = 0; i < VAR_Engine::GetVarCount(); ++i)
	{
		const char* name = VAR_Engine::GetVarNameAt(i);
		if (!name) continue;
		
		switch (VAR_Engine::GetVarTypeAt(i))
		{
		case VAR_VarType::Bool:
			Cout() << "extern bool " << name << ";\n";
			break;
		case VAR_VarType::Int:
			Cout() << "extern int " << name << ";\n";
			break;
		case VAR_VarType::String:
			Cout() << "extern const char* " << name << ";\n";
			break;
		case VAR_VarType::IntArray:
			Cout() << "extern int " << name << "[];\n";
			break;
		case VAR_VarType::StringArray:
			Cout() << "extern const char* " << name << "[];\n";
			break;
		case VAR_VarType::Action:
			Cout() << "extern Action " << name << ";\n";
			break;
		default:
			break;
		}
	}
}

void VarParserCLI::GenerateActions(const EpisodeVars& vars)
{
	Cout() << "// Generated actions for episode " << vars.GetEpisodeNumber() << "\n\n";
	
	for (int i = 0; i < VAR_Engine::GetVarCount(); ++i)
	{
		if (VAR_Engine::GetVarTypeAt(i) != VAR_VarType::Action)
			continue;
		
		const char* name = VAR_Engine::GetVarNameAt(i);
		Action* act = VAR_Engine::GetAction(name);
		if (!name || !act) continue;
		
		Cout() << "Action " << name << ";\n";
		Cout() << "//   chunkLeft = " << act->chunkLeft
		       << ", chunkRight = " << act->chunkRight
		       << ", type = " << act->type << "\n";
		Cout() << "//   timer = " << act->timer
		       << ", velX = " << act->velX
		       << ", velY = " << act->velY << "\n\n";
	}
}

void VarParserCLI::GenerateStatic(const EpisodeVars& vars)
{
	Cout() << "// Generated static data for episode " << vars.GetEpisodeNumber() << "\n\n";
	
	for (int i = 0; i < VAR_Engine::GetVarCount(); ++i)
	{
		const char* name = VAR_Engine::GetVarNameAt(i);
		if (!name) continue;
		
		switch (VAR_Engine::GetVarTypeAt(i))
		{
		case VAR_VarType::Bool:
			Cout() << "bool " << name << " = " << (VAR_Engine::GetIntAt(i) ? "true" : "false") << ";\n";
			break;
		case VAR_VarType::Int:
			Cout() << "int " << name << " = " << VAR_Engine::GetIntAt(i) << ";\n";
			break;
		case VAR_VarType::String:
			{
				const char* val = VAR_Engine::GetStringAt(i);
				Cout() << "const char* " << name << " = \"" << (val ? val : "") << "\";\n";
			}
			break;
		case VAR_VarType::IntArray:
			{
				const Vector<int32_t>* arr = VAR_Engine::GetIntArrayAt(i);
				Cout() << "int " << name << "[] = {";
				if (arr)
					for (int j = 0; j < arr->GetCount(); ++j)
					{
						if (j > 0) Cout() << ", ";
						Cout() << (*arr)[j];
					}
				Cout() << "};\n";
			}
			break;
		case VAR_VarType::StringArray:
			{
				const Vector<String>* arr = VAR_Engine::GetStringArrayAt(i);
				Cout() << "const char* " << name << "[] = {";
				if (arr)
					for (int j = 0; j < arr->GetCount(); ++j)
					{
						if (j > 0) Cout() << ", ";
						Cout() << "\"" << (*arr)[j] << "\"";
					}
				Cout() << "};\n";
			}
			break;
		default:
			break;
		}
	}
}

void VarParserCLI::GenerateHeaders(const EpisodeVars& vars)
{
	Cout() << "#ifndef _GENERATED_EPISODE_" << vars.GetEpisodeNumber() << "_H_\n";
	Cout() << "#define _GENERATED_EPISODE_" << vars.GetEpisodeNumber() << "_H_\n\n";
	
	GenerateDeclarations(vars);
	
	Cout() << "\n#endif\n";
}

int VarParserCLI::Run(int argc, char* argv[])
{
	Options opts = ParseArgs(argc, argv);
	
	if (opts.help || opts.inputFile.IsEmpty())
	{
		PrintHelp();
		return opts.help ? 0 : 1;
	}
	
	VAR_Engine::Startup();
	
	EpisodeVars vars;
	if (!vars.LoadFromFile(opts.inputFile))
	{
		Cerr() << "Failed to load episode file: " << opts.inputFile << "\n";
		return 1;
	}
	
	if (opts.decls) GenerateDeclarations(vars);
	if (opts.actions) GenerateActions(vars);
	if (opts.staticData) GenerateStatic(vars);
	if (opts.headers) GenerateHeaders(vars);
	
	if (!opts.decls && !opts.actions && !opts.staticData && !opts.headers)
	{
		// Default: generate all
		GenerateHeaders(vars);
	}
	
	return 0;
}

CONSOLE_APP_MAIN
{
	const Vector<String>& args = CommandLine();
	int argc = args.GetCount();
	Vector<char*> argv;
	argv.Reserve(argc + 1);
	for (int i = 0; i < argc; i++)
		argv.Add((char*)args[i].Begin());
	argv.Add(nullptr);
	
	int result = VarParserCLI::Run(argc, argv.Begin());
	SetExitCode(result);
}

END_UPP_NAMESPACE