#ifndef _Platformer2D_Resource_Resource_h_
#define _Platformer2D_Resource_Resource_h_

#include <game/Platformer2D/Platformer2D.h>
#include <game/Platformer2D/Core/Core.h>

NAMESPACE_UPP

// ============================================================================
// Filesystem Abstraction
// ============================================================================

class FS_PathManager
{
public:
	static void Startup();
	static void Shutdown();
	
	static String GetGamePath();      // KEENPATH - where GAMEMAPS.CK? etc. live
	static String GetUserPath();      // USERPATH - saves, config
	static String GetOmniPath();      // OMNIPATH - bundled support data (EGAHEAD.CK? etc.)
	
	static bool SetGamePath(const String& path);
	static bool SetUserPath(const String& path);
	static bool SetOmniPath(const String& path);
	
	// Case-insensitive file existence check
	static bool FileExists(const String& filename);
	static bool FileExistsInPaths(const String& filename, String* outFullPath = nullptr);
	
	// Open file from search paths (game -> user -> omni)
	static FileIn OpenFile(const String& filename);
	
private:
	static String s_gamePath;
	static String s_userPath;
	static String s_omniPath;
};

// ============================================================================
// Endian-Safe Binary Readers
// ============================================================================

class BinaryReader
{
public:
	explicit BinaryReader(Stream& stream) : stream(stream) {}
	
	bool ReadU8(uint8_t& out);
	bool ReadU16LE(uint16_t& out);
	bool ReadU32LE(uint32_t& out);
	bool ReadI16LE(int16_t& out);
	bool ReadI32LE(int32_t& out);
	
	bool ReadU16BE(uint16_t& out);
	bool ReadU32BE(uint32_t& out);
	
	template<typename T>
	bool ReadArray(T* out, size_t count)
	{
		return stream.Get(out, count * sizeof(T)) == count * sizeof(T);
	}
	
	bool Skip(size_t bytes);
	bool Seek(int64_t pos);
	int64_t GetPos() const;
	bool IsEOF() const;
	
private:
	Stream& stream;
};

// ============================================================================
// Map Data Structures (matching OmniSpeak id_ca.h/c exactly)
// ============================================================================

#define CA_NUMMAPPLANES 4
#define CA_NUMMAPS 100
#define CA_MAX_GRAPH_CHUNKS 8192

typedef struct CA_MapHead
{
	uint16_t rleTag;
	uint32_t headerOffsets[CA_NUMMAPS];
} CA_MapHead;

typedef struct CA_MapHeader
{
	uint32_t planeOffsets[CA_NUMMAPPLANES];
	uint16_t planeLengths[CA_NUMMAPPLANES];
	uint16_t width, height;
	char name[16];
	char signature[4];
} CA_MapHeader;

// ============================================================================
// EGAGRAPH Data Structures (matching OmniSpeak id_ca.h ca_gfxinfo)
// ============================================================================

typedef struct CA_GfxInfo
{
	uint16_t numTiles8, numTiles8m, numTiles16, numTiles16m, numTiles32, numTiles32m;
	uint16_t offTiles8, offTiles8m, offTiles16, offTiles16m, offTiles32, offTiles32m;
	uint16_t numBitmaps, numMasked, numSprites;
	uint16_t offBitmaps, offMasked, offSprites;
	uint16_t hdrBitmaps, hdrMasked, hdrSprites;
	uint16_t numBinaries, offBinaries;
} CA_GfxInfo;

typedef struct VH_BitmapTableEntry
{
	uint16_t width;
	uint16_t height;
} VH_BitmapTableEntry;

typedef struct VH_SpriteTableEntry
{
	uint16_t width, height;
	int16_t originX, originY;
	int16_t xl, yl, xh, yh;
	uint16_t shifts;
} VH_SpriteTableEntry;

typedef struct VH_Font
{
	uint16_t height;
	uint16_t location[256];
	uint8_t width[256];
} VH_Font;

// ============================================================================
// AUDIO Data Structures (matching OmniSpeak id_ca.h ca_audinfo)
// ============================================================================

#define CA_MAX_AUDIO_CHUNKS 256

typedef struct CA_AudioInfo
{
	uint16_t numSongs, numSounds, numSndChunks;
	uint16_t startPCSounds, startAdlibSounds, startDigiSounds, startMusic;
} CA_AudioInfo;

typedef struct HuffmanNodeAudio
{
	int16_t bit_0;
	int16_t bit_1;
} HuffmanNodeAudio;

// ============================================================================
// Chunk Metadata for Cache Management
// ============================================================================

struct ChunkInfo
{
	int32_t offset = -1;      // File offset in EGAGRAPH
	int32_t compLength = 0;   // Compressed length
	int32_t expLength = 0;    // Expanded length (0 = unknown)
	bool isCompressed = false;
};

// ============================================================================
// Cache Interface
// ============================================================================

class CacheEntry
{
public:
	virtual ~CacheEntry() {}
	virtual size_t GetSize() const = 0;
	virtual const void* GetData() const = 0;
	virtual void* GetData() = 0;
	virtual bool IsPurgeable() const = 0;
	virtual void SetPurgeable(bool purgeable) = 0;
	virtual int GetRefCount() const = 0;
	virtual void AddRef() = 0;
	virtual void Release() = 0;
};

class IResourceCache
{
public:
	virtual ~IResourceCache() {}
	virtual CacheEntry* GetChunk(int chunkNum) = 0;
	virtual CacheEntry* LockChunk(int chunkNum) = 0;
	virtual void UnlockChunk(int chunkNum) = 0;
	virtual void MarkChunk(int chunkNum) = 0;
	virtual void ClearMarks() = 0;
	virtual void CacheMarks(int priority = 0) = 0;
	virtual void PurgeUnmarked() = 0;
	virtual size_t GetTotalMemory() const = 0;
	virtual void SetMaxMemory(size_t bytes) = 0;
	virtual void PurgeEntry(CacheEntry* entry) = 0;
};

// Global cache instance
extern IResourceCache* g_resourceCache;

// ============================================================================
// Tile Info Globals (TILEINFO.CKx, layout from OmniSpeak id_ti.c)
//
// g_tileInfo is a set of parallel per-tile attribute arrays; the offsets
// between the arrays depend on the GFXINFOE tile counts. Declared here so the
// physics layer and tests can read/inject them.
// ============================================================================

extern uint8_t* g_tileInfo;
extern int g_tileInfoSize;
extern CA_GfxInfo g_gfxInfoE;

// Loaded-map globals (populated by LoadGAMEMAPS/LoadMap). Declared here so the
// physics layer and tests can read/inject them.
extern uint16_t* g_mapPlanes[CA_NUMMAPPLANES];
extern CA_MapHeader* g_mapHeaders[CA_NUMMAPS];
extern int g_mapOn;

// ============================================================================
// Resource Manager (main entry point)
// ============================================================================

class ResourceManager
{
public:
	static void Startup();
	static void Shutdown();
	
	// Chunk loading
	static CacheEntry* LoadChunk(int chunkNum);
	static CacheEntry* LoadChunkLocked(int chunkNum);
	static void UnlockChunk(int chunkNum);
	
	// Marking for cache management
	static void MarkGraphicsChunk(int chunkNum);
	static void MarkAudioChunk(int chunkNum);
	static void MarkMapChunk(int chunkNum);
	static void ClearAllMarks();
	static void CacheAllMarked(int priority = 0);
	
	// Episode resource files
	static bool LoadEpisodeIndex(const char* episodeFile);
	static bool LoadGAMEMAPS();
	static bool LoadEGAGRAPH();
	static bool LoadAUDIO();
	
	// Map loading
	static bool LoadMap(int mapNum);
	static bool LoadMapPlanes(int mapNum);
	
	// Map data access
	static uint16_t GetMapWidth();
	static uint16_t GetMapHeight();
	static uint16_t* GetMapPlane(int plane);
	static uint16_t GetTileAt(int x, int y, int plane);
	static void SetTileAt(int x, int y, int plane, uint16_t value);
	static const char* GetMapName(int mapNum);
	
	// EGAGRAPH access
	static const CA_GfxInfo* GetGfxInfo();
	static void* GetGraphChunk(int chunkNum);
	static void* GetGraphChunkLocked(int chunkNum);
	static void UnlockGraphChunk(int chunkNum);
	static int GetGraphChunkExpLength(int chunkNum);
	static int GetGraphChunkCompLength(int chunkNum);
	static int GetGraphChunkStart(int chunkNum);
	
	// AUDIO access
	static const CA_AudioInfo* GetAudioInfo();
	static void* GetAudioChunk(int chunkNum);
	static void* GetAudioChunkLocked(int chunkNum);
	static void UnlockAudioChunk(int chunkNum);
	static int GetAudioChunkExpLength(int chunkNum);
	static int GetAudioChunkCompLength(int chunkNum);
	static int GetAudioChunkStart(int chunkNum);
	
	// Palette
	static void SetDefaultPalette();
	static void LoadPalette(int paletteChunk);
	
	// Tile info
	static uint8_t* GetTileInfo();
	static bool LoadTileInfo();
	
	// Tile info accessors (OmniSpeak id_ti.c TILEINFO layout)
	static uint8_t TileForeTop(uint16_t tile);
	static uint8_t TileForeBottom(uint16_t tile);
	static uint8_t TileForeLeft(uint16_t tile);
	static uint8_t TileForeRight(uint16_t tile);
	static uint8_t TileForeMisc(uint16_t tile);
	static uint8_t TileBackAnimTime(uint16_t tile);
	static int8_t TileBackAnimTile(uint16_t tile);
	static int8_t TileForeAnimTile(uint16_t tile);
	static uint8_t TileForeAnimTime(uint16_t tile);
	
	// Load GFXINFOE.<ext> (tile/sprite counts used by the TILEINFO layout)
	static bool LoadGfxInfo();
	
private:
	static bool s_initialized;
};

// Helper macro for chunk numbers (matching OmniSpeak)
#define CK_CHUNKNUM(chunk) static_cast<int>(chunk)
#define CK_FILENAME(base, ext) (FS_PathManager::GetGamePath() + "/" + base + "." + g_currentEpisode->ext)
#define CK_CHUNKID(chunk) chunk
#define CK_INT(name, defaultVal) (defaultVal) // Will be replaced by variable lookup
#define CK_INTELEMENT(arr, i) (arr[i])
#define CK_STRING(name) (name)
#define CK_STRINGELEMENT(arr, i) (arr[i])

// ============================================================================
// Chunk Number Constants (matching OmniSpeak EGAGRAPH chunks)
// ============================================================================

enum class EGAGraphChunk : uint16_t
{
	FON_MAINFONT = 0,      // Main font
	FON_WATCHFONT = 1,     // Watch font
	FON_BONUSFONT = 2,     // Bonus font
	// ... other chunks as needed
};

// Map planes: 0=background, 1=foreground, 2=info, 3=collision
enum MapPlane : uint8_t
{
	MAP_PLANE_BACKGROUND = 0,
	MAP_PLANE_FOREGROUND = 1,
	MAP_PLANE_INFO = 2,
	MAP_PLANE_COLLISION = 3,
};

END_UPP_NAMESPACE

#endif // _Platformer2D_Resource_Resource_h_