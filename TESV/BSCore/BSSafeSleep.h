#pragma once

#include <windows.h>

// Back-off for spin loops: yields for the first 10000 waits, then sleeps.
struct BSSafeSleep
{
	void Wait()
	{
		if (uiCounter >= 10000)
		{
			Sleep(1);
		}
		else
		{
			++uiCounter;
			Sleep(0);
		}
	}

	unsigned int uiCounter = 0;
};
