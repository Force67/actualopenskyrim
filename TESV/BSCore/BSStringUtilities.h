#pragma once

#include <cstddef>

namespace BSStringUtilities
{
	bool IsWhitespace(char acTestChar);
	bool IsWhitespaceString(const char* apTestString);
	bool IsNumericString(const char* apTestString);
	char* Rstrip(char* apString);
	char* Strip(char* apString);
	bool FindStrippedEnds(const char* apString, size_t& aruiStart, size_t& aruiEnd);
}
