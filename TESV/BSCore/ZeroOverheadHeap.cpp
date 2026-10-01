#include "BSCore/ZeroOverheadHeap.h"

ZeroOverheadHeap::ZeroOverheadHeap(size_t aiSize, const char* apName, uint32_t) :
	iSize(aiSize),
	pName(apName),
	iAllocations(0)
{
	pHeap = VirtualAlloc(nullptr, aiSize, MEM_RESERVE, PAGE_READWRITE);
	if (pHeap && !VirtualAlloc(pHeap, aiSize, MEM_COMMIT, PAGE_READWRITE))
	{
		VirtualFree(pHeap, 0, MEM_RELEASE);
		pHeap = nullptr;
	}
	pCurrentFree = pHeap;
}

ZeroOverheadHeap::~ZeroOverheadHeap()
{
	if (pHeap)
		VirtualFree(pHeap, 0, MEM_RELEASE);
}

const char* ZeroOverheadHeap::GetName() const { return pName; }
uint32_t ZeroOverheadHeap::GetPageSize() const { return 0; }
bool ZeroOverheadHeap::ShouldTrySmallBlockPools(size_t, MEM_CONTEXT) { return false; }
void ZeroOverheadHeap::Deallocate(void*, uint32_t) {}

bool ZeroOverheadHeap::PointerInHeap(const void* apBlock) const
{
	uintptr_t uiPointer = reinterpret_cast<uintptr_t>(apBlock);
	uintptr_t uiBase = reinterpret_cast<uintptr_t>(pHeap);
	return uiPointer > uiBase && uiPointer < uiBase + iSize;
}

bool ZeroOverheadHeap::ContainsBlockImpl(const void* apBlock) const
{
	return ZeroOverheadHeap::PointerInHeap(apBlock);
}

size_t ZeroOverheadHeap::Size(const void* apBlock) const
{
	return *(static_cast<const uintptr_t*>(apBlock) - 1) - reinterpret_cast<uintptr_t>(apBlock);
}

size_t ZeroOverheadHeap::TotalSize(const void* apBlock) const
{
	return Size(apBlock) + 4;
}

void* ZeroOverheadHeap::AllocateAlignImpl(size_t auiSize, uint32_t)
{
	return Allocate(auiSize, 8);
}

void* ZeroOverheadHeap::Allocate(size_t auiSize, uint32_t auiAlignment)
{
	if (!pHeap)
		return nullptr;
	if (!auiAlignment)
		auiAlignment = 8;
	Lock.Lock();
	uintptr_t uiMem = ((reinterpret_cast<uintptr_t>(pCurrentFree) + 7) & ~uintptr_t(7)) + 8;
	uiMem = (uiMem + uint32_t(auiAlignment - 1)) & ~uintptr_t(uint32_t(auiAlignment - 1));
	uintptr_t uiEnd = uiMem + auiSize;
	void* pResult = nullptr;
	if (uiEnd <= reinterpret_cast<uintptr_t>(pHeap) + iSize)
	{
		*(reinterpret_cast<uintptr_t*>(uiMem) - 1) = uiEnd;
		pCurrentFree = reinterpret_cast<void*>(uiEnd);
		pResult = reinterpret_cast<void*>(uiMem);
	}
	Lock.Unlock();
	return pResult;
}

void ZeroOverheadHeap::GetMemoryStats(MemoryStats* apStats)
{
	size_t uiUsed = reinterpret_cast<uintptr_t>(pCurrentFree) - reinterpret_cast<uintptr_t>(pHeap);
	apStats->pName = GetName();
	apStats->uiUsedSize = uiUsed;
	apStats->uiCommittedSize = iSize;
	apStats->uiReservedSize = iSize;
	*reinterpret_cast<size_t*>(&apStats->uiOverhead) = sizeof(ZeroOverheadHeap);
	apStats->uiFreeSize = iSize - uiUsed;
}

void ZeroOverheadHeap::GetHeapStats(HeapStats* apStats, bool)
{
	if (!apStats)
		return;
	size_t uiUsed = reinterpret_cast<uintptr_t>(pCurrentFree) - reinterpret_cast<uintptr_t>(pHeap);
	apStats->pHeapName = GetName();
	apStats->uiMemHeapSize = iSize;
	apStats->uiMemHeapCommitted = iSize;
	apStats->uiMemAllocatedToBlocks = uiUsed;
	apStats->iNumBlocks = iAllocations;
	apStats->iNumFreeBlocks = 0;
	apStats->uiMemFreeInBlocks = 0;
	apStats->uiMemUsedInBlocks = uiUsed;
	apStats->uiSmallestFreeBlock = 0;
	apStats->uiLargestFreeBlock = 0;
	apStats->uiHeapOverhead = sizeof(ZeroOverheadHeap);
	apStats->uiFreeListOverhead = 0;
	apStats->uiBlockOverhead = uint64_t(int64_t(iAllocations)) * 4;
}

UnitTestZeroOverheadHeap::UnitTestZeroOverheadHeap(size_t auiSize, const char* apName, uint32_t auiPageSizeFlag) :
	ZeroOverheadHeap(auiSize, apName, auiPageSizeFlag)
{
}
UnitTestZeroOverheadHeap::~UnitTestZeroOverheadHeap() = default;
const void* UnitTestZeroOverheadHeap::QHeap() { return pHeap; }
