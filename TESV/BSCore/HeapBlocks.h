#pragma once

#include <cstddef>
#include <cstdint>

struct HeapBlockFreeHead;

struct HeapBlock
{
	static constexpr uint64_t SIZE_MASK = 0x1FFFFFFFFFFFFFFF;
	static constexpr uint64_t DECOMMITTED = uint64_t(1) << 61;
	static constexpr uint64_t FREE_HEAD = uint64_t(1) << 63;
	static constexpr uint64_t FREE = uint64_t(1) << 62;

	void Init(size_t aiSize);
	void InitAsFree(size_t aiSize);
	static unsigned int VerifyMemoryCommitted(const HeapBlock* apBlock, size_t auiExpectedSize, unsigned int auiPageSize);

	size_t GetSize() const;
	size_t GetTotalSize() const;
	static size_t TotalSize(size_t auiSize);
	bool GuardInTact() const;
	bool IsMarkedFree() const;
	bool IsFreeHead() const;
	bool IsDecommitted() const;
	void SetSize(size_t auiSize);
	void SetAsFree();
	void SetAsAllocated();
	void SetAsAllocatedWithSize(size_t auiSize);
	void SetFreeHead(bool abFreeHead);
	void SetAsDecommitted();
	void SetAsNotDecommitted();
	void MarkDecommitted(bool abDecommitted);
	void InheritDecommitted(const HeapBlock* apBlock);
	char* GetMem(size_t aiOffset);
	static HeapBlock* MemToBlock(const void* apMemory);
	HeapBlock* Next();
	HeapBlock* Prev();
	HeapBlock* NextFree();
	HeapBlock* PrevFree();
	void SetPrev(HeapBlock* apBlock);
	void SetNextFree(HeapBlock* apBlock);
	void SetPrevFree(HeapBlock* apBlock);
	bool ListInsertBefore(HeapBlock* apBefore);
	bool ListInsertAfter(HeapBlock* apAfter);
	void ListRemove();
	HeapBlockFreeHead* GetFreeHead();

	uint64_t uiMemSize;
	HeapBlock* pPrevious;
	union
	{
		HeapBlock* pPrevFree;
		uint32_t uiAllocInfo;
	} FreeOrUsed;
	HeapBlock* pNextFree;
};
static_assert(sizeof(HeapBlock) == 0x20);

struct HeapBlockFreeHead : HeapBlock
{
	static void TreeInsert(HeapBlockFreeHead*& arpTreeRoot, HeapBlockFreeHead* apInsertBlock);
	static void TreeRemove(HeapBlockFreeHead* apRemoveBlock);
	static HeapBlockFreeHead* TreeSearch(HeapBlockFreeHead* apTreeRoot, size_t auiMinimumSize);
	static HeapBlockFreeHead* GetPredecessor(HeapBlockFreeHead* apBlock);
	static HeapBlockFreeHead* GetSuccessor(HeapBlockFreeHead* apBlock);

	HeapBlockFreeHead* QLeftChild() const;
	HeapBlockFreeHead* QRightChild() const;
	HeapBlockFreeHead* QParent() const;
	HeapBlockFreeHead** QRootPtr() const;
	void SetLeftChild(HeapBlockFreeHead* apChild);
	void SetRightChild(HeapBlockFreeHead* apChild);
	void SetParent(HeapBlockFreeHead* apParent);
	bool IsBlack() const;
	bool IsRed() const;
	void SetBlack(bool abBlack);
	void SetRed(bool abRed);

	uint64_t uiParentPtrAndBlackBit;
	HeapBlockFreeHead* pLeftChild;
	HeapBlockFreeHead* pRightChild;
	HeapBlockFreeHead** ppRoot;
};
static_assert(sizeof(HeapBlockFreeHead) == 0x40);

#include "BSCore/HeapBlocks.inl"
