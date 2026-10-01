#pragma once

#include "BSCore/IMemoryHeap.h"
#include "BSCore/HeapBlocks.h"
#include <windows.h>

class AbstractHeap : public IMemoryHeap
{
public:
	AbstractHeap(size_t aiSize, size_t aiInitialSize, const char* apName, uint32_t aiPageSize, bool abSupportsSwapping, bool abAllowDecommits);
	~AbstractHeap() override;

	void GetMemoryStats(MemoryStats* apStats) override;
	const char* GetName() const override;
	bool PointerInHeap(const void* apBlock) const override;
	void GetHeapStats(HeapStats* apStats, bool abFullBlockInfo) override;
	bool ShouldTrySmallBlockPools(size_t auiSize, MEM_CONTEXT aeContext) override;
	uint32_t GetPageSize() const override;

	virtual void* DoHeapAllocation(size_t aiSize, size_t aiInitialSize) = 0;
	virtual void DoHeapFree(void* apPtr) = 0;
	virtual size_t CreateMorePages(void* apMem, size_t aiCurrentSize, size_t aiRequestedBytes);
	virtual size_t CleanExtraPages(void* apMem, size_t aiCurrentSize, size_t aiFreeBytes);
	virtual void DecommitPages(HeapBlock* apBlock);
	virtual void CommitPages(HeapBlock* apBlock, uint32_t auiSize);

	void InitHeap();
	void DeleteHeap();
	void* BaseAllocate(size_t auiSize, size_t auiAlignment, bool abSplitAndCommit);
	void BaseFree(void* apPointer);
	void MergeFreeBlocks(HeapBlock* apFirstBlock, HeapBlock* apSecondBlock);
	void AddBlockToFreeList(HeapBlock* apBlock);
	void RemoveBlockFromFreeList(HeapBlock* apBlock);
	void TryAndDefragment(HeapBlockFreeHead* apNewFree);
	static size_t GetLargeFreeTreeForSize(size_t auiSize);
	static size_t GetSmallFreeListForSize(size_t auiSize)
	{
		size_t uiUnits = (auiSize + 15) >> 4;
		return (uiUnits > 1 ? uiUnits : 1) - 1;
	}
	void AddBlockToList(HeapBlock* apNewBlock)
	{
		if (pBlockTail)
			apNewBlock->SetPrev(pBlockTail);
		else
			pBlockHead = apNewBlock;
		pBlockTail = apNewBlock;
	}
	void CleanEnd();

	CRITICAL_SECTION CriticalSectionO;
	const char* pName;
	size_t MinFreeBlockSize;
	uint32_t iPageSize;
	uint32_t iPageSizeFlag;
	size_t MemHeapSize;
	size_t iInitialSize;
	size_t iCurrentSize;
	size_t iWastedMemory;
	size_t iMemAllocated;
	size_t iMemAllocatedHigh;
	size_t iBlockMemAllocated;
	char* pMemHeap;
	int32_t iNumBlocks;
	HeapBlock* pBlockHead;
	HeapBlock* pBlockTail;
	int32_t iNumFreeBlocks;
	bool bAllowDecommits;
	bool bSupportsSwapping;
	HeapBlock* SmallFreeListsA[32];
	HeapBlockFreeHead* LargeFreeTreeA[32];

private:
	void* AllocateAlignImpl(size_t auiSize, uint32_t auiAlignment) override;
	void* TryAllocateImpl(size_t auiSize, uint32_t auiAlignment) override;
};
static_assert(sizeof(AbstractHeap) == 0x2A8);
static_assert(offsetof(AbstractHeap, CriticalSectionO) == 0x08);
static_assert(offsetof(AbstractHeap, pName) == 0x30);
static_assert(offsetof(AbstractHeap, iPageSize) == 0x40);
static_assert(offsetof(AbstractHeap, pMemHeap) == 0x80);
static_assert(offsetof(AbstractHeap, SmallFreeListsA) == 0xA8);
static_assert(offsetof(AbstractHeap, LargeFreeTreeA) == 0x1A8);

inline void AbstractHeap::CleanEnd()
{
	uintptr_t uiEnd = reinterpret_cast<uintptr_t>(pMemHeap) + iMemAllocated;
	HeapBlock* pTail = pBlockTail;
	HeapBlock* pHead = pBlockHead;
	while (pTail && (pTail->uiMemSize & HeapBlock::FREE) && reinterpret_cast<uintptr_t>(pTail + 1) + (pTail->uiMemSize & HeapBlock::SIZE_MASK) == uiEnd)
	{
		RemoveBlockFromFreeList(pTail);
		iNumBlocks = int32_t(uint32_t(iNumBlocks) - 1);
		uiEnd -= sizeof(HeapBlock) + (((pTail->uiMemSize & HeapBlock::SIZE_MASK) + 15) & ~size_t(15));
		if (pHead == pTail)
		{
			pTail = nullptr;
			pBlockHead = nullptr;
			break;
		}
		pTail = pTail->pPrevious;
	}
	pBlockTail = pTail;
	iMemAllocated = uiEnd - reinterpret_cast<uintptr_t>(pMemHeap);
	if (iPageSize)
	{
		size_t uiMinimum = iMemAllocated >= iInitialSize ? iMemAllocated : iInitialSize;
		size_t uiFreeBytes = iCurrentSize - uiMinimum;
		if (uiFreeBytes >= iPageSize && bAllowDecommits)
			iCurrentSize -= CleanExtraPages(pMemHeap, iCurrentSize, uiFreeBytes);
	}
}
