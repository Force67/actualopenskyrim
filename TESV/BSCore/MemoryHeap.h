#pragma once

#include "BSCore/AbstractHeap.h"

class MemoryHeap : public AbstractHeap
{
public:
	MemoryHeap(size_t aiSize, size_t aiInitialSize, const char* apName, bool abSupportsSwapping, bool abAllowDecommits);
	~MemoryHeap() override;

	void* Allocate(size_t auiSize, uint32_t auiAlignment) override;
	void Deallocate(void* apPointer, uint32_t auiAlignment) override;
	size_t Size(const void* apPointer) const override;
	size_t TotalSize(const void* apPointer) const override;
	void GetHeapStats(HeapStats* apStats, bool abFullBlockInfo) override;

	void* DoHeapAllocation(size_t aiSize, size_t aiInitialSize) override;
	void DoHeapFree(void* apPtr) override;
	size_t CreateMorePages(void* apMem, size_t aiCurrentSize, size_t aiRequestedBytes) override;
	size_t CleanExtraPages(void* apMem, size_t aiCurrentSize, size_t aiFreeBytes) override;
	void DecommitPages(HeapBlock* apBlock) override;

	size_t GetCalculatedMemoryUsed() const;
	size_t GetReportedMemoryUsed() const;
	bool BlockIsInHeap(const void* apPointer) const;

	bool bDeletingHeap;
};
static_assert(sizeof(MemoryHeap) == 0x2B0);
static_assert(offsetof(MemoryHeap, bDeletingHeap) == 0x2A8);

class UnitTestMemoryHeap : public MemoryHeap
{
public:
	UnitTestMemoryHeap(size_t aiSize, size_t aiInitialSize, const char* apName, bool abSupportsSwapping, bool abAllowDecommits);
	~UnitTestMemoryHeap() override;

	bool TestMergeFreeBlocks(void* apFirstBlock, void* apSecondBlock);
	void DeallocateWithoutDefragment(void* apPointer);
	size_t QLargeBlockSize() const;
	const HeapBlockFreeHead* QLargeBlockTree(uint32_t auiSize) const;
	const HeapBlock* QSmallBlockList(uint32_t auiSize) const;
	size_t QAlignSize() const;
};
static_assert(sizeof(UnitTestMemoryHeap) == 0x2B0);
