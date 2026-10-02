#pragma once

#include_next <string.h>

extern "C" int _stricmp(const char* apFirst, const char* apSecond);
extern "C" int _strnicmp(const char* apFirst, const char* apSecond, size_t auiCount);
extern "C" int strcpy_s(char* apDest, size_t auiSize, const char* apSource);
extern "C" int strcat_s(char* apDest, size_t auiSize, const char* apSource);

extern "C" int strncpy_s(char* apDest, size_t auiSize, const char* apSource, size_t auiCount);

extern "C" char* strtok_s(char* apString, const char* apDelimiters, char** appContext);
