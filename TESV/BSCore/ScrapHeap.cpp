#include "BSCore/ScrapHeap.h"

#include "BSCore/BSTIntrusiveRBTree.h"
#include "BSCore/MemoryManager.h"

#include <windows.h>

ScrapHeap::ScrapHeap(size_t auiMaxMemory, size_t auiMinCommit) :
	pSmallBlockA{},
	pFreeList(nullptr),
	pLastBlock(nullptr),
	pBaseAddress(nullptr),
	pEndAddress(nullptr),
	pCommitEnd(nullptr),
	ReserveSize((auiMaxMemory + 0xFFFF) & ~size_t(0xFFFF)),
	MinCommit(((auiMinCommit > 0x10000 ? auiMinCommit : 0x10000) + 0xFFFF) & ~size_t(0xFFFF)),
	TotalAllocated(0),
	KeepPagesRequest(0),
	TotalFreeBlocks(0),
	FreeSmallBlocks(0),
	TotalAllocatedBlocks(0),
	PMPBarrier(0)
{
	if (MinCommit > ReserveSize)
		MinCommit = ReserveSize;
	pBaseAddress = VirtualAlloc(nullptr, ReserveSize, MEM_RESERVE, PAGE_READWRITE);
	if (VirtualAlloc(pBaseAddress, MinCommit, MEM_COMMIT, PAGE_READWRITE))
	{
		pLastBlock = static_cast<Block*>(pBaseAddress);
		pCommitEnd = static_cast<char*>(pBaseAddress) + MinCommit;
		pEndAddress = static_cast<char*>(pBaseAddress) + auiMaxMemory;
		pLastBlock->uiSizeFlags = MinCommit - sizeof(Block);
		pLastBlock->pPrev = nullptr;
		InsertFreeBlock(static_cast<FreeTreeNode*>(pLastBlock));
	}
}

ScrapHeap::~ScrapHeap()
{
	VirtualFree(pBaseAddress, 0, MEM_RELEASE);
}

size_t ScrapHeap::QMaxMemory()
{
	return 0x4000000;
}

size_t ScrapHeap::Size(const void* apBlock) const
{
	// This interface reads a block header, rather than the allocation payload.
	return static_cast<const Block*>(apBlock)->uiSizeFlags & 0x3FFFFFFFFFFFFFFF;
}

void ScrapHeap::GetMemoryStats(MemoryStats*)
{
}

bool ScrapHeap::IsHeapOf(const void* apBlock) const
{
	const auto uiAddress = reinterpret_cast<uintptr_t>(apBlock);
	return uiAddress >= reinterpret_cast<uintptr_t>(pBaseAddress) && uiAddress <= reinterpret_cast<uintptr_t>(pEndAddress);
}

bool ScrapHeap::ContainsBlockImpl(const void* apBlock) const
{
	return IsHeapOf(apBlock);
}

void* ScrapHeap::AllocateAlignImpl(size_t auiSize, uint32_t auiAlignment)
{
	return Allocate(auiSize, auiAlignment);
}

void ScrapHeap::DeallocateAlignImpl(void*& arpBlock)
{
	Deallocate(arpBlock);
	arpBlock = nullptr;
}

void* ScrapHeap::Allocate(size_t auiSize, const char*, int, size_t auiAlignment)
{
	return Allocate(auiSize, auiAlignment);
}

void ScrapHeap::SetKeepPages()
{
	++KeepPagesRequest;
}

void ScrapHeap::ClearKeepPages()
{
	--KeepPagesRequest;
	Clean();
}

void ScrapHeap::Clean()
{
	if (KeepPagesRequest || (pLastBlock->uiSizeFlags & 0x8000000000000000))
		return;
	const auto uiEnd = (reinterpret_cast<uintptr_t>(pLastBlock) + 0xFFFF) & ~uintptr_t(0xFFFF);
	if (uiEnd >= reinterpret_cast<uintptr_t>(pCommitEnd) || uiEnd - reinterpret_cast<uintptr_t>(pBaseAddress) <= MinCommit)
		return;
	const size_t uiRemaining = uiEnd - reinterpret_cast<uintptr_t>(pLastBlock);
	if (uiRemaining && uiRemaining < 0x20)
		return;
	auto* pFreeBlock = static_cast<FreeTreeNode*>(pLastBlock);
	if (!uiRemaining)
	{
		pLastBlock = pFreeBlock->pPrev;
		RemoveFreeBlock(pFreeBlock);
	}
	else
	{
		RemoveFreeBlock(pFreeBlock);
		pFreeBlock->uiSizeFlags = uiRemaining - sizeof(Block);
		InsertFreeBlock(pFreeBlock);
	}
	VirtualFree(reinterpret_cast<void*>(uiEnd), reinterpret_cast<uintptr_t>(pCommitEnd) - uiEnd, MEM_DECOMMIT);
	pCommitEnd = reinterpret_cast<char*>(uiEnd);
}

void ScrapHeap::CheckReset()
{
}

unsigned int ScrapHeap::GetSmallBlockIndex(size_t auiSize)
{
	return static_cast<unsigned int>((auiSize + 7) >> 3) - 1;
}

ScrapHeap::Block* ScrapHeap::GetSuccessor(Block* apBlock)
{
	return apBlock == pLastBlock ? nullptr : reinterpret_cast<Block*>(reinterpret_cast<char*>(apBlock) + (apBlock->uiSizeFlags & 0x3FFFFFFFFFFFFFFF) + sizeof(Block));
}

struct FreeTreeNodeAccess
{
	using Node = ScrapHeap::FreeTreeNode;
	static size_t GetKey(Node* apNode) { return apNode->uiSizeFlags & 0x3FFFFFFFFFFFFFFF; }
	static Node*& Left(Node* apNode) { return apNode->pLeftNode; }
	static Node*& Right(Node* apNode) { return apNode->pRightNode; }
	static Node**& Root(Node* apNode) { return apNode->ppRoot; }
	static Node* Parent(Node* apNode) { return reinterpret_cast<Node*>(apNode->uiParentAndBlack & ~size_t(1)); }
	static bool Black(Node* apNode) { return (apNode->uiParentAndBlack & 1) != 0; }
	static void SetParent(Node* apNode, Node* apParent) { apNode->uiParentAndBlack = reinterpret_cast<size_t>(apParent) | (apNode->uiParentAndBlack & 1); }
	static void SetBlack(Node* apNode, bool abBlack) { apNode->uiParentAndBlack = (apNode->uiParentAndBlack & ~size_t(1)) | abBlack; }
};

template class BSTIntrusiveRBTree<ScrapHeap::FreeTreeNode, size_t, FreeTreeNodeAccess>;

void ScrapHeap::InsertFreeBlock(FreeTreeNode* apBlock)
{
	const unsigned int uiIndex = GetSmallBlockIndex(apBlock->uiSizeFlags);
	FreeBlock* pHead;
	if (uiIndex < 6)
	{
		pHead = pSmallBlockA[uiIndex];
		pSmallBlockA[uiIndex] = apBlock;
		++FreeSmallBlocks;
	}
	else
	{
		FreeTreeNode* pCurrent = nullptr;
		BSTIntrusiveRBTree<FreeTreeNode, size_t, FreeTreeNodeAccess>::Insert(pFreeList, apBlock, pCurrent);
		pHead = pCurrent == apBlock ? nullptr : pCurrent;
		if (!pHead)
			apBlock->uiSizeFlags |= 0x4000000000000000;
	}
	if (pHead)
	{
		apBlock->pRight = pHead;
		apBlock->pLeft = pHead->pLeft;
		pHead->pLeft->pRight = apBlock;
		pHead->pLeft = apBlock;
	}
	else
	{
		apBlock->pLeft = apBlock;
		apBlock->pRight = apBlock;
	}
	++TotalFreeBlocks;
}

void ScrapHeap::RemoveFreeBlock(FreeTreeNode* apBlock)
{
	FreeBlock* pNext = apBlock->pRight;
	const unsigned int uiIndex = GetSmallBlockIndex(apBlock->uiSizeFlags);
	if (uiIndex < 6)
	{
		if (pNext == apBlock)
			pSmallBlockA[uiIndex] = nullptr;
		else
		{
			if (pSmallBlockA[uiIndex] == apBlock)
				pSmallBlockA[uiIndex] = pNext;
			pNext->pLeft = apBlock->pLeft;
			apBlock->pLeft->pRight = pNext;
		}
		--FreeSmallBlocks;
	}
	else if (apBlock->uiSizeFlags & 0x4000000000000000)
	{
		if (pNext == apBlock)
			BSTIntrusiveRBTree<FreeTreeNode, size_t, FreeTreeNodeAccess>::Remove(apBlock);
		else
		{
			auto* pReplacement = static_cast<FreeTreeNode*>(pNext);
			pReplacement->uiParentAndBlack = apBlock->uiParentAndBlack;
			auto* pParent = FreeTreeNodeAccess::Parent(apBlock);
			if (!pParent)
				pFreeList = pReplacement;
			else if (pParent->pLeftNode == apBlock)
				pParent->pLeftNode = pReplacement;
			else
				pParent->pRightNode = pReplacement;
			pReplacement->pLeftNode = apBlock->pLeftNode;
			pReplacement->pRightNode = apBlock->pRightNode;
			if (pReplacement->pLeftNode)
				FreeTreeNodeAccess::SetParent(pReplacement->pLeftNode, pReplacement);
			if (pReplacement->pRightNode)
				FreeTreeNodeAccess::SetParent(pReplacement->pRightNode, pReplacement);
			pReplacement->uiSizeFlags |= 0x4000000000000000;
			pReplacement->ppRoot = &pFreeList;
			pReplacement->pLeft = apBlock->pLeft;
			apBlock->pLeft->pRight = pReplacement;
		}
		apBlock->uiSizeFlags &= ~size_t(0x4000000000000000);
	}
	else
	{
		pNext->pLeft = apBlock->pLeft;
		apBlock->pLeft->pRight = pNext;
	}
	--TotalFreeBlocks;
}

void* ScrapHeap::Allocate(size_t auiSize, size_t auiAlignment)
{
	if (auiSize >= static_cast<unsigned int>(QMaxMemory()))
		return nullptr;
	const size_t uiAlignment = ((auiAlignment > 8 ? auiAlignment : 8) + 7) & ~size_t(7);
	const size_t uiSize = ((auiSize > 16 ? auiSize : 16) + 7) & ~size_t(7);
	unsigned int uiPass = 0;
	bool bProcessedProblem = false;
	for (;;)
	{
		FreeTreeNode* pSelected = nullptr;
		uintptr_t uiPayload = 0;
		if (FreeSmallBlocks)
		{
			for (unsigned int uiIndex = static_cast<unsigned int>(uiSize >> 3) - 1; uiIndex < 6 && !pSelected; ++uiIndex)
			{
				FreeBlock* pHead = pSmallBlockA[uiIndex];
				if (!pHead)
					continue;
				FreeBlock* pCandidate = pHead;
				do
				{
					const auto uiAddress = reinterpret_cast<uintptr_t>(pCandidate) + sizeof(Block);
					if (!(uiAddress & (uiAlignment - 1)))
					{
						pSelected = static_cast<FreeTreeNode*>(pCandidate);
						uiPayload = uiAddress;
						break;
					}
					pCandidate = pCandidate->pRight;
				} while (pCandidate != pHead);
			}
		}
		Block* pTail = pLastBlock;
		size_t uiTailFlags = pTail->uiSizeFlags;
		if (!pSelected && !(uiTailFlags & 0x8000000000000000))
		{
			const auto uiStart = reinterpret_cast<uintptr_t>(pTail);
			uintptr_t uiAligned = (uiStart + uiAlignment + 15) & ~(uiAlignment - 1);
			if (uiAligned != uiStart + sizeof(Block))
				uiAligned = (uiStart + uiAlignment + 47) & ~(uiAlignment - 1);
			if (uiAligned + uiSize <= uiStart + (uiTailFlags & 0x3FFFFFFFFFFFFFFF) + sizeof(Block))
			{
				pSelected = static_cast<FreeTreeNode*>(pTail);
				uiPayload = uiAligned;
			}
		}
		if (!pSelected)
		{
			FreeTreeNode* pGroup = nullptr;
			size_t uiBestSize = SIZE_MAX;
			for (FreeTreeNode* pNode = pFreeList; pNode && uiBestSize != uiSize;)
			{
				const size_t uiNodeSize = pNode->uiSizeFlags & 0x3FFFFFFFFFFFFFFF;
				if (uiNodeSize >= uiSize && uiNodeSize < uiBestSize)
				{
					pGroup = pNode;
					uiBestSize = uiNodeSize;
				}
				pNode = uiSize < uiNodeSize ? pNode->pLeftNode : uiSize > uiNodeSize ? pNode->pRightNode : nullptr;
			}
			while (pGroup && !pSelected)
			{
				size_t uiBestPadding = UINT32_MAX;
				FreeBlock* pCandidate = pGroup;
				do
				{
					const auto uiStart = reinterpret_cast<uintptr_t>(pCandidate);
					uintptr_t uiAligned = (uiStart + uiAlignment + 15) & ~(uiAlignment - 1);
					if (uiAligned != uiStart + sizeof(Block))
						uiAligned = (uiStart + uiAlignment + 47) & ~(uiAlignment - 1);
					const size_t uiPadding = uiAligned - uiStart - sizeof(Block) + ((pCandidate->uiSizeFlags >> 62) & 1);
					if (uiAligned + uiSize <= uiStart + (pCandidate->uiSizeFlags & 0x3FFFFFFFFFFFFFFF) + sizeof(Block) && uiPadding < uiBestPadding)
					{
						pSelected = static_cast<FreeTreeNode*>(pCandidate);
						uiPayload = uiAligned;
						uiBestPadding = uiPadding;
					}
					pCandidate = pCandidate->pRight;
				} while (pCandidate != pGroup);
				if (pGroup->pRightNode)
				{
					pGroup = pGroup->pRightNode;
					while (pGroup->pLeftNode)
						pGroup = pGroup->pLeftNode;
				}
				else
				{
					FreeTreeNode* pParent = FreeTreeNodeAccess::Parent(pGroup);
					while (pParent && pParent->pLeftNode != pGroup)
					{
						pGroup = pParent;
						pParent = FreeTreeNodeAccess::Parent(pParent);
					}
					pGroup = pParent;
				}
			}
		}
		if (pSelected)
			RemoveFreeBlock(pSelected);
		else
		{
			const auto uiTailStart = reinterpret_cast<uintptr_t>(pTail);
			uintptr_t uiAligned;
			if (!(uiTailFlags & 0x8000000000000000))
			{
				uiAligned = (uiTailStart + sizeof(Block) + uiAlignment - 1) & ~(uiAlignment - 1);
				// Growth uses the caller's alignment when a leading fragment is needed.
				if (uiAligned != uiTailStart + sizeof(Block))
					uiAligned = (uiTailStart + sizeof(Block) + auiAlignment + 31) & -auiAlignment;
			}
			else
			{
				const auto uiStart = uiTailStart + sizeof(Block) + (uiTailFlags & 0x3FFFFFFFFFFFFFFF);
				uiAligned = (uiStart + uiAlignment + 15) & ~(uiAlignment - 1);
				if (uiAligned - uiStart - 1 <= 0x2E)
					uiAligned = (uiStart + uiAlignment + 47) & ~(uiAlignment - 1);
			}
			const auto uiCommitEnd = (uiAligned + uiSize + 0xFFFF) & ~uintptr_t(0xFFFF);
			if (uiCommitEnd <= reinterpret_cast<uintptr_t>(pEndAddress) && VirtualAlloc(pCommitEnd, uiCommitEnd - reinterpret_cast<uintptr_t>(pCommitEnd), MEM_COMMIT, PAGE_READWRITE))
			{
				uiPayload = uiAligned;
				if (!(uiTailFlags & 0x8000000000000000))
				{
					pSelected = static_cast<FreeTreeNode*>(pLastBlock);
					RemoveFreeBlock(pSelected);
				}
				else
				{
					pSelected = reinterpret_cast<FreeTreeNode*>(reinterpret_cast<char*>(pLastBlock) + (pLastBlock->uiSizeFlags & 0x3FFFFFFFFFFFFFFF) + sizeof(Block));
					pSelected->pPrev = pLastBlock;
					pLastBlock = pSelected;
				}
				pSelected->uiSizeFlags = uiCommitEnd - reinterpret_cast<uintptr_t>(pSelected) - sizeof(Block);
				pCommitEnd = reinterpret_cast<char*>(uiCommitEnd);
			}
		}
		if (!pSelected)
		{
			if (++PMPBarrier == 1)
			{
				bool bAllowSystemAllocs = false;
				uiPass = MemoryManager::Instance().ProcessMemoryProblem(nullptr, uiPass, &bAllowSystemAllocs);
				--PMPBarrier;
				bProcessedProblem = true;
				continue;
			}
			if (bProcessedProblem)
				continue;
			return nullptr;
		}
		auto* pAllocated = reinterpret_cast<Block*>(uiPayload - sizeof(Block));
		size_t uiBlockSize = pSelected->uiSizeFlags & 0x3FFFFFFFFFFFFFFF;
		if (pAllocated != pSelected)
		{
			pAllocated->pPrev = pSelected;
			uiBlockSize -= reinterpret_cast<uintptr_t>(pAllocated) - reinterpret_cast<uintptr_t>(pSelected);
			pAllocated->uiSizeFlags = uiBlockSize;
			pSelected->uiSizeFlags = reinterpret_cast<uintptr_t>(pAllocated) - reinterpret_cast<uintptr_t>(pSelected) - sizeof(Block);
			InsertFreeBlock(pSelected);
			if (pSelected == pLastBlock)
				pLastBlock = pAllocated;
			else
				GetSuccessor(pAllocated)->pPrev = pAllocated;
		}
		if (uiBlockSize - uiSize > 0x20)
		{
			auto* pRemainder = reinterpret_cast<FreeTreeNode*>(uiPayload + uiSize);
			pRemainder->pPrev = pAllocated;
			pRemainder->uiSizeFlags = uiBlockSize - uiSize - sizeof(Block);
			InsertFreeBlock(pRemainder);
			if (pAllocated == pLastBlock)
				pLastBlock = pRemainder;
			else
				GetSuccessor(pRemainder)->pPrev = pRemainder;
			uiBlockSize = uiSize;
		}
		pAllocated->uiSizeFlags = uiBlockSize | 0x8000000000000000;
		TotalAllocated += uiBlockSize;
		++TotalAllocatedBlocks;
		return reinterpret_cast<void*>(uiPayload);
	}
}

void ScrapHeap::Deallocate(void* apBlock)
{
	if (!apBlock || !IsHeapOf(apBlock))
		return;
	--TotalAllocatedBlocks;
	auto* pBlock = reinterpret_cast<FreeTreeNode*>(static_cast<char*>(apBlock) - sizeof(Block));
	TotalAllocated -= pBlock->uiSizeFlags & 0x3FFFFFFFFFFFFFFF;
	for (Block* pPrev = pBlock->pPrev; pPrev && !(pPrev->uiSizeFlags & 0x8000000000000000); pPrev = pBlock->pPrev)
	{
		RemoveFreeBlock(static_cast<FreeTreeNode*>(pPrev));
		pPrev->uiSizeFlags += (pBlock->uiSizeFlags & 0x3FFFFFFFFFFFFFFF) + sizeof(Block);
		if (pBlock == pLastBlock)
			pLastBlock = pPrev;
		pBlock = static_cast<FreeTreeNode*>(pPrev);
	}
	Block* pNext = GetSuccessor(pBlock);
	while (pNext && !(pNext->uiSizeFlags & 0x8000000000000000))
	{
		RemoveFreeBlock(static_cast<FreeTreeNode*>(pNext));
		if (pNext == pLastBlock)
			pLastBlock = pBlock;
		pBlock->uiSizeFlags += (pNext->uiSizeFlags & 0x3FFFFFFFFFFFFFFF) + sizeof(Block);
		pNext = GetSuccessor(pBlock);
	}
	pBlock->uiSizeFlags &= 0x7FFFFFFFFFFFFFFF;
	InsertFreeBlock(pBlock);
	if (pNext)
		pNext->pPrev = pBlock;
	else
		pLastBlock = pBlock;
	Clean();
}
