#include "BSSystem/FilePathUtilities.h"

#include <cstring>

bool FilePathUtilities::Join(const char* apDirPathStart, const char* apDirPathEnd, BSStaticStringT<260>& arsReturnPath)
{
	BSStaticStringT<260> sDirPathEnd;
	sDirPathEnd.SetFromString(apDirPathEnd, 0);
	bool bAppendSlash = false;
	if (apDirPathStart)
	{
		uint32_t uLen = strlen(apDirPathStart);
		if (uLen > 1)
			bAppendSlash = (uLen - 1) != FindLastSlash(apDirPathStart);
	}
	arsReturnPath.SetFromString(apDirPathStart, 0);
	if (bAppendSlash)
		arsReturnPath.Append("\\");
	arsReturnPath.Append(sDirPathEnd.pString ? sDirPathEnd.pString : "");
	return arsReturnPath.GetLength() != 0;
}

bool FilePathUtilities::SplitPath(const char* apFilePath, BSStaticStringT<260>& arsDirName, BSStaticStringT<260>& arsFileName)
{
	uint32_t uLastSlash = FindLastSlash(apFilePath);
	if (uLastSlash == -1u)
	{
		arsFileName.SetFromString(apFilePath, 0);
		arsDirName.SetFromString("", 0);
		return false;
	}
	arsDirName.SetFromString(apFilePath, uLastSlash);
	uint32_t uLen = strlen(apFilePath);
	if (uLastSlash == uLen - 1)
	{
		arsFileName.SetFromString("", 0);
		return true;
	}
	arsFileName.SetFromString(&apFilePath[uLastSlash + 1], 0);
	return true;
}

bool FilePathUtilities::SplitExt(const char* apFilePath, BSStaticStringT<260>& arsRootName, BSStaticStringT<260>& arsExtension)
{
	BSStaticStringT<260> sFilePath;
	sFilePath.SetFromString(apFilePath, 0);
	uint32_t uDot = -1u;
	uint32_t uLen = sFilePath.GetLength();
	if (uLen)
	{
		for (uint32_t uIndex = uLen - 1; uIndex != -1u; --uIndex)
		{
			if (sFilePath.pString[uIndex] == '.')
			{
				uDot = uIndex;
				break;
			}
		}
	}
	if (uDot - 1 <= 0xFFFFFFFD)
	{
		arsExtension.SetFromString(&apFilePath[uDot], 0);
		arsRootName.SetFromString(sFilePath.pString, uDot);
		return true;
	}
	arsRootName.SetFromString(sFilePath.pString, 0);
	arsExtension.SetFromString("", 0);
	return false;
}

bool FilePathUtilities::NormPath(const char* apFilePath, BSStaticStringT<260>& arsNormPath, bool abUseBackslashes)
{
	char sBuffer[280];
	const char* pIn = apFilePath;
	char* pOut = sBuffer;
	bool bResult = true;
	char cChar = *pIn;
	if (cChar)
	{
		while (bResult)
		{
			if (cChar == '.' && (pIn[1] == '\\' || (pIn[1] == '.' && pIn[2] == '\\')))
			{
				if (pIn[1] == '.')
				{
					bResult = pOut > sBuffer;
					if (pOut > sBuffer)
					{
						--pOut;
						while (pOut > sBuffer && *(pOut - 1) != '\\')
							--pOut;
					}
					pIn += 3;
				}
				else
				{
					pIn += 2;
				}
			}
			else
			{
				*pOut = cChar;
				if (abUseBackslashes)
				{
					if (cChar == '/')
						*pOut = '\\';
				}
				else if (cChar == '\\')
				{
					*pOut = '/';
				}
				++pOut;
				++pIn;
			}
			cChar = *pIn;
			if (!cChar)
				break;
		}
	}
	if (bResult)
	{
		*pOut = 0;
		arsNormPath.SetFromString(sBuffer, 0);
	}
	else
	{
		arsNormPath.SetFromString("", 0);
	}
	return bResult;
}

uint32_t FilePathUtilities::FindLastChar(const BSStaticStringT<260>& arsPath, char cChar)
{
	uint32_t uLen = arsPath.GetLength();
	if (!uLen)
		return -1u;
	for (uint32_t uIndex = uLen - 1; uIndex != -1u; --uIndex)
	{
		if (arsPath.pString[uIndex] == cChar)
			return uIndex;
	}
	return -1u;
}

uint32_t FilePathUtilities::FindLastSlash(const char* apDirPath)
{
	if (!apDirPath)
		return -1u;
	BSStaticStringT<260> sDirPath;
	sDirPath.SetFromString(apDirPath, 0);
	uint32_t uBackslash = FindLastChar(sDirPath, '\\');
	uint32_t uSlash = FindLastChar(sDirPath, '/');
	if (uBackslash != -1u && (uSlash == -1u || uSlash <= uBackslash))
		return uBackslash;
	return uSlash;
}
