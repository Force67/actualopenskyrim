#include "BSCore/BSReadWriteLock.h"

#include "BSCore/BSSafeSleep.h"

#include <emmintrin.h>
#include <windows.h>

namespace
{
	volatile LONG* LockWord(BSReadWriteLock* apLock)
	{
		return reinterpret_cast<volatile LONG*>(&apLock->uiLock);
	}

	constexpr LONG WRITE_LOCKED_ONCE = static_cast<LONG>(BSReadWriteLock::LOCKED_FOR_WRITE | 1);
}

BSReadWriteLock::BSReadWriteLock() :
	uiWriterThread(0),
	uiLock(0)
{
}

void BSReadWriteLock::LockForRead()
{
	if (uiWriterThread == GetCurrentThreadId())
	{
		InterlockedIncrement(LockWord(this));
		return;
	}

	BSSafeSleep kSleep;
	LONG iExpected = uiLock & LOCK_COUNT_MASK;
	LONG iPrevious = InterlockedCompareExchange(LockWord(this), iExpected + 1, iExpected);
	while (iPrevious != iExpected)
	{
		if (iPrevious < 0)
		{
			kSleep.Wait();
			iPrevious = uiLock;
		}
		iExpected = iPrevious & LOCK_COUNT_MASK;
		iPrevious = InterlockedCompareExchange(LockWord(this), iExpected + 1, iExpected);
	}
}

void BSReadWriteLock::LockForWrite()
{
	const DWORD uiThread = GetCurrentThreadId();
	if (uiWriterThread == uiThread)
	{
		InterlockedIncrement(LockWord(this));
		return;
	}

	BSSafeSleep kSleep;
	while (InterlockedCompareExchange(LockWord(this), WRITE_LOCKED_ONCE, 0))
		kSleep.Wait();
	uiWriterThread = uiThread;
	_mm_mfence();
}

void BSReadWriteLock::LockForReadAndWrite()
{
	if (uiWriterThread == GetCurrentThreadId())
	{
		InterlockedIncrement(LockWord(this));
		return;
	}

	BSSafeSleep kSleep;
	while (InterlockedCompareExchange(LockWord(this), 1, 0))
		kSleep.Wait();
}

void BSReadWriteLock::StartWriteLock()
{
	const DWORD uiThread = GetCurrentThreadId();
	if (uiWriterThread == uiThread)
		return;

	BSSafeSleep kSleep;
	while (InterlockedCompareExchange(LockWord(this), WRITE_LOCKED_ONCE, 1) != 1)
		kSleep.Wait();
	uiWriterThread = uiThread;
	_mm_mfence();
}

bool BSReadWriteLock::TryLockForRead()
{
	if (uiWriterThread == GetCurrentThreadId())
	{
		InterlockedIncrement(LockWord(this));
		return true;
	}

	LONG iExpected = uiLock & LOCK_COUNT_MASK;
	LONG iPrevious = InterlockedCompareExchange(LockWord(this), iExpected + 1, iExpected);
	while (iPrevious >= 0 && iPrevious != iExpected)
	{
		iExpected = iPrevious & LOCK_COUNT_MASK;
		iPrevious = InterlockedCompareExchange(LockWord(this), iExpected + 1, iExpected);
	}
	return iPrevious >= 0;
}

bool BSReadWriteLock::TryLockForWrite()
{
	const DWORD uiThread = GetCurrentThreadId();
	if (uiWriterThread == uiThread)
	{
		InterlockedIncrement(LockWord(this));
		return true;
	}

	if (InterlockedCompareExchange(LockWord(this), WRITE_LOCKED_ONCE, 0))
		return false;
	uiWriterThread = uiThread;
	_mm_mfence();
	return true;
}

void BSReadWriteLock::UnlockRead()
{
	InterlockedDecrement(LockWord(this));
}

void BSReadWriteLock::UnlockWrite()
{
	if (uiLock == static_cast<unsigned int>(WRITE_LOCKED_ONCE))
	{
		uiWriterThread = 0;
		_mm_mfence();
		InterlockedExchange(LockWord(this), 0);
	}
	else
	{
		InterlockedDecrement(LockWord(this));
	}
}

bool BSReadWriteLock::IsWritingThread()
{
	return GetCurrentThreadId() == uiWriterThread;
}

BSAutoReadAndWriteLock::BSAutoReadAndWriteLock(BSReadWriteLock& arLock) :
	rLock(&arLock)
{
	if (GetCurrentThreadId() == arLock.uiWriterThread)
		rLock->LockForWrite();
	else
		rLock->LockForReadAndWrite();
}

BSAutoReadAndWriteLock::~BSAutoReadAndWriteLock()
{
	if (GetCurrentThreadId() == rLock->uiWriterThread)
		rLock->UnlockWrite();
	else
		rLock->UnlockRead();
}
