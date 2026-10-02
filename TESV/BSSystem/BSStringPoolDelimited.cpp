#include "BSSystem/BSStringPool.h"
#include "BSCore/BSCoreMessage.h"
#include "BSCore/MemoryContextTracker.h"

thread_local constinit unsigned char BSStringPool::cDelimiterPrefixST[104];
thread_local constinit unsigned char BSStringPool::cDelimiterLookupST[140];

namespace
{
	unsigned char& DelimiterFlag(char acChar)
	{
		// Signed character indices also access neighboring context fields.
		const unsigned int uiIndex = static_cast<signed char>(acChar) + 128;
		if (uiIndex < 104)
			return BSStringPool::cDelimiterPrefixST[uiIndex];
		if (uiIndex < 108)
			return reinterpret_cast<unsigned char*>(&etMemContextS)[uiIndex - 104];
		if (uiIndex < 112)
			return reinterpret_cast<unsigned char*>(&BSCoreMessage::eMessageContextS)[uiIndex - 108];
		if (uiIndex < 116)
			return reinterpret_cast<unsigned char*>(&BSCoreMessage::eWarningContextS)[uiIndex - 112];
		return BSStringPool::cDelimiterLookupST[uiIndex - 116];
	}
}

unsigned int BSStringPool::GenerateCRC(unsigned int& arCRC, char*& arString, char*& arNext, char* apSource, const char* apDelimiters)
{
	if (apDelimiters)
		for (const char* pChar = apDelimiters; *pChar; ++pChar)
			DelimiterFlag(*pChar) = 1;
	unsigned int uiCRC = 0;
	unsigned int uiLength = 0;
	char* pChar = apSource;
	while (*pChar && DelimiterFlag(*pChar))
		++pChar;
	if (!*pChar)
	{
		arNext = nullptr;
		arString = nullptr;
	}
	else
	{
		char* pStart = pChar;
		while (*pChar && !DelimiterFlag(*pChar))
		{
			unsigned int uiChar = static_cast<unsigned char>(*pChar);
			if (uiChar >= 'a' && uiChar <= 'z')
				uiChar -= 'a' - 'A';
			uiCRC = CRCTable[(uiCRC ^ uiChar) & 0xFF] ^ (uiCRC >> 8);
			++uiLength;
			++pChar;
		}
		if (*pChar)
		{
			arNext = pChar + 1;
			arString = pStart;
			pStart[uiLength] = 0;
		}
		else
		{
			arNext = nullptr;
			arString = pStart;
		}
	}
	if (apDelimiters)
		for (const char* pDelimiter = apDelimiters; *pDelimiter; ++pDelimiter)
			DelimiterFlag(*pDelimiter) = 0;
	arCRC = uiCRC;
	return uiLength;
}
