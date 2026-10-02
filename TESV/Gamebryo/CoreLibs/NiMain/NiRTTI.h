#pragma once

#include <cstddef>

class NiRTTI
{
public:
	NiRTTI(const char* pcName, const NiRTTI* pkBaseRTTI);

	const char* GetName() const { return m_pcName; }
	const NiRTTI* GetBaseRTTI() const { return m_pkBaseRTTI; }

	const char* m_pcName;
	const NiRTTI* m_pkBaseRTTI;
};
static_assert(sizeof(NiRTTI) == 0x10);
static_assert(offsetof(NiRTTI, m_pcName) == 0);
static_assert(offsetof(NiRTTI, m_pkBaseRTTI) == 8);
