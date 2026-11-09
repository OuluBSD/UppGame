#ifndef _Platformer2D_Resource_Decompress_h_
#define _Platformer2D_Resource_Decompress_h_

#include <game/Platformer2D/Platformer2D.h>
#include <game/Platformer2D/Resource/Resource.h>

NAMESPACE_UPP

// ============================================================================
// Decompression Result
// ============================================================================

enum class DecompressResult : uint8_t
{
	OK = 0,
	INVALID_INPUT = 1,
	OUTPUT_BUFFER_TOO_SMALL = 2,
	CORRUPTED_DATA = 3,
	UNSUPPORTED_FORMAT = 4,
};

struct DecompressInfo
{
	DecompressResult result = DecompressResult::OK;
	size_t bytesRead = 0;
	size_t bytesWritten = 0;
	String errorMessage;
};

// ============================================================================
// Huffman Decoder (EGAGRAPH, AUDIO)
// ============================================================================

struct HuffmanNode
{
	int16_t left;  // -1 = leaf, else index
	int16_t right; // -1 = leaf, else index
	uint16_t value; // Leaf value
};

class HuffmanDecoder
{
public:
	// Build decoder from tree stored in EGAHEAD/AUDIOHHD format
	static bool BuildFromEGAHEAD(const uint8_t* data, size_t size, HuffmanDecoder& outDecoder);
	static bool BuildFromAUDIOHHD(const uint8_t* data, size_t size, HuffmanDecoder& outDecoder);
	
	// Decode compressed stream to output buffer
	DecompressInfo Decode(const uint8_t* input, size_t inputSize, uint8_t* output, size_t outputSize) const;
	
	// Streaming decode (for large chunks)
	DecompressInfo DecodeStream(Stream& input, Stream& output, size_t maxBytes = SIZE_MAX) const;
	
private:
	Vector<HuffmanNode> nodes;
	int rootIndex = -1;
	
	bool BuildTree(const uint8_t* data, size_t size);
	int DecodeSymbol(const uint8_t*& input, const uint8_t* inputEnd) const;
};

// ============================================================================
// Carmack Decoder (GAMEMAPS tile data)
// ============================================================================

class CarmackDecoder
{
public:
	// Decode Carmack-compressed plane data
	// Format: [tag byte][data...] where tag high bit = repeat count, low bit = copy count
	static DecompressInfo Decode(const uint8_t* input, size_t inputSize, uint16_t* output, size_t outputWords, size_t* bytesRead = nullptr);
	
	// In-place decode for known size
	static DecompressInfo DecodeInPlace(uint8_t* buffer, size_t bufferSize, size_t expectedWords);
	
private:
	static DecompressResult ProcessTag(uint8_t tag, const uint8_t*& input, const uint8_t* inputEnd, uint16_t*& output, uint16_t* outputEnd);
};

// ============================================================================
// RLEW Decoder (GAMEMAPS RLE words)
// ============================================================================

class RLEWDecoder
{
public:
	// Decode RLEW-compressed data
	// Format: [word] - if word == 0xFEFE, next word = count, next word = value to repeat
	static DecompressInfo Decode(const uint16_t* input, size_t inputWords, uint16_t* output, size_t outputWords, size_t* wordsRead = nullptr);
	
	static DecompressInfo DecodeBytes(const uint8_t* input, size_t inputSize, uint16_t* output, size_t outputWords, size_t* bytesRead = nullptr);
	
private:
	static const uint16_t RLEW_TAG = 0xFEFE;
};

// ============================================================================
// LZW Decoder (EGAGRAPH sprite data)
// ============================================================================

class LZWDecoder
{
public:
	// Decode LZW-compressed data (GIF-style, variable width codes)
	// maxCodeSize: typically 12 bits (4096 codes)
	static DecompressInfo Decode(const uint8_t* input, size_t inputSize, uint8_t* output, size_t outputSize, int maxCodeSize = 12, size_t* bytesRead = nullptr);
	
	// Streaming decode
	static DecompressInfo DecodeStream(Stream& input, Stream& output, int maxCodeSize = 12, size_t maxBytes = SIZE_MAX);
	
private:
	static const int MAX_CODES = 4096;
	static const int CLEAR_CODE = 256;
	static const int END_CODE = 257;
	static const int FIRST_CODE = 258;
	
	struct LZWState
	{
		Vector<uint16_t> prefix;
		Vector<uint8_t> suffix;
		Vector<uint8_t> stack;
		int nextCode;
		int codeSize;
		int clearCode;
		int endCode;
		int oldCode;
		
		LZWState() : nextCode(0), codeSize(0), clearCode(0), endCode(0), oldCode(-1) {}
		void Reset(int maxCodeSize);
	};
};

// ============================================================================
// High-Level Decompression Dispatch
// ============================================================================

enum class CompressionMethod : uint8_t
{
	NONE = 0,
	HUFFMAN = 1,
	CARMACK = 2,
	RLEW = 3,
	LZW = 4,
};

DecompressInfo DecompressChunk(const uint8_t* input, size_t inputSize, void* output, size_t outputSize, CompressionMethod method, const HuffmanDecoder* huffman = nullptr);

END_UPP_NAMESPACE

#endif // _Platformer2D_Resource_Decompress_h_