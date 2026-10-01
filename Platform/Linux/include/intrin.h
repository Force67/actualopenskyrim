#pragma once

#include <immintrin.h>

inline unsigned char _BitScanReverse64(unsigned long* apIndex, unsigned long long auiMask)
{
	if (!auiMask)
		return 0;
	*apIndex = 63 - __builtin_clzll(auiMask);
	return 1;
}
