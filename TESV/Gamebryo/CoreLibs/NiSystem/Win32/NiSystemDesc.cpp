#include "Gamebryo/CoreLibs/NiSystem/NiSystemDesc.h"
#include "BSCore/MemoryManager.h"

#include <new>
#include <windows.h>

void operator delete(void* apMemory, size_t stSize) noexcept;

NiSystemDesc* NiSystemDesc::ms_pkSystemDesc;

NiSystemDesc::NiSystemDesc()
{
	SYSTEM_INFO kInfo;
	GetSystemInfo(&kInfo);
	m_uiNumLogicalProcessors = kInfo.dwNumberOfProcessors;
	LARGE_INTEGER kFrequency;
	QueryPerformanceFrequency(&kFrequency);
	m_fPCCyclesPerSecond = static_cast<float>(kFrequency.QuadPart);
}

NiSystemDesc::NiSystemDesc(const NiSystemDesc&)
{
}

void NiSystemDesc::InitSystemDesc()
{
	NiSystemDesc* pkDesc = static_cast<NiSystemDesc*>(MemoryManager::Instance().Allocate(sizeof(NiSystemDesc), 0, false));
	if (pkDesc)
		new (pkDesc) NiSystemDesc();
	ms_pkSystemDesc = pkDesc;
}

void NiSystemDesc::ShutdownSystemDesc()
{
	operator delete(ms_pkSystemDesc, sizeof(NiSystemDesc));
}
