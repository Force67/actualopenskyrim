#include "Gamebryo/CoreLibs/NiSystem/BSNiAllocator.h"
#include "BSCore/MemoryManager.h"

#include <cstring>

BSNiAllocator::BSNiAllocator()
{
}

BSNiAllocator::~BSNiAllocator()
{
}

void* BSNiAllocator::Allocate(size_t& stSizeInBytes, size_t& stAlignment, NiMemEventType eEventType, bool, const char*, int, const char*)
{
	MemoryManager& kManager = MemoryManager::Instance();
	bool bAligned = eEventType == NI_ALIGNEDMALLOC;
	void* pvMemory = kManager.Allocate(stSizeInBytes, bAligned ? static_cast<uint32_t>(stAlignment) : 0, bAligned);
	std::memset(pvMemory, 0, stSizeInBytes);
	return pvMemory;
}

void BSNiAllocator::Deallocate(void* pvMemory, NiMemEventType eEventType, size_t)
{
	MemoryManager::Instance().Deallocate(pvMemory, eEventType == NI_ALIGNEDFREE);
}

void* BSNiAllocator::Reallocate(void* pvMemory, size_t& stSizeInBytes, size_t& stAlignment, NiMemEventType eEventType, bool, size_t, const char*, int, const char*)
{
	MemoryManager& kManager = MemoryManager::Instance();
	bool bAligned = eEventType == NI_ALIGNEDREALLOC;
	return kManager.Reallocate(pvMemory, stSizeInBytes, bAligned ? static_cast<uint32_t>(stAlignment) : 0, bAligned);
}

void* BSNiAllocator::AllocateExternal(size_t& stSizeInBytes, size_t& stAlignment, NiMemEventType eEventType, bool, const char*, int, const char*)
{
	MemoryManager& kManager = MemoryManager::Instance();
	bool bAligned = eEventType == NI_EXTERNAL_ALIGNEDMALLOC;
	void* pvMemory = kManager.Allocate(stSizeInBytes, bAligned ? static_cast<uint32_t>(stAlignment) : 0, bAligned);
	std::memset(pvMemory, 0, stSizeInBytes);
	return pvMemory;
}

void BSNiAllocator::DeallocateExternal(void* pvMemory, NiMemEventType eEventType, size_t)
{
	MemoryManager::Instance().Deallocate(pvMemory, eEventType == NI_EXTERNAL_ALIGNEDFREE);
}

void* BSNiAllocator::ReallocateExternal(void* pvMemory, size_t& stSizeInBytes, size_t& stAlignment, NiMemEventType eEventType, bool, size_t, const char*, int, const char*)
{
	MemoryManager& kManager = MemoryManager::Instance();
	bool bAligned = eEventType == NI_EXTERNAL_ALGINEDREALLOC;
	return kManager.Reallocate(pvMemory, stSizeInBytes, bAligned ? static_cast<uint32_t>(stAlignment) : 0, bAligned);
}

void BSNiAllocator::Initialize()
{
}

void BSNiAllocator::Shutdown()
{
}

bool BSNiAllocator::VerifyAddress(const void*)
{
	return true;
}
