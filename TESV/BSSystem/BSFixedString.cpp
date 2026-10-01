#include "BSSystem/BSFixedString.h"
#include "BSSystem/BSStringPool.h"

#include <windows.h>

#include <cstring>
#include <cwchar>

namespace
{
	constexpr std::array<uint16_t, 256> MakeCRCTable()
	{
		std::array<uint16_t, 256> kTable{};
		for (unsigned int i = 0; i < kTable.size(); ++i)
		{
			unsigned int uiCRC = i;
			for (unsigned int j = 0; j < 8; ++j)
				uiCRC = (uiCRC >> 1) ^ ((uiCRC & 1) ? 0xA001 : 0);
			kTable[i] = static_cast<uint16_t>(uiCRC);
		}
		return kTable;
	}

	template <class Char>
	const Char* Intern(const Char* apString)
	{
		if (!apString)
			return nullptr;
		unsigned int uiCRC = 0;
		unsigned int uiLength = 0;
		for (const Char* pChar = apString; *pChar; ++pChar)
		{
			unsigned int uiChar = static_cast<uint16_t>(*pChar);
			if (uiChar >= 'a' && uiChar <= 'z')
				uiChar -= 'a' - 'A';
			uiCRC = BSStringPool::CRCTable[(uiCRC ^ uiChar) & 0xFF] ^ (uiCRC >> 8);
			if constexpr (sizeof(Char) != 1)
				uiCRC = BSStringPool::CRCTable[(uiCRC ^ (uiChar >> 8)) & 0xFF] ^ (uiCRC >> 8);
			++uiLength;
		}
		BSStringPool::Entry* pEntry = nullptr;
		BSStringPool::GetEntry(pEntry, apString, uiCRC, uiLength);
		return reinterpret_cast<const Char*>(reinterpret_cast<uintptr_t>(pEntry) + sizeof(BSStringPool::Entry));
	}

	template <class Char>
	const Char* Acquire(const Char* apString)
	{
		if (apString)
		{
			auto* pFlags = reinterpret_cast<volatile LONG*>(reinterpret_cast<uintptr_t>(apString) - 16);
			unsigned int uiFlags = *pFlags;
			while ((uiFlags & 0x7FFF) != 0x7FFF)
			{
				const unsigned int uiPrevious = InterlockedCompareExchange(pFlags, uiFlags + 1, uiFlags);
				if (uiPrevious == uiFlags)
					break;
				uiFlags = uiPrevious;
			}
		}
		return apString;
	}
}

const std::array<uint16_t, 256> BSStringPool::CRCTable = MakeCRCTable();

char* BSFixedString::CreateDelimited(BSFixedString& arString, char* apSource, const char* apDelimiters)
{
	const char* pPrevious = arString.pString;
	char* pNext = nullptr;
	const char* pString = nullptr;
	if (apSource && *apSource)
	{
		BSStringPool::Entry* pEntry = nullptr;
		unsigned int uiCRC;
		char* pStart;
		const unsigned int uiLength = BSStringPool::GenerateCRC(uiCRC, pStart, pNext, apSource, apDelimiters);
		if (uiLength)
			BSStringPool::GetEntry(pEntry, pStart, uiCRC, uiLength);
		pString = reinterpret_cast<const char*>(reinterpret_cast<uintptr_t>(pEntry) + sizeof(BSStringPool::Entry));
	}
	arString.pString = pString;
	BSStringPool::Release(pPrevious);
	return pNext;
}

bool operator==(const BSFixedString& arFirst, const char* apSecond)
{
	if (!arFirst.pString)
		return !apSecond;
	return apSecond && !_stricmp(arFirst.pString, apSecond);
}

bool operator!=(const BSFixedString& arFirst, const char* apSecond)
{
	return !(arFirst == apSecond);
}

bool operator==(const char* apFirst, const BSFixedString& arSecond)
{
	return arSecond == apFirst;
}

bool operator!=(const char* apFirst, const BSFixedString& arSecond)
{
	return !(arSecond == apFirst);
}

bool operator==(const BSFixedStringW& arFirst, const wchar_t* apSecond)
{
	if (!arFirst.pString)
		return !apSecond;
	return apSecond && !_wcsicmp(arFirst.pString, apSecond);
}

bool operator!=(const BSFixedStringW& arFirst, const wchar_t* apSecond)
{
	return !(arFirst == apSecond);
}

bool operator==(const wchar_t* apFirst, const BSFixedStringW& arSecond)
{
	return arSecond == apFirst;
}

bool operator!=(const wchar_t* apFirst, const BSFixedStringW& arSecond)
{
	return !(arSecond == apFirst);
}

BSFixedString::BSFixedString(const char* apString) : pString(nullptr)
{
	pString = Intern(apString);
}

BSFixedString::BSFixedString(const BSFixedString& arOther) : pString(Acquire(arOther.pString))
{
}

BSFixedString::BSFixedString(BSFixedString&& arOther) : pString(Acquire(arOther.pString))
{
	BSStringPool::Release(arOther.pString);
	arOther.pString = nullptr;
}

BSFixedString::~BSFixedString()
{
	BSStringPool::Release(pString);
}

const BSFixedString& BSFixedString::operator<<(const char* apString)
{
	const char* pPrevious = pString;
	pString = Intern(apString);
	BSStringPool::Release(pPrevious);
	return *this;
}

const BSFixedString& BSFixedString::operator=(const BSFixedString& arOther)
{
	if (pString != arOther.pString)
	{
		const char* pPrevious = pString;
		pString = Acquire(arOther.pString);
		BSStringPool::Release(pPrevious);
	}
	return *this;
}

const BSFixedString& BSFixedString::operator=(BSFixedString&& arOther)
{
	const char* pPrevious = pString;
	if (arOther.pString != pPrevious)
	{
		pString = arOther.pString;
		arOther.pString = pPrevious;
	}
	return *this;
}

bool BSFixedString::operator!() const
{
	return pString == nullptr;
}

unsigned int BSFixedString::QLength() const
{
	if (!pString)
		return 0;
	const auto* pEntry = reinterpret_cast<const BSStringPool::Entry*>(pString) - 1;
	return pEntry->uiLength & 0xFFFFFF;
}

BSFixedStringW::BSFixedStringW(const wchar_t* apString) : pString(nullptr)
{
	pString = Intern(apString);
}

BSFixedStringW::BSFixedStringW(const BSFixedStringW& arOther) : pString(Acquire(arOther.pString))
{
}

BSFixedStringW::~BSFixedStringW()
{
	BSStringPool::Release(pString);
}

const BSFixedStringW& BSFixedStringW::operator<<(const wchar_t* apString)
{
	const wchar_t* pPrevious = pString;
	pString = Intern(apString);
	BSStringPool::Release(pPrevious);
	return *this;
}

const BSFixedStringW& BSFixedStringW::operator=(const BSFixedStringW& arOther)
{
	if (pString != arOther.pString)
	{
		const wchar_t* pPrevious = pString;
		pString = Acquire(arOther.pString);
		BSStringPool::Release(pPrevious);
	}
	return *this;
}

BSFixedStringW::operator const wchar_t*() const
{
	return pString;
}
