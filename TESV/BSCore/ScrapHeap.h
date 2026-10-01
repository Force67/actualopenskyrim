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

	struct FreeTreeNode : FreeBlock
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
	void* Allocate(size_t auiSize, const char* apFile, int aiLine, size_t auiAlignment);
	bool IsHeapOf(const void* apBlock) const;
	void SetKeepPages();
	void ClearKeepPages();
	void Clean();
	void CheckReset();
	static unsigned int GetSmallBlockIndex(size_t auiSize);
	Block* GetSuccessor(Block* apBlock);
	void InsertFreeBlock(FreeTreeNode* apBlock);
	void RemoveFreeBlock(FreeTreeNode* apBlock);

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
static_assert(sizeof(ScrapHeap::Block) == 0x10);
static_assert(sizeof(ScrapHeap::FreeBlock) == 0x20);
static_assert(sizeof(ScrapHeap::FreeTreeNode) == 0x40);
static_assert(offsetof(ScrapHeap::FreeTreeNode, ppRoot) == 0x20);
static_assert(offsetof(ScrapHeap::FreeTreeNode, pLeftNode) == 0x28);
static_assert(offsetof(ScrapHeap::FreeTreeNode, pRightNode) == 0x30);
static_assert(offsetof(ScrapHeap::FreeTreeNode, uiParentAndBlack) == 0x38);
static_assert(offsetof(ScrapHeap, pFreeList) == 0x38);
static_assert(offsetof(ScrapHeap, pLastBlock) == 0x40);
static_assert(offsetof(ScrapHeap, pBaseAddress) == 0x48);
static_assert(offsetof(ScrapHeap, pEndAddress) == 0x50);
static_assert(offsetof(ScrapHeap, pCommitEnd) == 0x58);
static_assert(offsetof(ScrapHeap, ReserveSize) == 0x60);
static_assert(offsetof(ScrapHeap, MinCommit) == 0x68);
static_assert(offsetof(ScrapHeap, TotalAllocated) == 0x70);
static_assert(offsetof(ScrapHeap, KeepPagesRequest) == 0x78);
static_assert(offsetof(ScrapHeap, TotalFreeBlocks) == 0x7C);
static_assert(offsetof(ScrapHeap, FreeSmallBlocks) == 0x80);
static_assert(offsetof(ScrapHeap, TotalAllocatedBlocks) == 0x84);
static_assert(offsetof(ScrapHeap, PMPBarrier) == 0x88);
