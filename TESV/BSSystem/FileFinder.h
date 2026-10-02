#pragma once

#include <cstdint>

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSTArray.h"
#include "Gamebryo/CoreLibs/NiSystem/NiFile.h"

class BSFixedString;
struct _FILETIME;

class FileFinder
{
public:
	enum FILE_LOCATION
	{
		NOTFOUND = 0,
		HARDDRIVE = 1,
		ARCHIVE = 2,
	};

	FileFinder(const char* apPath, int32_t aiFileLogging, bool abUseDDXTextures);
	~FileFinder();

	void _AddPath(const char* apPath);
	FILE_LOCATION Exist(const char* apFileName, char* apFilePath, uint32_t auiFlags, uint32_t auiArchiveType);

	static bool AccessFile(const char* apFileName, NiFile::OpenMode aeMode);
	static NiFile* GetFile(const char* apFileName, NiFile::OpenMode aeMode, uint32_t auiBufferSize);
	static NiFile* GetFile(const char* apFileName, NiFile::OpenMode aeMode, uint32_t auiBufferSize, uint32_t auiArchiveType);
	static bool BuildFileArray(const char* apPattern, BSTArray<BSFixedString>& arFiles);
	static BSSimpleList<const char*>* BuildFileList(const char* apPattern, const char* apPath);
	static BSSimpleList<const char*>* BuildFileList(const char* apPattern, const char* apPath, uint32_t auiArchiveType, BSSimpleList<const char*>* apList);
	static BSSimpleList<const char*>* BuildSingleFileList(const char* apFileName, const char* apPath, BSSimpleList<const char*>* apList);
	static BSSimpleList<const char*>* BuildSingleFileList(const char* apFileName, const char* apPath, uint32_t auiArchiveType, BSSimpleList<const char*>* apList);
	static void DeleteFileNameList(BSSimpleList<const char*>*& arpList);
	static bool GetFileTime(const char* apFileName, _FILETIME& arFileTime);
	static bool GetFileTime(const char* apFileName, _FILETIME& arFileTime, uint32_t auiArchiveType);
	static void CombineFileNames(const char* apBase, const char* apAddition, char* apResult);

	static FileFinder* pFinder;
	static bool bUseDDXTextures;

protected:
	FILE_LOCATION LookForFile(char* apFilePath, uint32_t auiFlags);
	FILE_LOCATION LookForFile(char* apFilePath, uint32_t auiFlags, uint32_t auiArchiveType);
	bool SearchCWD(int32_t aiFlags, int32_t aiArchiveType);
	bool SearchAlternateSubdirectories(int32_t aiFlags, int32_t aiArchiveType);

	static uint32_t PlatformBuildFileList(const char* apPattern, const char* apPath, BSSimpleList<const char*>* apList);
	static bool PlatformFindSingleFile(const char* apFileName);
	static bool PlatformGetFileTime(const char* apFileName, _FILETIME& arFileTime);

	BSTArray<const char*> PathA;
};
static_assert(sizeof(FileFinder) == 0x18);
