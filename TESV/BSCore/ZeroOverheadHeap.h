#pragma once

#include "BSCore/IMemoryHeap.h"
#include "BSCore/BSSpinLock.h"

class ZeroOverheadHeap : public IMemoryHeap
{
public:
	ZeroOverheadHeap(size_t aiSize, const char* apName, uint32_t aiPageSizeFlag);
	~ZeroOverheadHeap() override;
	const char* GetName() const override;
	void* Allocate(size_t auiSize, uint32_t auiAlignment) override;
	void Deallocate(void* apBlock, uint32_t auiAlignment) override;
	bool PointerInHeap(const void* apBlock) const override;
	size_t Size(const void* apBlock) const override;
	size_t TotalSize(const void* apBlock) const override;
	void GetHeapStats(HeapStats* apStats, bool abFullBlockInfo) override;
	void GetMemoryStats(MemoryStats* apStats) override;
	bool ShouldTrySmallBlockPools(size_t auiSize, MEM_CONTEXT aeContext) override;
	uint32_t GetPageSize() const override;

	size_t iSize;
	const char* pName;
	void* pHeap;
	void* pCurrentFree;
	int32_t iAllocations;
	BSSpinLock Lock;

private:
	void* AllocateAlignImpl(size_t auiSize, uint32_t auiAlignment) override;
	bool ContainsBlockImpl(const void* apBlock) const override;
};
static_assert(sizeof(ZeroOverheadHeap) == 0x38);
static_assert(offsetof(ZeroOverheadHeap, pHeap) == 0x18);
static_assert(offsetof(ZeroOverheadHeap, pCurrentFree) == 0x20);
static_assert(offsetof(ZeroOverheadHeap, iAllocations) == 0x28);
static_assert(offsetof(ZeroOverheadHeap, Lock) == 0x2C);

class UnitTestZeroOverheadHeap : public ZeroOverheadHeap
{
public:
	UnitTestZeroOverheadHeap(size_t auiSize, const char* apName, uint32_t auiPageSizeFlag);
	~UnitTestZeroOverheadHeap() override;
	const void* QHeap();
};
static_assert(sizeof(UnitTestZeroOverheadHeap) == 0x38);
