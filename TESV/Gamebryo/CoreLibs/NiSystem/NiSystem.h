#pragma once

#include <cstddef>
#include <cstdint>

int NiStricmp(const char* pcFirst, const char* pcSecond);
int NiStrnicmp(const char* pcFirst, const char* pcSecond, size_t stCount);
char* NiStrdup(const char* pcString);
float NiGetCurrentTimeInSec();
void NiStandardizeFilePath(char* pcPath);
uint32_t NiGetFileSize(const char* pcFilename);
uint32_t NiGetPerformanceCounter();
