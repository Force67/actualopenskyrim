#pragma once

#include "BSCore/IMemoryStore.h"
#include "BSCore/MemoryDefs.h"

struct HeapStats
{
	const char* pHeapName;
	size_t uiMemHeapSize;
	size_t uiMemHeapCommitted;
	size_t uiMemAllocatedToBlocks;
	int32_t iNumBlocks;
	int32_t iNumFreeBlocks;
	size_t uiMemFreeInBlocks;
	size_t uiMemUsedInBlocks;
	size_t uiSmallestFreeBlock;
	size_t uiLargestFreeBlock;
	size_t uiHeapOverhead;
	size_t uiFreeListOverhead;
	size_t uiBlockOverhead;
	size_t uiTotalFree;
};
static_assert(sizeof(HeapStats) == 0x68);

class IMemoryHeap : public IMemoryStore
{
public:
	~IMemoryHeap() override;

	virtual const char* GetName() const = 0;
	virtual void* Allocate(size_t auiSize, uint32_t auiAlignment) = 0;
	virtual void Deallocate(void* apBlock, uint32_t auiAlignment) = 0;
	virtual bool PointerInHeap(const void* apBlock) const = 0;
	virtual size_t TotalSize(const void* apBlock) const = 0;
	virtual void GetHeapStats(HeapStats* apStats, bool abFullBlockInfo) = 0;
	virtual bool ShouldTrySmallBlockPools(size_t auiSize, MEM_CONTEXT aeContext) = 0;
	virtual uint32_t GetPageSize() const = 0;

private:
	bool ContainsBlockImpl(const void* apBlock) const override;
	void* AllocateAlignImpl(size_t auiSize, uint32_t auiAlignment) override;
	void DeallocateAlignImpl(void*& arpBlock) override;
};
