#include "BSSystem/BSStringPool.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/BSMemoryutility.h"
#include "BSCore/BSAutoLock.h"

#include <cstring>
#include <cwchar>

BSStringPool::IterationCallback::~IterationCallback()
{
}

void BSStringPool::Iterate(IterationCallback& arCallback)
{
	BucketTable& kTable = BucketTable::GetSingleton();
	for (unsigned int uiBucket = 0; uiBucket < 0x10000; ++uiBucket)
	{
		BSAutoLock<BSSpinLock> kAutoLock(kTable.kLocks[uiBucket & 31]);
		for (Entry* pEntry = kTable.pBuckets[uiBucket]; pEntry; pEntry = pEntry->pNext)
		{
			if (pEntry->uiFlags & 0x8000)
				arCallback(static_cast<unsigned short>(uiBucket), reinterpret_cast<const wchar_t*>(pEntry + 1));
			else
				arCallback(static_cast<unsigned short>(uiBucket), reinterpret_cast<const char*>(pEntry + 1));
		}
	}
}

namespace
{
	template <class Char, class Compare>
	void GetStringEntry(BSStringPool::Entry*& arEntry, const Char* apString, unsigned int auiCRC, unsigned int auiLength, bool abWide, Compare aCompare)
	{
		BSSpinLock& kLock = BSStringPool::BucketTable::GetSingleton().kLocks[auiCRC & 31];
		BSAutoLock<BSSpinLock> kAutoLock(kLock);
		BSStringPool::Entry* pEntry = BSStringPool::BucketTable::GetSingleton().pBuckets[auiCRC];
		while (pEntry)
		{
			if (static_cast<bool>(pEntry->uiFlags & 0x8000) == abWide && !aCompare(reinterpret_cast<const Char*>(pEntry + 1), apString))
				break;
			pEntry = pEntry->pNext;
		}
		if (pEntry)
		{
			unsigned int uiFlags = pEntry->uiFlags;
			while ((uiFlags & 0x7FFF) != 0x7FFF)
			{
				const unsigned int uiPrevious = InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&pEntry->uiFlags), uiFlags + 1, uiFlags);
				if (uiPrevious == uiFlags)
					break;
				uiFlags = uiPrevious;
			}
		}
		else
		{
			const unsigned int uiBytes = abWide ? auiLength * 2 + 2 : auiLength + 1;
			BSStringPool::CreateEntry(pEntry, apString, auiCRC, auiLength, uiBytes, abWide);
		}
		arEntry = pEntry;
	}

	template <class Char>
	void ReleaseString(const Char*& arString)
	{
		if (arString)
		{
			auto* pEntry = reinterpret_cast<BSStringPool::Entry*>(const_cast<Char*>(arString)) - 1;
			BSStringPool::BucketTable& kTable = BSStringPool::BucketTable::GetSingleton();
			if (kTable.bInitialized)
			{
				unsigned int uiFlags = pEntry->uiFlags;
				for (;;)
				{
					bool bReleased;
					if ((uiFlags & 0x7FFF) == 1)
					{
						const unsigned int uiBucket = uiFlags >> 16;
						BSSpinLock& kLock = kTable.kLocks[uiBucket & 31];
						kLock.Lock();
						const unsigned int uiCurrent = pEntry->uiFlags;
						if ((uiCurrent & 0x7FFF) == 1)
						{
							BSStringPool::Entry** ppEntry = &kTable.pBuckets[uiBucket];
							while (*ppEntry)
							{
								if (*ppEntry == pEntry)
								{
									*ppEntry = pEntry->pNext;
									BSStringPool::FreeEntry(pEntry);
									break;
								}
								ppEntry = &(*ppEntry)->pNext;
							}
							bReleased = true;
						}
						else
						{
							uiFlags = InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&pEntry->uiFlags), uiCurrent - 1, uiCurrent);
							bReleased = uiFlags == uiCurrent;
						}
						kLock.Unlock();
					}
					else
					{
						if ((uiFlags & 0x7FFF) == 0x7FFF)
							break;
						const unsigned int uiPrevious = InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&pEntry->uiFlags), uiFlags - 1, uiFlags);
						bReleased = uiPrevious == uiFlags;
						uiFlags = uiPrevious;
					}
					if (bReleased)
						break;
				}
			}
		}
		arString = nullptr;
	}
}

void BSStringPool::GetEntry(Entry*& arEntry, const char* apString, unsigned int auiCRC, unsigned int auiLength)
{
	GetStringEntry(arEntry, apString, auiCRC, auiLength, false, _stricmp);
}

void BSStringPool::GetEntry(Entry*& arEntry, const wchar_t* apString, unsigned int auiCRC, unsigned int auiLength)
{
	GetStringEntry(arEntry, apString, auiCRC, auiLength, true, _wcsicmp);
}

void BSStringPool::Release(const char*& arString)
{
	ReleaseString(arString);
}

void BSStringPool::Release(const wchar_t*& arString)
{
	ReleaseString(arString);
}

BSStringPool::Entry* BSStringPool::AllocateEntry(unsigned int auiLength, unsigned int auiBytes)
{
	const unsigned int uiSize = auiBytes + 24;
	auto* pEntry = static_cast<Entry*>(MemoryManager::Instance().Allocate(uiSize, 0, false));
	pEntry->uiLength = auiLength;
	return pEntry;
}

void BSStringPool::FreeEntry(Entry* apEntry)
{
	(void)static_cast<volatile Entry*>(apEntry)->uiFlags;
	MemoryManager::Instance().Deallocate(apEntry, false);
}

void BSStringPool::CreateEntry(Entry*& arEntry, const void* apString, unsigned int auiCRC, unsigned int auiLength, unsigned int auiBytes, bool abWide)
{
	BucketTable& kTable = BucketTable::GetSingleton();
	Entry* pEntry = AllocateEntry(auiLength, auiBytes);
	BSmemcpy(pEntry + 1, auiBytes, apString, auiBytes);
	pEntry->uiFlags = ((static_cast<unsigned int>(abWide) | (auiCRC * 2)) << 15) | 1;
	pEntry->pNext = kTable.pBuckets[auiCRC];
	kTable.pBuckets[auiCRC] = pEntry;
	arEntry = pEntry;
}

void BSStringPool::CreateEntryNoLink(Entry*& arEntry, const void* apString, unsigned int auiCRC, unsigned int auiLength, unsigned int auiBytes, bool abWide)
{
	Entry* pEntry = AllocateEntry(auiLength, auiBytes);
	BSmemcpy(pEntry + 1, auiBytes, apString, auiBytes);
	pEntry->uiFlags = ((static_cast<unsigned int>(abWide) | (auiCRC * 2)) << 15) | 1;
	arEntry = pEntry;
}

void BSStringPool::KillSDM()
{
	BucketTable& kTable = BucketTable::GetSingleton();
	if (!kTable.bInitialized)
		return;
	for (Entry*& pHead : kTable.pBuckets)
	{
		Entry* pEntry = pHead;
		while (pEntry)
		{
			Entry* pNext = pEntry->pNext;
			MemoryManager::Instance().Deallocate(pEntry, false);
			pEntry = pNext;
		}
		pHead = nullptr;
	}
	kTable.bInitialized = false;
}
