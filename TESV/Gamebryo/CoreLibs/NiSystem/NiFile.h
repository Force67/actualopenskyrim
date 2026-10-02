#pragma once

#include "NiBinaryStream.h"
#include <stdio.h>

#undef CreateDirectory

class NiFile : public NiBinaryStream
{
public:
	enum OpenMode { READ_ONLY, WRITE_ONLY, APPEND_ONLY };
	using FILECREATEFUNC = NiFile* (*)(const char*, OpenMode, uint32_t);
	using FILEACCESSFUNC = bool (*)(const char*, OpenMode);
	using CREATEDIRFUNC = bool (*)(const char*);
	using DIREXISTSFUNC = bool (*)(const char*);
	NiFile(const char* pcName, OpenMode eMode, uint32_t uiBufferSize = 32768);
	~NiFile() override;
	operator bool() const override;
	void Seek(int32_t iNumBytes) override;
	void GetBufferInfo(BufferInfo& kInfo) override;
	void SetEndianSwap(bool bDoSwap) override;
	virtual void Seek(int32_t iOffset, int32_t iWhence);
	virtual uint32_t GetFileSize() const;
	uint32_t DiskRead(void* pvBuffer, uint32_t uiBytes);
	uint32_t GetActualFilePosition();
	char* GetBuffer();
	static NiFile* GetFile(const char* pcName, OpenMode eMode, uint32_t uiBufferSize = 32768);
	static bool Access(const char* pcName, OpenMode eMode);
	static bool CreateDirectory(const char* pcDir);
	static bool DirectoryExists(const char* pcDir);
	static void SetFileCreateFunc(FILECREATEFUNC pfnFunc);
	static void SetFileAccessFunc(FILEACCESSFUNC pfnFunc);
	static void SetCreateDirectoryFunc(CREATEDIRFUNC pfnFunc);
	static void SetDirectoryExistsFunc(DIREXISTSFUNC pfnFunc);

protected:
	NiFile();
	bool Flush();
	uint32_t DiskWrite(const void* pvBuffer, uint32_t uiBytes);
	uint32_t FileRead(void* pvBuffer, uint32_t uiBytes);
	uint32_t FileWrite(const void* pvBuffer, uint32_t uiBytes);
	static bool DefaultDirectoryExistsFunc(const char* pcDir);
	static bool DefaultCreateDirectoryFunc(const char* pcDir);
	static NiFile* DefaultFileCreateFunc(const char* pcName, OpenMode eMode, uint32_t uiBufferSize);
	static bool DefaultFileAccessFunc(const char* pcName, OpenMode eMode);
	static uint32_t ReadNoSwap(NiBinaryStream* pkStream, void* pvBuffer, uint32_t uiBytes, uint32_t* puiComponentSizes, uint32_t uiNumComponents);
	static uint32_t WriteNoSwap(NiBinaryStream* pkStream, const void* pvBuffer, uint32_t uiBytes, uint32_t* puiComponentSizes, uint32_t uiNumComponents);
	static uint32_t ReadAndSwap(NiBinaryStream* pkStream, void* pvBuffer, uint32_t uiBytes, uint32_t* puiComponentSizes, uint32_t uiNumComponents);
	static uint32_t WriteAndSwap(NiBinaryStream* pkStream, const void* pvBuffer, uint32_t uiBytes, uint32_t* puiComponentSizes, uint32_t uiNumComponents);

	uint32_t m_uiBufferAllocSize;
	uint32_t m_uiBufferReadSize;
	uint32_t m_uiPos;
	uint32_t m_uiCurrentFilePos;
	char* m_pBuffer;
	FILE* m_pFile;
	OpenMode m_eMode;
	bool m_bGood;
	static FILECREATEFUNC ms_pfnFileCreateFunc;
	static FILEACCESSFUNC ms_pfnFileAccessFunc;
	static CREATEDIRFUNC ms_pfnCreateDirFunc;
	static DIREXISTSFUNC ms_pfnDirExistsFunc;
};
static_assert(sizeof(NiFile) == 72);
