#include "Platformer2DDumpPrinter.h"

NAMESPACE_UPP

int DumpPrinter::g_episodeNum = 4;

size_t DumpPrinter::ReadU16LE(FileIn& fp, void* ptr, size_t count)
{
	size_t read = fp.Get(ptr, count * 2);
	if (read != count * 2) return read / 2;
	
	// Convert endianness if needed (assuming little-endian host)
	// U++ runs on little-endian, so no conversion needed
	return count;
}

size_t DumpPrinter::ReadU32LE(FileIn& fp, void* ptr, size_t count)
{
	size_t read = fp.Get(ptr, count * 4);
	if (read != count * 4) return read / 4;
	return count;
}

bool DumpPrinter::ProcessTimeCount(FileIn& fp)
{
	uint32_t timecount;
	if (ReadU32LE(fp, &timecount, 1) != 1)
		return false;
	
	Cout() << Format("Timecount: %08X\n", (int64)timecount);
	return true;
}

bool DumpPrinter::ProcessGameState(FileIn& fp)
{
	GameState state;
	int i;
	
	if (ReadU16LE(fp, &state.mapPosX, 1) != 1) return false;
	if (ReadU16LE(fp, &state.mapPosY, 1) != 1) return false;
	if (ReadU16LE(fp, state.levelsDone, 25) != 25) return false;
	if (ReadU32LE(fp, &state.keenScore, 1) != 1) return false;
	if (ReadU32LE(fp, &state.nextKeenAt, 1) != 1) return false;
	if (ReadU16LE(fp, &state.numShots, 1) != 1) return false;
	if (ReadU16LE(fp, &state.numCentilife, 1) != 1) return false;
	
	if (g_episodeNum == 4)
	{
		if (ReadU16LE(fp, &state.ep.ck4.wetsuit, 1) != 1) return false;
		if (ReadU16LE(fp, &state.ep.ck4.membersRescued, 1) != 1) return false;
	}
	else if (g_episodeNum == 5)
	{
		if (ReadU16LE(fp, &state.ep.ck5.securityCard, 1) != 1) return false;
		if (ReadU16LE(fp, &state.ep.ck5.word_4729C, 1) != 1) return false;
		if (ReadU16LE(fp, &state.ep.ck5.fusesRemaining, 1) != 1) return false;
	}
	else if (g_episodeNum == 6)
	{
		if (ReadU16LE(fp, &state.ep.ck6.sandwich, 1) != 1) return false;
		if (ReadU16LE(fp, &state.ep.ck6.rope, 1) != 1) return false;
		if (ReadU16LE(fp, &state.ep.ck6.passcard, 1) != 1) return false;
		if (ReadU16LE(fp, &state.ep.ck6.inRocket, 1) != 1) return false;
	}
	
	if (ReadU16LE(fp, state.keyGems, 4) != 4) return false;
	if (ReadU16LE(fp, &state.currentLevel, 1) != 1) return false;
	if (ReadU16LE(fp, &state.numLives, 1) != 1) return false;
	if (ReadU16LE(fp, &state.difficulty, 1) != 1) return false;
	if (ReadU16LE(fp, &state.platformOffset, 1) != 1) return false;
	
	Cout() << Format("Gamestate: %04X %04X", (int)state.mapPosX, (int)state.mapPosY);
	
	for (i = 0; i < 25; ++i)
		Cout() << Format(" %04X", (int)state.levelsDone[i]);
	
	Cout() << Format(" %08X %08X %04X %04X", (int64)state.keenScore, (int64)state.nextKeenAt, (int)state.numShots, (int)state.numCentilife);
	
	if (g_episodeNum == 4)
		Cout() << Format(" %04X %04X", (int)state.ep.ck4.wetsuit, (int)state.ep.ck4.membersRescued);
	else if (g_episodeNum == 5)
		Cout() << Format(" %04X %04X %04X", (int)state.ep.ck5.securityCard, (int)state.ep.ck5.word_4729C, (int)state.ep.ck5.fusesRemaining);
	else if (g_episodeNum == 6)
		Cout() << Format(" %04X %04X %04X %04X", (int)state.ep.ck6.sandwich, (int)state.ep.ck6.rope, (int)state.ep.ck6.passcard, (int)state.ep.ck6.inRocket);
	
	for (i = 0; i < 4; ++i)
		Cout() << Format(" %04X", (int)state.keyGems[i]);
	
	Cout() << Format(" %04X %04X %04X %04X\n", (int)state.currentLevel, (int)state.numLives, (int)state.difficulty, (int)state.platformOffset);
	
	return true;
}

bool DumpPrinter::ProcessObjects(FileIn& fp)
{
	uint16_t objFields[38];
	
	for (int j = 0; j < 100; ++j)
	{
		if (ReadU16LE(fp, objFields, 38) != 38)
			return false;
		
		Cout() << Format("Obj %02X:", (int)j);
		for (int i = 0; i < 38; ++i)
			Cout() << Format(" %04X", (int)objFields[i]);
		Cout() << "\n";
	}
	return true;
}

int DumpPrinter::Run(int argc, char* argv[])
{
	if (argc != 3)
	{
		Cerr() << "Usage: Platformer2DDumpPrinter <inFile> <episode>\n";
		return 0;
	}
	
	g_episodeNum = atoi(argv[2]);
	if (g_episodeNum < 4 || g_episodeNum > 6)
	{
		Cerr() << "<episode> argument must be a number in the range 4-6.\n";
		return 1;
	}
	
	FileIn fp(argv[1]);
	if (!fp.IsOpen())
	{
		Cerr() << "ERROR - Cannot open file " << argv[1] << " for reading\n";
		return 1;
	}
	
	while (ProcessTimeCount(fp) && ProcessGameState(fp) && ProcessObjects(fp))
		;
	
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
	
	int result = DumpPrinter::Run(argc, argv.Begin());
	SetExitCode(result);
}

END_UPP_NAMESPACE