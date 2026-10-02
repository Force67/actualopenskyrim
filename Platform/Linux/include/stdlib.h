#pragma once

#include_next <stdlib.h>

extern "C" void _invalid_parameter_noinfo();

extern "C" int _splitpath_s(const char* path, char* drive, size_t driveSize, char* dir, size_t dirSize, char* fname, size_t fnameSize, char* ext, size_t extSize);
