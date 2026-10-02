#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>

#include "BSCore/BSMemoryutility.h"
#include "BSCore/MemoryManager.h"

// Fixed capacity string; the buffer doubles as the storage (no heap). A size
// of 0xFFFF in sSize means "unknown, count it".
template <uint32_t SIZE>
class BSStaticStringT
{
public:
	BSStaticStringT()
	{
		pString = nullptr;
		sSize = 0;
		sCapacity = 0;
	}

	bool SetFromString(const char* pStr, uint32_t uiCount);
	BSStaticStringT& Append(const char* pStr);
	uint32_t GetLength() const
	{
		return sSize == 0xFFFF ? static_cast<uint32_t>(strlen(pString)) : sSize;
	}

	char sBuffer[SIZE];
	const char* pString;
	uint16_t sSize;
	uint16_t sCapacity;
};
static_assert(sizeof(BSStaticStringT<260>) == 0x118);
static_assert(offsetof(BSStaticStringT<260>, pString) == 0x108);
static_assert(offsetof(BSStaticStringT<260>, sSize) == 0x110);
static_assert(offsetof(BSStaticStringT<260>, sCapacity) == 0x112);

extern template class BSStaticStringT<260>;

template <uint32_t SIZE>
bool BSStaticStringT<SIZE>::SetFromString(const char* pStr, uint32_t uiCount)
{
	uint32_t uStrLen = pStr ? strlen(pStr) : 0;
	uint32_t uCapacity = sCapacity;
	uint32_t uSpace = uiCount ? uiCount : uStrLen;
	uSpace += 1;
	if (uSpace > uCapacity)
	{
		uCapacity = SIZE;
		pString = sBuffer;
		sCapacity = static_cast<uint16_t>(SIZE);
		if (uSpace > SIZE)
			uSpace = SIZE;
	}
	uint32_t uCopyLen = uSpace - 1;
	if (uSpace > uStrLen)
		uCopyLen = uStrLen;
	if (!pString)
		return uCopyLen != 0;
	if (!pStr)
	{
		if (uSpace)
		{
			*const_cast<char*>(pString) = 0;
			sSize = 0;
		}
		else
		{
			pString = nullptr;
			*reinterpret_cast<uint32_t*>(&sSize) = 0;
		}
		return uCopyLen != 0;
	}
	if (!uCapacity)
	{
		sSize = 0;
		return uCopyLen != 0;
	}
	if (pString == pStr)
	{
		// Same buffer, nothing to move.
	}
	else if (pString > pStr && pString <= pStr + uCopyLen)
	{
		BSmemmove(const_cast<char*>(pString), uCapacity, pStr, uCopyLen);
	}
	else
	{
		BSmemcpy(const_cast<char*>(pString), uCapacity, pStr, uCopyLen);
	}
	const_cast<char*>(pString)[uCopyLen] = 0;
	sSize = uCopyLen <= 0xFFFF ? static_cast<uint16_t>(uCopyLen) : 0xFFFF;
	return uCopyLen != 0;
}

template <uint32_t SIZE>
BSStaticStringT<SIZE>& BSStaticStringT<SIZE>::Append(const char* pStr)
{
	if (!pStr)
		return *this;
	uint32_t uAddLen = strlen(pStr);
	uint32_t uSize = GetLength();
	uint32_t uCapacity = sCapacity;
	uint32_t uTotal = uSize + uAddLen;
	if (static_cast<int32_t>(uSize) + static_cast<int32_t>(uAddLen) >= static_cast<int32_t>(uCapacity))
	{
		uint32_t uOldTotal = uTotal;
		SetFromString(pString, uOldTotal);
		uCapacity = sCapacity;
		uint32_t uNewSize = GetLength();
		uTotal = uCapacity - 1;
		if (uOldTotal < uCapacity - 1)
			uTotal = uOldTotal;
		uAddLen = uTotal - uNewSize;
	}
	char* pBuffer = const_cast<char*>(pString);
	if (!pBuffer)
	{
		SetFromString(pStr, 0);
		return *this;
	}
	uSize = GetLength();
	if (pBuffer + uSize > pStr && pBuffer + uSize <= pStr + uAddLen)
	{
		BSmemmove(pBuffer + uSize, uCapacity - uSize, pStr, uAddLen);
	}
	else if (pBuffer != pStr)
	{
		BSmemcpy(pBuffer + uSize, uCapacity - uSize, pStr, uAddLen);
	}
	pBuffer[uTotal] = 0;
	sSize = uTotal <= 0xFFFF ? static_cast<uint16_t>(uTotal) : 0xFFFF;
	return *this;
}

class DynamicMemoryManagementPol {};

template <class Char, int SIZE, class MemoryPolicy> class BSStringT;

// Heap backed string. sSize of 0xFFFF again means "unknown, count it"; a null
// pString means "no allocation yet". auiMaxLen of 0 to Set means "use strlen".
template <class Char>
class BSStringT<Char, -1, DynamicMemoryManagementPol>
{
public:
	BSStringT();
	~BSStringT();
	bool Set(const Char* apString, uint32_t auiMaxLen);
	BSStringT& Append(const Char* apString);
	uint32_t GetLength() const
	{
		if (sSize != 0xFFFF)
			return sSize;
		uint32_t uLength = 0;
		while (pString[uLength])
			++uLength;
		return uLength;
	}
	const Char* QPtr() const { return pString; }

	Char* pString;
	uint16_t sSize;
	uint16_t sCapacity;
};
using BSString = BSStringT<char, -1, DynamicMemoryManagementPol>;
static_assert(sizeof(BSString) == 16);
static_assert(offsetof(BSString, pString) == 0);
static_assert(offsetof(BSString, sSize) == 8);
static_assert(offsetof(BSString, sCapacity) == 10);

extern template class BSStringT<char, -1, DynamicMemoryManagementPol>;
extern template class BSStringT<wchar_t, -1, DynamicMemoryManagementPol>;
