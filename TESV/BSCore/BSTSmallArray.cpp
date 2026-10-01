#include "BSCore/BSTSmallArray.h"
#include "BSCore/MemoryManager.h"

#include <windows.h>
#include <cstring>

bool BSTSmallArrayHeapAllocatorCore::Allocate(unsigned int auiMinNewSize, unsigned int auiElemSize,
	unsigned int auiActualInternalBufferSize)
{
	if (auiElemSize * auiMinNewSize <= auiActualInternalBufferSize)
	{
		uiAllocSize = (uiAllocSize & 0x80000000u) | ((auiActualInternalBufferSize / auiElemSize) & 0x7FFFFFFFu);
		return true;
	}
	unsigned int uiCapacity = (auiElemSize + 15) / auiElemSize;
	if (auiMinNewSize > uiCapacity)
		uiCapacity = auiMinNewSize;
	Buffer.pExternal = MemoryManager::Instance().Allocate(auiElemSize * uiCapacity);
	if (!Buffer.pExternal)
		return false;
	uiAllocSize = uiCapacity & 0x7FFFFFFFu;
	return true;
}

bool BSTSmallArrayHeapAllocatorCore::Reallocate(unsigned int auiMinNewSizeInItems, unsigned int auiFrontCopyCount,
	unsigned int auiShiftCount, unsigned int auiBackCopyCount, unsigned int auiElemSize,
	unsigned int auiActualInternalBufferSize)
{
	if (!(uiAllocSize & 0x7FFFFFFFu))
		return Allocate(auiMinNewSizeInItems, auiElemSize, auiActualInternalBufferSize);
	void* pOldBuffer = QBuffer();
	void* pNewBuffer;
	unsigned int uiCapacity;
	if (auiElemSize * auiMinNewSizeInItems <= auiActualInternalBufferSize)
	{
		uiCapacity = (auiActualInternalBufferSize / auiElemSize) | 0x80000000u;
		pNewBuffer = &Buffer;
	}
	else
	{
		unsigned int uiMinimum = (auiElemSize + 15) / auiElemSize;
		if (auiMinNewSizeInItems > uiMinimum)
			uiMinimum = auiMinNewSizeInItems;
		uiCapacity = 2 * uiAllocSize;
		if (uiMinimum > uiCapacity)
			uiCapacity = uiMinimum;
		pNewBuffer = MemoryManager::Instance().Allocate(auiElemSize * uiCapacity);
		if (!pNewBuffer)
			return false;
		uiCapacity &= 0x7FFFFFFFu;
	}
	uiAllocSize = uiCapacity;
	if (auiFrontCopyCount && pOldBuffer != pNewBuffer)
	{
		unsigned int uiBytes = auiElemSize * auiFrontCopyCount;
		memmove_s(pNewBuffer, uiBytes, pOldBuffer, uiBytes);
	}
	if (auiBackCopyCount && (pOldBuffer != pNewBuffer || auiShiftCount))
	{
		unsigned int uiBytes = auiElemSize * auiBackCopyCount;
		memmove_s(static_cast<char*>(pNewBuffer) + auiElemSize * (auiFrontCopyCount + auiShiftCount), uiBytes,
			static_cast<char*>(pOldBuffer) + auiElemSize * auiFrontCopyCount, uiBytes);
	}
	if (pOldBuffer != &Buffer)
		MemoryManager::Instance().Deallocate(pOldBuffer);
	if (pNewBuffer != &Buffer)
		Buffer.pExternal = pNewBuffer;
	return true;
}

void BSTSmallArrayHeapAllocatorCore::Deallocate()
{
	if (!(uiAllocSize & 0x80000000u))
		MemoryManager::Instance().Deallocate(Buffer.pExternal);
	uiAllocSize = 0x80000000u;
}

void BSTSmallArrayHeapAllocatorCore::Adopt(BSTSmallArrayHeapAllocatorCore& arOther, unsigned int auiOtherItemCount,
	unsigned int auiElemSize, unsigned int auiActualInternalBufferSize)
{
	unsigned int uiBytes = auiElemSize * auiOtherItemCount;
	if ((arOther.uiAllocSize & 0x80000000u) || uiBytes <= auiActualInternalBufferSize)
	{
		Reallocate(auiOtherItemCount, 0, 0, 0, auiElemSize, auiActualInternalBufferSize);
		std::memcpy(QBuffer(), arOther.QBuffer(), uiBytes);
		arOther.Deallocate();
	}
	else
	{
		if (!(uiAllocSize & 0x80000000u))
			MemoryManager::Instance().Deallocate(Buffer.pExternal);
		uiAllocSize = arOther.uiAllocSize & 0x7FFFFFFFu;
		Buffer.pExternal = arOther.Buffer.pExternal;
		arOther.uiAllocSize = 0x80000000u;
	}
}
