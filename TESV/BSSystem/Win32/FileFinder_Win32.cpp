#include "BSSystem/FileFinder.h"

#include <cstring>
#include <windows.h>

#include "BSCore/MemoryManager.h"

bool FileFinder::SearchCWD(int32_t aiFlags, int32_t)
{
	return !(aiFlags & 2);
}

bool FileFinder::SearchAlternateSubdirectories(int32_t aiFlags, int32_t)
{
	return !(aiFlags & 4);
}

// apPath supplies the directory part of each result (up to its last
// backslash); without one no names are added and the buffer is leaked.
uint32_t FileFinder::PlatformBuildFileList(const char* apPattern, const char* apPath, BSSimpleList<const char*>* apList)
{
	uint32_t uiCount = 0;
	WIN32_FIND_DATAA kFindData;
	HANDLE hFind = FindFirstFileA(apPattern, &kFindData);
	if (hFind == INVALID_HANDLE_VALUE)
		return 0;
	int32_t iDirLength = static_cast<int32_t>(strlen(apPath));
	const char* pSlash = strrchr(apPath, '\\');
	if (pSlash)
		iDirLength -= static_cast<int32_t>(strlen(pSlash + 1));
	do
	{
		size_t uiSize = static_cast<uint32_t>(strlen(kFindData.cFileName) + iDirLength + 1);
		char* pName = static_cast<char*>(MemoryManager::Instance().Allocate(uiSize, 0, false));
		memcpy(pName, apPath, iDirLength);
		pName[iDirLength] = 0;
		char* pNameSlash = strrchr(pName, '\\');
		if (pNameSlash)
		{
			strcpy_s(pNameSlash + 1, pName + uiSize - (pNameSlash + 1), kFindData.cFileName);
			apList->AddHead(pName);
			++uiCount;
		}
	} while (FindNextFileA(hFind, &kFindData));
	FindClose(hFind);
	return uiCount;
}

bool FileFinder::PlatformFindSingleFile(const char* apFileName)
{
	WIN32_FIND_DATAA kFindData;
	HANDLE hFind = FindFirstFileA(apFileName, &kFindData);
	if (hFind == INVALID_HANDLE_VALUE)
		return false;
	FindClose(hFind);
	return true;
}

bool FileFinder::PlatformGetFileTime(const char* apFileName, _FILETIME& arFileTime)
{
	WIN32_FIND_DATAA kFindData;
	HANDLE hFind = FindFirstFileA(apFileName, &kFindData);
	if (hFind == INVALID_HANDLE_VALUE)
		return false;
	arFileTime = kFindData.ftLastWriteTime;
	FindClose(hFind);
	return true;
}
