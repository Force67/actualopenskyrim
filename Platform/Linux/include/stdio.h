#pragma once
#include_next <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif
int fopen_s(FILE** pFile, const char* filename, const char* mode);
#ifdef __cplusplus
}
#endif
