#include "BSResource/BSResourceID.h"
#include "BSCore/BSCRC32.h"

#include <cctype>
#include <cstdint>

namespace BSResource
{
	bool ID::operator<(const ID& arID) const
	{
		return uiDir < arID.uiDir || (uiDir == arID.uiDir && QFileAndExt() < arID.QFileAndExt());
	}

	bool ID::operator<=(const ID& arID) const
	{
		return uiDir < arID.uiDir || (uiDir == arID.uiDir && QFileAndExt() <= arID.QFileAndExt());
	}

	RemapTable::RemapTable()
	{
		for (unsigned int uiChar = 0; uiChar < 128; ++uiChar)
		{
			char cChar = static_cast<char>(uiChar);
			if (cChar == '/')
				cChar = '\\';
			else if (cChar >= 'A' && cChar <= 'Z')
				cChar = static_cast<char>(tolower(cChar));
			TableA[uiChar] = cChar;
		}
	}

	FileID::FileID(unsigned int auiFile, unsigned int auiExt) : uiFile(auiFile), uiExt(auiExt)
	{
	}

	static inline unsigned int PackExtension(const char* apExtension, const char* apEnd, const char* apTable)
	{
		unsigned int uiExtension = 0;
		for (unsigned int uiByte = 0; uiByte < 4 && apExtension < apEnd; ++uiByte, ++apExtension)
			uiExtension |= static_cast<unsigned int>(static_cast<int>(apTable[static_cast<signed char>(*apExtension)])) << (uiByte * 8);
		return uiExtension;
	}

	static inline void GenerateFile(FileID& arID, const char* apName)
	{
		const char* pTable = RemapTable::QInstance()->QTable();
		const unsigned int* pCRC = nullptr;
		BSCRC32::GetCRCTable(pCRC);
		const char* pStart = apName;
		const char* pEnd = apName;
		unsigned int uiHash = 0;
		for (; *pEnd; ++pEnd)
		{
			if (*pEnd == '.')
			{
				while (pStart < pEnd)
				{
					const char cChar = pTable[static_cast<signed char>(*pStart++)];
					uiHash = (uiHash >> 8) ^ pCRC[static_cast<unsigned char>(uiHash ^ cChar)];
				}
			}
		}
		arID.uiFile = uiHash;
		arID.uiExt = pStart < pEnd && *pStart == '.' ? PackExtension(pStart + 1, pEnd, pTable) : 0;
	}

	FileID::FileID(const char* apName) : uiFile(0), uiExt(0)
	{
		GenerateFile(*this, apName);
	}

	ID::ID(const char* apPath) : FileID(0, 0), uiDir(0)
	{
		if (apPath)
			GenerateFromPath(*this, apPath);
	}

	ID::ID(const char* apDirectory, const char* apFile) : FileID(0, 0), uiDir(0)
	{
		if (apDirectory)
		{
			const char* pEnd = apDirectory;
			while (*pEnd)
				++pEnd;
			if (pEnd > apDirectory + 1 && (pEnd[-1] == '\\' || pEnd[-1] == '/'))
				--pEnd;
			const char* pTable = RemapTable::QInstance()->QTable();
			BSCRC32::GenerateCRC(uiDir, apDirectory, static_cast<unsigned int>(pEnd - apDirectory), pTable);
		}
		if (apFile)
			GenerateFile(*this, apFile);
	}

	void ID::GenerateFromPath(ID& arID, const char* apPath)
	{
		const char* pEnd = apPath;
		const char* pFile = apPath;
		const char* pDot = apPath;
		for (; *pEnd; ++pEnd)
		{
			if (*pEnd == '.')
				pDot = pEnd;
			else if (*pEnd == '/' || *pEnd == '\\')
				pFile = pEnd;
		}
		if (pEnd == apPath)
			return;

		const char* pTable = RemapTable::QInstance()->QTable();
		unsigned int uiHash = 0;
		if (*pFile == '/' || *pFile == '\\')
		{
			BSCRC32::GenerateCRC(arID.uiDir, apPath, static_cast<unsigned int>(pFile - apPath), pTable);
			++pFile;
		}
		if (*pFile != '.' && *pFile)
			BSCRC32::GenerateCRC(uiHash, pFile, static_cast<unsigned int>((pDot > pFile ? pDot : pEnd) - pFile), pTable);
		arID.uiExt = *pDot == '.' && pDot + 1 < pEnd ? PackExtension(pDot + 1, pEnd, pTable) : 0;
		arID.uiFile = uiHash;
	}
}
