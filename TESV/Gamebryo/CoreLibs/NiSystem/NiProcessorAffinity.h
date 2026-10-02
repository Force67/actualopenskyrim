#pragma once

#include <cstdint>

class NiProcessorAffinity
{
public:
	uint32_t m_eIdealProcessor;
	unsigned int m_uiAffinityMask;
};
static_assert(sizeof(NiProcessorAffinity) == 8);
