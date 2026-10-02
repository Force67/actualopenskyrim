#include "NiSearchPath.h"
#include "NiFilename.h"
#include "NiPath.h"

#include <cstddef>
#include <cstring>

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
