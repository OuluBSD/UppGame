#include "Decompress.h"
#include <algorithm>

NAMESPACE_UPP

// ============================================================================
// HuffmanDecoder Implementation
// ============================================================================

bool HuffmanDecoder::BuildTree(const uint8_t* data, size_t size)
{
	// EGAHEAD/AUDIOHHD format: 256 16-bit values (leaf nodes), then internal nodes
	// This is a simplified version - real implementation parses the exact format
	nodes.Clear();
	nodes.Reserve(512);
	
	// Read leaf values (256 uint16)
	if (size < 512) return false;
	
	for (int i = 0; i < 256; ++i)
	{
		HuffmanNode node;
		node.left = -1;
		node.right = -1;
		node.value = *(const uint16_t*)(data + i * 2);
		nodes.Add(node);
	}
	
	// Build internal nodes from remaining data
	// Real format has specific tree structure - simplified here
	rootIndex = 0; // Placeholder
	
	return true;
}

bool HuffmanDecoder::BuildFromEGAHEAD(const uint8_t* data, size_t size, HuffmanDecoder& outDecoder)
{
	return outDecoder.BuildTree(data, size);
}

bool HuffmanDecoder::BuildFromAUDIOHHD(const uint8_t* data, size_t size, HuffmanDecoder& outDecoder)
{
	return outDecoder.BuildTree(data, size);
}

int HuffmanDecoder::DecodeSymbol(const uint8_t*& input, const uint8_t* inputEnd) const
{
	// Bit-stream decoding
	// Simplified - real implementation reads bits and traverses tree
	if (input >= inputEnd) return -1;
	return *input++;
}

DecompressInfo HuffmanDecoder::Decode(const uint8_t* input, size_t inputSize, uint8_t* output, size_t outputSize) const
{
	DecompressInfo info;
	const uint8_t* inputEnd = input + inputSize;
	uint8_t* outputEnd = output + outputSize;
	
	while (input < inputEnd && output < outputEnd)
	{
		int sym = DecodeSymbol(input, inputEnd);
		if (sym < 0) break;
		*output++ = (uint8_t)sym;
	}
	
	info.bytesRead = input - (inputEnd - inputSize);
	info.bytesWritten = output - (outputEnd - outputSize);
	info.result = (input >= inputEnd || output >= outputEnd) ? DecompressResult::OK : DecompressResult::CORRUPTED_DATA;
	return info;
}

DecompressInfo HuffmanDecoder::DecodeStream(Stream& input, Stream& output, size_t maxBytes) const
{
	DecompressInfo info;
	// Streaming implementation would read chunks
	info.result = DecompressResult::UNSUPPORTED_FORMAT;
	return info;
}

// ============================================================================
// CarmackDecoder Implementation
// ============================================================================

DecompressInfo CarmackDecoder::Decode(const uint8_t* input, size_t inputSize, uint16_t* output, size_t outputWords, size_t* bytesRead)
{
	DecompressInfo info;
	const uint8_t* inputEnd = input + inputSize;
	uint16_t* outputEnd = output + outputWords;
	const uint8_t* startInput = input;
	
	while (input < inputEnd && output < outputEnd)
	{
		uint8_t tag = *input++;
		DecompressResult res = ProcessTag(tag, input, inputEnd, output, outputEnd);
		if (res != DecompressResult::OK)
		{
			info.result = res;
			break;
		}
	}
	
	if (bytesRead) *bytesRead = input - startInput;
	info.bytesRead = input - startInput;
	info.bytesWritten = output - (outputEnd - outputWords);
	return info;
}

DecompressResult CarmackDecoder::ProcessTag(uint8_t tag, const uint8_t*& input, const uint8_t* inputEnd, uint16_t*& output, uint16_t* outputEnd)
{
	// Simplified Carmack decoding
	// Real format: tag bits 0-6 = count, bit 7 = repeat(1)/copy(0)
	// For now, just copy
	if (input >= inputEnd) return DecompressResult::INVALID_INPUT;
	if (output >= outputEnd) return DecompressResult::OUTPUT_BUFFER_TOO_SMALL;
	
	*output++ = *input++;
	return DecompressResult::OK;
}

DecompressInfo CarmackDecoder::DecodeInPlace(uint8_t* buffer, size_t bufferSize, size_t expectedWords)
{
	DecompressInfo info;
	info.result = DecompressResult::UNSUPPORTED_FORMAT;
	return info;
}

// ============================================================================
// RLEWDecoder Implementation
// ============================================================================

DecompressInfo RLEWDecoder::Decode(const uint16_t* input, size_t inputWords, uint16_t* output, size_t outputWords, size_t* wordsRead)
{
	DecompressInfo info;
	const uint16_t* inputEnd = input + inputWords;
	uint16_t* outputEnd = output + outputWords;
	const uint16_t* startInput = input;
	
	while (input < inputEnd && output < outputEnd)
	{
		uint16_t word = *input++;
		if (word == RLEW_TAG)
		{
			if (input + 1 >= inputEnd) { info.result = DecompressResult::INVALID_INPUT; break; }
			uint16_t count = *input++;
			uint16_t value = *input++;
			if (output + count > outputEnd) { info.result = DecompressResult::OUTPUT_BUFFER_TOO_SMALL; break; }
			for (uint16_t i = 0; i < count; ++i) *output++ = value;
		}
		else
		{
			*output++ = word;
		}
	}
	
	if (wordsRead) *wordsRead = input - startInput;
	info.bytesRead = (input - startInput) * 2;
	info.bytesWritten = (output - (outputEnd - outputWords)) * 2;
	info.result = (input <= inputEnd && output <= outputEnd) ? DecompressResult::OK : DecompressResult::CORRUPTED_DATA;
	return info;
}

DecompressInfo RLEWDecoder::DecodeBytes(const uint8_t* input, size_t inputSize, uint16_t* output, size_t outputWords, size_t* bytesRead)
{
	// Convert byte stream to word stream and decode
	const uint16_t* words = (const uint16_t*)input;
	size_t wordCount = inputSize / 2;
	return Decode(words, wordCount, output, outputWords, bytesRead);
}

// ============================================================================
// LZWDecoder Implementation
// ============================================================================

void LZWDecoder::LZWState::Reset(int maxCodeSize)
{
	prefix.SetCount(MAX_CODES);
	suffix.SetCount(MAX_CODES);
	stack.SetCount(MAX_CODES);
	codeSize = 9; // Start with 9-bit codes
	clearCode = CLEAR_CODE;
	endCode = END_CODE;
	nextCode = FIRST_CODE;
	oldCode = -1;
	
	// Initialize single-byte codes
	for (int i = 0; i < 256; ++i)
	{
		prefix[i] = 0xFFFF;
		suffix[i] = (uint8_t)i;
	}
}

DecompressInfo LZWDecoder::Decode(const uint8_t* input, size_t inputSize, uint8_t* output, size_t outputSize, int maxCodeSize, size_t* bytesRead)
{
	DecompressInfo info;
	LZWState state;
	state.Reset(maxCodeSize);
	
	// Bit stream reading
	size_t bitPos = 0;
	auto readCode = [&](int bits) -> int {
		if (bitPos + bits > inputSize * 8) return -1;
		int bytePos = bitPos >> 3;
		int bitOffset = bitPos & 7;
		int val = 0;
		for (int i = 0; i < bits; ++i)
		{
			if (bytePos >= inputSize) return -1;
			val |= ((input[bytePos] >> bitOffset) & 1) << i;
			bitOffset++;
			if (bitOffset == 8) { bitOffset = 0; bytePos++; }
		}
		bitPos += bits;
		return val;
	};
	
	uint8_t* outputEnd = output + outputSize;
	uint8_t* startOutput = output;
	
	while (output < outputEnd)
	{
		int code = readCode(state.codeSize);
		if (code < 0) { info.result = DecompressResult::CORRUPTED_DATA; break; }
		if (code == state.endCode) break;
		if (code == state.clearCode)
		{
			state.Reset(maxCodeSize);
			continue;
		}
		
		// Decode code to output
		if (code < 256)
		{
			*output++ = (uint8_t)code;
		}
		else if (code < state.nextCode)
		{
			// Existing code - decode recursively
			int idx = code;
			uint8_t* stackPtr = state.stack.Begin();
			while (idx >= 256)
			{
				*stackPtr++ = state.suffix[idx];
				idx = state.prefix[idx];
			}
			*stackPtr++ = (uint8_t)idx;
			while (stackPtr > state.stack.Begin() && output < outputEnd)
				*output++ = *--stackPtr;
		}
		else if (code == state.nextCode && state.oldCode >= 0)
		{
			// Special case: KwKwK
			int idx = state.oldCode;
			uint8_t* stackPtr = state.stack.Begin();
			while (idx >= 256)
			{
				*stackPtr++ = state.suffix[idx];
				idx = state.prefix[idx];
			}
			*stackPtr++ = (uint8_t)idx;
			while (stackPtr > state.stack.Begin() && output < outputEnd)
				*output++ = *--stackPtr;
			*output++ = (uint8_t)idx;
		}
		else
		{
			info.result = DecompressResult::CORRUPTED_DATA;
			break;
		}
		
		// Add new code
		if (state.oldCode >= 0 && state.nextCode < MAX_CODES)
		{
			state.prefix[state.nextCode] = (uint16_t)state.oldCode;
			state.suffix[state.nextCode] = (output > startOutput) ? *(output - 1) : 0;
			state.nextCode++;
			
			// Increase code size if needed
			if (state.nextCode == (1 << state.codeSize) && state.codeSize < maxCodeSize)
				state.codeSize++;
		}
		state.oldCode = code;
	}
	
	if (bytesRead) *bytesRead = (bitPos + 7) / 8;
	info.bytesRead = (bitPos + 7) / 8;
	info.bytesWritten = output - startOutput;
	if (info.result == DecompressResult::OK) info.result = DecompressResult::OK;
	return info;
}

DecompressInfo LZWDecoder::DecodeStream(Stream& input, Stream& output, int maxCodeSize, size_t maxBytes)
{
	DecompressInfo info;
	info.result = DecompressResult::UNSUPPORTED_FORMAT;
	return info;
}

// ============================================================================
// High-Level Dispatch
// ============================================================================

DecompressInfo DecompressChunk(const uint8_t* input, size_t inputSize, void* output, size_t outputSize, CompressionMethod method, const HuffmanDecoder* huffman)
{
	DecompressInfo info;
	
	switch (method)
	{
	case CompressionMethod::NONE:
		if (inputSize <= outputSize)
		{
			memcpy(output, input, inputSize);
			info.bytesRead = inputSize;
			info.bytesWritten = inputSize;
			info.result = DecompressResult::OK;
		}
		else
		{
			info.result = DecompressResult::OUTPUT_BUFFER_TOO_SMALL;
		}
		break;
		
	case CompressionMethod::HUFFMAN:
		if (huffman)
			info = huffman->Decode(input, inputSize, (uint8_t*)output, outputSize);
		else
			info.result = DecompressResult::INVALID_INPUT;
		break;
		
	case CompressionMethod::CARMACK:
		info = CarmackDecoder::Decode(input, inputSize, (uint16_t*)output, outputSize / 2);
		break;
		
	case CompressionMethod::RLEW:
		info = RLEWDecoder::DecodeBytes(input, inputSize, (uint16_t*)output, outputSize / 2);
		break;
		
	case CompressionMethod::LZW:
		info = LZWDecoder::Decode(input, inputSize, (uint8_t*)output, outputSize);
		break;
		
	default:
		info.result = DecompressResult::UNSUPPORTED_FORMAT;
		break;
	}
	
	return info;
}

END_UPP_NAMESPACE