#pragma once

#include <cstdint>
#include <cstddef>

#include <windows.h>

// Win32 file layer; a thin owning wrapper around a file handle. Async access
// goes through BSSystemFileAsyncFunctor, whose OVERLAPPED doubles as the
// completion state.

class BSSystemFileAsyncFunctor;

class BSSystemFile
{
public:
	enum ErrorCode : uint32_t
	{
		EC_NONE = 0,
		EC_INVALID_FILE = 1,
		EC_NOT_EXIST = 2,
		EC_ALREADY_EXISTS = 3,
		EC_INVALID_PATH = 4,
		EC_NO_PERMISSION = 5,
		EC_FAILED = 6,
		// Shared code stops at EC_FAILED; this build reports any unmatched
		// error and a synchronous call on an async file as 7.
		EC_NOT_OPEN = 7,
	};

	enum AccessMode : uint32_t
	{
		AM_RDONLY = 0,
		AM_RDWR = 1,
		AM_WRONLY = 2,
	};

	enum OpenMode : uint32_t
	{
		OM_NONE = 0,
		OM_APPEND = 1,
		OM_CREAT = 2,
		OM_TRUNC = 3,
		OM_EXCL = 4,
	};

	enum SeekMode : uint32_t
	{
		SM_SET = 0,
		SM_CUR = 1,
		SM_END = 2,
	};

	struct Info
	{
		FILETIME AccessTime;
		FILETIME ModifyTime;
		FILETIME CreateTime;
		uint64_t uiFileSize;
	};

	BSSystemFile();
	BSSystemFile(const char* pFileName, AccessMode eAccessMode, OpenMode eOpenMode, bool abAllowAsync);
	~BSSystemFile();
	BSSystemFile(BSSystemFile&& arFile);
	BSSystemFile& operator=(BSSystemFile&& arFile);
	bool operator==(const BSSystemFile& arFile) const;

	ErrorCode GetErrorCode() const;

	ErrorCode DoOpen(const char* pFileName, AccessMode eAccessMode, OpenMode eOpenMode);
	ErrorCode DoRead(void* apvBuffer, uint32_t auiBytes, uint64_t* apuiBytesRead);
	ErrorCode DoWrite(const void* apvBuffer, uint32_t auiBytes, uint64_t* apuiBytesWritten);
	ErrorCode DoSeek(int64_t aiOffset, SeekMode eSeekMode, int64_t* apoiNewPosition);
	ErrorCode DoGetSize(uint64_t* apuiSize);
	ErrorCode DoSetEndOfFile();
	ErrorCode DoFlush();
	ErrorCode DoGetInfo(Info* apInfo);
	bool DoClose();

	ErrorCode DoReadAsync(void* apvBuffer, uint32_t auiBytes, int64_t aiOffset, BSSystemFileAsyncFunctor* apFunctor);
	ErrorCode DoWriteAsync(const void* apvBuffer, uint32_t auiBytes, int64_t aiOffset, BSSystemFileAsyncFunctor* apFunctor);

	static ErrorCode Delete(const char* pFileName);
	static ErrorCode Copy(const char* pFrom, const char* pTo, bool abFailIfExists);
	static ErrorCode Move(const char* pFrom, const char* pTo, bool abNoOverwrite);
	static ErrorCode SetTime(const char* pFileName, const FILETIME* apCreationTime, const FILETIME* apAccessTime,
		const FILETIME* apModifyTime);
	static ErrorCode TranslateErrorCode(DWORD uError);

	// Bit 31: asynchronous access allowed; the low bits carry the last error.
	volatile uint32_t uiFlags;
	HANDLE hFile;
};
static_assert(sizeof(BSSystemFile) == 0x10);
static_assert(offsetof(BSSystemFile, hFile) == 0x8);

inline bool BSSystemFile::operator==(const BSSystemFile& arFile) const
{
	return hFile == arFile.hFile;
}

inline BSSystemFile::ErrorCode BSSystemFile::GetErrorCode() const
{
	return static_cast<ErrorCode>(uiFlags & 0x5FFFFFFF);
}

class BSSystemFileAsyncFunctor
{
public:
	virtual ~BSSystemFileAsyncFunctor();
	virtual void Process(BSSystemFile::ErrorCode eErrorCode, uint64_t auiBytesProcessed);

	// 0 idle, -1 in flight, 1 completed
	volatile int32_t uiStatus;
	uint32_t uiThreadId;
	OVERLAPPED Overlapped;
};
static_assert(sizeof(BSSystemFileAsyncFunctor) == 0x30);
static_assert(offsetof(BSSystemFileAsyncFunctor, uiStatus) == 0x8);
static_assert(offsetof(BSSystemFileAsyncFunctor, Overlapped) == 0x10);

