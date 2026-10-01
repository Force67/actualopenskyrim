#include "BSCore/BSTArray.h"
#include "BSCore/BSTAlignedHeapArrayAllocator.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/BSScrapArray.h"

#include <windows.h>
#include <cstring>

BSTArrayBase::BSTArrayBase() : iSize(0)
{}

BSTArrayBase::~BSTArrayBase() = default;

BSTArrayHeapAllocator::~BSTArrayHeapAllocator() = default;
BSTAlignedHeapArrayAllocatorBase::~BSTAlignedHeapArrayAllocatorBase() = default;

void BSTArrayBase::MoveItems(void* apBuffer, unsigned int auiTo, unsigned int auiFrom, unsigned int auiCount, unsigned int auiElemSize)
{
	unsigned int uiBytes = auiCount * auiElemSize;
	if (!uiBytes)
		return;
	char* pBuffer = static_cast<char*>(apBuffer);
	memmove_s(pBuffer + auiTo * auiElemSize, uiBytes, pBuffer + auiFrom * auiElemSize, uiBytes);
}

bool BSTArrayBase::InitialReserve(const IAllocatorFunctor& arFunctor, unsigned int auiReserveSize, unsigned int auiElemSize)
{
	return !auiReserveSize || arFunctor.Allocate(auiReserveSize, auiElemSize);
}

unsigned int BSTArrayBase::AddUninitialized(const IAllocatorFunctor& arFunctor, unsigned int auiAllocSize, unsigned int auiElemSize)
{
	if (iSize < auiAllocSize)
		return iSize++;
	if (!auiAllocSize)
	{
		if (!arFunctor.Allocate(1, auiElemSize))
			return 0xFFFFFFFFu;
		iSize = 1;
		return 0;
	}
	if (!arFunctor.Reallocate(iSize + 1, iSize, 0, 0, auiElemSize))
		return 0xFFFFFFFFu;
	return iSize++;
}

bool BSTArrayBase::InsertUninitialized(const IAllocatorFunctor& arFunctor, void* apBuffer, unsigned int auiAllocSize, unsigned int auiIndex, unsigned int auiElemSize)
{
	if (auiIndex == iSize)
		return AddUninitialized(arFunctor, auiAllocSize, auiElemSize) != 0xFFFFFFFFu;
	unsigned int uiBackCount = iSize - auiIndex;
	if (iSize == auiAllocSize)
	{
		if (!arFunctor.Reallocate(iSize + 1, auiIndex, 1, uiBackCount, auiElemSize))
			return false;
	}
	else
		MoveItems(apBuffer, auiIndex + 1, auiIndex, uiBackCount, auiElemSize);
	++iSize;
	return true;
}

bool BSTArrayBase::SetAllocSize(const IAllocatorFunctor& arFunctor, unsigned int auiAllocSize, unsigned int auiNewAllocSize, unsigned int auiElemSize)
{
	if (auiNewAllocSize < iSize)
		auiNewAllocSize = iSize;
	if (auiNewAllocSize == auiAllocSize)
		return true;
	if (!auiNewAllocSize)
	{
		arFunctor.Deallocate();
		return true;
	}
	if (auiNewAllocSize <= auiAllocSize)
		return true;
	if (auiAllocSize)
		return arFunctor.Reallocate(auiNewAllocSize, iSize, 0, 0, auiElemSize);
	return arFunctor.Allocate(auiNewAllocSize, auiElemSize);
}

void BSTArrayInternal::SwapElements(void* apBuffer1, void* apBuffer2, void* apBufferTemp, unsigned int aiElemSize)
{
	memmove_s(apBufferTemp, aiElemSize, apBuffer1, aiElemSize);
	memmove_s(apBuffer1, aiElemSize, apBuffer2, aiElemSize);
	memmove_s(apBuffer2, aiElemSize, apBufferTemp, aiElemSize);
}

BSTArrayHeapAllocator::BSTArrayHeapAllocator() : pBuffer(nullptr), iAllocSize(0)
{}

bool BSTArrayHeapAllocator::Allocate(unsigned int aiMinNewSize, unsigned int aiElemSize)
{
	unsigned int iCapacity = aiMinNewSize > 4 ? aiMinNewSize : 4;
	pBuffer = MemoryManager::Instance().Allocate(aiElemSize * iCapacity);
	if (pBuffer)
		iAllocSize = iCapacity;
	return pBuffer != nullptr;
}

bool BSTArrayHeapAllocator::Reallocate(unsigned int aiMinNewSizeInItems, unsigned int aiFrontCopyCount, unsigned int aiShiftCount, unsigned int aiBackCopyCount, unsigned int aiElemSize)
{
	unsigned int iCapacity = iAllocSize * 2;
	if (aiMinNewSizeInItems > iCapacity)
		iCapacity = aiMinNewSizeInItems;
	if (!pBuffer)
		return Allocate(iCapacity, aiElemSize);
	char* pNewBuffer = static_cast<char*>(MemoryManager::Instance().Allocate(aiElemSize * iCapacity));
	if (!pNewBuffer)
		return false;
	unsigned int iFrontBytes = aiElemSize * aiFrontCopyCount;
	if (aiFrontCopyCount)
		memmove_s(pNewBuffer, iFrontBytes, pBuffer, iFrontBytes);
	if (aiBackCopyCount)
	{
		unsigned int iBackBytes = aiElemSize * aiBackCopyCount;
		unsigned int iOffset = aiElemSize * (aiFrontCopyCount + aiShiftCount);
		memmove_s(pNewBuffer + iOffset, iBackBytes, static_cast<char*>(pBuffer) + iFrontBytes, iBackBytes);
	}
	void* pOldBuffer = pBuffer;
	MemoryManager::Instance().Deallocate(pOldBuffer);
	pBuffer = pNewBuffer;
	iAllocSize = iCapacity;
	return true;
}

void BSTArrayHeapAllocator::Deallocate()
{
	void* pOldBuffer = pBuffer;
	MemoryManager::Instance().Deallocate(pOldBuffer);
	pBuffer = nullptr;
	iAllocSize = 0;
}

BSTAlignedHeapArrayAllocatorBase::BSTAlignedHeapArrayAllocatorBase() : pBuffer(nullptr), iAllocSize(0)
{}

bool BSTAlignedHeapArrayAllocatorBase::Allocate(unsigned char acAlignment, unsigned int aiMinNewSize, unsigned int aiElemSize)
{
	unsigned int iCapacity = aiMinNewSize > 4 ? aiMinNewSize : 4;
	pBuffer = MemoryManager::Instance().AllocateAligned(aiElemSize * iCapacity, acAlignment);
	if (pBuffer)
		iAllocSize = iCapacity;
	return pBuffer != nullptr;
}

bool BSTAlignedHeapArrayAllocatorBase::Reallocate(unsigned char acAlignment, unsigned int aiMinNewSizeInItems, unsigned int aiFrontCopyCount, unsigned int aiShiftCount, unsigned int aiBackCopyCount, unsigned int aiElemSize)
{
	unsigned int iCapacity = iAllocSize * 2;
	if (aiMinNewSizeInItems > iCapacity)
		iCapacity = aiMinNewSizeInItems;
	if (!pBuffer)
		return Allocate(acAlignment, iCapacity, aiElemSize);
	char* pNewBuffer = static_cast<char*>(MemoryManager::Instance().AllocateAligned(aiElemSize * iCapacity, acAlignment));
	if (!pNewBuffer)
		return false;
	unsigned int iFrontBytes = aiElemSize * aiFrontCopyCount;
	if (aiFrontCopyCount)
		memmove_s(pNewBuffer, iFrontBytes, pBuffer, iFrontBytes);
	if (aiBackCopyCount)
	{
		unsigned int iBackBytes = aiElemSize * aiBackCopyCount;
		unsigned int iOffset = aiElemSize * (aiFrontCopyCount + aiShiftCount);
		memmove_s(pNewBuffer + iOffset, iBackBytes, static_cast<char*>(pBuffer) + iFrontBytes, iBackBytes);
	}
	void* pOldBuffer = pBuffer;
	if (pOldBuffer)
		MemoryManager::Instance().DeallocateAligned(pOldBuffer);
	pBuffer = pNewBuffer;
	iAllocSize = iCapacity;
	return true;
}

void BSTAlignedHeapArrayAllocatorBase::Deallocate()
{
	void* pOldBuffer = pBuffer;
	if (pOldBuffer)
		MemoryManager::Instance().DeallocateAligned(pOldBuffer);
	pBuffer = nullptr;
	iAllocSize = 0;
}

bool BSScrapArrayAllocator::Allocate(unsigned int auiMinNewSize, unsigned int auiElemSize)
{
	pScrapHeap = MemoryManager::Instance().GetThreadScrapHeap();
	pBuffer = pScrapHeap->Allocate(auiElemSize * auiMinNewSize, 8);
	if (pBuffer)
		uiAllocSize = auiMinNewSize;
	return pBuffer != nullptr;
}

bool BSScrapArrayAllocator::Reallocate(unsigned int aMinNewSizeInItems, unsigned int aFrontCopyCount, unsigned int aShiftCount, unsigned int aBackCopyCount, unsigned int aElemSize)
{
	unsigned int uiCapacity = uiAllocSize * 2;
	if (aMinNewSizeInItems > uiCapacity)
		uiCapacity = aMinNewSizeInItems;
	if (!pBuffer)
		return Allocate(uiCapacity, aElemSize);
	char* pNewBuffer = static_cast<char*>(pScrapHeap->Allocate(aElemSize * uiCapacity, 8));
	if (!pNewBuffer)
		return false;
	unsigned int uiFrontBytes = aElemSize * aFrontCopyCount;
	if (aFrontCopyCount)
		memmove_s(pNewBuffer, uiFrontBytes, pBuffer, uiFrontBytes);
	if (aBackCopyCount)
	{
		unsigned int uiBackBytes = aElemSize * aBackCopyCount;
		unsigned int uiOffset = aElemSize * (aFrontCopyCount + aShiftCount);
		memmove_s(pNewBuffer + uiOffset, uiBackBytes, static_cast<char*>(pBuffer) + uiFrontBytes, uiBackBytes);
	}
	pScrapHeap->Deallocate(pBuffer);
	pBuffer = pNewBuffer;
	uiAllocSize = uiCapacity;
	return true;
}

void BSScrapArrayAllocator::Deallocate()
{
	if (pBuffer)
	{
		pScrapHeap->Deallocate(pBuffer);
		pBuffer = nullptr;
		pScrapHeap = nullptr;
	}
	uiAllocSize = 0;
}

BSTAlignedScrapArrayAllocatorBase::BSTAlignedScrapArrayAllocatorBase() : pScrapHeap(MemoryManager::Instance().GetThreadScrapHeap()), pBuffer(nullptr), iAllocSize(0)
{}

bool BSTAlignedScrapArrayAllocatorBase::Allocate(unsigned char aucAlignment, unsigned int aiMinNewSize, unsigned int aiElemSize)
{
	unsigned int iCapacity = aiMinNewSize > 4 ? aiMinNewSize : 4;
	pBuffer = pScrapHeap->Allocate(aiElemSize * iCapacity, aucAlignment);
	if (pBuffer)
		iAllocSize = iCapacity;
	return pBuffer != nullptr;
}

bool BSTAlignedScrapArrayAllocatorBase::Reallocate(unsigned char aucAlignment, unsigned int aiMinNewSizeInItems, unsigned int aiFrontCopyCount, unsigned int aiShiftCount, unsigned int aiBackCopyCount, unsigned int aiElemSize)
{
	unsigned int iCapacity = iAllocSize * 2;
	if (aiMinNewSizeInItems > iCapacity)
		iCapacity = aiMinNewSizeInItems;
	if (!pBuffer)
		return Allocate(aucAlignment, iCapacity, aiElemSize);
	char* pNewBuffer = static_cast<char*>(pScrapHeap->Allocate(aiElemSize * iCapacity, aucAlignment));
	if (!pNewBuffer)
		return false;
	unsigned int iFrontBytes = aiElemSize * aiFrontCopyCount;
	if (aiFrontCopyCount)
		memmove_s(pNewBuffer, iFrontBytes, pBuffer, iFrontBytes);
	if (aiBackCopyCount)
	{
		unsigned int iBackBytes = aiElemSize * aiBackCopyCount;
		unsigned int iOffset = aiElemSize * (aiFrontCopyCount + aiShiftCount);
		memmove_s(pNewBuffer + iOffset, iBackBytes, static_cast<char*>(pBuffer) + iFrontBytes, iBackBytes);
	}
	void* pOldBuffer = pBuffer;
	pScrapHeap->Deallocate(pOldBuffer);
	pBuffer = pNewBuffer;
	iAllocSize = iCapacity;
	return true;
}

void BSTAlignedScrapArrayAllocatorBase::Deallocate()
{
	void* pOldBuffer = pBuffer;
	pScrapHeap->Deallocate(pOldBuffer);
	pBuffer = nullptr;
	iAllocSize = 0;
}

BSTAlignedScrapArrayAllocatorBase::~BSTAlignedScrapArrayAllocatorBase() = default;
