#pragma once

#include <cstddef>

class BSTAlignedHeapArrayAllocatorBase
{
public:
	BSTAlignedHeapArrayAllocatorBase();
	bool Allocate(unsigned char acAlignment, unsigned int aiMinNewSize, unsigned int aiElemSize);
	bool Reallocate(unsigned char acAlignment, unsigned int aiMinNewSizeInItems, unsigned int aiFrontCopyCount, unsigned int aiShiftCount, unsigned int aiBackCopyCount, unsigned int aiElemSize);
	void Deallocate();
	void* QBuffer() const { return pBuffer; }
	unsigned int QAllocSize() const { return iAllocSize; }

protected:
	~BSTAlignedHeapArrayAllocatorBase();

public:
	void* pBuffer;
	unsigned int iAllocSize;
	char cPadding[4];
};
static_assert(sizeof(BSTAlignedHeapArrayAllocatorBase) == 0x10);
static_assert(offsetof(BSTAlignedHeapArrayAllocatorBase, iAllocSize) == 0x8);

class ScrapHeap;

class BSTAlignedScrapArrayAllocatorBase
{
public:
	BSTAlignedScrapArrayAllocatorBase();
	bool Allocate(unsigned char aucAlignment, unsigned int aiMinNewSize, unsigned int aiElemSize);
	bool Reallocate(unsigned char aucAlignment, unsigned int aiMinNewSizeInItems, unsigned int aiFrontCopyCount, unsigned int aiShiftCount, unsigned int aiBackCopyCount, unsigned int aiElemSize);
	void Deallocate();
	void* QBuffer() const { return pBuffer; }
	unsigned int QAllocSize() const { return iAllocSize; }

protected:
	~BSTAlignedScrapArrayAllocatorBase();

public:
	ScrapHeap* pScrapHeap;
	void* pBuffer;
	unsigned int iAllocSize;
	char cPadding[4];
};
static_assert(sizeof(BSTAlignedScrapArrayAllocatorBase) == 0x18);
static_assert(offsetof(BSTAlignedScrapArrayAllocatorBase, pBuffer) == 0x8);
static_assert(offsetof(BSTAlignedScrapArrayAllocatorBase, iAllocSize) == 0x10);

template<unsigned int Alignment>
struct BSTAlignedHeapArrayAllocator
{
	class Allocator : public BSTAlignedHeapArrayAllocatorBase
	{
	public:
		bool Allocate(unsigned int aiMinNewSize, unsigned int aiElemSize)
		{
			return BSTAlignedHeapArrayAllocatorBase::Allocate(static_cast<unsigned char>(Alignment), aiMinNewSize, aiElemSize);
		}
		bool Reallocate(unsigned int aiMinNewSizeInItems, unsigned int aiFrontCopyCount, unsigned int aiShiftCount, unsigned int aiBackCopyCount, unsigned int aiElemSize)
		{
			return BSTAlignedHeapArrayAllocatorBase::Reallocate(static_cast<unsigned char>(Alignment), aiMinNewSizeInItems, aiFrontCopyCount, aiShiftCount, aiBackCopyCount, aiElemSize);
		}
	};
};
static_assert(sizeof(BSTAlignedHeapArrayAllocator<16>::Allocator) == 0x10);
