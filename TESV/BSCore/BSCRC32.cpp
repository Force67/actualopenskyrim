#include "BSCore/BSCRC32.h"

#include <array>

namespace
{
	constexpr std::array<unsigned int, 256> MakeCRCTable()
	{
		std::array<unsigned int, 256> table{};
		for (unsigned int i = 0; i < 256; ++i)
		{
			unsigned int crc = i;
			for (int bit = 0; bit < 8; ++bit)
				crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320u : crc >> 1;
			table[i] = crc;
		}
		return table;
	}

	constexpr std::array<unsigned int, 256> CRCTable = MakeCRCTable();

	// The original calls the MSVC CRT in the "C" locale, which only folds ASCII.
	int AsciiToLower(int c) { return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c; }
	int AsciiToUpper(int c) { return (c >= 'a' && c <= 'z') ? c - ('a' - 'A') : c; }

	unsigned int Step(unsigned int crc, int c)
	{
		return CRCTable[(crc ^ c) & 0xFF] ^ (crc >> 8);
	}

	int Fold(int c, BSCRC32::ForceCaseMode aeCaseMode)
	{
		switch (aeCaseMode)
		{
		case BSCRC32::FORCE_CASE_LOWER:
			return AsciiToLower(c);
		case BSCRC32::FORCE_CASE_UPPER:
			return AsciiToUpper(c);
		default:
			return c;
		}
	}
}

namespace BSCRC32
{
	void GetCRCTable(const unsigned int*& arTable)
	{
		arTable = CRCTable.data();
	}

	unsigned int GenerateCRC(unsigned int& arCRC, const char* apString, ForceCaseMode aeCaseMode)
	{
		unsigned int crc = 0;
		unsigned int count = 0;
		if (apString)
		{
			for (int c; (c = *apString++) != 0; ++count)
				crc = Step(crc, Fold(c, aeCaseMode));
		}
		arCRC = crc;
		return count;
	}

	unsigned int GenerateCRC(unsigned int& arCRC, const char* apString, unsigned int auiLength, ForceCaseMode aeCaseMode)
	{
		unsigned int crc = 0;
		unsigned int count = 0;
		for (int c; auiLength-- && (c = *apString++) != 0; ++count)
			crc = Step(crc, Fold(c, aeCaseMode));
		arCRC = crc;
		return count;
	}

	unsigned int GenerateCRC(unsigned int& arCRC, const char* apString, unsigned int auiLength, const char* apTranslation)
	{
		unsigned int crc = 0;
		unsigned int count = 0;
		// Indexed with a signed char, matching the original.
		for (int c; auiLength-- && (c = apTranslation[static_cast<signed char>(*apString++)]) != 0; ++count)
			crc = Step(crc, c);
		arCRC = crc;
		return count;
	}

	void GenerateCRC(unsigned int& arCRC, const void* apData, unsigned int auiSize)
	{
		const auto* data = static_cast<const unsigned char*>(apData);
		unsigned int crc = 0;
		while (auiSize--)
			crc = Step(crc, *data++);
		arCRC = crc;
	}

	void GenerateCRC(unsigned int& arCRC, unsigned int auiValue)
	{
		unsigned int crc = 0;
		for (int i = 0; i < 4; ++i)
			crc = Step(crc, (auiValue >> (i * 8)) & 0xFF);
		arCRC = crc;
	}

	void GenerateCRC(unsigned int& arCRC, uint64_t auiValue)
	{
		unsigned int crc = 0;
		for (int i = 0; i < 8; ++i)
			crc = Step(crc, static_cast<unsigned int>(auiValue >> (i * 8)) & 0xFF);
		arCRC = crc;
	}
}
