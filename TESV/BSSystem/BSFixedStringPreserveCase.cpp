#include "BSSystem/BSFixedString.h"
#include "BSSystem/BSStringPool.h"
#include "BSCore/BSCore.h"
#include "BSCore/BSAutoLock.h"

#include <atomic>
#include <cstdlib>
#include <cstring>
#include <cwchar>

const char* BSFixedString::pEmptyStringS;
int BSFixedString::iEmptyStringInitS;
const wchar_t* BSFixedStringW::pEmptyStringS;
int BSFixedStringW::iEmptyStringInitS;

namespace
{
	void AcquireEntry(BSStringPool::Entry* apEntry)
	{
		unsigned int uiFlags = apEntry->uiFlags;
		while ((uiFlags & 0x7FFF) != 0x7FFF)
		{
			const unsigned int uiPrevious = InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&apEntry->uiFlags), uiFlags + 1, uiFlags);
			if (uiPrevious == uiFlags)
				break;
			uiFlags = uiPrevious;
		}
	}

	template <class Char, class String>
	const Char* EmptyString()
	{
		if (std::atomic_ref<int>(String::iEmptyStringInitS).load(std::memory_order_acquire) > BSCore::iThreadInitEpochS)
		{
			BSCore::InitThreadHeader(&String::iEmptyStringInitS);
			if (String::iEmptyStringInitS == -1)
			{
				try
				{
					String::pEmptyStringS = nullptr;
					BSStringPool::Entry* pEntry = nullptr;
					const Char cEmpty = 0;
					BSStringPool::GetEntry(pEntry, &cEmpty, 0, 0);
					String::pEmptyStringS = reinterpret_cast<const Char*>(reinterpret_cast<uintptr_t>(pEntry) + sizeof(BSStringPool::Entry));
					std::atexit(String::DestroyEmptyString);
					BSCore::InitThreadFooter(&String::iEmptyStringInitS);
				}
				catch (...)
				{
					BSCore::InitThreadAbort(&String::iEmptyStringInitS);
					throw;
				}
			}
		}
		auto* pEntry = reinterpret_cast<BSStringPool::Entry*>(const_cast<Char*>(String::pEmptyStringS)) - 1;
		AcquireEntry(pEntry);
		return String::pEmptyStringS;
	}

	template <class Char>
	bool MatchesExact(BSStringPool::Entry* apEntry, const Char* apString)
	{
		const Char* pChar = reinterpret_cast<const Char*>(apEntry + 1);
		while (*pChar == *apString && *apString)
		{
			++pChar;
			++apString;
		}
		return *pChar == *apString;
	}

	template <class Char>
	BSStringPool::Entry* FindExact(BSStringPool::Entry* apEntry, const Char* apString)
	{
		for (; apEntry; apEntry = apEntry->pNext)
			if (static_cast<bool>(apEntry->uiFlags & 0x8000) == (sizeof(Char) != 1) && MatchesExact(apEntry, apString))
				break;
		return apEntry;
	}

	template <class Char, class String, class Compare>
	void PreserveCase(String& arString, const Char* apString, Compare aCompare)
	{
		const Char* pPrevious = arString.pString;
		const Char* pString = nullptr;
		if (apString && !*apString)
			pString = EmptyString<Char, String>();
		else if (apString)
		{
			unsigned int uiCRC = 0;
			unsigned int uiFoldedCRC = 0;
			unsigned int uiLength = 0;
			for (const Char* pChar = apString; *pChar; ++pChar)
			{
				const unsigned int uiChar = static_cast<uint16_t>(*pChar);
				uiCRC = BSStringPool::CRCTable[(uiCRC ^ uiChar) & 0xFF] ^ (uiCRC >> 8);
				if constexpr (sizeof(Char) != 1)
					uiCRC = BSStringPool::CRCTable[(uiCRC ^ (uiChar >> 8)) & 0xFF] ^ (uiCRC >> 8);
				++uiLength;
			}
			for (const Char* pChar = apString; *pChar; ++pChar)
			{
				unsigned int uiChar = static_cast<uint16_t>(*pChar);
				if (uiChar >= 'a' && uiChar <= 'z')
					uiChar -= 'a' - 'A';
				uiFoldedCRC = BSStringPool::CRCTable[(uiFoldedCRC ^ uiChar) & 0xFF] ^ (uiFoldedCRC >> 8);
				if constexpr (sizeof(Char) != 1)
					uiFoldedCRC = BSStringPool::CRCTable[(uiFoldedCRC ^ (uiChar >> 8)) & 0xFF] ^ (uiFoldedCRC >> 8);
			}
			BSSpinLock& kLock = BSStringPool::BucketTable::GetSingleton().kLocks[uiFoldedCRC & 31];
			BSStringPool::Entry* pEntry = nullptr;
			const unsigned int uiBytes = sizeof(Char) == 1 ? uiLength + 1 : uiLength * 2 + 2;
			if (uiCRC == uiFoldedCRC)
			{
				BSAutoLock<BSSpinLock> kAutoLock(kLock);
				BSStringPool::Entry* pEquivalent = nullptr;
				for (pEntry = BSStringPool::BucketTable::GetSingleton().pBuckets[uiFoldedCRC]; pEntry; pEntry = pEntry->pNext)
				{
					if (static_cast<bool>(pEntry->uiFlags & 0x8000) != (sizeof(Char) != 1))
						continue;
					if (MatchesExact(pEntry, apString))
					{
						pEquivalent = nullptr;
						break;
					}
					if (!pEquivalent && !aCompare(reinterpret_cast<const Char*>(pEntry + 1), apString))
						pEquivalent = pEntry;
				}
				if (pEquivalent)
				{
					BSStringPool::CreateEntryNoLink(pEntry, apString, uiFoldedCRC, uiLength, uiBytes, sizeof(Char) != 1);
					pEntry->pNext = pEquivalent->pNext;
					pEquivalent->pNext = pEntry;
				}
				else if (pEntry)
					AcquireEntry(pEntry);
				else
					BSStringPool::CreateEntry(pEntry, apString, uiFoldedCRC, uiLength, uiBytes, sizeof(Char) != 1);
			}
			else
			{
				{
					BSAutoLock<BSSpinLock> kAutoLock(kLock);
					pEntry = FindExact(BSStringPool::BucketTable::GetSingleton().pBuckets[uiFoldedCRC], apString);
				}
				if (pEntry)
					AcquireEntry(pEntry);
				else
				{
					BSAutoLock<BSSpinLock> kAutoLock(BSStringPool::BucketTable::GetSingleton().kLocks[uiCRC & 31]);
					pEntry = FindExact(BSStringPool::BucketTable::GetSingleton().pBuckets[uiCRC], apString);
					if (pEntry)
						AcquireEntry(pEntry);
					else
						BSStringPool::CreateEntry(pEntry, apString, uiCRC, uiLength, uiBytes, sizeof(Char) != 1);
				}
			}
			pString = reinterpret_cast<const Char*>(reinterpret_cast<uintptr_t>(pEntry) + sizeof(BSStringPool::Entry));
		}
		arString.pString = pString;
		BSStringPool::Release(pPrevious);
	}
}

void BSFixedString::CreatePreserveCase(BSFixedString& arString, const char* apSource)
{
	PreserveCase(arString, apSource, _stricmp);
}

void BSFixedStringW::CreatePreserveCase(BSFixedStringW& arString, const wchar_t* apSource)
{
	PreserveCase(arString, apSource, _wcsicmp);
}

void BSFixedString::DestroyEmptyString()
{
	BSStringPool::Release(pEmptyStringS);
}

void BSFixedStringW::DestroyEmptyString()
{
	BSStringPool::Release(pEmptyStringS);
}
