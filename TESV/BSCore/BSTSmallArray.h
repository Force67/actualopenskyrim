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

template<unsigned int Size>
class BSTSmallArrayHeapAllocator
{
public:
	BSTSmallArrayHeapAllocator() { ExtendedBuffer.Core.Initialize(); }
	bool Allocate(unsigned int uiSize, unsigned int uiElementSize)
	{
		return ExtendedBuffer.Core.Allocate(uiSize, uiElementSize, Size);
	}
	bool Reallocate(unsigned int uiSize, unsigned int uiFront, unsigned int uiShift, unsigned int uiBack, unsigned int uiElementSize)
	{
		return ExtendedBuffer.Core.Reallocate(uiSize, uiFront, uiShift, uiBack, uiElementSize, Size);
	}
	void Deallocate() { ExtendedBuffer.Core.Deallocate(); }
	void* QBuffer() { return ExtendedBuffer.Core.QBuffer(); }
	const void* QBuffer() const { return ExtendedBuffer.Core.QBuffer(); }
	unsigned int QAllocSize() const { return ExtendedBuffer.Core.uiAllocSize & 0x7fffffffu; }

	union
	{
		BSTSmallArrayHeapAllocatorCore Core;
		unsigned char BufferSpaceExtension[Size + 8];
	} ExtendedBuffer;
};

#include "BSTArray.h"

template<class T, unsigned int Size>
class BSTSmallArray : public BSTArray<T, BSTSmallArrayHeapAllocator<Size * sizeof(T)>>
{
public:
	explicit BSTSmallArray(unsigned int uiReserveSize = 0) :
		BSTArray<T, BSTSmallArrayHeapAllocator<Size * sizeof(T)>>(uiReserveSize) {}
};
static_assert(sizeof(BSTSmallArray<void*, 4>) == 48);
