#include "BSSystem/FileFinder.h"

#include <cstring>
#include <io.h>
#include <stdlib.h>

#include "BSCore/MemoryContextTracker.h"
#include "BSCore/MemoryManager.h"
#include "BSSystem/Archive.h"
#include "BSSystem/BSFile.h"
#include "BSSystem/BSFixedString.h"
#include "BSSystem/FilePathUtilities.h"

FileFinder* FileFinder::pFinder = nullptr;
bool FileFinder::bUseDDXTextures = false;

FileFinder::FileFinder(const char*, int32_t, bool abUseDDXTextures)
{
	AutoMemContext kContext(MC_FILE_SYSTEM);
	pFinder = pFinder ? pFinder : this;
	NiFile::SetFileCreateFunc(GetFile);
	NiFile::SetFileAccessFunc(AccessFile);
	bUseDDXTextures = abUseDDXTextures;
}

FileFinder::~FileFinder()
{
	for (uint32_t i = PathA.QSize(); i;)
	{
		--i;
		MemoryManager::Instance().Deallocate(const_cast<char*>(PathA[i]), false);
		PathA.RemoveFast(i);
	}
	if (pFinder == this)
		pFinder = nullptr;
}

bool FileFinder::AccessFile(const char* apFileName, NiFile::OpenMode aeMode)
{
	if (aeMode != NiFile::READ_ONLY)
	{
		NiFile kFile(apFileName, aeMode, 0);
		return static_cast<bool>(kFile);
	}
	return pFinder && pFinder->Exist(apFileName, nullptr, 0, 0xFFFFFFFF) != NOTFOUND;
}

NiFile* FileFinder::GetFile(const char* apFileName, NiFile::OpenMode aeMode, uint32_t auiBufferSize)
{
	return GetFile(apFileName, aeMode, auiBufferSize, ARCHIVE_TYPE_ALL);
}

NiFile* FileFinder::GetFile(const char* apFileName, NiFile::OpenMode aeMode, uint32_t auiBufferSize, uint32_t auiArchiveType)
{
	if (!pFinder)
		return nullptr;
	if (aeMode == NiFile::WRITE_ONLY)
		return new BSFile(apFileName, NiFile::WRITE_ONLY, auiBufferSize, false);
	char acFilePath[784];
	FILE_LOCATION eLocation = pFinder->Exist(apFileName, acFilePath, 0, auiArchiveType);
	if (eLocation == NOTFOUND)
		return nullptr;
	if (eLocation == ARCHIVE)
		return ArchiveManager::GetFile(acFilePath, auiBufferSize, static_cast<ARCHIVE_TYPE>(auiArchiveType));
	BSFile* pFile = new BSFile(acFilePath, aeMode, auiBufferSize, false);
	pFile->Open(false, false);
	return pFile;
}

bool FileFinder::BuildFileArray(const char* apPattern, BSTArray<BSFixedString>& arFiles)
{
	BSStaticStringT<260> sDirectory;
	BSStaticStringT<260> sFileName;
	FilePathUtilities::SplitPath(apPattern, sDirectory, sFileName);
	FilePathUtilities::Join(sDirectory.pString ? sDirectory.pString : "", "", sDirectory);
	BSSimpleList<const char*>* pList = new BSSimpleList<const char*>;
	uint32_t uiCount = PlatformBuildFileList(apPattern, sDirectory.pString ? sDirectory.pString : "", pList);
	if (uiCount)
	{
		if (arFiles.QSize() + uiCount > arFiles.QAllocSize())
			arFiles.SetAllocSize(arFiles.QSize() + uiCount);
		for (BSSimpleList<const char*>* pNode = pList; pNode && pNode->m_item; pNode = pNode->m_pkNext)
			arFiles.Add(BSFixedString(pNode->m_item));
	}
	if (pList)
	{
		while (!pList->IsEmpty())
		{
			MemoryManager::Instance().Deallocate(const_cast<char*>(pList->m_item), false);
			pList->RemoveHead();
		}
		delete pList;
	}
	return uiCount != 0;
}

BSSimpleList<const char*>* FileFinder::BuildFileList(const char* apPattern, const char* apPath)
{
	return BuildFileList(apPattern, apPath, ARCHIVE_TYPE_ALL, nullptr);
}

BSSimpleList<const char*>* FileFinder::BuildFileList(const char* apPattern, const char* apPath, uint32_t auiArchiveType, BSSimpleList<const char*>* apList)
{
	if (!apPattern || !apPath)
		return nullptr;
	if (!strstr(apPattern, "*") && !strstr(apPattern, "?"))
		return BuildSingleFileList(apPattern, apPath, auiArchiveType, apList);
	if (!apList)
		apList = new BSSimpleList<const char*>;
	if (!ArchiveManager::ArchivesEnabled() || ArchiveManager::QInvalidateOlderFiles())
		PlatformBuildFileList(apPattern, apPath, apList);
	ArchiveManager::AddToFileList(apList, apPattern, apPath, static_cast<ARCHIVE_TYPE>(auiArchiveType));
	if (apList->IsEmpty())
	{
		delete apList;
		return nullptr;
	}
	return apList;
}

// Goes through BuildFileList, so a pattern with wildcards still lists them.
BSSimpleList<const char*>* FileFinder::BuildSingleFileList(const char* apFileName, const char* apPath, BSSimpleList<const char*>* apList)
{
	return BuildFileList(apFileName, apPath, ARCHIVE_TYPE_ALL, apList);
}

// An empty result also deletes a list passed in by the caller.
BSSimpleList<const char*>* FileFinder::BuildSingleFileList(const char* apFileName, const char* apPath, uint32_t auiArchiveType, BSSimpleList<const char*>* apList)
{
	BSSimpleList<const char*>* pList = apList ? apList : new BSSimpleList<const char*>;
	if (((!ArchiveManager::ArchivesEnabled() || ArchiveManager::QInvalidateOlderFiles()) && PlatformFindSingleFile(apFileName)) ||
		ArchiveManager::FindFile(apFileName, static_cast<ARCHIVE_TYPE>(auiArchiveType)))
	{
		size_t uiPathLength = strlen(apPath);
		const char* pPathSlash = strrchr(apPath, '\\');
		const char* pNameSlash = strrchr(apFileName, '\\');
		if (pPathSlash && pNameSlash)
		{
			size_t uiDirLength = uiPathLength - strlen(pPathSlash + 1);
			const char* pName = pNameSlash + 1;
			size_t uiSize = strlen(pName) + uiDirLength + 1;
			char* pResult = new char[uiSize];
			memcpy(pResult, apPath, uiDirLength);
			pResult[uiDirLength] = 0;
			strcat_s(pResult, uiSize, pName);
			pList->AddHead(pResult);
		}
	}
	if (pList->IsEmpty())
	{
		delete pList;
		return nullptr;
	}
	return pList;
}

void FileFinder::DeleteFileNameList(BSSimpleList<const char*>*& arpList)
{
	if (arpList)
	{
		while (!arpList->IsEmpty())
		{
			MemoryManager::Instance().Deallocate(const_cast<char*>(arpList->m_item), false);
			arpList->RemoveHead();
		}
		delete arpList;
	}
	arpList = nullptr;
}

// Flag 1 skips the archives, 2 the current directory, 4 the search paths;
// with loose files taking priority over archives the flags are ignored.
FileFinder::FILE_LOCATION FileFinder::Exist(const char* apFileName, char* apFilePath, uint32_t auiFlags, uint32_t auiArchiveType)
{
	if (!apFileName || !*apFileName)
		return NOTFOUND;
	uint32_t uiArchiveType = auiArchiveType == 0xFFFFFFFF ? ARCHIVE_TYPE_ALL : auiArchiveType;
	char c = apFileName[0];
	if (apFileName[1] == ':')
	{
		auiFlags |= 5;
	}
	else if (c == '.' && apFileName[1] == '\\')
	{
		c = apFileName[2];
		auiFlags |= 4;
		apFileName += 2;
	}
	if (c == '\\')
		++apFileName;
	char acPath[260];
	strcpy_s(acPath, 260, apFileName);
	FILE_LOCATION eLocation = LookForFile(acPath, auiFlags, uiArchiveType);
	if (apFilePath)
	{
		if (eLocation == NOTFOUND)
			*apFilePath = 0;
		else
			strcpy_s(apFilePath, 260, acPath);
	}
	return eLocation;
}

void FileFinder::_AddPath(const char* apPath)
{
	AutoMemContext kContext(MC_FILE_SYSTEM);
	if (!apPath)
		return;
	size_t uiLength = strlen(apPath);
	bool bNeedsSlash = apPath[uiLength - 1] != '\\';
	size_t uiSize = uiLength + bNeedsSlash + 1;
	char* pPath = static_cast<char*>(MemoryManager::Instance().Allocate(uiSize, 0, false));
	strcpy_s(pPath, uiSize, apPath);
	if (bNeedsSlash)
	{
		pPath[uiLength] = '\\';
		pPath[uiLength + 1] = 0;
	}
	PathA.Add(pPath);
}

bool FileFinder::GetFileTime(const char* apFileName, _FILETIME& arFileTime)
{
	return GetFileTime(apFileName, arFileTime, ARCHIVE_TYPE_ALL);
}

bool FileFinder::GetFileTime(const char* apFileName, _FILETIME& arFileTime, uint32_t auiArchiveType)
{
	memset(&arFileTime, 0, 8);
	Archive* pArchive = ArchiveManager::GetArchiveForFile(apFileName, static_cast<ARCHIVE_TYPE>(auiArchiveType));
	if (pArchive)
		apFileName = pArchive->FileName();
	return PlatformGetFileTime(apFileName, arFileTime);
}

void FileFinder::CombineFileNames(const char* apBase, const char* apAddition, char* apResult)
{
	*apResult = 0;
	char acDirectory[260];
	_splitpath_s(apBase, nullptr, 0, acDirectory, 260, nullptr, 0, nullptr, 0);
	size_t uiLength = strlen(acDirectory);
	if (acDirectory[uiLength - 1] == '\\')
		acDirectory[uiLength - 1] = 0;
	if (strncmp(apAddition, "..\\", 3))
	{
		strcpy_s(apResult, 260, apAddition);
		return;
	}
	// Each "..\" drops the last directory; with none left the whole string goes.
	do
	{
		int64_t i = static_cast<int64_t>(strlen(acDirectory)) - 1;
		while (i && acDirectory[i] != '\\')
			--i;
		acDirectory[i] = 0;
		apAddition += 3;
	} while (!strncmp(apAddition, "..\\", 3));
	strcat_s(apResult, 260, acDirectory);
	strcat_s(apResult, 260, "\\");
	strcat_s(apResult, 260, apAddition);
}

FileFinder::FILE_LOCATION FileFinder::LookForFile(char* apFilePath, uint32_t auiFlags)
{
	return LookForFile(apFilePath, auiFlags, ARCHIVE_TYPE_ALL);
}

FileFinder::FILE_LOCATION FileFinder::LookForFile(char* apFilePath, uint32_t auiFlags, uint32_t auiArchiveType)
{
	if (!(auiFlags & 1) && ArchiveManager::FindFile(apFilePath, static_cast<ARCHIVE_TYPE>(auiArchiveType)))
		return ARCHIVE;
	if (!ArchiveManager::ArchivesEnabled() || ArchiveManager::QInvalidateOlderFiles())
		auiFlags = 0;
	if (SearchCWD(auiFlags, auiArchiveType) && _access(apFilePath, 0) != -1)
		return HARDDRIVE;
	if (!SearchAlternateSubdirectories(auiFlags, auiArchiveType))
		return NOTFOUND;
	uint32_t uiCount = PathA.QSize();
	char acPath[260];
	for (uint32_t i = 0; i < uiCount; ++i)
	{
		const char* pPath = PathA[i];
		if (!pPath || !_strnicmp(apFilePath, pPath, strlen(pPath)))
			continue;
		strcpy_s(acPath, 260, pPath);
		strcat_s(acPath, 260, apFilePath);
		if (_access(acPath, 0) != -1)
		{
			strcpy_s(apFilePath, 260, acPath);
			return HARDDRIVE;
		}
	}
	return NOTFOUND;
}
