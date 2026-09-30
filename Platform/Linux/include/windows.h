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
