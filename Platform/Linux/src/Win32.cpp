#include <windows.h>

#include <cstdio>

#include <sys/sysinfo.h>

BOOL GlobalMemoryStatusEx(MEMORYSTATUSEX* lpBuffer)
{
	struct sysinfo info;
	if (sysinfo(&info))
		return 0;
	const uint64_t unit = info.mem_unit;
	lpBuffer->dwMemoryLoad = 0;
	lpBuffer->ullTotalPhys = info.totalram * unit;
	lpBuffer->ullAvailPhys = info.freeram * unit;
	lpBuffer->ullTotalPageFile = (info.totalram + info.totalswap) * unit;
	lpBuffer->ullAvailPageFile = (info.freeram + info.freeswap) * unit;
	lpBuffer->ullTotalVirtual = UINT64_MAX;
	lpBuffer->ullAvailVirtual = UINT64_MAX;
	lpBuffer->ullAvailExtendedVirtual = 0;
	return 1;
}

int MessageBoxA(void*, const char* lpText, const char* lpCaption, unsigned int)
{
	fprintf(stderr, "%s: %s\n", lpCaption, lpText);
	return 1;
}
