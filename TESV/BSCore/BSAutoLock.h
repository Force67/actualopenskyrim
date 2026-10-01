#pragma once

struct BSAutoLockDefaultPolicy
{
	template <class LockType>
	static void Lock(LockType& arLock) { arLock.Lock(); }

	template <class LockType>
	static void Unlock(LockType& arLock) { arLock.Unlock(); }
};

template <class LockType, class Policy = BSAutoLockDefaultPolicy>
class BSAutoLock
{
public:
	BSAutoLock(LockType& arLock) : pLock(&arLock) { Policy::Lock(arLock); }
	~BSAutoLock()
	{
		if (pLock)
			Policy::Unlock(*pLock);
	}

private:
	LockType* pLock;
};
