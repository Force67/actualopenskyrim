#include "BSCore/AbstractHeap.h"
#include "BSCore/HeapBlocks.h"

#include <cstdlib>
#include <cstring>
#include <intrin.h>

AbstractHeap::AbstractHeap(size_t aiSize, size_t aiInitialSize, const char* apName, uint32_t aiPageSize, bool abSupportsSwapping, bool abAllowDecommits) :
	pName(apName),
	MinFreeBlockSize(48),
	iPageSize(aiPageSize),
	iPageSizeFlag(0),
	MemHeapSize(aiSize),
	iInitialSize(aiInitialSize),
	iCurrentSize(aiInitialSize),
	iWastedMemory(0),
	iMemAllocated(0),
	iMemAllocatedHigh(0),
	iBlockMemAllocated(0),
	pMemHeap(nullptr),
	iNumBlocks(0),
	pBlockHead(nullptr),
	pBlockTail(nullptr),
	iNumFreeBlocks(0),
	bAllowDecommits(abAllowDecommits),
	bSupportsSwapping(abSupportsSwapping)
{
	InitializeCriticalSection(&CriticalSectionO);
}

AbstractHeap::~AbstractHeap()
{
	DeleteCriticalSection(&CriticalSectionO);
}

const char* AbstractHeap::GetName() const
{
	return pName;
}

bool AbstractHeap::PointerInHeap(const void* apBlock) const
{
	const auto uiPointer = reinterpret_cast<uintptr_t>(apBlock);
	const auto uiBase = reinterpret_cast<uintptr_t>(pMemHeap);
	return uiPointer >= uiBase && uiPointer < uiBase + MemHeapSize;
}

void AbstractHeap::GetMemoryStats(MemoryStats* apStats)
{
	apStats->pName = pName;
	apStats->uiUsedSize = static_cast<uint32_t>(iMemAllocated);
	apStats->uiCommittedSize = static_cast<uint32_t>(iCurrentSize);
	apStats->uiReservedSize = static_cast<uint32_t>(MemHeapSize);
	// This entry stores the overhead as a full machine word.
	const size_t uiOverhead = sizeof(AbstractHeap) + sizeof(HeapBlock) * static_cast<size_t>(iNumBlocks);
	std::memcpy(&apStats->uiOverhead, &uiOverhead, sizeof(uiOverhead));
	apStats->uiFreeSize = static_cast<uint32_t>(iCurrentSize - iBlockMemAllocated);
}

void AbstractHeap::GetHeapStats(HeapStats* apStats, bool abFullBlockInfo)
{
	EnterCriticalSection(&CriticalSectionO);
	apStats->pHeapName = pName;
	apStats->uiMemHeapSize = MemHeapSize;
	apStats->uiMemHeapCommitted = iCurrentSize;
	apStats->uiMemAllocatedToBlocks = iMemAllocated;
	apStats->iNumBlocks = iNumBlocks;
	apStats->iNumFreeBlocks = iNumFreeBlocks;
	apStats->uiHeapOverhead = sizeof(AbstractHeap);
	apStats->uiFreeListOverhead = sizeof(SmallFreeListsA) + sizeof(LargeFreeTreeA);
	apStats->uiBlockOverhead = static_cast<uint32_t>(iNumBlocks) * uint32_t(sizeof(HeapBlock));
	apStats->uiMemFreeInBlocks = iMemAllocated - iBlockMemAllocated;
	apStats->uiMemUsedInBlocks = iBlockMemAllocated;
	apStats->uiSmallestFreeBlock = 0xFFFFFFF;
	apStats->uiLargestFreeBlock = 0;
	if (abFullBlockInfo)
	{
		apStats->uiMemFreeInBlocks = 0;
		apStats->uiMemUsedInBlocks = 0;
		for (HeapBlock* pBlock = pBlockHead; pBlock;)
		{
			const size_t uiSize = pBlock->uiMemSize & HeapBlock::SIZE_MASK;
			if (!(pBlock->uiMemSize & HeapBlock::FREE) || (pBlock->uiMemSize & HeapBlock::DECOMMITTED))
				apStats->uiMemUsedInBlocks += uiSize + sizeof(HeapBlock);
			else
			{
				apStats->uiMemFreeInBlocks += uiSize + sizeof(HeapBlock);
				if (uiSize < apStats->uiSmallestFreeBlock)
					apStats->uiSmallestFreeBlock = uiSize;
				if (uiSize > apStats->uiLargestFreeBlock)
					apStats->uiLargestFreeBlock = uiSize;
			}
			if (pBlock == pBlockTail)
				break;
			pBlock = reinterpret_cast<HeapBlock*>(reinterpret_cast<char*>(pBlock + 1) + uiSize);
		}
		if (!apStats->iNumFreeBlocks)
			apStats->uiSmallestFreeBlock = 0;
	}
	apStats->uiTotalFree = apStats->uiMemFreeInBlocks + apStats->uiMemHeapSize - apStats->uiMemAllocatedToBlocks;
	LeaveCriticalSection(&CriticalSectionO);
}

bool AbstractHeap::ShouldTrySmallBlockPools(size_t, MEM_CONTEXT)
{
	return true;
}

uint32_t AbstractHeap::GetPageSize() const
{
	return iPageSize;
}

void AbstractHeap::InitHeap()
{
	pMemHeap = static_cast<char*>(DoHeapAllocation(MemHeapSize, iCurrentSize));
	if (!pMemHeap)
		std::exit(0);
	std::memset(SmallFreeListsA, 0, sizeof(SmallFreeListsA));
	std::memset(LargeFreeTreeA, 0, sizeof(LargeFreeTreeA));
}

size_t AbstractHeap::CreateMorePages(void*, size_t, size_t)
{
	return 0;
}

size_t AbstractHeap::CleanExtraPages(void*, size_t, size_t)
{
	return 0;
}

void AbstractHeap::DecommitPages(HeapBlock*)
{
}

void AbstractHeap::CommitPages(HeapBlock*, uint32_t)
{
}

void* AbstractHeap::AllocateAlignImpl(size_t auiSize, uint32_t auiAlignment)
{
	return BaseAllocate(auiSize, auiAlignment, true);
}

void* AbstractHeap::TryAllocateImpl(size_t auiSize, uint32_t auiAlignment)
{
	return BaseAllocate(auiSize, auiAlignment, false);
}

size_t AbstractHeap::GetLargeFreeTreeForSize(size_t auiSize)
{
	unsigned long uiIndex;
	_BitScanReverse64(&uiIndex, (auiSize + 1023) >> 10);
	return uiIndex;
}

void AbstractHeap::AddBlockToFreeList(HeapBlock* apBlock)
{
	apBlock->SetAsFree();
	size_t uiSize = apBlock->uiMemSize & HeapBlock::SIZE_MASK;
	apBlock->pNextFree = nullptr;
	apBlock->FreeOrUsed.pPrevFree = nullptr;
	size_t uiIndex = GetSmallFreeListForSize(uiSize);
	if (uiIndex >= 32)
		HeapBlockFreeHead::TreeInsert(LargeFreeTreeA[GetLargeFreeTreeForSize(uiSize)], static_cast<HeapBlockFreeHead*>(apBlock));
	else
	{
		HeapBlock* pHead = SmallFreeListsA[uiIndex];
		if (pHead)
			apBlock->ListInsertBefore(pHead);
		SmallFreeListsA[uiIndex] = apBlock;
	}
	iNumFreeBlocks = int32_t(uint32_t(iNumFreeBlocks) + 1);
}

void AbstractHeap::RemoveBlockFromFreeList(HeapBlock* apBlock)
{
	size_t uiSize = apBlock->uiMemSize & HeapBlock::SIZE_MASK;
	size_t uiIndex = GetSmallFreeListForSize(uiSize);
	if (uiIndex >= 32)
		HeapBlockFreeHead::TreeRemove(static_cast<HeapBlockFreeHead*>(apBlock));
	else
	{
		if (SmallFreeListsA[uiIndex] == apBlock)
			SmallFreeListsA[uiIndex] = apBlock->pNextFree;
		apBlock->ListRemove();
	}
	apBlock->SetAsAllocated();
	iNumFreeBlocks = int32_t(uint32_t(iNumFreeBlocks) - 1);
}

void AbstractHeap::MergeFreeBlocks(HeapBlock* apFirstBlock, HeapBlock* apSecondBlock)
{
	RemoveBlockFromFreeList(apFirstBlock);
	RemoveBlockFromFreeList(apSecondBlock);
	apFirstBlock->InheritDecommitted(apSecondBlock);
	size_t uiSize = (apFirstBlock->uiMemSize & HeapBlock::SIZE_MASK) + (((apSecondBlock->uiMemSize & HeapBlock::SIZE_MASK) + 15) & ~size_t(15)) + sizeof(HeapBlock);
	apFirstBlock->SetSize(uiSize);
	if (apSecondBlock == pBlockTail)
		pBlockTail = apFirstBlock;
	else
		reinterpret_cast<HeapBlock*>(reinterpret_cast<char*>(apSecondBlock + 1) + (apSecondBlock->uiMemSize & HeapBlock::SIZE_MASK))->pPrevious = apFirstBlock;
	iNumBlocks = int32_t(uint32_t(iNumBlocks) - 1);
	AddBlockToFreeList(apFirstBlock);
}

void AbstractHeap::TryAndDefragment(HeapBlockFreeHead* apNewFree)
{
	HeapBlock* pBlock = apNewFree;
	if (pBlock)
	{
		for (HeapBlock* pPrevious = pBlock->pPrevious; pPrevious && (pPrevious->uiMemSize & HeapBlock::FREE); pPrevious = pPrevious->pPrevious)
		{
			MergeFreeBlocks(pPrevious, pBlock);
			pBlock = pPrevious;
		}
		while (pBlock != pBlockTail)
		{
			auto* pNext = pBlock->Next();
			if (!(pNext->uiMemSize & HeapBlock::FREE))
				break;
			MergeFreeBlocks(pBlock, pNext);
		}
	}
	CleanEnd();
}

void AbstractHeap::DeleteHeap()
{
	for (HeapBlock*& rpHead : SmallFreeListsA)
		while (rpHead)
			RemoveBlockFromFreeList(rpHead);
	for (HeapBlockFreeHead*& rpRoot : LargeFreeTreeA)
		while (rpRoot)
			RemoveBlockFromFreeList(rpRoot);
	DoHeapFree(pMemHeap);
	pMemHeap = nullptr;
	MemHeapSize = 0;
}

void AbstractHeap::BaseFree(void* apPointer)
{
	if (!apPointer)
		return;
	EnterCriticalSection(&CriticalSectionO);
	auto* pBlock = HeapBlock::MemToBlock(apPointer);
	iBlockMemAllocated -= sizeof(HeapBlock) + (((pBlock->uiMemSize & HeapBlock::SIZE_MASK) + 15) & ~size_t(15));
	AddBlockToFreeList(pBlock);
	TryAndDefragment(static_cast<HeapBlockFreeHead*>(pBlock));
	LeaveCriticalSection(&CriticalSectionO);
}

void* AbstractHeap::BaseAllocate(size_t auiSize, size_t auiAlignment, bool abSplitAndCommit)
{
	EnterCriticalSection(&CriticalSectionO);
	void* pResult = [&]() -> void*
	{
		size_t uiAlignment = auiAlignment > 16 ? auiAlignment : 16;
		size_t uiMask = ~(uiAlignment - 1);
		size_t uiSize = ((auiSize > 16 ? auiSize : 16) + uiAlignment - 1) & uiMask;
		if (uiSize >= MemHeapSize)
			return nullptr;
		auto AlignedStart = [&](HeapBlock* apBlock)
		{
			uintptr_t uiStart = reinterpret_cast<uintptr_t>(apBlock + 1);
			uintptr_t uiAligned = (uiStart + uiAlignment - 1) & uiMask;
			if (apBlock == pBlockHead && uiAligned != uiStart)
				uiAligned = (uiStart + MinFreeBlockSize + uiAlignment - 1) & uiMask;
			return uiAligned;
		};
		HeapBlock* pBlock = nullptr;
		size_t uiSmallIndex = GetSmallFreeListForSize(uiSize);
		if (uiSmallIndex < 32)
		{
			do
			{
				for (HeapBlock* pNode = SmallFreeListsA[uiSmallIndex]; pNode; pNode = pNode->pNextFree)
				{
					if (AlignedStart(pNode) + uiSize <= reinterpret_cast<uintptr_t>(pNode + 1) + (pNode->uiMemSize & HeapBlock::SIZE_MASK))
					{
						pBlock = pNode;
						break;
					}
				}
				if (pBlock || abSplitAndCommit)
					break;
			} while (++uiSmallIndex < 32);
		}
		if (!pBlock && (uiSmallIndex >= 32 || !abSplitAndCommit))
		{
			for (size_t uiIndex = GetLargeFreeTreeForSize(uiSize); uiIndex < 32 && !pBlock; ++uiIndex)
			{
				for (HeapBlockFreeHead* pHead = HeapBlockFreeHead::TreeSearch(LargeFreeTreeA[uiIndex], uiSize); pHead && !pBlock; pHead = HeapBlockFreeHead::GetSuccessor(pHead))
				{
					size_t uiBestScore = size_t(-1);
					for (HeapBlock* pNode = pHead; pNode && uiBestScore; pNode = pNode->pNextFree)
					{
						uintptr_t uiStart = reinterpret_cast<uintptr_t>(pNode + 1);
						uintptr_t uiEnd = uiStart + (pNode->uiMemSize & HeapBlock::SIZE_MASK);
						uintptr_t uiAligned = AlignedStart(pNode);
						if (uiAligned + uiSize > uiEnd || (!abSplitAndCommit && uiEnd - (uiAligned + uiSize) >= MinFreeBlockSize))
							continue;
						size_t uiScore = uiAligned - uiStart;
						if (uiScore >= MinFreeBlockSize)
							uiScore -= sizeof(HeapBlock);
						if (pNode == pHead)
							++uiScore;
						if (uiScore < uiBestScore)
						{
							uiBestScore = uiScore;
							pBlock = pNode;
						}
					}
				}
			}
		}
		if (!pBlock && abSplitAndCommit)
		{
			uintptr_t uiEnd = reinterpret_cast<uintptr_t>(pMemHeap) + iMemAllocated;
			size_t uiRequired;
			if (pBlockTail && (pBlockTail->uiMemSize & HeapBlock::FREE))
				uiRequired = uiSize + ((reinterpret_cast<uintptr_t>(pBlockTail + 1) + uiAlignment - 1) & uiMask) - (reinterpret_cast<uintptr_t>(pBlockTail + 1) + (pBlockTail->uiMemSize & HeapBlock::SIZE_MASK));
			else if (pBlockTail)
				uiRequired = uiSize + ((uiEnd + sizeof(HeapBlock) + uiAlignment - 1) & uiMask) - uiEnd;
			else
			{
				uintptr_t uiAligned = (uiEnd + sizeof(HeapBlock) + uiAlignment - 1) & uiMask;
				if (uiAligned != uiEnd + sizeof(HeapBlock))
					uiAligned = (uiEnd + sizeof(HeapBlock) + MinFreeBlockSize + uiAlignment - 1) & uiMask;
				uiRequired = uiAligned + uiSize - uiEnd;
			}
			size_t uiAllocated = iMemAllocated + uiRequired;
			if (iPageSize && uiAllocated > iCurrentSize && uiAllocated <= MemHeapSize)
				iCurrentSize += CreateMorePages(pMemHeap, iCurrentSize, uiAllocated - iCurrentSize);
			if (uiAllocated > iCurrentSize)
				return nullptr;
			if (pBlockTail && (pBlockTail->uiMemSize & HeapBlock::FREE))
			{
				pBlock = pBlockTail;
				RemoveBlockFromFreeList(pBlock);
				pBlock->uiMemSize = (pBlock->uiMemSize & ~HeapBlock::SIZE_MASK) | (uiAllocated + reinterpret_cast<uintptr_t>(pMemHeap) - reinterpret_cast<uintptr_t>(pBlock + 1)) | HeapBlock::DECOMMITTED;
			}
			else
			{
				pBlock = reinterpret_cast<HeapBlock*>(uiEnd);
				if (!VirtualAlloc(pBlock, uiRequired, MEM_COMMIT, PAGE_READWRITE))
					return nullptr;
				pBlock->Init(uiAllocated - iMemAllocated - sizeof(HeapBlock));
				iNumBlocks = int32_t(uint32_t(iNumBlocks) + 1);
				AddBlockToList(pBlock);
			}
			AddBlockToFreeList(pBlock);
			iMemAllocated = uiAllocated;
			if (uiAllocated > iMemAllocatedHigh)
				iMemAllocatedHigh = uiAllocated;
		}
		if (!pBlock)
			return nullptr;
		uintptr_t uiStart = reinterpret_cast<uintptr_t>(pBlock + 1);
		uintptr_t uiEnd = uiStart + (pBlock->uiMemSize & HeapBlock::SIZE_MASK);
		uintptr_t uiAligned = AlignedStart(pBlock);
		bool bDecommitted = (pBlock->uiMemSize & HeapBlock::DECOMMITTED) != 0;
		if (bDecommitted)
		{
			uintptr_t uiCommitEnd = uiAligned + uiSize + 64;
			if (uiCommitEnd > uiEnd)
				uiCommitEnd = uiEnd;
			uiCommitEnd = (uiCommitEnd + iPageSize - 1) & ~size_t(uint32_t(iPageSize - 1));
			if (!VirtualAlloc(pBlock, uiCommitEnd - reinterpret_cast<uintptr_t>(pBlock), MEM_COMMIT, PAGE_READWRITE))
				return nullptr;
			pBlock->uiMemSize &= ~HeapBlock::DECOMMITTED;
			bDecommitted = uiCommitEnd < uiEnd;
		}
		RemoveBlockFromFreeList(pBlock);
		auto* pAllocated = reinterpret_cast<HeapBlock*>(uiAligned - sizeof(HeapBlock));
		bool bSplitTail = uiAligned + uiSize + MinFreeBlockSize <= uiEnd;
		size_t uiAllocatedSize = (bSplitTail ? uiAligned + uiSize : uiEnd) - uiAligned;
		HeapBlock* pFollowing = pBlock == pBlockTail ? nullptr : reinterpret_cast<HeapBlock*>(uiEnd);
		if (uiAligned < uiStart + MinFreeBlockSize)
		{
			if (uiAligned <= uiStart)
				pAllocated->SetAsAllocatedWithSize(uint32_t(uiAllocatedSize));
			else
			{
				HeapBlock* pPrevious = pBlock->pPrevious;
				size_t uiPreviousSize = uiAligned + (pPrevious->uiMemSize & HeapBlock::SIZE_MASK) - uiStart;
				bool bPreviousFree = (pPrevious->uiMemSize & HeapBlock::FREE) != 0;
				if (bPreviousFree)
					RemoveBlockFromFreeList(pPrevious);
				pPrevious->uiMemSize = (pPrevious->uiMemSize & ~HeapBlock::SIZE_MASK) | uiPreviousSize;
				if (bPreviousFree)
					AddBlockToFreeList(pPrevious);
				pAllocated->Init(uiAllocatedSize);
				pAllocated->pPrevious = pPrevious;
				if (pBlock == pBlockTail)
					pBlockTail = pAllocated;
				else
					pFollowing->pPrevious = pAllocated;
				if (bPreviousFree)
					TryAndDefragment(static_cast<HeapBlockFreeHead*>(pPrevious));
			}
		}
		else
		{
			pBlock->uiMemSize = (pBlock->uiMemSize & ~HeapBlock::SIZE_MASK) | (uiAligned - uiStart - sizeof(HeapBlock));
			AddBlockToFreeList(pBlock);
			pAllocated->Init(uiAllocatedSize);
			pAllocated->pPrevious = pBlock;
			iNumBlocks = int32_t(uint32_t(iNumBlocks) + 1);
			if (pBlock == pBlockTail)
				pBlockTail = pAllocated;
			else
				pFollowing->pPrevious = pAllocated;
			TryAndDefragment(static_cast<HeapBlockFreeHead*>(pBlock));
		}
		if (bSplitTail)
		{
			auto* pFree = reinterpret_cast<HeapBlock*>(uiAligned + (pAllocated->uiMemSize & HeapBlock::SIZE_MASK));
			pFree->Init(uiEnd - reinterpret_cast<uintptr_t>(pFree + 1));
			if (bDecommitted)
				pFree->uiMemSize |= HeapBlock::DECOMMITTED;
			pFree->pPrevious = pAllocated;
			iNumBlocks = int32_t(uint32_t(iNumBlocks) + 1);
			if (pBlockTail == pAllocated)
				pBlockTail = pFree;
			else
				pFollowing->pPrevious = pFree;
			AddBlockToFreeList(pFree);
			TryAndDefragment(static_cast<HeapBlockFreeHead*>(pFree));
		}
		iBlockMemAllocated += sizeof(HeapBlock) + (((pAllocated->uiMemSize & HeapBlock::SIZE_MASK) + 15) & ~size_t(15));
		return reinterpret_cast<void*>(uiAligned);
	}();
	LeaveCriticalSection(&CriticalSectionO);
	return pResult;
}
