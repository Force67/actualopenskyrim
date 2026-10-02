#include "BSResource/BSResourceLooseFiles.h"

#include <windows.h>

namespace BSResource
{
	bool LooseFileLocation::FileExists(const char* apPath, uint64_t& arSize)
	{
		WIN32_FILE_ATTRIBUTE_DATA Info;
		if (!GetFileAttributesExA(apPath, GetFileExInfoStandard, &Info) || (Info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			return false;
		arSize = (static_cast<uint64_t>(Info.nFileSizeHigh) << 32) | Info.nFileSizeLow;
		return true;
	}

	bool LooseFileLocation::DirectoryExists(const char* apPath)
	{
		WIN32_FILE_ATTRIBUTE_DATA Info;
		return GetFileAttributesExA(apPath, GetFileExInfoStandard, &Info) && (Info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
	}

	bool LooseFileLocation::GetLocationFreeSpace(uint64_t& arFreeSpace) const
	{
		ULARGE_INTEGER Available;
		if (!GetDiskFreeSpaceExA(Prefix.pString, &Available, nullptr, nullptr))
			return false;
		arFreeSpace = Available.QuadPart;
		return true;
	}
}
