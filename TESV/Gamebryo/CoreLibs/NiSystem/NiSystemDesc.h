#pragma once

#include <cstddef>
#include <cstdint>

class NiSystemDesc
{
public:
	static void InitSystemDesc();
	static void ShutdownSystemDesc();
	static const NiSystemDesc& GetSystemDesc() { return *ms_pkSystemDesc; }

	uint32_t m_uiNumLogicalProcessors;
	float m_fPCCyclesPerSecond;

protected:
	NiSystemDesc();
	NiSystemDesc(const NiSystemDesc& kDesc);
	static NiSystemDesc* ms_pkSystemDesc;
};
static_assert(sizeof(NiSystemDesc) == 8);
static_assert(offsetof(NiSystemDesc, m_uiNumLogicalProcessors) == 0);
static_assert(offsetof(NiSystemDesc, m_fPCCyclesPerSecond) == 4);
