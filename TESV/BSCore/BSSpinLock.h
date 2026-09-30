#pragma once

#include "BSCore/BSSafeSleep.h"

#include <emmintrin.h>
#include <windows.h>

// A reentrant spin lock. Inlined everywhere; there are no out-of-line copies.
class BSSpinLock
{
public:
	BSSpinLock() :
		OwningThread(0),
		uiLockCount(0)
	{
	}

	void Lock()
	{
		const DWORD uiThread = GetCurrentThreadId();
		_mm_lfence();
		if (OwningThread == uiThread)
		{
			InterlockedIncrement(LockWord());
			return;
		}

		if (InterlockedCompareExchange(LockWord(), 1, 0))
		{
			_mm_pause();
			if (InterlockedCompareExchange(LockWord(), 1, 0))
			{
				BSSafeSleep kSleep;
				do
					kSleep.Wait();
				while (InterlockedCompareExchange(LockWord(), 1, 0));
			}
			_mm_lfence();
		}
		OwningThread = uiThread;
		_mm_sfence();
	}

	// Only the owning thread unlocks; a call from another thread does nothing.
	void Unlock()
	{
		_mm_lfence();
		if (OwningThread != GetCurrentThreadId())
			return;
		if (uiLockCount == 1)
		{
			OwningThread = 0;
			_mm_mfence();
			InterlockedCompareExchange(LockWord(), 0, 1);
		}
		else
		{
			InterlockedDecrement(LockWord());
		}
	}

	unsigned int OwningThread;
	volatile unsigned int uiLockCount;

private:
	volatile LONG* LockWord() { return reinterpret_cast<volatile LONG*>(&uiLockCount); }
};
static_assert(sizeof(BSSpinLock) == 8);
