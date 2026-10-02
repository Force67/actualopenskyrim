#include "BSSystem/BSSystemDir.h"

bool BSstrempty(const char* pStr)
{
	return !pStr || !*pStr;
}

BSSystemFile::ErrorCode BSSystemDir::DoOpen(const char* pDirectory)
{
	if (!pDirectory)
		return BSSystemFile::EC_INVALID_PATH;
	strcpy_s(cDirectory, sizeof(cDirectory), pDirectory);
	strcat_s(cDirectory, sizeof(cDirectory), "\\*");
	hFind = nullptr;
	eLastError = BSSystemFile::EC_NONE;
	return BSSystemFile::EC_NONE;
}

BSSystemFile::ErrorCode BSSystemDir::GetNext(bool* abEnd)
{
	if (hFind)
	{
		if (hFind == INVALID_HANDLE_VALUE)
		{
			eLastError = BSSystemFile::EC_NOT_OPEN;
			return BSSystemFile::EC_NOT_OPEN;
		}
		*abEnd = !FindNextFileA(hFind, &FindData);
		return eLastError;
	}
	hFind = FindFirstFileA(cDirectory, &FindData);
	eLastError = hFind == INVALID_HANDLE_VALUE ? BSSystemFile::EC_NOT_OPEN : BSSystemFile::EC_NONE;
	return eLastError;
}

BSSystemFile::ErrorCode BSSystemDir::Close()
{
	if (hFind != INVALID_HANDLE_VALUE)
		FindClose(hFind);
	return eLastError;
}

BSSystemFile::ErrorCode BSSystemDir::MakeDir(const char* pDirectory)
{
	CreateDirectoryA(pDirectory, nullptr);
	return BSSystemFile::TranslateErrorCode(GetLastError());
}

bool BSSystemDir::MakePartialDirPath(const char* pPath, uint64_t uiOffset)
{
	char cBuffer[0x110];
	BSSystemFile::ErrorCode uResult = BSSystemFile::EC_NONE;
	bool bGood = true;
	if (uiOffset)
		strcpy_s(cBuffer, 0x104, pPath);
	const char* pSrc = pPath + uiOffset;
	char* pDest = cBuffer + uiOffset;
	do
	{
		char cChar = *pSrc++;
		if (!cChar)
			break;
		if (cChar == '/' || cChar == '\\')
		{
			*pDest = 0;
			if (!BSstrempty(cBuffer) && pDest[-1] != ':')
				uResult = MakeDir(cBuffer);
			bGood = !uResult || uResult == BSSystemFile::EC_ALREADY_EXISTS;
			*pDest = '\\';
		}
		else
		{
			*pDest = cChar;
		}
		++pDest;
	} while (bGood);
	return bGood;
}

bool BSSystemDir::MakeEntireDirPath(const char* pPath)
{
	char cBuffer[0x110];
	BSSystemFile::ErrorCode uResult = BSSystemFile::EC_NONE;
	bool bGood = true;
	const char* pSrc = pPath;
	char* pDest = cBuffer;
	do
	{
		char cChar = *pSrc++;
		if (!cChar)
			break;
		if (cChar == '/' || cChar == '\\')
		{
			*pDest = 0;
			if (!BSstrempty(cBuffer) && pDest[-1] != ':')
				uResult = MakeDir(cBuffer);
			bGood = !uResult || uResult == BSSystemFile::EC_ALREADY_EXISTS;
			*pDest = '\\';
		}
		else
		{
			*pDest = cChar;
		}
		++pDest;
	} while (bGood);
	return bGood;
}
