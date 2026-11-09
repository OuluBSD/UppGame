#include "Resource.h"
#include "Decompress.h"
#include "VAR.h"
#include <Core/Core.h>

NAMESPACE_UPP

// ============================================================================
// Global State (matching OmniSpeak id_ca.c)
// ============================================================================

// g_currentEpisode is defined in Core.h as CK_EpisodeDef*

CA_MapHead* g_mapHead = nullptr;
CA_MapHeader* g_mapHeaders[CA_NUMMAPS] = {nullptr};
uint16_t* g_mapPlanes[CA_NUMMAPPLANES] = {nullptr, nullptr, nullptr, nullptr};
int g_mapOn = -1;
int g_currentLevel = 0;
uint8_t* g_tileInfo = nullptr;
int g_tileInfoSize = 0;

uint8_t g_levelnum = 0, g_levelbit = 1;
uint8_t g_graphChunkNeeded[CA_MAX_GRAPH_CHUNKS] = {0};

// EGAGRAPH globals (matching OmniSpeak id_ca.c)
CA_GfxInfo g_gfxInfoE = {0};
FileIn* g_graphHandle = nullptr; // EGAGRAPH file handle kept open
int64_t g_graphFileSize = 0;
ChunkInfo g_chunkInfo[CA_MAX_GRAPH_CHUNKS] = {0};
void* g_graphChunks[CA_MAX_GRAPH_CHUNKS] = {nullptr};
HuffmanDecoder g_grHuffDict;

// AUDIO globals (matching OmniSpeak id_ca.c)
CA_AudioInfo g_audioInfoE = {0};
FileIn* g_audioHandle = nullptr; // AUDIO file handle kept open
int64_t g_audioFileSize = 0;
ChunkInfo g_audioChunkInfo[CA_MAX_AUDIO_CHUNKS] = {0};
void* g_audioChunks[CA_MAX_AUDIO_CHUNKS] = {nullptr};
HuffmanDecoder g_audioHuffDict;
uint8_t g_audioChunkNeeded[CA_MAX_AUDIO_CHUNKS] = {0};

// ============================================================================
// FS_PathManager Implementation
// ============================================================================

String FS_PathManager::s_gamePath = ".";
String FS_PathManager::s_userPath = ".";
String FS_PathManager::s_omniPath = ".";

void FS_PathManager::Startup()
{
	// TODO: Read from config, command line, XDG
	LOG("FS_PathManager::Startup - using default paths");
}

void FS_PathManager::Shutdown()
{
}

String FS_PathManager::GetGamePath()
{
	return s_gamePath;
}

String FS_PathManager::GetUserPath()
{
	return s_userPath;
}

String FS_PathManager::GetOmniPath()
{
	return s_omniPath;
}

bool FS_PathManager::SetGamePath(const String& path)
{
	if (DirectoryExists(path))
	{
		s_gamePath = path;
		return true;
	}
	return false;
}

bool FS_PathManager::SetUserPath(const String& path)
{
	if (DirectoryExists(path) || FileExists(path + "/.")) // Allow creating
	{
		s_userPath = path;
		return true;
	}
	return false;
}

bool FS_PathManager::SetOmniPath(const String& path)
{
	if (DirectoryExists(path))
	{
		s_omniPath = path;
		return true;
	}
	return false;
}

bool FS_PathManager::FileExists(const String& filename)
{
	return Upp::FileExists(filename);
}

bool FS_PathManager::FileExistsInPaths(const String& filename, String* outFullPath)
{
	// Search order: game -> user -> omni
	Vector<String> paths = { s_gamePath, s_userPath, s_omniPath };
	for (const String& path : paths)
	{
		String full = AppendFileName(path, filename);
		if (FileExists(full))
		{
			if (outFullPath) *outFullPath = full;
			return true;
		}
	}
	return false;
}

FileIn FS_PathManager::OpenFile(const String& filename)
{
	String fullPath;
	if (FileExistsInPaths(filename, &fullPath))
		return FileIn(fullPath);
	return FileIn(); // Invalid
}

// ============================================================================
// BinaryReader Implementation
// ============================================================================

bool BinaryReader::ReadU8(uint8_t& out)
{
	return stream.Get(&out, 1) == 1;
}

bool BinaryReader::ReadU16LE(uint16_t& out)
{
	uint8_t b[2];
	if (stream.Get(b, 2) != 2) return false;
	out = uint16_t(b[0]) | (uint16_t(b[1]) << 8);
	return true;
}

bool BinaryReader::ReadU32LE(uint32_t& out)
{
	uint8_t b[4];
	if (stream.Get(b, 4) != 4) return false;
	out = uint32_t(b[0]) | (uint32_t(b[1]) << 8) | (uint32_t(b[2]) << 16) | (uint32_t(b[3]) << 24);
	return true;
}

bool BinaryReader::ReadI16LE(int16_t& out)
{
	uint16_t u;
	if (!ReadU16LE(u)) return false;
	out = (int16_t)u;
	return true;
}

bool BinaryReader::ReadI32LE(int32_t& out)
{
	uint32_t u;
	if (!ReadU32LE(u)) return false;
	out = (int32_t)u;
	return true;
}

bool BinaryReader::ReadU16BE(uint16_t& out)
{
	uint8_t b[2];
	if (stream.Get(b, 2) != 2) return false;
	out = (uint16_t(b[0]) << 8) | uint16_t(b[1]);
	return true;
}

bool BinaryReader::ReadU32BE(uint32_t& out)
{
	uint8_t b[4];
	if (stream.Get(b, 4) != 4) return false;
	out = (uint32_t(b[0]) << 24) | (uint32_t(b[1]) << 16) | (uint32_t(b[2]) << 8) | uint32_t(b[3]);
	return true;
}

bool BinaryReader::Skip(size_t bytes)
{
	int64_t pos = stream.GetPos();
	stream.SeekCur((int)bytes);
	return stream.GetPos() - pos == (int64_t)bytes && !stream.IsError();
}

bool BinaryReader::Seek(int64_t pos)
{
	stream.Seek(pos);
	return stream.GetPos() == pos && !stream.IsError();
}

int64_t BinaryReader::GetPos() const
{
	return stream.GetPos();
}

bool BinaryReader::IsEOF() const
{
	return stream.IsEof();
}

// ============================================================================
// Helper: Read file into buffer
// ============================================================================

static bool LoadFileToBuffer(const String& filename, Vector<uint8_t>& outBuffer, int* outSize = nullptr)
{
	FileIn file = FS_PathManager::OpenFile(filename);
	if (!file.IsOpen())
	{
		LOG("LoadFileToBuffer: Could not open " << filename);
		return false;
	}
	
	int64_t size = file.GetSize();
	if (size <= 0 || size > 100*1024*1024) // Sanity check
	{
		LOG("LoadFileToBuffer: Invalid file size " << size);
		return false;
	}
	
	outBuffer.SetCount((size_t)size);
	size_t read = file.Get(outBuffer.Begin(), (size_t)size);
	if (read != (size_t)size)
	{
		LOG("LoadFileToBuffer: Short read " << read << " != " << size);
		return false;
	}
	
	if (outSize) *outSize = (int)size;
	return true;
}

// ============================================================================
// Byte swap helpers (LE to host)
// ============================================================================

static inline uint16_t SwapLE16(uint16_t v) { return (v >> 8) | (v << 8); }
static inline uint32_t SwapLE32(uint32_t v) { return (v >> 24) | ((v >> 8) & 0xFF00) | ((v << 8) & 0xFF0000) | (v << 24); }

// ============================================================================
// GAMEMAPS Loading (matching OmniSpeak CAL_SetupMapFile / CA_CacheMap)
// ============================================================================

bool ResourceManager::LoadGAMEMAPS()
{
	if (!g_currentEpisode)
	{
		LOG("LoadGAMEMAPS: No current episode set");
		return false;
	}
	
	if (!g_currentEpisode->mapHeadFile)
	{
		LOG("LoadGAMEMAPS: Episode missing mapHeadFile");
		return false;
	}
	
	LOG("ResourceManager::LoadGAMEMAPS: Loading " << g_currentEpisode->mapHeadFile);
	
	// Load MAPHEAD file
	Vector<uint8_t> mapHeadData;
	int mapHeadSize = 0;
	if (!LoadFileToBuffer(g_currentEpisode->mapHeadFile, mapHeadData, &mapHeadSize))
	{
		LOG("LoadGAMEMAPS: Failed to load MAPHEAD");
		return false;
	}
	
	if (mapHeadSize < (int)sizeof(CA_MapHead))
	{
		LOG("LoadGAMEMAPS: MAPHEAD file too small");
		return false;
	}
	
	// Parse MAPHEAD
	g_mapHead = (CA_MapHead*)MemoryAlloc(sizeof(CA_MapHead));
	
	// Read rleTag (uint16 LE)
	g_mapHead->rleTag = SwapLE16(*(const uint16_t*)mapHeadData.Begin());
	
	// Read headerOffsets (100 x uint32 LE)
	const uint8_t* ptr = mapHeadData.Begin() + 2;
	for (int i = 0; i < CA_NUMMAPS; ++i)
	{
		g_mapHead->headerOffsets[i] = SwapLE32(*(const uint32_t*)(ptr + i * 4));
	}
	
	LOG("LoadGAMEMAPS: rleTag = 0x" << Format("%04X", g_mapHead->rleTag));
	
	// Try loading TILEINFO
	if (!LoadTileInfo())
	{
		// Fall back to tileinfo embedded in MAPHEAD
		if (mapHeadSize > (int)sizeof(CA_MapHead))
		{
			size_t tileInfoSize = mapHeadSize - sizeof(CA_MapHead);
			g_tileInfo = (uint8_t*)MemoryAlloc(tileInfoSize);
			memcpy(g_tileInfo, mapHeadData.Begin() + sizeof(CA_MapHead), tileInfoSize);
			LOG("LoadGAMEMAPS: Using embedded tileinfo (" << tileInfoSize << " bytes)");
		}
		else
		{
			LOG("LoadGAMEMAPS: No tileinfo available");
			return false;
		}
	}
	
	return true;
}

bool ResourceManager::LoadTileInfo()
{
	if (!g_currentEpisode || !g_currentEpisode->tileInfoFile)
		return false;
	
	Vector<uint8_t> tileInfoData;
	int tileInfoSize = 0;
	if (LoadFileToBuffer(g_currentEpisode->tileInfoFile, tileInfoData, &tileInfoSize))
	{
		if (g_tileInfo)
		{
			MemoryFree(g_tileInfo);
			g_tileInfo = nullptr;
		}
		g_tileInfo = (uint8_t*)MemoryAlloc(tileInfoSize);
		memcpy(g_tileInfo, tileInfoData.Begin(), tileInfoSize);
		g_tileInfoSize = tileInfoSize;
		LOG("LoadTileInfo: Loaded " << tileInfoSize << " bytes from " << g_currentEpisode->tileInfoFile);
		return true;
	}
	return false;
}

uint8_t* ResourceManager::GetTileInfo()
{
	return g_tileInfo;
}

// ============================================================================
// Tile Info Accessors (OmniSpeak id_ti.c)
//
// TILEINFO.CKx is a set of parallel per-tile byte arrays whose offsets depend
// on the GFXINFOE tile counts. Each accessor returns 0 (or logs) on an
// out-of-range tile or when TILEINFO has not been loaded.
// ============================================================================

static inline bool TI_IndexValid(int index)
{
	return g_tileInfo && index >= 0 && index < g_tileInfoSize;
}

uint8_t ResourceManager::TileForeTop(uint16_t tile)
{
	const int index = tile + g_gfxInfoE.numTiles16 * 2;
	if (tile > g_gfxInfoE.numTiles16m || !TI_IndexValid(index))
		return 0;
	return g_tileInfo[index];
}

uint8_t ResourceManager::TileForeBottom(uint16_t tile)
{
	const int index = tile + g_gfxInfoE.numTiles16 * 2 + g_gfxInfoE.numTiles16m * 2;
	if (tile > g_gfxInfoE.numTiles16m || !TI_IndexValid(index))
		return 0;
	return g_tileInfo[index];
}

uint8_t ResourceManager::TileForeLeft(uint16_t tile)
{
	const int index = tile + g_gfxInfoE.numTiles16 * 2 + g_gfxInfoE.numTiles16m * 3;
	if (tile > g_gfxInfoE.numTiles16m || !TI_IndexValid(index))
		return 0;
	return g_tileInfo[index];
}

uint8_t ResourceManager::TileForeRight(uint16_t tile)
{
	const int index = tile + g_gfxInfoE.numTiles16 * 2 + g_gfxInfoE.numTiles16m;
	if (tile > g_gfxInfoE.numTiles16m || !TI_IndexValid(index))
		return 0;
	return g_tileInfo[index];
}

uint8_t ResourceManager::TileForeMisc(uint16_t tile)
{
	const int index = tile + g_gfxInfoE.numTiles16 * 2 + g_gfxInfoE.numTiles16m * 5;
	if (tile > g_gfxInfoE.numTiles16m || !TI_IndexValid(index))
		return 0;
	return g_tileInfo[index];
}

uint8_t ResourceManager::TileBackAnimTime(uint16_t tile)
{
	if (tile > g_gfxInfoE.numTiles16 || !TI_IndexValid(tile))
		return 0;
	return g_tileInfo[tile];
}

int8_t ResourceManager::TileBackAnimTile(uint16_t tile)
{
	const int index = tile + g_gfxInfoE.numTiles16;
	if (tile > g_gfxInfoE.numTiles16 || !TI_IndexValid(index))
		return 0;
	return (int8_t)g_tileInfo[index];
}

int8_t ResourceManager::TileForeAnimTile(uint16_t tile)
{
	const int index = tile + g_gfxInfoE.numTiles16 * 2 + g_gfxInfoE.numTiles16m * 4;
	if (tile > g_gfxInfoE.numTiles16m || !TI_IndexValid(index))
		return 0;
	return (int8_t)g_tileInfo[index];
}

uint8_t ResourceManager::TileForeAnimTime(uint16_t tile)
{
	const int index = tile + g_gfxInfoE.numTiles16 * 2 + g_gfxInfoE.numTiles16m * 6;
	if (tile > g_gfxInfoE.numTiles16m || !TI_IndexValid(index))
		return 0;
	return g_tileInfo[index];
}

// ============================================================================
// GFXINFOE loading
// ============================================================================

bool ResourceManager::LoadGfxInfo()
{
	if (!g_currentEpisode)
	{
		LOG("LoadGfxInfo: No episode selected");
		return false;
	}

	String gfxInfoFile = g_currentEpisode->gfxInfoFile;
	if (gfxInfoFile.IsEmpty())
		gfxInfoFile = "GFXINFOE." + String(g_currentEpisode->graphHeadExt);

	Vector<uint8_t> gfxInfoData;
	int gfxInfoSize = 0;
	if (!LoadFileToBuffer(gfxInfoFile, gfxInfoData, &gfxInfoSize))
	{
		LOG("LoadGfxInfo: GFXINFOE not found: " << gfxInfoFile);
		return false;
	}

	if (gfxInfoSize < (int)sizeof(CA_GfxInfo))
	{
		LOG("LoadGfxInfo: GFXINFOE too small: " << gfxInfoSize);
		return false;
	}

	memcpy(&g_gfxInfoE, gfxInfoData.Begin(), sizeof(CA_GfxInfo));
	LOG("LoadGfxInfo: " << g_gfxInfoE.numBitmaps << " bitmaps, "
		<< g_gfxInfoE.numMasked << " masked, " << g_gfxInfoE.numSprites << " sprites");
	return true;
}

// ============================================================================
// Map Loading (matching OmniSpeak CA_CacheMap)
// ============================================================================

bool ResourceManager::LoadMap(int mapNum)
{
	if (!g_mapHead)
	{
		LOG("LoadMap: GAMEMAPS not loaded");
		return false;
	}
	
	if (mapNum < 0 || mapNum >= CA_NUMMAPS)
	{
		LOG("LoadMap: Invalid map number " << mapNum);
		return false;
	}
	
	uint32_t headerOffset = g_mapHead->headerOffsets[mapNum];
	if (headerOffset == 0xFFFFFFFF)
	{
		LOG("LoadMap: Map " << mapNum << " does not exist (offset = 0xFFFFFFFF)");
		return false;
	}
	
	// Unload previous map planes
	for (int plane = 0; plane < CA_NUMMAPPLANES; ++plane)
	{
		if (g_mapPlanes[plane])
		{
			MemoryFree(g_mapPlanes[plane]);
			g_mapPlanes[plane] = nullptr;
		}
	}
	
	// Unload previous map header
	if (g_mapHeaders[mapNum])
	{
		MemoryFree(g_mapHeaders[mapNum]);
		g_mapHeaders[mapNum] = nullptr;
	}
	
	g_mapOn = mapNum;
	
	// Load map header from GAMEMAPS ("GAMEMAPS.CK4" etc.)
	String gameMapsFileStr = g_currentEpisode->mapFile;
	if (gameMapsFileStr.IsEmpty())
	{
		gameMapsFileStr = "GAMEMAPS.";
		if (g_currentEpisode->mapHeadExt)
			gameMapsFileStr += g_currentEpisode->mapHeadExt;
	}
	
	FileIn mapFile = FS_PathManager::OpenFile(gameMapsFileStr);
	if (!mapFile.IsOpen())
	{
		LOG("LoadMap: Could not open GAMEMAPS file: " << gameMapsFileStr);
		return false;
	}
	
	// Allocate map header
	g_mapHeaders[mapNum] = (CA_MapHeader*)MemoryAlloc(sizeof(CA_MapHeader));
	
	// Seek to header offset
	mapFile.Seek(headerOffset);
	
	// Read planeOffsets (4 x uint32 LE)
	for (int i = 0; i < CA_NUMMAPPLANES; ++i)
	{
		uint32_t val;
		mapFile.Get(&val, 4);
		g_mapHeaders[mapNum]->planeOffsets[i] = SwapLE32(val);
	}
	
	// Read planeLengths (4 x uint16 LE)
	for (int i = 0; i < CA_NUMMAPPLANES; ++i)
	{
		uint16_t val;
		mapFile.Get(&val, 2);
		g_mapHeaders[mapNum]->planeLengths[i] = SwapLE16(val);
	}
	
	// Read width, height (uint16 LE each)
	{
		uint16_t val;
		mapFile.Get(&val, 2);
		g_mapHeaders[mapNum]->width = SwapLE16(val);
		mapFile.Get(&val, 2);
		g_mapHeaders[mapNum]->height = SwapLE16(val);
	}
	
	// Read name (16 bytes)
	mapFile.Get(g_mapHeaders[mapNum]->name, 16);
	
	// Read signature (4 bytes)
	mapFile.Get(g_mapHeaders[mapNum]->signature, 4);
	
	LOG("LoadMap: Loaded map " << mapNum << " \"" << g_mapHeaders[mapNum]->name 
		<< "\" " << g_mapHeaders[mapNum]->width << "x" << g_mapHeaders[mapNum]->height);
	
	// Now load the map planes
	return LoadMapPlanes(mapNum);
}

bool ResourceManager::LoadMapPlanes(int mapNum)
{
	if (mapNum < 0 || mapNum >= CA_NUMMAPS || !g_mapHeaders[mapNum])
	{
		LOG("LoadMapPlanes: Invalid map or header not loaded");
		return false;
	}
	
	CA_MapHeader* header = g_mapHeaders[mapNum];
	
	// Open GAMEMAPS file
	String gameMapsFileStr = g_currentEpisode ? g_currentEpisode->mapFile : nullptr;
	if (gameMapsFileStr.IsEmpty())
	{
		gameMapsFileStr = "GAMEMAPS.";
		if (g_currentEpisode && g_currentEpisode->mapHeadExt)
			gameMapsFileStr += g_currentEpisode->mapHeadExt;
	}
	
	FileIn mapFile = FS_PathManager::OpenFile(gameMapsFileStr);
	if (!mapFile.IsOpen())
	{
		LOG("LoadMapPlanes: Could not open GAMEMAPS: " << gameMapsFileStr);
		return false;
	}
	
	int planeSize = header->width * header->height * 2; // uint16 per tile
	
	// Load each plane
	for (int plane = 0; plane < CA_NUMMAPPLANES; ++plane)
	{
		int32_t planeOffset = header->planeOffsets[plane];
		int32_t planeCompLength = header->planeLengths[plane];
		
		// Allocate output buffer for decompressed plane
		g_mapPlanes[plane] = (uint16_t*)MemoryAlloc(planeSize);
		
		// Seek to plane data
		mapFile.Seek(planeOffset);
		
		// Read compressed data
		Vector<uint8_t> compBuffer;
		compBuffer.SetCount(planeCompLength);
		size_t read = mapFile.Get(compBuffer.Begin(), planeCompLength);
		if (read != (size_t)planeCompLength)
		{
			LOG("LoadMapPlanes: Short read for plane " << plane);
			return false;
		}
		
		// Decompress: Carmack -> RLEW
		// First word of compressed data is the Carmack-expanded size (uint16 LE)
		uint16_t carmackExpanded = SwapLE16(*(const uint16_t*)compBuffer.Begin());
		
		// Allocate intermediate buffer for Carmack output
		Vector<uint16_t> rlewBuffer;
		rlewBuffer.SetCount(carmackExpanded);
		
		// Carmack expand (skip first 2 bytes which is the length)
		DecompressInfo carmackInfo = CarmackDecoder::Decode(
			compBuffer.Begin() + 2, planeCompLength - 2,
			rlewBuffer.Begin(), carmackExpanded);
		
		if (carmackInfo.result != DecompressResult::OK)
		{
			LOG("LoadMapPlanes: Carmack decode failed for plane " << plane 
				<< " result=" << (int)carmackInfo.result);
			return false;
		}
		
		// RLEW expand
		DecompressInfo rlewInfo = RLEWDecoder::Decode(
			rlewBuffer.Begin(), carmackExpanded,
			g_mapPlanes[plane], planeSize / 2);
		
		if (rlewInfo.result != DecompressResult::OK)
		{
			LOG("LoadMapPlanes: RLEW decode failed for plane " << plane
				<< " result=" << (int)rlewInfo.result);
			return false;
		}
		
		LOG("LoadMapPlanes: Plane " << plane << " loaded (" 
			<< planeSize << " bytes, carmack=" << carmackExpanded << " words)");
	}
	
	return true;
}

// ============================================================================
// Map Data Access
// ============================================================================

uint16_t ResourceManager::GetMapWidth()
{
	if (g_mapOn >= 0 && g_mapHeaders[g_mapOn])
		return g_mapHeaders[g_mapOn]->width;
	return 0;
}

uint16_t ResourceManager::GetMapHeight()
{
	if (g_mapOn >= 0 && g_mapHeaders[g_mapOn])
		return g_mapHeaders[g_mapOn]->height;
	return 0;
}

uint16_t* ResourceManager::GetMapPlane(int plane)
{
	if (plane >= 0 && plane < CA_NUMMAPPLANES)
		return g_mapPlanes[plane];
	return nullptr;
}

uint16_t ResourceManager::GetTileAt(int x, int y, int plane)
{
	if (g_mapOn < 0 || !g_mapHeaders[g_mapOn] || !g_mapPlanes[plane])
		return 0;
	
	CA_MapHeader* header = g_mapHeaders[g_mapOn];
	if (x >= 0 && x < header->width && y >= 0 && y < header->height)
	{
		return g_mapPlanes[plane][y * header->width + x];
	}
	
	// OmniSpeak behavior for out of bounds - return pseudo-random tile
	return (y * x * y * x) % 1000;
}

void ResourceManager::SetTileAt(int x, int y, int plane, uint16_t value)
{
	if (g_mapOn < 0 || !g_mapHeaders[g_mapOn] || !g_mapPlanes[plane])
		return;
	
	CA_MapHeader* header = g_mapHeaders[g_mapOn];
	if (x >= 0 && x < header->width && y >= 0 && y < header->height)
	{
		g_mapPlanes[plane][y * header->width + x] = value;
	}
}

const char* ResourceManager::GetMapName(int mapNum)
{
	if (mapNum >= 0 && mapNum < CA_NUMMAPS && g_mapHeaders[mapNum])
		return g_mapHeaders[mapNum]->name;
	return "";
}

// ============================================================================
// ResourceManager Implementation (stubs for remaining functions)
// ============================================================================

bool ResourceManager::s_initialized = false;

void ResourceManager::Startup()
{
	if (s_initialized) return;
	FS_PathManager::Startup();
	VAR_Engine::Startup();
	s_initialized = true;
	LOG("ResourceManager::Startup");
}

void ResourceManager::Shutdown()
{
	if (!s_initialized) return;
	
	// Free map data
	for (int plane = 0; plane < CA_NUMMAPPLANES; ++plane)
	{
		if (g_mapPlanes[plane])
		{
			MemoryFree(g_mapPlanes[plane]);
			g_mapPlanes[plane] = nullptr;
		}
	}
	
	for (int i = 0; i < CA_NUMMAPS; ++i)
	{
		if (g_mapHeaders[i])
		{
			MemoryFree(g_mapHeaders[i]);
			g_mapHeaders[i] = nullptr;
		}
	}
	
	if (g_mapHead)
	{
		MemoryFree(g_mapHead);
		g_mapHead = nullptr;
	}
	
	if (g_tileInfo)
	{
		MemoryFree(g_tileInfo);
		g_tileInfo = nullptr;
		g_tileInfoSize = 0;
	}
	
	// Free EGAGRAPH data
	if (g_graphHandle)
	{
		if (g_graphHandle->IsOpen())
		{
			g_graphHandle->Close();
		}
		delete g_graphHandle;
		g_graphHandle = nullptr;
	}
	for (int i = 0; i < CA_MAX_GRAPH_CHUNKS; ++i)
	{
		if (g_graphChunks[i])
		{
			MemoryFree(g_graphChunks[i]);
			g_graphChunks[i] = nullptr;
		}
	}
	g_gfxInfoE = {0};
	memset(g_chunkInfo, 0, sizeof(g_chunkInfo));
	memset(g_graphChunkNeeded, 0, sizeof(g_graphChunkNeeded));
	g_graphFileSize = 0;
	
	// Free AUDIO data
	if (g_audioHandle)
	{
		if (g_audioHandle->IsOpen())
		{
			g_audioHandle->Close();
		}
		delete g_audioHandle;
		g_audioHandle = nullptr;
	}
	for (int i = 0; i < CA_MAX_AUDIO_CHUNKS; ++i)
	{
		if (g_audioChunks[i])
		{
			MemoryFree(g_audioChunks[i]);
			g_audioChunks[i] = nullptr;
		}
	}
	g_audioInfoE = {0};
	memset(g_audioChunkInfo, 0, sizeof(g_audioChunkInfo));
	memset(g_audioChunkNeeded, 0, sizeof(g_audioChunkNeeded));
	g_audioFileSize = 0;
	
	VAR_Engine::Shutdown();
	FS_PathManager::Shutdown();
	s_initialized = false;
	LOG("ResourceManager::Shutdown");
}

CacheEntry* ResourceManager::LoadChunk(int chunkNum)
{
	LOG("ResourceManager::LoadChunk: " << chunkNum << " - Not implemented");
	return nullptr;
}

CacheEntry* ResourceManager::LoadChunkLocked(int chunkNum)
{
	LOG("ResourceManager::LoadChunkLocked: " << chunkNum << " - Not implemented");
	return nullptr;
}

void ResourceManager::UnlockChunk(int chunkNum)
{
	LOG("ResourceManager::UnlockChunk: " << chunkNum << " - Not implemented");
}

void ResourceManager::MarkGraphicsChunk(int chunkNum)
{
	LOG("ResourceManager::MarkGraphicsChunk: " << chunkNum);
}

void ResourceManager::MarkAudioChunk(int chunkNum)
{
	LOG("ResourceManager::MarkAudioChunk: " << chunkNum);
}

void ResourceManager::MarkMapChunk(int chunkNum)
{
	LOG("ResourceManager::MarkMapChunk: " << chunkNum);
}

void ResourceManager::ClearAllMarks()
{
	LOG("ResourceManager::ClearAllMarks");
}

void ResourceManager::CacheAllMarked(int priority)
{
	LOG("ResourceManager::CacheAllMarked: priority=" << priority);
}

bool ResourceManager::LoadEpisodeIndex(const char* episodeFile)
{
	LOG("ResourceManager::LoadEpisodeIndex: " << episodeFile << " - Not implemented");
	return false;
}

// Forward declarations for static helpers
static void LoadGraphChunk(int chunk);
static void LockGraphChunk(int chunk);
static void UnlockGraphChunk(int chunk);

bool ResourceManager::LoadEGAGRAPH()
{
	if (!g_currentEpisode)
	{
		LOG("LoadEGAGRAPH: No episode selected");
		return false;
	}
	
	LOG("LoadEGAGRAPH: Loading EGAHEAD/EGADICT/EGAGRAPH...");
	
	// 1. Load EGADICT (Huffman dictionary)
	String dictFile = g_currentEpisode->graphDictFile; // "EGADICT.CK4" etc.
	Vector<uint8_t> dictData;
	int dictSize = 0;
	if (!LoadFileToBuffer(dictFile, dictData, &dictSize))
	{
		LOG("LoadEGAGRAPH: Could not load Huffman dictionary: " << dictFile);
		return false;
	}
	
	if (!HuffmanDecoder::BuildFromEGAHEAD(dictData.Begin(), dictSize, g_grHuffDict))
	{
		LOG("LoadEGAGRAPH: Failed to build Huffman decoder from EGADICT");
		return false;
	}
	LOG("LoadEGAGRAPH: Loaded EGADICT (" << dictSize << " bytes)");
	
	// 2. Load EGAHEAD (chunk offset/size table)
	String headFile = g_currentEpisode->graphHeadFile; // "EGAHEAD.CK4" etc.
	Vector<uint8_t> headData;
	int headSize = 0;
	if (!LoadFileToBuffer(headFile, headData, &headSize))
	{
		LOG("LoadEGAGRAPH: Could not load chunk header table: " << headFile);
		return false;
	}
	
	// EGAHEAD format: 3-byte entries per chunk
	// First half: 3-byte offsets for each chunk
	// Second half: 3-byte compressed lengths for each chunk
	int numChunks = headSize / 6;
	if (numChunks > CA_MAX_GRAPH_CHUNKS)
		numChunks = CA_MAX_GRAPH_CHUNKS;
	
	const uint8_t* headPtr = headData.Begin();
	// Read offsets
	for (int i = 0; i < numChunks; ++i)
	{
		uint32_t offset = headPtr[0] | (headPtr[1] << 8) | (headPtr[2] << 16);
		g_chunkInfo[i].offset = offset;
		headPtr += 3;
	}
	// Read compressed lengths
	for (int i = 0; i < numChunks; ++i)
	{
		uint32_t compLen = headPtr[0] | (headPtr[1] << 8) | (headPtr[2] << 16);
		g_chunkInfo[i].compLength = compLen;
		headPtr += 3;
	}
	
	LOG("LoadEGAGRAPH: Loaded EGAHEAD (" << headSize << " bytes, " << numChunks << " chunks)");
	
	// 3. Load GFXINFOE (chunk metadata from TED5)
	LoadGfxInfo();
	
	// 4. Open EGAGRAPH file (keep open for chunk streaming)
	String graphFile = g_currentEpisode->graphFile; // "EGAGRAPH.CK4" etc.
	if (g_graphHandle)
	{
		delete g_graphHandle;
		g_graphHandle = nullptr;
	}
	g_graphHandle = new FileIn(FS_PathManager::OpenFile(graphFile));
	if (!g_graphHandle->IsOpen())
	{
		LOG("LoadEGAGRAPH: Could not open EGAGRAPH: " << graphFile);
		delete g_graphHandle;
		g_graphHandle = nullptr;
		return false;
	}
	
	g_graphFileSize = g_graphHandle->GetSize();
	LOG("LoadEGAGRAPH: Opened EGAGRAPH (" << g_graphFileSize << " bytes)");
	
	// 5. Load and lock header chunks (bitmaps, masked, sprites)
	if (g_gfxInfoE.hdrBitmaps > 0 && g_gfxInfoE.hdrBitmaps < CA_MAX_GRAPH_CHUNKS)
	{
		LoadGraphChunk(g_gfxInfoE.hdrBitmaps);
		LockGraphChunk(g_gfxInfoE.hdrBitmaps);
	}
	if (g_gfxInfoE.hdrMasked > 0 && g_gfxInfoE.hdrMasked < CA_MAX_GRAPH_CHUNKS)
	{
		LoadGraphChunk(g_gfxInfoE.hdrMasked);
		LockGraphChunk(g_gfxInfoE.hdrMasked);
	}
	if (g_gfxInfoE.hdrSprites > 0 && g_gfxInfoE.hdrSprites < CA_MAX_GRAPH_CHUNKS)
	{
		LoadGraphChunk(g_gfxInfoE.hdrSprites);
		LockGraphChunk(g_gfxInfoE.hdrSprites);
	}
	
	LOG("LoadEGAGRAPH: Complete");
	return true;
}

// Internal helper to load a single graphics chunk
static void LoadGraphChunk(int chunk)
{
	if (chunk < 0 || chunk >= CA_MAX_GRAPH_CHUNKS)
		return;
	
	if (g_graphChunks[chunk])
		return; // Already loaded
	
	if (g_chunkInfo[chunk].offset < 0)
		return; // Chunk not in EGAHEAD
	
	int compLength = g_chunkInfo[chunk].compLength;
	if (compLength <= 0 && chunk + 1 < CA_MAX_GRAPH_CHUNKS)
	{
		// Compute from next chunk
		compLength = g_chunkInfo[chunk + 1].offset - g_chunkInfo[chunk].offset;
	}
	if (compLength <= 0)
	{
		compLength = (int)g_graphFileSize - g_chunkInfo[chunk].offset;
	}
	
	if (compLength <= 0)
		return;
	
	// Seek and read compressed data
	if (!g_graphHandle || !g_graphHandle->IsOpen())
		return;
	g_graphHandle->Seek(g_chunkInfo[chunk].offset);
	Vector<uint8_t> compData;
	compData.SetCount(compLength);
	size_t read = g_graphHandle->Get(compData.Begin(), compLength);
	if (read != (size_t)compLength)
	{
		LOG("LoadGraphChunk: Short read for chunk " << chunk);
		return;
	}
	
	// Determine expected expanded length
	// For header chunks, we know the structure sizes
	int expLength = g_chunkInfo[chunk].expLength;
	if (expLength == 0)
	{
		// Try to determine from chunk type
		if (chunk == g_gfxInfoE.hdrBitmaps)
			expLength = g_gfxInfoE.numBitmaps * sizeof(VH_BitmapTableEntry);
		else if (chunk == g_gfxInfoE.hdrMasked)
			expLength = g_gfxInfoE.numMasked * sizeof(VH_BitmapTableEntry);
		else if (chunk == g_gfxInfoE.hdrSprites)
			expLength = g_gfxInfoE.numSprites * sizeof(VH_SpriteTableEntry);
		else
		{
			// For regular chunks, first 4 bytes of compressed data is expanded length (LE)
			if (compLength >= 4)
			{
				expLength = compData[0] | (compData[1] << 8) | (compData[2] << 16) | (compData[3] << 24);
			}
		}
	}
	
	if (expLength <= 0)
		expLength = compLength * 2; // Guess
	
	// Allocate output buffer
	void* outPtr = MemoryAlloc(expLength);
	
	// Decompress: Huffman
	DecompressInfo info = g_grHuffDict.Decode(compData.Begin(), compLength, (uint8_t*)outPtr, expLength);
	if (info.result != DecompressResult::OK)
	{
		LOG("LoadGraphChunk: Huffman decode failed for chunk " << chunk 
			<< " result=" << (int)info.result);
		MemoryFree(outPtr);
		return;
	}
	
	g_graphChunks[chunk] = outPtr;
	g_chunkInfo[chunk].expLength = info.bytesWritten;
	
	// Handle endianness for known structures
	if (chunk == g_gfxInfoE.hdrBitmaps)
	{
		VH_BitmapTableEntry* table = (VH_BitmapTableEntry*)outPtr;
		for (int i = 0; i < g_gfxInfoE.numBitmaps; ++i)
		{
			table[i].width = SwapLE16(table[i].width);
			table[i].height = SwapLE16(table[i].height);
		}
	}
	else if (chunk == g_gfxInfoE.hdrMasked)
	{
		VH_BitmapTableEntry* table = (VH_BitmapTableEntry*)outPtr;
		for (int i = 0; i < g_gfxInfoE.numMasked; ++i)
		{
			table[i].width = SwapLE16(table[i].width);
			table[i].height = SwapLE16(table[i].height);
		}
	}
	else if (chunk == g_gfxInfoE.hdrSprites)
	{
		VH_SpriteTableEntry* table = (VH_SpriteTableEntry*)outPtr;
		for (int i = 0; i < g_gfxInfoE.numSprites; ++i)
		{
			table[i].width = SwapLE16(table[i].width);
			table[i].height = SwapLE16(table[i].height);
			table[i].originX = SwapLE16(table[i].originX);
			table[i].originY = SwapLE16(table[i].originY);
			table[i].xl = SwapLE16(table[i].xl);
			table[i].yl = SwapLE16(table[i].yl);
			table[i].xh = SwapLE16(table[i].xh);
			table[i].yh = SwapLE16(table[i].yh);
			table[i].shifts = SwapLE16(table[i].shifts);
		}
	}
	
	LOG("LoadGraphChunk: Loaded chunk " << chunk << " (" << info.bytesWritten << " bytes expanded)");
}

// Lock/unlock helpers for graphics chunks
static void LockGraphChunk(int chunk)
{
	if (chunk >= 0 && chunk < CA_MAX_GRAPH_CHUNKS)
	{
		g_graphChunkNeeded[chunk] |= 0x80;
	}
}

static void UnlockGraphChunk(int chunk)
{
	if (chunk >= 0 && chunk < CA_MAX_GRAPH_CHUNKS)
	{
		g_graphChunkNeeded[chunk] &= ~0x80;
	}
}

// ============================================================================
// EGAGRAPH Access Functions
// ============================================================================

const CA_GfxInfo* ResourceManager::GetGfxInfo()
{
	return &g_gfxInfoE;
}

void* ResourceManager::GetGraphChunk(int chunkNum)
{
	if (chunkNum < 0 || chunkNum >= CA_MAX_GRAPH_CHUNKS)
		return nullptr;
	
	if (!g_graphChunks[chunkNum])
	{
		LoadGraphChunk(chunkNum);
	}
	
	return g_graphChunks[chunkNum];
}

void* ResourceManager::GetGraphChunkLocked(int chunkNum)
{
	void* chunk = GetGraphChunk(chunkNum);
	if (chunk)
	{
		// Mark as locked (increase ref count)
		g_graphChunkNeeded[chunkNum] |= 0x80; // Use high bit as lock flag
	}
	return chunk;
}

void ResourceManager::UnlockGraphChunk(int chunkNum)
{
	if (chunkNum >= 0 && chunkNum < CA_MAX_GRAPH_CHUNKS)
	{
		g_graphChunkNeeded[chunkNum] &= ~0x80;
	}
}

int ResourceManager::GetGraphChunkExpLength(int chunkNum)
{
	if (chunkNum < 0 || chunkNum >= CA_MAX_GRAPH_CHUNKS)
		return 0;
	return g_chunkInfo[chunkNum].expLength;
}

int ResourceManager::GetGraphChunkCompLength(int chunkNum)
{
	if (chunkNum < 0 || chunkNum >= CA_MAX_GRAPH_CHUNKS)
		return 0;
	return g_chunkInfo[chunkNum].compLength;
}

int ResourceManager::GetGraphChunkStart(int chunkNum)
{
	if (chunkNum < 0 || chunkNum >= CA_MAX_GRAPH_CHUNKS)
		return -1;
	return g_chunkInfo[chunkNum].offset;
}

bool ResourceManager::LoadAUDIO()
{
	if (!g_currentEpisode)
	{
		LOG("LoadAUDIO: No episode selected");
		return false;
	}
	
	LOG("LoadAUDIO: Loading AUDIOHHD/AUDIODCT/AUDIO...");
	
	// 1. Load AUDINFOE (audio info from TED5)
	String audioInfoFile = g_currentEpisode->audioInfoFile;
	if (audioInfoFile.IsEmpty())
		audioInfoFile = "AUDINFOE." + String(g_currentEpisode->audioHeadExt); // "AUDINFOE.CK4"
	Vector<uint8_t> audioInfoData;
	int audioInfoSize = 0;
	if (LoadFileToBuffer(audioInfoFile, audioInfoData, &audioInfoSize))
	{
		if (audioInfoSize >= (int)sizeof(CA_AudioInfo))
		{
			memcpy(&g_audioInfoE, audioInfoData.Begin(), sizeof(CA_AudioInfo));
			LOG("LoadAUDIO: Loaded AUDINFOE - " << g_audioInfoE.numSongs << " songs, "
				<< g_audioInfoE.numSounds << " sounds, " << g_audioInfoE.numSndChunks << " chunks");
		}
		else
		{
			LOG("LoadAUDIO: AUDINFOE too small: " << audioInfoSize);
		}
	}
	else
	{
		LOG("LoadAUDIO: AUDINFOE not found, using defaults");
	}
	
	// 2. Load AUDIODCT (Huffman dictionary)
	String dictFile = g_currentEpisode->audioDictFile; // "AUDIODCT.CK4" etc.
	Vector<uint8_t> dictData;
	int dictSize = 0;
	if (LoadFileToBuffer(dictFile, dictData, &dictSize))
	{
		if (!HuffmanDecoder::BuildFromAUDIOHHD(dictData.Begin(), dictSize, g_audioHuffDict))
		{
			LOG("LoadAUDIO: Failed to build Huffman decoder from AUDIODCT");
			return false;
		}
		LOG("LoadAUDIO: Loaded AUDIODCT (" << dictSize << " bytes)");
	}
	else
	{
		LOG("LoadAUDIO: Could not load AUDIODCT: " << dictFile);
		return false;
	}
	
	// 3. Load AUDIOHHD (chunk offset/size table)
	String headFile = g_currentEpisode->audioHeadFile; // "AUDIOHHD.CK4" etc.
	Vector<uint8_t> headData;
	int headSize = 0;
	if (!LoadFileToBuffer(headFile, headData, &headSize))
	{
		LOG("LoadAUDIO: Could not load audio chunk header table: " << headFile);
		return false;
	}
	
	// AUDIOHHD format: 3-byte offsets + 3-byte lengths per chunk
	int numChunks = headSize / 6;
	if (numChunks > CA_MAX_AUDIO_CHUNKS)
		numChunks = CA_MAX_AUDIO_CHUNKS;
	
	const uint8_t* headPtr = headData.Begin();
	// Read offsets
	for (int i = 0; i < numChunks; ++i)
	{
		uint32_t offset = headPtr[0] | (headPtr[1] << 8) | (headPtr[2] << 16);
		g_audioChunkInfo[i].offset = offset;
		headPtr += 3;
	}
	// Read compressed lengths
	for (int i = 0; i < numChunks; ++i)
	{
		uint32_t compLen = headPtr[0] | (headPtr[1] << 8) | (headPtr[2] << 16);
		g_audioChunkInfo[i].compLength = compLen;
		headPtr += 3;
	}
	
	LOG("LoadAUDIO: Loaded AUDIOHHD (" << headSize << " bytes, " << numChunks << " chunks)");
	
	// 4. Open AUDIO file (keep open for chunk streaming)
	String audioFile = g_currentEpisode->audioFile; // "AUDIO.CK4" etc.
	if (g_audioHandle)
	{
		delete g_audioHandle;
		g_audioHandle = nullptr;
	}
	g_audioHandle = new FileIn(FS_PathManager::OpenFile(audioFile));
	if (!g_audioHandle->IsOpen())
	{
		LOG("LoadAUDIO: Could not open AUDIO: " << audioFile);
		delete g_audioHandle;
		g_audioHandle = nullptr;
		return false;
	}
	
	g_audioFileSize = g_audioHandle->GetSize();
	LOG("LoadAUDIO: Opened AUDIO (" << g_audioFileSize << " bytes)");
	
	LOG("LoadAUDIO: Complete");
	return true;
}

// Internal helper to load a single audio chunk
static void LoadAudioChunk(int chunk)
{
	if (chunk < 0 || chunk >= CA_MAX_AUDIO_CHUNKS)
		return;
	
	if (g_audioChunks[chunk])
		return; // Already loaded
	
	if (g_audioChunkInfo[chunk].offset < 0)
		return; // Chunk not in AUDIOHHD
	
	if (!g_audioHandle || !g_audioHandle->IsOpen())
		return;
	
	int compLength = g_audioChunkInfo[chunk].compLength;
	if (compLength <= 0 && chunk + 1 < CA_MAX_AUDIO_CHUNKS)
	{
		compLength = g_audioChunkInfo[chunk + 1].offset - g_audioChunkInfo[chunk].offset;
	}
	if (compLength <= 0)
	{
		compLength = (int)g_audioFileSize - g_audioChunkInfo[chunk].offset;
	}
	
	if (compLength <= 0)
		return;
	
	// Seek and read compressed data
	g_audioHandle->Seek(g_audioChunkInfo[chunk].offset);
	Vector<uint8_t> compData;
	compData.SetCount(compLength);
	size_t read = g_audioHandle->Get(compData.Begin(), compLength);
	if (read != (size_t)compLength)
	{
		LOG("LoadAudioChunk: Short read for chunk " << chunk);
		return;
	}
	
	// Determine expected expanded length
	// For audio chunks, first 4 bytes of compressed data is expanded length (LE)
	int expLength = g_audioChunkInfo[chunk].expLength;
	if (expLength == 0)
	{
		if (compLength >= 4)
		{
			expLength = compData[0] | (compData[1] << 8) | (compData[2] << 16) | (compData[3] << 24);
		}
	}
	
	if (expLength <= 0)
		expLength = compLength * 2; // Guess
	
	// Allocate output buffer
	void* outPtr = MemoryAlloc(expLength);
	
	// Decompress: Huffman
	DecompressInfo info = g_audioHuffDict.Decode(compData.Begin(), compLength, (uint8_t*)outPtr, expLength);
	if (info.result != DecompressResult::OK)
	{
		LOG("LoadAudioChunk: Huffman decode failed for chunk " << chunk 
			<< " result=" << (int)info.result);
		MemoryFree(outPtr);
		return;
	}
	
	g_audioChunks[chunk] = outPtr;
	g_audioChunkInfo[chunk].expLength = info.bytesWritten;
	
	LOG("LoadAudioChunk: Loaded chunk " << chunk << " (" << info.bytesWritten << " bytes expanded)");
}

// Lock/unlock helpers for audio chunks
static void LockAudioChunk(int chunk)
{
	if (chunk >= 0 && chunk < CA_MAX_AUDIO_CHUNKS)
	{
		g_audioChunkNeeded[chunk] |= 0x80;
	}
}

static void UnlockAudioChunk(int chunk)
{
	if (chunk >= 0 && chunk < CA_MAX_AUDIO_CHUNKS)
	{
		g_audioChunkNeeded[chunk] &= ~0x80;
	}
}

// ============================================================================
// AUDIO Access Functions
// ============================================================================

const CA_AudioInfo* ResourceManager::GetAudioInfo()
{
	return &g_audioInfoE;
}

void* ResourceManager::GetAudioChunk(int chunkNum)
{
	if (chunkNum < 0 || chunkNum >= CA_MAX_AUDIO_CHUNKS)
		return nullptr;
	
	if (!g_audioChunks[chunkNum])
	{
		LoadAudioChunk(chunkNum);
	}
	
	return g_audioChunks[chunkNum];
}

void* ResourceManager::GetAudioChunkLocked(int chunkNum)
{
	void* chunk = GetAudioChunk(chunkNum);
	if (chunk)
	{
		LockAudioChunk(chunkNum);
	}
	return chunk;
}

void ResourceManager::UnlockAudioChunk(int chunkNum)
{
	if (chunkNum >= 0 && chunkNum < CA_MAX_AUDIO_CHUNKS)
	{
		g_audioChunkNeeded[chunkNum] &= ~0x80;
	}
}

int ResourceManager::GetAudioChunkExpLength(int chunkNum)
{
	if (chunkNum < 0 || chunkNum >= CA_MAX_AUDIO_CHUNKS)
		return 0;
	return g_audioChunkInfo[chunkNum].expLength;
}

int ResourceManager::GetAudioChunkCompLength(int chunkNum)
{
	if (chunkNum < 0 || chunkNum >= CA_MAX_AUDIO_CHUNKS)
		return 0;
	return g_audioChunkInfo[chunkNum].compLength;
}

int ResourceManager::GetAudioChunkStart(int chunkNum)
{
	if (chunkNum < 0 || chunkNum >= CA_MAX_AUDIO_CHUNKS)
		return -1;
	return g_audioChunkInfo[chunkNum].offset;
}

void ResourceManager::SetDefaultPalette()
{
	LOG("ResourceManager::SetDefaultPalette - Not implemented");
}

void ResourceManager::LoadPalette(int paletteChunk)
{
	LOG("ResourceManager::LoadPalette: " << paletteChunk << " - Not implemented");
}

END_UPP_NAMESPACE