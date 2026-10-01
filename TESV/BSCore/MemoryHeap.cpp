#include "BSCore/MemoryHeap.h"
#include "BSCore/HeapBlocks.h"

MemoryHeap::MemoryHeap(size_t aiSize, size_t aiInitialSize, const char* apName, bool abSupportsSwapping, bool abAllowDecommits) :
	AbstractHeap(aiSize, aiInitialSize, apName, 0x10000, abSupportsSwapping, abAllowDecommits),
	bDeletingHeap(false)
{
	InitHeap();
}

MemoryHeap::~MemoryHeap()
{
	DeleteHeap();
	bDeletingHeap = true;
}

void* MemoryHeap::Allocate(size_t auiSize, uint32_t auiAlignment)
{
	return BaseAllocate(auiSize, auiAlignment, true);
}

void MemoryHeap::Deallocate(void* apPointer, uint32_t)
{
	if (apPointer)
		BaseFree(apPointer);
}

size_t MemoryHeap::Size(const void* apPointer) const
{
	return HeapBlock::MemToBlock(apPointer)->GetSize();
}

size_t MemoryHeap::TotalSize(const void* apPointer) const
{
	return HeapBlock::MemToBlock(apPointer)->GetTotalSize();
}

void MemoryHeap::GetHeapStats(HeapStats* apStats, bool abFullBlockInfo)
{
	AbstractHeap::GetHeapStats(apStats, abFullBlockInfo);
}

size_t MemoryHeap::GetCalculatedMemoryUsed() const
{
	return 0;
}

size_t MemoryHeap::GetReportedMemoryUsed() const
{
	return 0;
}

void* MemoryHeap::DoHeapAllocation(size_t aiSize, size_t aiInitialSize)
{
	void* pMemory = VirtualAlloc(nullptr, aiSize, MEM_RESERVE, PAGE_READWRITE);
	if (pMemory && aiInitialSize && !VirtualAlloc(pMemory, aiInitialSize, MEM_COMMIT, PAGE_READWRITE))
	{
		VirtualFree(pMemory, 0, MEM_RELEASE);
		return nullptr;
	}
	return pMemory;
}

void MemoryHeap::DoHeapFree(void* apPtr)
{
	VirtualFree(apPtr, 0, MEM_RELEASE);
}

size_t MemoryHeap::CreateMorePages(void* apMem, size_t aiCurrentSize, size_t aiRequestedBytes)
{
	const size_t uiSize = (iPageSize + aiRequestedBytes - 1) & ~size_t(iPageSize - 1);
	return VirtualAlloc(static_cast<char*>(apMem) + aiCurrentSize, uiSize, MEM_COMMIT, PAGE_READWRITE) ? uiSize : 0;
}

size_t MemoryHeap::CleanExtraPages(void* apMem, size_t aiCurrentSize, size_t aiFreeBytes)
{
	const size_t uiSize = aiFreeBytes & ~size_t(iPageSize - 1);
	if (uiSize >= iPageSize && VirtualFree(static_cast<char*>(apMem) + aiCurrentSize - uiSize, uiSize, MEM_DECOMMIT))
		return uiSize;
	return 0;
}

void MemoryHeap::DecommitPages(HeapBlock* apBlock)
{
	const uintptr_t uiBlock = reinterpret_cast<uintptr_t>(apBlock);
	const uintptr_t uiMask = ~uintptr_t(iPageSize - 1);
	const uintptr_t uiStart = (uiBlock + iPageSize + 63) & uiMask;
	const uintptr_t uiEnd = (uiBlock + (apBlock->uiMemSize & HeapBlock::SIZE_MASK) + sizeof(HeapBlock)) & uiMask;
	if (uiStart < uiEnd)
	{
		VirtualFree(reinterpret_cast<void*>(uiStart), uiEnd - uiStart, MEM_DECOMMIT);
		apBlock->uiMemSize |= HeapBlock::DECOMMITTED;
	}
}

bool MemoryHeap::BlockIsInHeap(const void* apPointer) const
{
	return PointerInHeap(apPointer) && !((static_cast<const HeapBlock*>(apPointer) - 1)->uiMemSize & HeapBlock::FREE);
}

UnitTestMemoryHeap::UnitTestMemoryHeap(size_t aiSize, size_t aiInitialSize, const char* apName, bool abSupportsSwapping, bool abAllowDecommits) :
	MemoryHeap(aiSize, aiInitialSize, apName, abSupportsSwapping, abAllowDecommits)
{
}

UnitTestMemoryHeap::~UnitTestMemoryHeap() = default;

bool UnitTestMemoryHeap::TestMergeFreeBlocks(void* apFirstBlock, void* apSecondBlock)
{
	const int32_t iBefore = iNumBlocks;
	if (reinterpret_cast<uintptr_t>(apFirstBlock) > reinterpret_cast<uintptr_t>(apSecondBlock))
	{
		void* pSwap = apFirstBlock;
		apFirstBlock = apSecondBlock;
		apSecondBlock = pSwap;
	}
	HeapBlock* pFirst = static_cast<HeapBlock*>(apFirstBlock) - 1;
	HeapBlock* pSecond = static_cast<HeapBlock*>(apSecondBlock) - 1;
	const size_t uiFirstSize = pFirst->uiMemSize & HeapBlock::SIZE_MASK;
	if (reinterpret_cast<char*>(pFirst + 1) + uiFirstSize != reinterpret_cast<char*>(pSecond) || pSecond->pPrevious != pFirst)
		return false;
	const size_t uiMergedSize = uiFirstSize + (((pSecond->uiMemSize & HeapBlock::SIZE_MASK) + 15) & ~size_t(15)) + sizeof(HeapBlock);
	if ((pFirst->uiMemSize & HeapBlock::FREE) && (pSecond->uiMemSize & HeapBlock::FREE))
		MergeFreeBlocks(pFirst, pSecond);
	return (pFirst->uiMemSize & HeapBlock::SIZE_MASK) == uiMergedSize && iBefore == iNumBlocks + 1;
}

void UnitTestMemoryHeap::DeallocateWithoutDefragment(void* apPointer)
{
	if (!apPointer)
		return;
	EnterCriticalSection(&CriticalSectionO);
	HeapBlock* pBlock = static_cast<HeapBlock*>(apPointer) - 1;
	iBlockMemAllocated -= ((pBlock->uiMemSize & HeapBlock::SIZE_MASK) + 15) & ~size_t(15);
	iBlockMemAllocated -= sizeof(HeapBlock);
	AddBlockToFreeList(pBlock);
	LeaveCriticalSection(&CriticalSectionO);
}

size_t UnitTestMemoryHeap::QLargeBlockSize() const
{
	return 528;
}

const HeapBlockFreeHead* UnitTestMemoryHeap::QLargeBlockTree(uint32_t auiSize) const
{
	return LargeFreeTreeA[GetLargeFreeTreeForSize(auiSize)];
}

const HeapBlock* UnitTestMemoryHeap::QSmallBlockList(uint32_t auiSize) const
{
	const size_t uiIndex = GetSmallFreeListForSize(auiSize);
	return uiIndex < 32 ? SmallFreeListsA[uiIndex] : nullptr;
}

size_t UnitTestMemoryHeap::QAlignSize() const
{
	return 16;
}
