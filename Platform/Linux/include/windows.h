#pragma once

#include <stdlib.h>
#include <string.h>

// The subset of the Win32 API the engine uses, implemented for Linux. Engine
// code keeps calling Win32 as the original does; this header grows as more of
// it is ported.

#include <cstdint>

#include <sched.h>
#include <time.h>
#include <unistd.h>

using BOOL = int;
using DWORD = uint32_t;
using LONG = int32_t;
#define WINAPI
constexpr BOOL TRUE = 1;
constexpr BOOL FALSE = 0;

using HANDLE = void*;
using ULONG_PTR = uintptr_t;

#define __try try
#define __except(filter) catch (...)
constexpr int EXCEPTION_CONTINUE_EXECUTION = -1;
void RaiseException(DWORD dwExceptionCode, DWORD dwExceptionFlags, DWORD nNumberOfArguments, const ULONG_PTR* lpArguments);

constexpr DWORD INFINITE = 0xFFFFFFFF;
constexpr DWORD WAIT_OBJECT_0 = 0;
constexpr DWORD WAIT_TIMEOUT = 258;
constexpr DWORD WAIT_FAILED = 0xFFFFFFFF;

HANDLE CreateSemaphoreA(void* lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, const char* lpName);
HANDLE CreateSemaphoreW(void* lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, const wchar_t* lpName);
BOOL ReleaseSemaphore(HANDLE hSemaphore, LONG lReleaseCount, LONG* lpPreviousCount);
DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
BOOL CloseHandle(HANDLE hObject);
[[noreturn]] void ExitThread(DWORD dwExitCode);
using LPTHREAD_START_ROUTINE = DWORD (WINAPI*)(void*);
HANDLE GetCurrentThread();
HANDLE CreateThread(void* lpThreadAttributes, size_t dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress,
	void* lpParameter, DWORD dwCreationFlags, DWORD* lpThreadId);
DWORD ResumeThread(HANDLE hThread);
DWORD SuspendThread(HANDLE hThread);
uintptr_t SetThreadAffinityMask(HANDLE hThread, uintptr_t dwThreadAffinityMask);
BOOL SetThreadPriority(HANDLE hThread, int nPriority);

union LARGE_INTEGER
{
	struct
	{
		DWORD LowPart;
		LONG HighPart;
	};
	int64_t QuadPart;
};

BOOL QueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount);
BOOL QueryPerformanceFrequency(LARGE_INTEGER* lpFrequency);

inline DWORD GetCurrentThreadId()
{
	return static_cast<DWORD>(gettid());
}

inline void Sleep(DWORD dwMilliseconds)
{
	if (!dwMilliseconds)
	{
		sched_yield();
		return;
	}
	timespec ts{ static_cast<time_t>(dwMilliseconds / 1000), static_cast<long>(dwMilliseconds % 1000) * 1000000 };
	nanosleep(&ts, nullptr);
}

inline LONG InterlockedCompareExchange(LONG volatile* Destination, LONG Exchange, LONG Comperand)
{
	__atomic_compare_exchange_n(Destination, &Comperand, Exchange, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
	return Comperand;
}

inline LONG InterlockedExchange(LONG volatile* Target, LONG Value)
{
	return __atomic_exchange_n(Target, Value, __ATOMIC_SEQ_CST);
}

inline LONG InterlockedIncrement(LONG volatile* Addend)
{
	return __atomic_add_fetch(Addend, 1, __ATOMIC_SEQ_CST);
}

inline LONG InterlockedDecrement(LONG volatile* Addend)
{
	return __atomic_sub_fetch(Addend, 1, __ATOMIC_SEQ_CST);
}

inline void* InterlockedCompareExchangePointer(void* volatile* Destination, void* Exchange, void* Comperand)
{
	__atomic_compare_exchange_n(Destination, &Comperand, Exchange, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
	return Comperand;
}

// File API subset; file handles are file descriptors, INVALID_HANDLE_VALUE is
// the only invalid marker.
inline HANDLE INVALID_HANDLE_VALUE = reinterpret_cast<HANDLE>(-1);

constexpr DWORD GENERIC_READ = 0x80000000;
constexpr DWORD GENERIC_WRITE = 0x40000000;
constexpr DWORD FILE_SHARE_READ = 0x1;

constexpr DWORD CREATE_NEW = 1;
constexpr DWORD CREATE_ALWAYS = 2;
constexpr DWORD OPEN_EXISTING = 3;
constexpr DWORD OPEN_ALWAYS = 4;
constexpr DWORD TRUNCATE_EXISTING = 5;

constexpr DWORD FILE_FLAG_WRITE_THROUGH = 0x80000000;
constexpr DWORD FILE_FLAG_NO_BUFFERING = 0x20000000;
constexpr DWORD FILE_FLAG_OVERLAPPED = 0x40000000;
constexpr DWORD FILE_FLAG_SEQUENTIAL_SCAN = 0x08000000;

constexpr DWORD FILE_BEGIN = 0;
constexpr DWORD FILE_CURRENT = 1;
constexpr DWORD FILE_END = 2;

constexpr DWORD NO_ERROR = 0;
constexpr DWORD ERROR_FILE_NOT_FOUND = 2;
constexpr DWORD ERROR_PATH_NOT_FOUND = 3;
constexpr DWORD ERROR_ACCESS_DENIED = 5;
constexpr DWORD ERROR_WRITE_PROTECT = 19;
constexpr DWORD ERROR_ALREADY_EXISTS = 183;
constexpr DWORD ERROR_GEN_FAILURE = 31;

struct FILETIME
{
	DWORD dwLowDateTime;
	DWORD dwHighDateTime;
};

struct OVERLAPPED
{
	uintptr_t Internal;
	uintptr_t InternalHigh;
	union
	{
		struct
		{
			DWORD Offset;
			DWORD OffsetHigh;
		};
		void* Pointer;
	};
	HANDLE hEvent;
};
using LPOVERLAPPED = OVERLAPPED*;
using LPOVERLAPPED_COMPLETION_ROUTINE = void (*)(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);

DWORD GetLastError();
void SetLastError(DWORD dwErrCode);
HANDLE CreateFileA(const char* lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, void* lpSecurityAttributes,
	DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
BOOL ReadFile(HANDLE hFile, void* lpBuffer, DWORD nNumberOfBytesToRead, DWORD* lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
BOOL WriteFile(HANDLE hFile, const void* lpBuffer, DWORD nNumberOfBytesToWrite, DWORD* lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
BOOL ReadFileEx(HANDLE hFile, void* lpBuffer, DWORD nNumberOfBytesToRead, LPOVERLAPPED lpOverlapped,
	LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);
BOOL WriteFileEx(HANDLE hFile, const void* lpBuffer, DWORD nNumberOfBytesToWrite, LPOVERLAPPED lpOverlapped,
	LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);
BOOL SetFilePointerEx(HANDLE hFile, LARGE_INTEGER liDistanceToMove, LARGE_INTEGER* lpNewFilePointer, DWORD dwMoveMethod);
BOOL GetFileSizeEx(HANDLE hFile, LARGE_INTEGER* lpFileSize);
BOOL SetEndOfFile(HANDLE hFile);
BOOL FlushFileBuffers(HANDLE hFile);
BOOL GetFileTime(HANDLE hFile, FILETIME* lpCreationTime, FILETIME* lpLastAccessTime, FILETIME* lpLastWriteTime);
BOOL SetFileTime(HANDLE hFile, const FILETIME* lpCreationTime, const FILETIME* lpLastAccessTime, const FILETIME* lpLastWriteTime);
BOOL DeleteFileA(const char* lpFileName);
BOOL CopyFileA(const char* lpExistingFileName, const char* lpNewFileName, BOOL bFailIfExists);
BOOL MoveFileA(const char* lpExistingFileName, const char* lpNewFileName);
DWORD SleepEx(DWORD dwMilliseconds, BOOL bAlertable);


// Memory status and message boxes; the engine only uses these at startup.
struct MEMORYSTATUSEX
{
	DWORD dwLength;
	DWORD dwMemoryLoad;
	uint64_t ullTotalPhys;
	uint64_t ullAvailPhys;
	uint64_t ullTotalPageFile;
	uint64_t ullAvailPageFile;
	uint64_t ullTotalVirtual;
	uint64_t ullAvailVirtual;
	uint64_t ullAvailExtendedVirtual;
};

BOOL GlobalMemoryStatusEx(MEMORYSTATUSEX* lpBuffer);
DWORD GetTickCount();

constexpr unsigned int MB_ICONERROR = 0x10;
int MessageBoxA(void* hWnd, const char* lpText, const char* lpCaption, unsigned int uType);

constexpr DWORD FILE_ATTRIBUTE_READONLY = 0x1;
constexpr DWORD FILE_ATTRIBUTE_HIDDEN = 0x2;
constexpr DWORD FILE_ATTRIBUTE_SYSTEM = 0x4;
constexpr DWORD FILE_ATTRIBUTE_DIRECTORY = 0x10;
constexpr DWORD FILE_ATTRIBUTE_ARCHIVE = 0x20;
constexpr DWORD FILE_ATTRIBUTE_NORMAL = 0x80;

constexpr DWORD ERROR_NO_MORE_FILES = 18;

struct WIN32_FIND_DATAA
{
	DWORD dwFileAttributes;
	FILETIME ftCreationTime;
	FILETIME ftLastAccessTime;
	FILETIME ftLastWriteTime;
	DWORD nFileSizeHigh;
	DWORD nFileSizeLow;
	DWORD dwReserved0;
	DWORD dwReserved1;
	char cFileName[260];
	char cAlternateFileName[14];
};
using LPWIN32_FIND_DATAA = WIN32_FIND_DATAA*;

HANDLE FindFirstFileA(const char* lpFileName, LPWIN32_FIND_DATAA lpFindFileData);
BOOL FindNextFileA(HANDLE hFindFile, LPWIN32_FIND_DATAA lpFindFileData);
BOOL FindClose(HANDLE hFindFile);
BOOL CreateDirectoryA(const char* lpPathName, void* lpSecurityAttributes);

// MSVC CRT functions the engine calls.
int64_t _time64(int64_t* apTime);
#include <cstdlib>
#include <cstring>
#include <malloc.h>

inline void* _aligned_malloc(size_t size, size_t alignment)
{
	void* p = nullptr;
	return posix_memalign(&p, alignment < sizeof(void*) ? sizeof(void*) : alignment, size) ? nullptr : p;
}

inline void _aligned_free(void* memblock)
{
	free(memblock);
}

inline size_t _msize(void* memblock)
{
	return malloc_usable_size(memblock);
}

inline int memcpy_s(void* dest, size_t destSize, const void* src, size_t count)
{
	if (count > destSize)
	{
		memset(dest, 0, destSize);
		return 34;  // ERANGE, as the MSVC CRT returns
	}
	memcpy(dest, src, count);
	return 0;
}

constexpr DWORD MEM_COMMIT = 0x1000;
constexpr DWORD MEM_RESERVE = 0x2000;
constexpr DWORD MEM_DECOMMIT = 0x4000;
constexpr DWORD MEM_RELEASE = 0x8000;
constexpr DWORD PAGE_READWRITE = 4;

void* VirtualAlloc(void* lpAddress, size_t dwSize, DWORD flAllocationType, DWORD flProtect);
BOOL VirtualFree(void* lpAddress, size_t dwSize, DWORD dwFreeType);

struct CRITICAL_SECTION
{
	void* DebugInfo;
	LONG LockCount;
	LONG RecursionCount;
	void* OwningThread;
	void* LockSemaphore;
	uintptr_t SpinCount;
};
static_assert(sizeof(CRITICAL_SECTION) == 40);

void InitializeCriticalSection(CRITICAL_SECTION* lpCriticalSection);
void DeleteCriticalSection(CRITICAL_SECTION* lpCriticalSection);
void EnterCriticalSection(CRITICAL_SECTION* lpCriticalSection);
void LeaveCriticalSection(CRITICAL_SECTION* lpCriticalSection);

constexpr DWORD TLS_OUT_OF_INDEXES = 0xFFFFFFFF;
DWORD TlsAlloc();
BOOL TlsFree(DWORD dwTlsIndex);
void* TlsGetValue(DWORD dwTlsIndex);
BOOL TlsSetValue(DWORD dwTlsIndex, void* lpTlsValue);

inline int memmove_s(void* dest, size_t destSize, const void* src, size_t count)
{
	if (!count)
		return 0;
	if (!dest || !src)
		abort();
	if (count > destSize)
		return 34;
	memmove(dest, src, count);
	return 0;
}

using HMODULE = void*;
HMODULE GetModuleHandleA(const char* lpModuleName);
DWORD GetModuleFileNameA(HMODULE hModule, char* lpFilename, DWORD nSize);

struct SYSTEM_INFO
{
	union
	{
		DWORD dwOEMId;
		struct
		{
			uint16_t wProcessorArchitecture;
			uint16_t wReserved;
		};
	};
	DWORD dwPageSize;
	void* lpMinimumApplicationAddress;
	void* lpMaximumApplicationAddress;
	uintptr_t dwActiveProcessorMask;
	DWORD dwNumberOfProcessors;
	DWORD dwProcessorType;
	DWORD dwAllocationGranularity;
	uint16_t wProcessorLevel;
	uint16_t wProcessorRevision;
};
static_assert(sizeof(SYSTEM_INFO) == 48);
void GetSystemInfo(SYSTEM_INFO* lpSystemInfo);

constexpr DWORD INVALID_FILE_ATTRIBUTES = 0xFFFFFFFF;
constexpr DWORD FILE_ATTRIBUTE_REPARSE_POINT = 0x400;
DWORD GetFileAttributesA(const char* lpFileName);
