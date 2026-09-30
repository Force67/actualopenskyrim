#pragma once

#include "BSCore/IMemoryStore.h"

// Stack-like heap for short-lived allocations, one per thread.
class ScrapHeap : public IMemoryStore
{
public:
	struct Block
	{
		size_t uiSizeFlags;
		Block* pPrev;
	};

	struct FreeBlock : Block
	{
		FreeBlock* pLeft;
		FreeBlock* pRight;
	};

	struct FreeTreeNode : Block
	{
		FreeTreeNode** ppRoot;
		FreeTreeNode* pLeftNode;
		FreeTreeNode* pRightNode;
		size_t uiParentAndBlack;
	};

	ScrapHeap(size_t auiMaxMemory, size_t auiMinCommit);
	~ScrapHeap() override;

	size_t Size(const void* apBlock) const override;
	void GetMemoryStats(MemoryStats* apStats) override;

	void* Allocate(size_t auiSize, size_t auiAlignment);
	void Deallocate(void* apBlock);

	static size_t QMaxMemory();

private:
	bool ContainsBlockImpl(const void* apBlock) const override;
	void* AllocateAlignImpl(size_t auiSize, uint32_t auiAlignment) override;
	void DeallocateAlignImpl(void*& arpBlock) override;

public:

	FreeBlock* pSmallBlockA[6];
	FreeTreeNode* pFreeList;
	Block* pLastBlock;
	void* pBaseAddress;
	char* pEndAddress;
	char* pCommitEnd;
	size_t ReserveSize;
	size_t MinCommit;
	size_t TotalAllocated;
	unsigned int KeepPagesRequest;
	unsigned int TotalFreeBlocks;
	unsigned int FreeSmallBlocks;
	unsigned int TotalAllocatedBlocks;
	unsigned int PMPBarrier;
};
static_assert(sizeof(ScrapHeap) == 0x90);
