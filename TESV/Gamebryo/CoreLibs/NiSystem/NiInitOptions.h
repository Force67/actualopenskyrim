#pragma once

#include <cstddef>

class NiAllocator;

class NiInitOptions
{
public:
	NiInitOptions();
	NiInitOptions(NiAllocator* pkAllocator);
	~NiInitOptions();
	NiAllocator* GetAllocator() const;

	NiAllocator* m_pkAllocator;
	bool m_bAllocatedInternally;
};
static_assert(sizeof(NiInitOptions) == 16);
static_assert(offsetof(NiInitOptions, m_pkAllocator) == 0);
static_assert(offsetof(NiInitOptions, m_bAllocatedInternally) == 8);
