#pragma once

#include <cstddef>

class BSTSmallArrayHeapAllocatorCore
{
public:
	void Initialize() { uiAllocSize = 0x80000000u; }
	void* QBuffer() { return uiAllocSize & 0x80000000u ? &Buffer : Buffer.pExternal; }
	const void* QBuffer() const { return uiAllocSize & 0x80000000u ? &Buffer : Buffer.pExternal; }
	bool Allocate(unsigned int auiMinNewSize, unsigned int auiElemSize, unsigned int auiActualInternalBufferSize);
	bool Reallocate(unsigned int auiMinNewSizeInItems, unsigned int auiFrontCopyCount, unsigned int auiShiftCount,
		unsigned int auiBackCopyCount, unsigned int auiElemSize, unsigned int auiActualInternalBufferSize);
	void Deallocate();
	void Adopt(BSTSmallArrayHeapAllocatorCore& arOther, unsigned int auiOtherItemCount,
		unsigned int auiElemSize, unsigned int auiActualInternalBufferSize);

	unsigned int uiAllocSize;
	union
	{
		void* pExternal;
		unsigned char cInternal[8];
	} Buffer;
};
static_assert(sizeof(BSTSmallArrayHeapAllocatorCore) == 0x10);
static_assert(offsetof(BSTSmallArrayHeapAllocatorCore, Buffer) == 8);
