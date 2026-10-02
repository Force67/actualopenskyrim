#pragma once

#include <cstddef>

class NiPath
{
public:
	static void RemoveDotDots(char* pcPath);
	static size_t ConvertToRelative(char* pcRelativePath, size_t stRelBytes, const char* pcAbsolutePath, const char* pcRelativeToHere);
	static size_t ConvertToAbsolute(char* pcPath, size_t stPathBytes, const char* pcRelativeToHere);
	static size_t ConvertToAbsolute(char* pcPath, size_t stPathBytes);
	static size_t ConvertToAbsolute(char* pcAbsolutePath, size_t stAbsBytes, const char* pcRelativePath, const char* pcRelativeToHere);
	static bool GetCurrentWorkingDirectory(char* pcPath, size_t stDestSize);
	static bool GetExecutableDirectory(char* pcPath, size_t stDestSize);
	static bool IsRelative(char* pcPath);
	static char* StripAbsoluteBase(const char* pcAbsolutePath);
	static bool Standardize(char* pcPath);
	static void ReplaceInvalidFilenameCharacters(char* pcFilename, char cReplacement);
	static void RemoveSlashDotSlash(char* pcPath);
	static bool IsUniqueAbsolute(char* pcPath);
};
