#pragma once

#include_next <sys/stat.h>
#include <cstdint>

#undef st_atime
#undef st_mtime
#undef st_ctime

struct _stat64i32
{
	uint32_t st_dev;
	uint16_t st_ino;
	uint16_t st_mode;
	int16_t st_nlink;
	int16_t st_uid;
	int16_t st_gid;
	uint32_t st_rdev;
	int32_t st_size;
	int64_t st_atime;
	int64_t st_mtime;
	int64_t st_ctime;
};
static_assert(sizeof(_stat64i32) == 48);

extern "C" int _stat64i32(const char* apFilename, struct _stat64i32* apStat);
