#include "BSSystem/BSSearchPath.h"

#include <new>

#include "BSCore/MemoryManager.h"
#include "windows.h"

BSSearchPath::BSSearchPath()
{
	m_uiNextPath = 0;
	m_acFilePath[0] = '\0';
	m_acReferencePath[0] = '\0';
}

void BSSearchPath::Reset()
{
	m_uiNextPath = 0;
}

bool BSSearchPath::GetNextSearchPath(char* apPath, uint32_t)
{
	if (m_uiNextPath)
		return false;

	strcpy_s(apPath, 0x104, m_acFilePath);
	++m_uiNextPath;
	return true;
}

bool BSSearchPath::Access(const char*)
{
	return true;
}

NiSearchPath* BSSearchPath::GetBSSearchPath()
{
	auto* pPath = static_cast<BSSearchPath*>(MemoryManager::Instance().Allocate(sizeof(BSSearchPath)));
	if (!pPath)
		return nullptr;

	return new (pPath) BSSearchPath();
}

void BSSearchPath::InitCallback()
{
	NiSearchPath::spSearchPathCreateCallback = BSSearchPath::GetBSSearchPath;
}
