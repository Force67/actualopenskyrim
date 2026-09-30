#pragma once

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

constexpr unsigned int MB_ICONERROR = 0x10;
int MessageBoxA(void* hWnd, const char* lpText, const char* lpCaption, unsigned int uType);

// MSVC CRT functions the engine calls.
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
