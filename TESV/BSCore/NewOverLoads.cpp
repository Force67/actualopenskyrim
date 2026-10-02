#include "BSCore/NewOverLoads.h"

#include "BSCore/MemoryManager.h"

void* operator new(size_t auiSize)
{
	return MemoryManager::Instance().Allocate(auiSize, 0, false);
}

void* operator new[](size_t auiSize)
{
	return MemoryManager::Instance().Allocate(auiSize, 0, false);
}

void operator delete(void* apMemory) noexcept
{
	MemoryManager::Instance().Deallocate(apMemory, false);
}

void operator delete[](void* apMemory) noexcept
{
	MemoryManager::Instance().Deallocate(apMemory, false);
}
