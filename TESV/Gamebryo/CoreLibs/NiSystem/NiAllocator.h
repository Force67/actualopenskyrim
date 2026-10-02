#pragma once

#include "Gamebryo/CoreLibs/NiSystem/NiMemoryDefines.h"

#include <cstddef>

class NiAllocator
{
public:
	virtual ~NiAllocator() {}
	virtual void* Allocate(size_t& stSizeInBytes, size_t& stAlignment, NiMemEventType eEventType, bool bProvideAccurateSizeOnDeallocate, const char* pcSourceFile, int iSourceLine, const char* pcFunction) = 0;
	virtual void Deallocate(void* pvMemory, NiMemEventType eEventType, size_t stSizeInBytes) = 0;
	virtual void* Reallocate(void* pvMemory, size_t& stSizeInBytes, size_t& stAlignment, NiMemEventType eEventType, bool bProvideAccurateSizeOnDeallocate, size_t stSizeCurrent, const char* pcSourceFile, int iSourceLine, const char* pcFunction) = 0;
	virtual void* AllocateExternal(size_t& stSizeInBytes, size_t& stAlignment, NiMemEventType eEventType, bool bProvideAccurateSizeOnDeallocate, const char* pcSourceFile, int iSourceLine, const char* pcFunction) = 0;
	virtual void DeallocateExternal(void* pvMemory, NiMemEventType eEventType, size_t stSizeInBytes) = 0;
	virtual void* ReallocateExternal(void* pvMemory, size_t& stSizeInBytes, size_t& stAlignment, NiMemEventType eEventType, bool bProvideAccurateSizeOnDeallocate, size_t stSizeCurrent, const char* pcSourceFile, int iSourceLine, const char* pcFunction) = 0;
	virtual void Initialize() = 0;
	virtual void Shutdown() = 0;
	virtual bool VerifyAddress(const void* pvMemory) = 0;
};
static_assert(sizeof(NiAllocator) == 8);
