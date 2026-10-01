#pragma once

#include <windows.h>

class BSSemaphoreBase
{
public:
	HANDLE hSemaphore;

protected:
	BSSemaphoreBase(int aiCount, int aiMaxCount);
	~BSSemaphoreBase();
	int Wait();
	int Signal(int aiCount);
};
static_assert(sizeof(BSSemaphoreBase) == 8);

class BSSemaphore : public BSSemaphoreBase
{
public:
	BSSemaphore(int aiCount = 0, int aiMaxCount = 1) : BSSemaphoreBase(aiCount, aiMaxCount) {}
	int Wait();
	int Signal(int aiCount = 1);
};
static_assert(sizeof(BSSemaphore) == 8);
