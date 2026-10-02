#include "BSSystem/BSFile.h"

#include <cstring>
#include <windows.h>

#include "BSCore/MemoryManager.h"

BSFile::BSFile(const char* pcName, OpenMode eMode, uint32_t auiBufferSize, bool abTextMode)
{
	SetEndianSwap(false);
	m_eMode = eMode;
	m_uiBufferAllocSize = auiBufferSize;
	m_uiBufferReadSize = 0;
	m_uiPos = 0;
	m_pBuffer = nullptr;
	m_pFile = nullptr;
	uiIOSize = 0;
	uiTrueFilePos = 0;
	uiResult = 0;
	uiFileSize = 0;
	bUseAuxBuffer = false;
	m_pAuxBuffer = nullptr;
	iAuxTrueFilePos = -1;
	iAuxBufferMinIndex = 0;
	iAuxBufferMaxIndex = 0;
	if (strlen(pcName) < 260)
	{
		strcpy_s(pFileName, 260, pcName);
		if (eMode == WRITE_ONLY)
		{
			Open(false, abTextMode);
			return;
		}
	}
	else
	{
		pFileName[0] = 0;
	}
	if (eMode == READ_ONLY)
		m_bGood = Exist();
}

void BSFile::AllocateBuffer(uint32_t auiSize)
{
	FreeBuffer();
	m_uiBufferAllocSize = auiSize;
	if (auiSize)
		m_pBuffer = static_cast<char*>(MemoryManager::Instance().Allocate(auiSize, 0, false));
}

void BSFile::FreeBuffer()
{
	if (m_pBuffer)
	{
		operator delete(m_pBuffer, size_t(1));
		m_pBuffer = nullptr;
	}
}

// When the file is not found the stale (uninitialised) time is converted.
_SYSTEMTIME BSFile::GetLastSaveTime()
{
	SYSTEMTIME kResult;
	if (m_pFile)
	{
		WIN32_FIND_DATAA kFindData;
		FILETIME kFileTime;
		HANDLE hFind = FindFirstFileA(pFileName, &kFindData);
		if (hFind != INVALID_HANDLE_VALUE)
			kFileTime = kFindData.ftLastWriteTime;
		FindClose(hFind);
		SYSTEMTIME kSystemTime;
		FileTimeToSystemTime(&kFileTime, &kSystemTime);
		SystemTimeToTzSpecificLocalTime(nullptr, &kSystemTime, &kResult);
	}
	else
	{
		memset(&kResult, 0, sizeof(kResult));
	}
	return kResult;
}

uint32_t BSFile::GetSize()
{
	uiFileSize = 0;
	if (m_eMode == WRITE_ONLY)
		Flush();
	Open(false, false);
	if (m_pFile)
	{
		int32_t iPosition = ftell(m_pFile);
		fseek(m_pFile, 0, SEEK_END);
		uiFileSize = ftell(m_pFile);
		fseek(m_pFile, iPosition, SEEK_SET);
	}
	return uiFileSize;
}

uint32_t BSFile::ReadF(void* pvBuffer, uint32_t uiBytes)
{
	if (!m_pFile)
		Open(false, false);
	uint32_t uiRead = FileRead(pvBuffer, uiBytes);
	uiTrueFilePos += uiRead;
	return uiRead;
}

// Read mode seeks are made relative so NiFile can stay inside its buffer.
void BSFile::Seek(int32_t iOffset, int32_t iWhence)
{
	int32_t iNewPos;
	switch (iWhence)
	{
	case SEEK_SET:
		iNewPos = iOffset;
		if (m_eMode == READ_ONLY)
		{
			iWhence = SEEK_CUR;
			iOffset -= uiTrueFilePos;
		}
		break;
	case SEEK_CUR:
		iNewPos = iOffset + uiTrueFilePos;
		break;
	case SEEK_END:
		iNewPos = GetSize() - iOffset;
		break;
	default:
		iNewPos = uiTrueFilePos;
		break;
	}
	if (static_cast<uint32_t>(iNewPos) != uiTrueFilePos)
	{
		NiFile::Seek(iOffset, iWhence);
		uiTrueFilePos = iNewPos;
	}
}
