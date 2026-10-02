#pragma once

#include <cstddef>
#include <cstring>

class NiFilename
{
public:
	NiFilename(const char* pcFullPath);
	bool GetFullPath(char* pcFullPath, unsigned int uiStrLen) const;
	void SetFilename(const char* pcFilename) { strcpy_s(m_acFname, sizeof(m_acFname), pcFilename); }
	void SetExt(const char* pcExt) { strcpy_s(m_acExt, sizeof(m_acExt), pcExt); }
	int Splitpath(const char* pcStr);

private:
	friend class NiSearchPath;

	bool Makepath(char* pcStr, size_t stStrLen) const;

	char m_acDir[256];
	char m_acDrive[3];
	char m_acExt[256];
	char m_acFname[256];
	char m_acSubDir[256];
};
static_assert(sizeof(NiFilename) == 1027);
