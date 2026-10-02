#pragma once

#include "Gamebryo/CoreLibs/NiSystem/NiAllocator.h"

class BSNiAllocator : public NiAllocator
{
public:
	BSNiAllocator();
	~BSNiAllocator() override;
	void* Allocate(size_t& stSizeInBytes, size_t& stAlignment, NiMemEventType eEventType, bool bProvideAccurateSizeOnDeallocate, const char* pcSourceFile, int iSourceLine, const char* pcFunction) override;
	void Deallocate(void* pvMemory, NiMemEventType eEventType, size_t stSizeInBytes) override;
	void* Reallocate(void* pvMemory, size_t& stSizeInBytes, size_t& stAlignment, NiMemEventType eEventType, bool bProvideAccurateSizeOnDeallocate, size_t stSizeCurrent, const char* pcSourceFile, int iSourceLine, const char* pcFunction) override;
	void* AllocateExternal(size_t& stSizeInBytes, size_t& stAlignment, NiMemEventType eEventType, bool bProvideAccurateSizeOnDeallocate, const char* pcSourceFile, int iSourceLine, const char* pcFunction) override;
	void DeallocateExternal(void* pvMemory, NiMemEventType eEventType, size_t stSizeInBytes) override;
	void* ReallocateExternal(void* pvMemory, size_t& stSizeInBytes, size_t& stAlignment, NiMemEventType eEventType, bool bProvideAccurateSizeOnDeallocate, size_t stSizeCurrent, const char* pcSourceFile, int iSourceLine, const char* pcFunction) override;
	void Initialize() override;
	void Shutdown() override;
	bool VerifyAddress(const void* pvMemory) override;
};
static_assert(sizeof(BSNiAllocator) == 8);
