#pragma once

#include <cstdint>

namespace BSCRC32
{
	enum ForceCaseMode : int32_t
	{
		FORCE_CASE_NONE = 0,
		FORCE_CASE_LOWER = 1,
		FORCE_CASE_UPPER = 2,
	};

	void GetCRCTable(const unsigned int*& arTable);

	// Null terminated. Returns the number of characters hashed.
	unsigned int GenerateCRC(unsigned int& arCRC, const char* apString, ForceCaseMode aeCaseMode);
	// Stops at auiLength characters or the terminator.
	unsigned int GenerateCRC(unsigned int& arCRC, const char* apString, unsigned int auiLength, ForceCaseMode aeCaseMode);
	// Every character is remapped through apTranslation before hashing.
	unsigned int GenerateCRC(unsigned int& arCRC, const char* apString, unsigned int auiLength, const char* apTranslation);
	void GenerateCRC(unsigned int& arCRC, const void* apData, unsigned int auiSize);
	void GenerateCRC(unsigned int& arCRC, unsigned int auiValue);
	void GenerateCRC(unsigned int& arCRC, uint64_t auiValue);
}
