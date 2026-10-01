#include "BSCore/BSStringUtilities.h"

#include <cstring>

namespace BSStringUtilities
{
	bool IsWhitespace(char acTestChar)
	{
		return acTestChar == ' ' || acTestChar == '\t' || acTestChar == '\n' || acTestChar == '\r';
	}

	bool IsWhitespaceString(const char* apTestString)
	{
		if (apTestString)
			for (; *apTestString; ++apTestString)
				if (!IsWhitespace(*apTestString))
					return false;
		return true;
	}

	bool IsNumericString(const char* apTestString)
	{
		if (!apTestString)
			return true;
		bool bFirst = true;
		bool bPoint = false;
		for (; *apTestString; ++apTestString)
		{
			const char cChar = *apTestString;
			if (cChar == '.')
			{
				if (bPoint)
					return false;
				bPoint = true;
			}
			else if (!(cChar >= '0' && cChar <= '9') && !(bFirst && cChar == '-'))
				return false;
			bFirst = false;
		}
		return true;
	}

	char* Rstrip(char* apString)
	{
		size_t uiLength = std::strlen(apString);
		while (uiLength && IsWhitespace(apString[uiLength - 1]))
			--uiLength;
		apString[uiLength] = '\0';
		return apString;
	}

	char* Strip(char* apString)
	{
		Rstrip(apString);
		while (IsWhitespace(*apString))
			++apString;
		return apString;
	}

	bool FindStrippedEnds(const char* apString, size_t& aruiStart, size_t& aruiEnd)
	{
		if (!apString)
			return false;
		size_t uiFirst = 0;
		while (IsWhitespace(apString[uiFirst]))
			++uiFirst;
		if (!apString[uiFirst])
			return false;
		size_t uiLast = uiFirst;
		for (size_t uiIndex = uiFirst + 1; apString[uiIndex]; ++uiIndex)
			if (!IsWhitespace(apString[uiIndex]))
				uiLast = uiIndex;
		aruiStart = uiFirst;
		aruiEnd = uiLast;
		return true;
	}
}
