#include "Gamebryo/CoreLibs/NiSystem/NiInitOptions.h"
#include "Gamebryo/CoreLibs/NiSystem/NiAllocator.h"
#include "Gamebryo/CoreLibs/NiSystem/NiStandardAllocator.h"
#include "BSCore/MemoryManager.h"

#include <new>

NiInitOptions::NiInitOptions()
{
	NiStandardAllocator* pkAllocator = static_cast<NiStandardAllocator*>(MemoryManager::Instance().Allocate(sizeof(NiStandardAllocator), 0, false));
	if (pkAllocator)
		new (pkAllocator) NiStandardAllocator();
	m_pkAllocator = pkAllocator;
	m_bAllocatedInternally = true;
}

NiInitOptions::NiInitOptions(NiAllocator* pkAllocator) :
	m_pkAllocator(pkAllocator),
	m_bAllocatedInternally(false)
{
}

NiInitOptions::~NiInitOptions()
{
	if (m_bAllocatedInternally)
		delete m_pkAllocator;
}

NiAllocator* NiInitOptions::GetAllocator() const
{
	return m_pkAllocator;
}
