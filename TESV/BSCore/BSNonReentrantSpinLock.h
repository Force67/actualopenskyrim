#pragma once

#include <emmintrin.h>
#include <windows.h>

class BSNonReentrantSpinLock
{
public:
	void Lock()
	{
		while (InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&uiLock), 1, 0))
			Sleep(0);
		_mm_mfence();
	}
	bool TryLock()
	{
		if (InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&uiLock), 1, 0))
			return false;
		_mm_mfence();
		return true;
	}
	void Unlock()
	{
		uiLock = 0;
		_mm_mfence();
	}
	volatile unsigned int uiLock;
};
static_assert(sizeof(BSNonReentrantSpinLock) == 4);
