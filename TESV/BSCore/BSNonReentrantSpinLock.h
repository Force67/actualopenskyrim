#pragma once

class BSNonReentrantSpinLock
{
public:
	volatile unsigned int uiLock;
};
static_assert(sizeof(BSNonReentrantSpinLock) == 4);
