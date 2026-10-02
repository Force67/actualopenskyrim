#pragma once

#include <cstdint>
#include <cstddef>

#include <windows.h>

#include "BSSystem/BSSystemFile.h"

// Win32 directory walk; DoOpen stores "dir\*", GetNext pulls entries with
// FindFirstFileA on the first call and FindNextFileA afterwards.
class BSSystemDir
{
public:
	BSSystemDir();

	BSSystemFile::ErrorCode DoOpen(const char* pDirectory);
	BSSystemFile::ErrorCode GetNext(bool* abEnd);
	BSSystemFile::ErrorCode Close();

	const char* QDirectory() const { return cDirectory; }
	BSSystemFile::ErrorCode QLastError() const { return eLastError; }
	const char* QName() const { return FindData.cFileName; }
	bool QRegular() const { return !(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY); }

	static BSSystemFile::ErrorCode MakeDir(const char* pDirectory);
	static bool MakePartialDirPath(const char* pPath, uint64_t uiOffset);
	static bool MakeEntireDirPath(const char* pPath);

	HANDLE hFind;
	WIN32_FIND_DATAA FindData;
	char cDirectory[260];
	BSSystemFile::ErrorCode eLastError;
	uint64_t uiEntryPos;
};
static_assert(sizeof(BSSystemDir) == 0x258);
static_assert(offsetof(BSSystemDir, FindData) == 0x8);
static_assert(offsetof(BSSystemDir, cDirectory) == 0x148);
static_assert(offsetof(BSSystemDir, eLastError) == 0x24C);
static_assert(offsetof(BSSystemDir, uiEntryPos) == 0x250);

bool BSstrempty(const char* pStr);
