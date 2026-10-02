#pragma once

#include <cstddef>
#include <cstdint>

class NiObjectGroup
{
public:
	explicit NiObjectGroup(uint32_t uiSize);

	uint32_t m_uiSize;
	void* m_pvBuffer;
	void* m_pvFree;
	uint32_t m_uiRefCount;
};
static_assert(sizeof(NiObjectGroup) == 32);
static_assert(offsetof(NiObjectGroup, m_pvBuffer) == 8);
static_assert(offsetof(NiObjectGroup, m_uiRefCount) == 24);
