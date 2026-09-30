#pragma once

// A reentrant reader/writer spin lock. uiLock holds the reader count, or the
// writer's recursion count with LOCKED_FOR_WRITE set.
class BSReadWriteLock
{
public:
	enum : unsigned int
	{
		LOCKED_FOR_WRITE = 0x80000000,
		LOCK_COUNT_MASK = 0x0FFFFFFF,
	};

	BSReadWriteLock();

	void LockForRead();
	void LockForWrite();
	// Locks for reading, in a way StartWriteLock can later upgrade.
	void LockForReadAndWrite();
	void StartWriteLock();
	bool TryLockForRead();
	bool TryLockForWrite();
	void UnlockRead();
	void UnlockWrite();
	bool IsWritingThread();

	unsigned int uiWriterThread;
	volatile unsigned int uiLock;
};
static_assert(sizeof(BSReadWriteLock) == 8);

// Write lock when this thread already writes, else a read-and-write lock.
class BSAutoReadAndWriteLock
{
public:
	BSAutoReadAndWriteLock(BSReadWriteLock& arLock);
	~BSAutoReadAndWriteLock();

	BSReadWriteLock* rLock;
};
static_assert(sizeof(BSAutoReadAndWriteLock) == 8);
