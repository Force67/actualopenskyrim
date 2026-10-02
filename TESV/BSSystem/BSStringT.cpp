#include "BSSystem/BSStringT.h"

#include <cerrno>
#include <stdlib.h>
#include <cstring>

template class BSStaticStringT<260>;

template <class Char>
static uint32_t QStringLength(const Char* apString)
{
	uint32_t uLength = 0;
	while (apString[uLength])
		++uLength;
	return uLength;
}

template <class Char>
BSStringT<Char, -1, DynamicMemoryManagementPol>::BSStringT()
{
	pString = nullptr;
	sSize = 0;
	sCapacity = 0;
}

template <class Char>
BSStringT<Char, -1, DynamicMemoryManagementPol>::~BSStringT()
{
	if (pString)
		MemoryManager::Instance().Deallocate(pString, false);
}

// The narrow and wide versions copy differently: the narrow one uses the
// checked memcpy/memmove pattern of the CRT (clearing the buffer on overflow
// only when the ranges do not overlap), the wide one BSmemcpy/BSmemmove.
template <class Char>
bool BSStringT<Char, -1, DynamicMemoryManagementPol>::Set(const Char* apString, uint32_t auiMaxLen)
{
	uint32_t uiLength = apString ? QStringLength(apString) : 0;
	uint32_t uiNeed = (auiMaxLen ? auiMaxLen : uiLength) + 1;
	Char* pOld = pString;
	uint16_t usCapacity = sCapacity;
	if (uiNeed > usCapacity)
	{
		size_t stBytes = static_cast<size_t>(uiNeed) * sizeof(Char);
		if constexpr (sizeof(Char) != 1)
			if (stBytes / sizeof(Char) != uiNeed)
				stBytes = static_cast<size_t>(-1);
		pString = static_cast<Char*>(MemoryManager::Instance().Allocate(stBytes, 0, false));
		uint32_t uiAllocated = pString ? uiNeed : 0;
		usCapacity = uiAllocated <= 0xFFFF ? static_cast<uint16_t>(uiAllocated) : 0xFFFF;
		sCapacity = usCapacity;
		if (usCapacity < uiNeed)
			uiNeed = usCapacity;
	}
	uint32_t uiCopy = uiNeed > uiLength ? uiLength : uiNeed - 1;
	Char* pDest = pString;
	if (!pDest)
		return uiCopy != 0;
	if (!apString)
	{
		if (!uiNeed)
		{
			pString = nullptr;
			sSize = 0;
			sCapacity = 0;
		}
		else
		{
			pDest[0] = 0;
			sSize = 0;
		}
	}
	else if (!usCapacity)
	{
		sSize = 0;
	}
	else
	{
		bool bOverlap = pDest > apString && pDest <= apString + uiCopy;
		if constexpr (sizeof(Char) == 1)
		{
			if (bOverlap)
			{
				if (uiCopy)
				{
					if (usCapacity >= uiCopy)
						memmove(pDest, apString, uiCopy);
					else
					{
						*_errno() = ERANGE;
						_invalid_parameter_noinfo();
					}
				}
			}
			else if (pDest != apString && uiCopy)
			{
				if (usCapacity >= uiCopy)
					memcpy(pDest, apString, uiCopy);
				else
				{
					memset(pDest, 0, usCapacity);
					*_errno() = ERANGE;
					_invalid_parameter_noinfo();
				}
			}
		}
		else
		{
			if (bOverlap)
				BSmemmove(pDest, sizeof(Char) * usCapacity, apString, sizeof(Char) * uiCopy);
			else if (pDest != apString)
				BSmemcpy(pDest, sizeof(Char) * usCapacity, apString, sizeof(Char) * uiCopy);
		}
		pString[uiCopy] = 0;
		sSize = uiCopy <= 0xFFFF ? static_cast<uint16_t>(uiCopy) : 0xFFFF;
	}
	if (pOld != pString && pOld)
		MemoryManager::Instance().Deallocate(pOld, false);
	return uiCopy != 0;
}

template <class Char>
BSStringT<Char, -1, DynamicMemoryManagementPol>& BSStringT<Char, -1, DynamicMemoryManagementPol>::Append(const Char* apString)
{
	if (!apString)
		return *this;
	uint32_t uAddLen = QStringLength(apString);
	uint32_t uSize = GetLength();
	uint32_t uCapacity = sCapacity;
	uint32_t uTotal = uSize + uAddLen;
	if (static_cast<int32_t>(uSize) + static_cast<int32_t>(uAddLen) >= static_cast<int32_t>(uCapacity))
	{
		uint32_t uOldTotal = uTotal;
		Set(pString, uOldTotal);
		uCapacity = sCapacity;
		uSize = GetLength();
		uTotal = uCapacity - 1;
		if (uOldTotal < uCapacity - 1)
			uTotal = uOldTotal;
		uAddLen = uTotal - uSize;
	}
	Char* pBuffer = pString;
	if (!pBuffer)
	{
		Set(apString, 0);
		return *this;
	}
	if (pBuffer + uSize > apString && pBuffer + uSize <= apString + uAddLen)
		BSmemmove(pBuffer + uSize, static_cast<size_t>(uCapacity) * sizeof(Char) - uSize * sizeof(Char),
			apString, static_cast<size_t>(uAddLen) * sizeof(Char));
	else if (pBuffer != apString)
		BSmemcpy(pBuffer + uSize, static_cast<size_t>(uCapacity) * sizeof(Char) - uSize * sizeof(Char),
			apString, static_cast<size_t>(uAddLen) * sizeof(Char));
	pBuffer[uTotal] = 0;
	sSize = uTotal <= 0xFFFF ? static_cast<uint16_t>(uTotal) : 0xFFFF;
	return *this;
}

template class BSStringT<char, -1, DynamicMemoryManagementPol>;
template class BSStringT<wchar_t, -1, DynamicMemoryManagementPol>;
