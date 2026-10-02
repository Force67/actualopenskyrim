#include "Gamebryo/CoreLibs/NiSystem/NiMemManager.h"
#include "Gamebryo/CoreLibs/NiSystem/NiAllocator.h"
#include "Gamebryo/CoreLibs/NiSystem/NiInitOptions.h"
#include "Gamebryo/CoreLibs/NiSystem/NiStaticDataManager.h"
#include "BSCore/MemoryManager.h"

NiMemManager* NiMemManager::ms_pkMemManager = nullptr;

bool NiMemManager::IsInitialized()
{
	return ms_pkMemManager != nullptr;
}

void NiMemManager::_SDMInit()
{
	ms_pkMemManager = static_cast<NiMemManager*>(MemoryManager::Instance().Allocate(sizeof(NiMemManager), 0, false));
	NiAllocator* pkAllocator = NiStaticDataManager::GetInitOptions()->GetAllocator();
	ms_pkMemManager->m_pkAllocator = pkAllocator;
	pkAllocator->Initialize();
}

void NiMemManager::_SDMShutdown()
{
}

bool NiMemManager::VerifyAddress(const void* pvMemory)
{
	return ms_pkMemManager->m_pkAllocator->VerifyAddress(pvMemory);
}
