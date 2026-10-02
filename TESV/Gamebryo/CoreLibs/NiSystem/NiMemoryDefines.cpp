#include "Gamebryo/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo/CoreLibs/NiSystem/NiAllocator.h"
#include "Gamebryo/CoreLibs/NiSystem/NiMemManager.h"

void* _NiMalloc(size_t stSizeInBytes)
{
	if (!stSizeInBytes)
		stSizeInBytes = 1;
	size_t stAlignment = 4;
	return NiMemManager::Get()->m_pkAllocator->Allocate(stSizeInBytes, stAlignment, NI_MALLOC, false, nullptr, -1, nullptr);
}

void* _NiRealloc(void* pvMemory, size_t stSizeInBytes)
{
	if (!pvMemory)
		return _NiMalloc(stSizeInBytes);
	if (!stSizeInBytes)
	{
		_NiFree(pvMemory);
		return nullptr;
	}
	size_t stAlignment = 4;
	return NiMemManager::Get()->m_pkAllocator->Reallocate(pvMemory, stSizeInBytes, stAlignment, NI_REALLOC, false, size_t(-1), nullptr, -1, nullptr);
}

void _NiFree(void* pvMemory)
{
	if (pvMemory)
		NiMemManager::Get()->m_pkAllocator->Deallocate(pvMemory, NI_FREE, size_t(-1));
}

void* _NiAlignedMalloc(size_t stSizeInBytes, size_t stAlignment)
{
	if (!stSizeInBytes)
		stSizeInBytes = 1;
	return NiMemManager::Get()->m_pkAllocator->Allocate(stSizeInBytes, stAlignment, NI_ALIGNEDMALLOC, false, nullptr, -1, nullptr);
}

void* _NiAlignedRealloc(void* pvMemory, size_t stSizeInBytes, size_t stAlignment)
{
	if (!pvMemory)
		return _NiAlignedMalloc(stSizeInBytes, stAlignment);
	if (!stSizeInBytes)
	{
		_NiAlignedFree(pvMemory);
		return nullptr;
	}
	return NiMemManager::Get()->m_pkAllocator->Reallocate(pvMemory, stSizeInBytes, stAlignment, NI_ALIGNEDREALLOC, false, size_t(-1), nullptr, -1, nullptr);
}

void _NiAlignedFree(void* pvMemory)
{
	if (pvMemory)
		NiMemManager::Get()->m_pkAllocator->Deallocate(pvMemory, NI_ALIGNEDFREE, size_t(-1));
}

void* _NiExternalMalloc(size_t stSizeInBytes)
{
	if (!stSizeInBytes)
		stSizeInBytes = 1;
	size_t stAlignment = 4;
	return NiMemManager::Get()->m_pkAllocator->AllocateExternal(stSizeInBytes, stAlignment, NI_EXTERNAL_MALLOC, false, nullptr, -1, nullptr);
}

void* _NiExternalRealloc(void* pvMemory, size_t stSizeInBytes)
{
	if (!pvMemory)
		return _NiExternalMalloc(stSizeInBytes);
	if (!stSizeInBytes)
	{
		_NiExternalFree(pvMemory);
		return nullptr;
	}
	size_t stAlignment = 4;
	return NiMemManager::Get()->m_pkAllocator->ReallocateExternal(pvMemory, stSizeInBytes, stAlignment, NI_EXTERNAL_REALLOC, false, size_t(-1), nullptr, -1, nullptr);
}

void _NiExternalFree(void* pvMemory)
{
	if (pvMemory)
		NiMemManager::Get()->m_pkAllocator->DeallocateExternal(pvMemory, NI_EXTERNAL_FREE, size_t(-1));
}

void* _NiExternalAlignedMalloc(size_t stSizeInBytes, size_t stAlignment)
{
	if (!stSizeInBytes)
		stSizeInBytes = 1;
	return NiMemManager::Get()->m_pkAllocator->AllocateExternal(stSizeInBytes, stAlignment, NI_EXTERNAL_ALIGNEDMALLOC, false, nullptr, -1, nullptr);
}

void* _NiExternalAlignedRealloc(void* pvMemory, size_t stSizeInBytes, size_t stAlignment)
{
	if (!pvMemory)
		return _NiExternalAlignedMalloc(stSizeInBytes, stAlignment);
	if (!stSizeInBytes)
	{
		_NiExternalAlignedFree(pvMemory);
		return nullptr;
	}
	return NiMemManager::Get()->m_pkAllocator->ReallocateExternal(pvMemory, stSizeInBytes, stAlignment, NI_EXTERNAL_ALGINEDREALLOC, false, size_t(-1), nullptr, -1, nullptr);
}

void _NiExternalAlignedFree(void* pvMemory)
{
	if (pvMemory)
		NiMemManager::Get()->m_pkAllocator->DeallocateExternal(pvMemory, NI_EXTERNAL_ALIGNEDFREE, size_t(-1));
}
