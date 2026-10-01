#include "BSCore/BSSemaphore.h"

BSSemaphoreBase::BSSemaphoreBase(int aiCount, int aiMaxCount)
{
	hSemaphore = CreateSemaphoreW(nullptr, aiCount, aiMaxCount, nullptr);
}

BSSemaphoreBase::~BSSemaphoreBase()
{
	CloseHandle(hSemaphore);
}

int BSSemaphoreBase::Wait()
{
	return static_cast<int>(WaitForSingleObject(hSemaphore, INFINITE));
}

int BSSemaphoreBase::Signal(int aiCount)
{
	return ReleaseSemaphore(hSemaphore, aiCount, nullptr);
}

int BSSemaphore::Wait()
{
	return BSSemaphoreBase::Wait();
}

int BSSemaphore::Signal(int aiCount)
{
	return BSSemaphoreBase::Signal(aiCount);
}
