#include "NiSearchPath.h"
#include "NiFilename.h"
#include "NiPath.h"

#include <cstddef>
#include <cstring>

NiSearchPath::CallbackPtr NiSearchPath::spSearchPathCreateCallback;
char NiSearchPath::ms_acDefPath[260];

NiSearchPath::NiSearchPath() : m_uiNextPath(0)
{
	static_assert(offsetof(NiSearchPath, m_uiNextPath) == 8);
	static_assert(offsetof(NiSearchPath, m_acFilePath) == 12);
	static_assert(offsetof(NiSearchPath, m_acReferencePath) == 272);
	m_acFilePath[0] = 0;
	m_acReferencePath[0] = 0;
}

NiSearchPath::~NiSearchPath()
{
}

void NiSearchPath::SetFilePath(const char* pcFilePath)
{
	if (pcFilePath && pcFilePath[0])
	{
		strncpy_s(m_acFilePath, sizeof(m_acFilePath), pcFilePath, sizeof(m_acFilePath) - 1);
		NiPath::Standardize(m_acFilePath);
	}
	else
		m_acFilePath[0] = 0;
}

void NiSearchPath::SetReferencePath(const char* pcReferencePath)
{
	if (pcReferencePath && pcReferencePath[0])
	{
		NiFilename kFilename(pcReferencePath);
		kFilename.SetFilename("");
		kFilename.SetExt("");
		kFilename.GetFullPath(m_acReferencePath, sizeof(m_acReferencePath));
		NiPath::Standardize(m_acReferencePath);
	}
	else
		m_acReferencePath[0] = 0;
}

void NiSearchPath::Reset()
{
	m_uiNextPath = 0;
}

bool NiSearchPath::GetNextSearchPath(char* pcPath, unsigned int uiStringLen)
{
	NiFilename kFullPath(m_acFilePath);
	switch (m_uiNextPath)
	{
	case 0:
		break;
	case 1:
		strcpy_s(kFullPath.m_acDrive, sizeof(kFullPath.m_acDrive), "");
		strcpy_s(kFullPath.m_acDir, sizeof(kFullPath.m_acDir), "");
		break;
	case 2:
	{
		strcpy_s(kFullPath.m_acDrive, sizeof(kFullPath.m_acDrive), "");
		strcpy_s(kFullPath.m_acDir, sizeof(kFullPath.m_acDir), m_acReferencePath);
		NiFilename kInputPath(m_acFilePath);
		strcpy_s(kFullPath.m_acSubDir, sizeof(kFullPath.m_acSubDir), kInputPath.m_acDir);
		break;
	}
	case 3:
		strcpy_s(kFullPath.m_acDrive, sizeof(kFullPath.m_acDrive), "");
		strcpy_s(kFullPath.m_acDir, sizeof(kFullPath.m_acDir), m_acReferencePath);
		strcpy_s(kFullPath.m_acSubDir, sizeof(kFullPath.m_acSubDir), "");
		break;
	case 4:
		if (!ms_acDefPath[0])
			return false;
		strcpy_s(kFullPath.m_acDrive, sizeof(kFullPath.m_acDrive), "");
		strcpy_s(kFullPath.m_acDir, sizeof(kFullPath.m_acDir), ms_acDefPath);
		break;
	default:
		return false;
	}
	kFullPath.GetFullPath(pcPath, uiStringLen);
	++m_uiNextPath;
	return true;
}
