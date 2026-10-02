#include "BSSystem/BSSystemFile.h"

BSSystemFile::BSSystemFile()
{
	uiFlags = EC_INVALID_FILE;
	hFile = INVALID_HANDLE_VALUE;
}

BSSystemFile::BSSystemFile(const char* pFileName, AccessMode eAccessMode, OpenMode eOpenMode, bool abAllowAsync)
{
	uiFlags = static_cast<uint32_t>(abAllowAsync) << 31;
	hFile = INVALID_HANDLE_VALUE;
	uiFlags = (uiFlags & 0xA0000000) | DoOpen(pFileName, eAccessMode, eOpenMode);
}

BSSystemFile::~BSSystemFile()
{
	DoClose();
}

BSSystemFile::BSSystemFile(BSSystemFile&& arFile)
{
	uiFlags = arFile.uiFlags;
	hFile = arFile.hFile;
	arFile.uiFlags = EC_INVALID_FILE;
	arFile.hFile = INVALID_HANDLE_VALUE;
}

BSSystemFile& BSSystemFile::operator=(BSSystemFile&& arFile)
{
	DoClose();
	uiFlags = arFile.uiFlags;
	hFile = arFile.hFile;
	arFile.uiFlags = EC_INVALID_FILE;
	arFile.hFile = INVALID_HANDLE_VALUE;
	return *this;
}
