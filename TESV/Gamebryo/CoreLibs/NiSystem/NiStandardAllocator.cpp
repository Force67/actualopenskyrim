#include "Gamebryo/CoreLibs/NiSystem/NiStandardAllocator.h"
#include "BSCore/MemoryManager.h"
#include "NiSmallObjectAllocator.h"

#include <cstring>
#include <new>

NiSmallObjectAllocator* NiStandardAllocator::ms_pkSmallAlloc;

void* NiStandardAllocator::AllocateExternal(size_t& stSizeInBytes, size_t& stAlignment, NiMemEventType eEventType, bool, const char*, int, const char*)
{
	if (eEventType != NI_EXTERNAL_ALIGNEDMALLOC)
	{
		size_t stSize = stSizeInBytes;
		MemoryManager::Instance().Allocate(stSize, 0, false);
	}
	size_t stSize = stSizeInBytes;
	uint32_t uiAlignment = static_cast<uint8_t>(stAlignment);
	void* pvMemory = MemoryManager::Instance().Allocate(stSize, uiAlignment, true);
	memset(pvMemory, 0, stSizeInBytes);
	return pvMemory;
}

void NiStandardAllocator::DeallocateExternal(void* pvMemory, NiMemEventType eEventType, size_t)
{
	if (pvMemory)
		MemoryManager::Instance().Deallocate(pvMemory, eEventType == NI_EXTERNAL_ALIGNEDFREE);
}

void* NiStandardAllocator::ReallocateExternal(void* pvMemory, size_t& stSizeInBytes, size_t& stAlignment, NiMemEventType eEventType, bool, size_t, const char*, int, const char*)
{
	if (eEventType != NI_EXTERNAL_ALGINEDREALLOC)
	{
		size_t stSize = stSizeInBytes;
		MemoryManager::Instance().Reallocate(pvMemory, stSize, 0, false);
	}
	size_t stSize = stSizeInBytes;
	uint32_t uiAlignment = static_cast<uint8_t>(stAlignment);
	if (pvMemory)
		pvMemory = MemoryManager::Instance().Reallocate(pvMemory, stSize, uiAlignment, true);
	// Preserve the allocation calls on the null pointer path.
	void* (*volatile pfnClear)(void*, int, size_t) = memset;
	pfnClear(pvMemory, 0, stSizeInBytes);
	return pvMemory;
}

bool NiStandardAllocator::VerifyAddress(const void*)
{
	return true;
}

NiSmallObjectAllocator* NiStandardAllocator::GetSmallAllocator()
{
	return ms_pkSmallAlloc;
}

void NiStandardAllocator::SetSizeToAddress(void* pvMemory, size_t stSize)
{
	unsigned char* pcBytes = static_cast<unsigned char*>(pvMemory);
	for (unsigned int uiByte = 0; uiByte < 4; ++uiByte)
		pcBytes[uiByte] = static_cast<unsigned char>(stSize >> (uiByte * 8));
}

void NiStandardAllocator::Initialize()
{
	void* pvMemory = MemoryManager::Instance().Allocate(sizeof(NiSmallObjectAllocator), 0, false);
	ms_pkSmallAlloc = pvMemory ? new (pvMemory) NiSmallObjectAllocator(25600) : nullptr;
}

void NiStandardAllocator::Shutdown()
{
	NiSmallObjectAllocator* pkSmallAlloc = ms_pkSmallAlloc;
	if (pkSmallAlloc)
	{
		NiFixedAllocator* pkPools = reinterpret_cast<NiFixedAllocator*>(pkSmallAlloc);
		for (size_t i = 256; i > 0; --i)
			pkPools[i - 1].~NiFixedAllocator();
		::operator delete(pkSmallAlloc, sizeof(NiSmallObjectAllocator));
	}
	ms_pkSmallAlloc = nullptr;
}

void* NiStandardAllocator::Allocate(size_t& stSizeInBytes, size_t& stAlignment, NiMemEventType eEventType, bool bProvideAccurateSizeOnDeallocate, const char*, int, const char*)
{
	bool bAligned = eEventType == NI_ALIGNEDMALLOC || eEventType == NI_ALIGNEDREALLOC;
	bool bStoreSize = stAlignment == 4 && !bProvideAccurateSizeOnDeallocate && !bAligned;
	if (bStoreSize)
		stSizeInBytes += 4;
	void* pvMemory;
	if (!bAligned && stSizeInBytes <= 256 && stAlignment == 4)
		pvMemory = ms_pkSmallAlloc->Allocate(stSizeInBytes);
	else
		pvMemory = _NiExternalAlignedMalloc(stSizeInBytes, stAlignment);
	if (pvMemory && bStoreSize)
	{
		SetSizeToAddress(pvMemory, stSizeInBytes);
		pvMemory = static_cast<unsigned char*>(pvMemory) + 4;
	}
	return pvMemory;
}

void* NiStandardAllocator::Reallocate(void* pvMemory, size_t& stSizeInBytes, size_t& stAlignment, NiMemEventType eEventType, bool bProvideAccurateSizeOnDeallocate, size_t stSizeCurrent, const char* pcSourceFile, int iSourceLine, const char* pcFunction)
{
	if (eEventType != NI_ALIGNEDREALLOC && eEventType != NI_ALIGNEDFREE && stSizeCurrent == size_t(-1) && !bProvideAccurateSizeOnDeallocate)
	{
		pvMemory = static_cast<unsigned char*>(pvMemory) - 4;
		unsigned char* pucSize = static_cast<unsigned char*>(pvMemory);
		uint32_t uiSize = 0;
		for (unsigned int uiByte = 0; uiByte < 4; ++uiByte)
			uiSize |= uint32_t(pucSize[uiByte]) << (uiByte * 8);
		stSizeCurrent = (stSizeCurrent & ~size_t(UINT32_MAX)) | uiSize;
	}
	bool bAligned = eEventType == NI_ALIGNEDMALLOC || eEventType == NI_ALIGNEDREALLOC;
	bool bStoreSize = stAlignment == 4 && !bProvideAccurateSizeOnDeallocate && !bAligned;
	if (bStoreSize)
		stSizeInBytes += 4;
	if (stSizeCurrent != size_t(-1) && stSizeInBytes <= stSizeCurrent && bStoreSize)
	{
		stSizeInBytes = stSizeCurrent;
		return static_cast<unsigned char*>(pvMemory) + 4;
	}
	void* pvResult;
	if (bAligned || (stSizeCurrent > 256 && stSizeInBytes > 256))
		pvResult = _NiExternalAlignedRealloc(pvMemory, stSizeInBytes, stAlignment);
	else
	{
		stSizeInBytes -= 4;
		pvResult = Allocate(stSizeInBytes, stAlignment, eEventType, bProvideAccurateSizeOnDeallocate, pcSourceFile, iSourceLine, pcFunction);
		bStoreSize = false;
		if (stSizeCurrent != size_t(-1))
		{
			memcpy(pvResult, static_cast<unsigned char*>(pvMemory) + 4, stSizeCurrent - 4);
			Deallocate(pvMemory, eEventType, stSizeCurrent);
		}
	}
	if (pvResult && bStoreSize)
	{
		SetSizeToAddress(pvResult, stSizeInBytes);
		pvResult = static_cast<unsigned char*>(pvResult) + 4;
	}
	return pvResult;
}
