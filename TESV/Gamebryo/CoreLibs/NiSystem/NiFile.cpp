#include "NiFile.h"
#include "NiMemoryDefines.h"
#include "BSCore/MemoryManager.h"
#include <cstring>
#include <new>

#undef CreateDirectory

NiFile::FILECREATEFUNC NiFile::ms_pfnFileCreateFunc = DefaultFileCreateFunc;
NiFile::FILEACCESSFUNC NiFile::ms_pfnFileAccessFunc = DefaultFileAccessFunc;
NiFile::CREATEDIRFUNC NiFile::ms_pfnCreateDirFunc = DefaultCreateDirectoryFunc;
NiFile::DIREXISTSFUNC NiFile::ms_pfnDirExistsFunc = DefaultDirectoryExistsFunc;

NiFile* NiFile::GetFile(const char* pcName, OpenMode eMode, uint32_t uiBufferSize)
{
	return ms_pfnFileCreateFunc(pcName, eMode, uiBufferSize);
}

bool NiFile::Access(const char* pcName, OpenMode eMode)
{
	return ms_pfnFileAccessFunc(pcName, eMode);
}

bool NiFile::CreateDirectory(const char* pcDir)
{
	return ms_pfnCreateDirFunc(pcDir);
}

bool NiFile::DirectoryExists(const char* pcDir)
{
	return ms_pfnDirExistsFunc(pcDir);
}

void NiFile::SetFileCreateFunc(FILECREATEFUNC pfnFunc)
{
	ms_pfnFileCreateFunc = pfnFunc ? pfnFunc : DefaultFileCreateFunc;
}

void NiFile::SetFileAccessFunc(FILEACCESSFUNC pfnFunc)
{
	ms_pfnFileAccessFunc = pfnFunc ? pfnFunc : DefaultFileAccessFunc;
}

void NiFile::SetCreateDirectoryFunc(CREATEDIRFUNC pfnFunc)
{
	ms_pfnCreateDirFunc = pfnFunc ? pfnFunc : DefaultCreateDirectoryFunc;
}

void NiFile::SetDirectoryExistsFunc(DIREXISTSFUNC pfnFunc)
{
	ms_pfnDirExistsFunc = pfnFunc ? pfnFunc : DefaultDirectoryExistsFunc;
}

char* NiFile::GetBuffer()
{
	return m_pBuffer;
}

NiFile* NiFile::DefaultFileCreateFunc(const char* pcName, OpenMode eMode, uint32_t uiBufferSize)
{
	void* pvStorage = MemoryManager::Instance().Allocate(sizeof(NiFile), 0, false);
	return pvStorage ? new (pvStorage) NiFile(pcName, eMode, uiBufferSize) : nullptr;
}

bool NiFile::DefaultFileAccessFunc(const char* pcName, OpenMode eMode)
{
	NiFile kFile(pcName, eMode, 0);
	return static_cast<bool>(kFile);
}

NiFile::operator bool() const
{
	return m_bGood;
}

void NiFile::SetEndianSwap(bool bDoSwap)
{
	m_pfnRead = bDoSwap ? ReadAndSwap : ReadNoSwap;
	m_pfnWrite = bDoSwap ? WriteAndSwap : WriteNoSwap;
}

void NiFile::Seek(int32_t iNumBytes)
{
	Seek(iNumBytes, SEEK_CUR);
}

void NiFile::GetBufferInfo(BufferInfo& kInfo)
{
	kInfo.pvBuffer = m_pBuffer;
	kInfo.uiTotalSize = GetFileSize();
	kInfo.uiBufferAllocSize = m_uiBufferAllocSize;
	kInfo.uiBufferReadSize = m_uiBufferReadSize;
	kInfo.uiBufferPos = m_uiPos;
	kInfo.uiStreamPos = GetPosition();
}

uint32_t NiFile::ReadNoSwap(NiBinaryStream* pkStream, void* pvBuffer, uint32_t uiBytes, uint32_t*, uint32_t)
{
	return static_cast<NiFile*>(pkStream)->FileRead(pvBuffer, uiBytes);
}

uint32_t NiFile::WriteNoSwap(NiBinaryStream* pkStream, const void* pvBuffer, uint32_t uiBytes, uint32_t*, uint32_t)
{
	return static_cast<NiFile*>(pkStream)->FileWrite(pvBuffer, uiBytes);
}

uint32_t NiFile::ReadAndSwap(NiBinaryStream* pkStream, void* pvBuffer, uint32_t uiBytes, uint32_t* puiComponentSizes, uint32_t uiNumComponents)
{
	if (!uiBytes)
		return 0;
	uint32_t uiRead = static_cast<NiFile*>(pkStream)->FileRead(pvBuffer, uiBytes);
	DoByteSwap(pvBuffer, uiBytes, puiComponentSizes, uiNumComponents);
	return uiRead;
}

uint32_t NiFile::WriteAndSwap(NiBinaryStream* pkStream, const void* pvBuffer, uint32_t uiBytes, uint32_t* puiComponentSizes, uint32_t uiNumComponents)
{
	if (!uiBytes)
		return 0;
	void* pvCopy = _NiMalloc(uiBytes);
	memcpy(pvCopy, pvBuffer, uiBytes);
	DoByteSwap(pvCopy, uiBytes, puiComponentSizes, uiNumComponents);
	uint32_t uiWritten = static_cast<NiFile*>(pkStream)->FileWrite(pvCopy, uiBytes);
	_NiFree(pvCopy);
	return uiWritten;
}
