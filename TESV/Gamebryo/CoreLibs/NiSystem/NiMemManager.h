#pragma once

#include <cstddef>

class NiAllocator;

class NiMemManager
{
public:
	static NiMemManager* Get() { return ms_pkMemManager; }

	static bool IsInitialized();
	static void _SDMInit();
	static void _SDMShutdown();
	static bool VerifyAddress(const void* pvMemory);

	NiAllocator* m_pkAllocator;
private:
	static NiMemManager* ms_pkMemManager;
};
static_assert(sizeof(NiMemManager) == 8);
static_assert(offsetof(NiMemManager, m_pkAllocator) == 0);
