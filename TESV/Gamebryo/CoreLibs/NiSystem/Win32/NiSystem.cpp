#include "Gamebryo/CoreLibs/NiSystem/NiSystem.h"
#include "Gamebryo/CoreLibs/NiSystem/NiMemoryDefines.h"

#include <windows.h>
#include <string.h>
#include <sys/stat.h>

bool g_bNiSystemFirstTime = true;
LARGE_INTEGER g_kNiSystemFrequency;
LARGE_INTEGER g_kNiSystemInitialCounter;

int NiStricmp(const char* pcFirst, const char* pcSecond)
{
	return _stricmp(pcFirst, pcSecond);
}

int NiStrnicmp(const char* pcFirst, const char* pcSecond, size_t stCount)
{
	return _strnicmp(pcFirst, pcSecond, stCount);
}

char* NiStrdup(const char* pcString)
{
	if (!pcString)
		return nullptr;
	size_t stLength = strlen(pcString);
	char* pcCopy = static_cast<char*>(_NiMalloc(stLength + 1));
	memcpy(pcCopy, pcString, stLength);
	pcCopy[stLength] = '\0';
	return pcCopy;
}

float NiGetCurrentTimeInSec()
{
	if (g_bNiSystemFirstTime)
	{
		QueryPerformanceFrequency(&g_kNiSystemFrequency);
		QueryPerformanceCounter(&g_kNiSystemInitialCounter);
		g_bNiSystemFirstTime = false;
	}
	LARGE_INTEGER kCounter;
	QueryPerformanceCounter(&kCounter);
	int64_t iDelta = static_cast<int64_t>(static_cast<uint64_t>(kCounter.QuadPart) - static_cast<uint64_t>(g_kNiSystemInitialCounter.QuadPart));
	return static_cast<float>(static_cast<double>(iDelta) / static_cast<double>(g_kNiSystemFrequency.QuadPart));
}

void NiStandardizeFilePath(char* pcPath)
{
	if (!pcPath)
		return;
	size_t stLength = strlen(pcPath);
	size_t stStored = 0;
	bool bPreviousSlash = false;
	for (size_t stIndex = 0; stIndex < stLength; ++stIndex)
	{
		char c = pcPath[stIndex];
		if (c == '/')
			c = '\\';
		if (c != '\\' || !bPreviousSlash || stIndex <= 1)
		{
			pcPath[stStored++] = c;
			bPreviousSlash = c == '\\';
		}
	}
	pcPath[stStored] = '\0';
}

uint32_t NiGetFileSize(const char* pcFilename)
{
	struct _stat64i32 kStat;
	if (_stat64i32(pcFilename, &kStat))
		return 0;
	return static_cast<uint32_t>(kStat.st_size);
}

uint32_t NiGetPerformanceCounter()
{
	LARGE_INTEGER kCounter;
	QueryPerformanceCounter(&kCounter);
	return kCounter.LowPart;
}
