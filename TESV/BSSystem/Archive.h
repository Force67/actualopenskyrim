#pragma once

#include <cstdint>

#include "BSCore/BSSimpleList.h"
#include "BSSystem/BSFile.h"

enum ARCHIVE_TYPE
{
	ARCHIVE_TYPE_ALL = 0xFFFF,
};

// Only what other code calls so far; the archive classes are not ported yet.
class Archive : public BSFile
{
};

class ArchiveFile : public BSFile
{
};

class ArchiveManager
{
public:
	static bool ArchivesEnabled() { return bUseArchives; }
	static bool QInvalidateOlderFiles() { return bInvalidateOlderFiles; }
	static ArchiveFile* GetFile(const char* apFileName, uint32_t auiBufferSize, ARCHIVE_TYPE aeType);
	static Archive* GetArchiveForFile(const char* apFileName, ARCHIVE_TYPE aeType);
	static bool FindFile(const char* apFileName, ARCHIVE_TYPE aeType);
	static void AddToFileList(BSSimpleList<const char*>* apList, const char* apPattern, const char* apPath, ARCHIVE_TYPE aeType);

	static bool bInvalidateOlderFiles;
	static bool bUseArchives;
};
