#include "Gamebryo/CoreLibs/NiSystem/NiFilename.h"

#include <stdlib.h>
#include <cstring>

NiFilename::NiFilename(const char* pcFullPath)
{
	static_assert(offsetof(NiFilename, m_acDir) == 0);
	static_assert(offsetof(NiFilename, m_acDrive) == 256);
	static_assert(offsetof(NiFilename, m_acExt) == 259);
	static_assert(offsetof(NiFilename, m_acFname) == 515);
	static_assert(offsetof(NiFilename, m_acSubDir) == 771);
	m_acSubDir[0] = 0;
	Splitpath(pcFullPath);
}

void NiFilename::Splitpath(const char* pcStr)
{
	_splitpath_s(pcStr, m_acDrive, sizeof(m_acDrive), m_acDir, sizeof(m_acDir), m_acFname, sizeof(m_acFname), m_acExt, sizeof(m_acExt));
}

bool NiFilename::GetFullPath(char* pcFullPath, unsigned int uiStrLen) const
{
	return Makepath(pcFullPath, uiStrLen);
}

bool NiFilename::Makepath(char* pcStr, size_t stStrLen) const
{
	if (!pcStr)
		return false;
	size_t stRequired = m_acDrive[0] ? 3 : 1;
	if (m_acDir[0])
	{
		size_t stLength = std::strlen(m_acDir);
		stRequired += stLength;
		char cLast = m_acDir[stLength - 1];
		if (cLast != '\\' && cLast != '/')
			++stRequired;
	}
	if (m_acSubDir[0])
	{
		size_t stLength = std::strlen(m_acSubDir);
		stRequired += stLength;
		char cLast = m_acSubDir[stLength - 1];
		if (cLast != '\\' && cLast != '/')
			++stRequired;
	}
	if (m_acFname[0])
		stRequired += std::strlen(m_acFname);
	if (m_acExt[0])
		stRequired += std::strlen(m_acExt) + (m_acExt[0] != '.');
	if (stRequired > stStrLen)
		return false;
	pcStr[0] = 0;
	if (m_acDrive[0])
	{
		pcStr[0] = m_acDrive[0];
		pcStr[1] = ':';
		pcStr[2] = 0;
	}
	if (m_acDir[0])
	{
		strcat_s(pcStr, stStrLen, m_acDir);
		char cLast = pcStr[std::strlen(pcStr) - 1];
		if (cLast != '\\' && cLast != '/')
			strcat_s(pcStr, stStrLen, "\\");
	}
	if (m_acSubDir[0])
	{
		strcat_s(pcStr, stStrLen, m_acSubDir);
		char cLast = pcStr[std::strlen(pcStr) - 1];
		if (cLast != '\\' && cLast != '/')
			strcat_s(pcStr, stStrLen, "\\");
	}
	if (m_acFname[0])
		strcat_s(pcStr, stStrLen, m_acFname);
	if (m_acExt[0])
	{
		if (m_acExt[0] != '.')
			strcat_s(pcStr, stStrLen, ".");
		strcat_s(pcStr, stStrLen, m_acExt);
	}
	return true;
}
