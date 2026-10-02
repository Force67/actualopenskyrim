#pragma once

#include "BSCore/BSSpinLock.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace BSStringPool
{
	class IterationCallback
	{
	public:
		virtual void operator()(unsigned short auiCRC, const wchar_t* apString) = 0;
		virtual void operator()(unsigned short auiCRC, const char* apString) = 0;
		virtual ~IterationCallback();
	};
	static_assert(sizeof(IterationCallback) == 8);

	struct Entry
	{
		Entry* pNext;
		unsigned int uiFlags;
		unsigned int uiReserved;
		unsigned int uiLength;
		unsigned int uiPadding;
	};
	static_assert(sizeof(Entry) == 0x18);
	static_assert(offsetof(Entry, uiFlags) == 0x8);
	static_assert(offsetof(Entry, uiLength) == 0x10);

	struct BucketTable
	{
		static BucketTable& GetSingleton();

		Entry* pBuckets[0x10000];
		BSSpinLock kLocks[32];
		bool bInitialized;
	};
	static_assert(sizeof(BucketTable) == 0x80108);
	static_assert(offsetof(BucketTable, kLocks) == 0x80000);
	static_assert(offsetof(BucketTable, bInitialized) == 0x80100);

	extern unsigned char cBucketTableS[sizeof(BucketTable)];
	extern int iBucketTableInitS;
	void DestroyBucketTable();

	extern thread_local constinit unsigned char cDelimiterPrefixST[104];
	extern thread_local constinit unsigned char cDelimiterLookupST[140];
	extern const std::array<uint16_t, 256> CRCTable;
	unsigned int GenerateCRC(unsigned int& arCRC, char*& arString, char*& arNext, char* apSource, const char* apDelimiters);
	void GetEntry(Entry*& arEntry, const char* apString, unsigned int auiCRC, unsigned int auiLength);
	void GetEntry(Entry*& arEntry, const wchar_t* apString, unsigned int auiCRC, unsigned int auiLength);
	Entry* AllocateEntry(unsigned int auiLength, unsigned int auiBytes);
	void FreeEntry(Entry* apEntry);
	void CreateEntry(Entry*& arEntry, const void* apString, unsigned int auiCRC, unsigned int auiLength, unsigned int auiBytes, bool abWide);
	void CreateEntryNoLink(Entry*& arEntry, const void* apString, unsigned int auiCRC, unsigned int auiLength, unsigned int auiBytes, bool abWide);
	void KillSDM();
	void Iterate(IterationCallback& arCallback);
	void Release(const char*& arString);
	void Release(const wchar_t*& arString);
}
