#pragma once

#include <cstddef>

namespace BSTArrayInternal
{
	void SwapElements(void* apBuffer1, void* apBuffer2, void* apBufferTemp, unsigned int aiElemSize);
}

class BSTArrayBase
{
public:
	struct IAllocatorFunctor
	{
		virtual bool Allocate(unsigned int auiSize, unsigned int auiElemSize) const = 0;
		virtual bool Reallocate(unsigned int auiSize, unsigned int auiFrontCopyCount, unsigned int auiShiftCount, unsigned int auiBackCopyCount, unsigned int auiElemSize) const = 0;
		virtual void Deallocate() const = 0;
		virtual ~IAllocatorFunctor() = default;
	};

	BSTArrayBase();
	void MoveItems(void* apBuffer, unsigned int auiTo, unsigned int auiFrom, unsigned int auiCount, unsigned int auiElemSize);
	bool InitialReserve(const IAllocatorFunctor& arFunctor, unsigned int auiReserveSize, unsigned int auiElemSize);
	unsigned int AddUninitialized(const IAllocatorFunctor& arFunctor, unsigned int auiAllocSize, unsigned int auiElemSize);
	bool InsertUninitialized(const IAllocatorFunctor& arFunctor, void* apBuffer, unsigned int auiAllocSize, unsigned int auiIndex, unsigned int auiElemSize);
	bool SetAllocSize(const IAllocatorFunctor& arFunctor, unsigned int auiAllocSize, unsigned int auiNewAllocSize, unsigned int auiElemSize);

protected:
	~BSTArrayBase();

public:
	unsigned int iSize;
};
static_assert(sizeof(BSTArrayBase) == 4);
static_assert(sizeof(BSTArrayBase::IAllocatorFunctor) == 8);

class BSTArrayHeapAllocator
{
public:
	BSTArrayHeapAllocator();
	bool Allocate(unsigned int aiMinNewSize, unsigned int aiElemSize);
	bool Reallocate(unsigned int aiMinNewSizeInItems, unsigned int aiFrontCopyCount, unsigned int aiShiftCount, unsigned int aiBackCopyCount, unsigned int aiElemSize);
	void Deallocate();
	void* QBuffer() const { return pBuffer; }
	unsigned int QAllocSize() const { return iAllocSize; }

protected:
	~BSTArrayHeapAllocator() = default;

public:
	void* pBuffer;
	unsigned int iAllocSize;
};
static_assert(sizeof(BSTArrayHeapAllocator) == 0x10);
static_assert(offsetof(BSTArrayHeapAllocator, iAllocSize) == 0x8);
