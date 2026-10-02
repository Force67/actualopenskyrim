#include "BSCore/CompactingStore.h"
#include "BSCore/BSCoreUtils.h"
#include <cstdlib>
#include <cstring>
#include <windows.h>

using namespace CompactingStore;

Accessor::Accessor() : pStoreBlock(nullptr), pAddress(nullptr) {}

Accessor::Accessor(StoreBlock* apStoreBlock) :
	pStoreBlock(apStoreBlock), pAddress(BeginStoreBlockAccess(apStoreBlock))
{
}

Accessor::Accessor(Accessor& arRhs) : pStoreBlock(arRhs.pStoreBlock), pAddress(arRhs.pAddress)
{
	arRhs.pStoreBlock = nullptr;
	arRhs.pAddress = nullptr;
}

Accessor::Accessor(const HandleType& arRhs) :
	pStoreBlock(arRhs.pStoreBlock), pAddress(pStoreBlock ? BeginStoreBlockAccess(pStoreBlock) : nullptr)
{
}

Accessor::~Accessor()
{
	if (pStoreBlock)
		EndStoreBlockAccess(pStoreBlock);
}

Accessor& Accessor::operator=(Accessor& arRhs)
{
	if (pStoreBlock != arRhs.pStoreBlock)
	{
		if (pStoreBlock)
			EndStoreBlockAccess(pStoreBlock);
		pStoreBlock = arRhs.pStoreBlock;
		pAddress = arRhs.pAddress;
		arRhs.pStoreBlock = nullptr;
		arRhs.pAddress = nullptr;
	}
	return *this;
}

Accessor& Accessor::operator=(const HandleType& arRhs)
{
	if (pStoreBlock != arRhs.pStoreBlock)
	{
		if (pStoreBlock)
			EndStoreBlockAccess(pStoreBlock);
		pStoreBlock = arRhs.pStoreBlock;
		pAddress = pStoreBlock ? BeginStoreBlockAccess(pStoreBlock) : nullptr;
	}
	return *this;
}

void Accessor::Replicate(Accessor& arDest) const
{
	if (pStoreBlock != arDest.pStoreBlock)
	{
		if (arDest.pStoreBlock)
			EndStoreBlockAccess(arDest.pStoreBlock);
		arDest.pStoreBlock = pStoreBlock;
		arDest.pAddress = pStoreBlock ? BeginStoreBlockAccess(pStoreBlock) : nullptr;
	}
}

void HandleType::Access(Accessor& arAccessor) const
{
	arAccessor = *this;
}

void* Accessor::BeginStoreBlockAccess(volatile StoreBlock* apStoreBlock)
{
	uint32_t uiWait = 0;
	LONG iFlags = apStoreBlock->uiAccessFlags & 0x7FFFFFFF;
	for (;;)
	{
		LONG iPrevious = InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&apStoreBlock->uiAccessFlags), LONG(uint32_t(iFlags) + 1), iFlags);
		if (iPrevious == iFlags)
			break;
		iFlags = iPrevious & 0x7FFFFFFF;
		if (uiWait >= 10000)
			Sleep(1);
		else
		{
			++uiWait;
			Sleep(0);
		}
	}
	return apStoreBlock->pAddress;
}

void Accessor::EndStoreBlockAccess(volatile StoreBlock* apStoreBlock)
{
	InterlockedDecrement(reinterpret_cast<volatile LONG*>(&apStoreBlock->uiAccessFlags));
}

size_t Store::Size(const void* apBlock) const
{
	const auto* pBlock = static_cast<const StoreBlock*>(apBlock);
	return reinterpret_cast<const BlockHeader*>(static_cast<const char*>(pBlock->pAddress) - 16)->uiSize;
}

size_t Store::QSizePinned(const void* apBlock) const
{
	if (!apBlock)
		return 0;
	auto* pStore = const_cast<Store*>(this);
	pStore->Lock.Lock();
	pStore->CurrentThread = GetCurrentThreadId();
	size_t uiSize = reinterpret_cast<const BlockHeader*>(static_cast<const char*>(apBlock) - 16)->uiSize & 0xFFFFFFFC;
	pStore->CurrentThread = 0;
	pStore->Lock.Unlock();
	return uiSize;
}

bool Store::ContainsBlockImpl(const void* apBlock) const
{
	uintptr_t uiAddress = reinterpret_cast<uintptr_t>(apBlock);
	uintptr_t uiBase = reinterpret_cast<uintptr_t>(pStoreBlockMin);
	return ((uiAddress - uiBase) & 15) == 0 && uiAddress >= uiBase && uiAddress < reinterpret_cast<uintptr_t>(pStoreEnd);
}

void Store::GetMemoryStats(MemoryStats* apStats)
{
	apStats->pName = "Compacting Store";
	uint32_t uiHandles = (uint32_t(reinterpret_cast<uintptr_t>(pStoreEnd)) - uint32_t(reinterpret_cast<uintptr_t>(pStoreBlockMin)) + 0xFFFF) & 0xFFFF0000;
	apStats->uiCommittedSize = uint32_t(uint32_t(reinterpret_cast<uintptr_t>(pAllocEnd)) + uiHandles - uint32_t(reinterpret_cast<uintptr_t>(pAllocBase)));
	apStats->uiFreeSize = uint32_t(uiFree);
	apStats->uiUsedSize = uint32_t(uint32_t(uiAllocated) + 16 * uiNumAllocatedBlocks);
	apStats->uiReservedSize = uint32_t(reinterpret_cast<uintptr_t>(pStoreEnd) - reinterpret_cast<uintptr_t>(pAllocBase));
	const size_t uiOverhead = sizeof(*this);
	std::memcpy(&apStats->uiOverhead, &uiOverhead, sizeof(uiOverhead));
}

void Store::GetHeapStats(HeapStats* apStats, bool abFullBlockInfo)
{
	uint32_t uiHandleBytes = (uint32_t(reinterpret_cast<uintptr_t>(pStoreEnd)) - uint32_t(reinterpret_cast<uintptr_t>(pStoreBlockMin)) + 0xFFFF) & 0xFFFF0000;
	apStats->pHeapName = "Compacting Store";
	apStats->uiMemHeapSize = uint32_t(reinterpret_cast<uintptr_t>(pStoreEnd) - reinterpret_cast<uintptr_t>(pAllocBase));
	apStats->uiMemHeapCommitted = uint32_t(reinterpret_cast<uintptr_t>(pAllocEnd) + uiHandleBytes - reinterpret_cast<uintptr_t>(pAllocBase));
	apStats->uiMemAllocatedToBlocks = size_t(int64_t(int32_t(uint32_t(uiFree) + uint32_t(uiAllocated))));
	apStats->iNumBlocks = int32_t(uiNumFreeBlocks + uiNumAllocatedBlocks);
	apStats->iNumFreeBlocks = int32_t(uiNumFreeBlocks);
	apStats->uiMemFreeInBlocks = size_t(int64_t(int32_t(uiFree)));
	apStats->uiMemUsedInBlocks = size_t(int64_t(int32_t(uiAllocated)));
	apStats->uiSmallestFreeBlock = 0xFFFFFFF;
	apStats->uiLargestFreeBlock = 0;
	apStats->uiHeapOverhead = sizeof(*this);
	apStats->uiBlockOverhead = 32 * size_t(uiNumAllocatedBlocks) + 24 * size_t(uiNumFreeBlocks);
	apStats->uiFreeListOverhead = uiHandleBytes - apStats->uiBlockOverhead;
	if (!abFullBlockInfo)
		return;
	bool bLocked = Lock.TryLock();
	if (!bLocked && CurrentThread != GetCurrentThreadId())
		return;
	apStats->uiMemFreeInBlocks = 0;
	apStats->uiMemUsedInBlocks = 0;
	if (bLocked)
	{
		for (auto* pBlock = static_cast<BlockHeader*>(pAllocBase); reinterpret_cast<uintptr_t>(pBlock) < reinterpret_cast<uintptr_t>(pAllocEnd);)
		{
			size_t uiSize = pBlock->uiSize & 0xFFFFFFFC;
			// The block walk reports every block in the used total.
			apStats->uiMemUsedInBlocks += uiSize;
			if (!(pBlock->uiSize & 1))
			{
				if (uiSize < apStats->uiSmallestFreeBlock)
					apStats->uiSmallestFreeBlock = uiSize;
				if (uiSize > apStats->uiLargestFreeBlock)
					apStats->uiLargestFreeBlock = uiSize;
			}
			pBlock = reinterpret_cast<BlockHeader*>(reinterpret_cast<char*>(pBlock) + uiSize);
		}
		Lock.Unlock();
	}
}

size_t Store::QListIndexForSize(size_t auiSize) const
{
	return uint32_t((auiSize + 7) >> 3) - 1u;
}

void Store::RemoveFreeBlock(FreeBlock* apBlock, size_t auiSize)
{
	size_t uiIndex = QListIndexForSize(auiSize);
	FreeBlock*& rpHead = uiIndex < 66 ? pSmallFreeA[uiIndex] : pCurrentFree;
	if (apBlock->pRight == apBlock)
		rpHead = nullptr;
	else
	{
		apBlock->pRight->pLeft = apBlock->pLeft;
		apBlock->pLeft->pRight = apBlock->pRight;
		if (rpHead == apBlock)
			rpHead = apBlock->pRight;
		apBlock->pLeft = apBlock->pRight = reinterpret_cast<FreeBlock*>(uintptr_t(0xDECAFBAD));
	}
	--uiNumFreeBlocks;
	if (pNextMerge == apBlock)
		pNextMerge = pCurrentFree;
}

void Store::InsertFreeBlock(FreeBlock* apBlock, size_t auiSize)
{
	size_t uiIndex = QListIndexForSize(auiSize);
	FreeBlock*& rpHead = uiIndex < 66 ? pSmallFreeA[uiIndex] : pCurrentFree;
	if (rpHead)
	{
		apBlock->pRight = rpHead;
		apBlock->pLeft = rpHead->pLeft;
		rpHead->pLeft->pRight = apBlock;
		rpHead->pLeft = apBlock;
	}
	else
		apBlock->pLeft = apBlock->pRight = apBlock;
	rpHead = apBlock;
	++uiNumFreeBlocks;
}

size_t Store::MergeFrom(FreeBlock* apBlock)
{
	size_t uiSize = apBlock->uiSize;
	FreeBlock* pLast = apBlock;
	auto* pNext = reinterpret_cast<FreeBlock*>(reinterpret_cast<char*>(apBlock) + uiSize);
	if (reinterpret_cast<uintptr_t>(pNext) < reinterpret_cast<uintptr_t>(pAllocEnd) && !(pNext->uiSize & 1))
	{
		RemoveFreeBlock(apBlock, uiSize);
		do
		{
			pLast = pNext;
			size_t uiNextSize = pNext->uiSize;
			RemoveFreeBlock(pNext, uiNextSize);
			uiSize += uiNextSize;
			pNext = reinterpret_cast<FreeBlock*>(reinterpret_cast<char*>(pNext) + uiNextSize);
		} while (reinterpret_cast<uintptr_t>(pNext) < reinterpret_cast<uintptr_t>(pAllocEnd) && !(pNext->uiSize & 1));
		apBlock->uiSize = uiSize;
		InsertFreeBlock(apBlock, uiSize);
	}
	if (pLastBlock == pLast)
		pLastBlock = apBlock;
	return uiSize;
}

FreeBlock* Store::MergeBlockList(FreeBlock*& arpList, FreeBlock* apLowest)
{
	FreeBlock* pFirst = arpList;
	FreeBlock* pBlock = pFirst;
	FreeBlock* pResult = apLowest;
	while (pBlock)
	{
		size_t uiSize = pBlock->uiSize;
		FreeBlock* pNext = pBlock->pRight;
		if (pBlock < apLowest)
			pResult = pBlock;
		size_t uiMerged = MergeFrom(pBlock);
		if (uiMerged <= uiSize)
			pBlock = pNext != pFirst ? pNext : nullptr;
		else
			pFirst = pBlock = arpList;
	}
	return pResult;
}

StoreBlock* Store::NewStoreBlock()
{
	StoreBlock* pBlock = pFreeStoreBlockList;
	if (pBlock)
		pFreeStoreBlockList = pBlock->pNext;
	else
	{
		pBlock = pNextStoreBlock;
		if (reinterpret_cast<uintptr_t>(pBlock) <= reinterpret_cast<uintptr_t>(pStoreBlockMin))
		{
			uintptr_t uiNewMin = (reinterpret_cast<uintptr_t>(pStoreBlockMin) - 0x10000) & ~uintptr_t(0xFFFF);
			if (uiNewMin < reinterpret_cast<uintptr_t>(pAllocEnd) || !VirtualAlloc(reinterpret_cast<void*>(uiNewMin), 0x10000, MEM_COMMIT, PAGE_READWRITE))
				return nullptr;
			pBlock = pNextStoreBlock;
			pStoreBlockMin = reinterpret_cast<StoreBlock*>(reinterpret_cast<uintptr_t>(pStoreBlockMin) - ((reinterpret_cast<uintptr_t>(pStoreBlockMin) - uiNewMin) & ~uintptr_t(15)));
		}
		pNextStoreBlock = reinterpret_cast<StoreBlock*>(reinterpret_cast<uintptr_t>(pBlock) - 16);
	}
	if (pBlock)
		pBlock->pAddress = nullptr;
	return pBlock;
}

FreeBlock* Store::AddPages(size_t auiAmount)
{
	auto* pBlock = static_cast<FreeBlock*>(pLastBlock);
	bool bFree = pBlock && !(pBlock->uiSize & 1);
	if (!bFree)
		pBlock = static_cast<FreeBlock*>(pAllocEnd);
	uintptr_t uiEnd = (reinterpret_cast<uintptr_t>(pBlock) + auiAmount + 65559) & ~uintptr_t(0xFFFF);
	size_t uiExtra = uiEnd - reinterpret_cast<uintptr_t>(pAllocEnd);
	if (uiEnd > (reinterpret_cast<uintptr_t>(pStoreBlockMin) & ~uintptr_t(0xFFFF)) || !VirtualAlloc(pAllocEnd, uiExtra, MEM_COMMIT, PAGE_READWRITE))
		return nullptr;
	if (bFree)
		RemoveFreeBlock(pBlock, pBlock->uiSize);
	else
		pLastBlock = pBlock;
	pAllocEnd = reinterpret_cast<void*>(uiEnd);
	pBlock->uiSize = uiEnd - reinterpret_cast<uintptr_t>(pBlock);
	uiFree += uiExtra;
	return pBlock;
}

void Store::TryDecommit()
{
	auto* pBlock = static_cast<FreeBlock*>(pLastBlock);
	if (!pBlock || (pBlock->uiSize & 1))
		return;
	uintptr_t uiEnd = (reinterpret_cast<uintptr_t>(pAllocEnd) + 0xFFFF) & ~uintptr_t(0xFFFF);
	if (uiEnd > reinterpret_cast<uintptr_t>(pStoreBlockMin))
		return;
	uintptr_t uiNewEnd = (reinterpret_cast<uintptr_t>(pBlock) + 65559) & ~uintptr_t(0xFFFF);
	if (uiNewEnd < reinterpret_cast<uintptr_t>(pAllocEndMin))
		uiNewEnd = reinterpret_cast<uintptr_t>(pAllocEndMin);
	if (uiNewEnd < reinterpret_cast<uintptr_t>(pBlock) + 24 || uiNewEnd >= uiEnd)
		return;
	size_t uiSize = pBlock->uiSize;
	size_t uiExtra = uiEnd - uiNewEnd;
	RemoveFreeBlock(pBlock, uiSize);
	uiFree -= uiExtra;
	pAllocEnd = reinterpret_cast<void*>(uiNewEnd);
	VirtualFree(pAllocEnd, uiExtra, MEM_DECOMMIT);
	pBlock->uiSize = uiSize - uiExtra;
	InsertFreeBlock(pBlock, pBlock->uiSize);
}

Store::Store(size_t auiSize, size_t auiPrecommit) :
	pAllocBase(nullptr), pAllocEndMin(nullptr), pAllocEnd(nullptr), pStoreEnd(nullptr), pLastBlock(nullptr),
	pCurrentFree(nullptr), pNextMerge(nullptr), pStoreBlockMin(nullptr), pNextStoreBlock(nullptr), pFreeStoreBlockList(nullptr),
	CurrentThread(GetCurrentThreadId()), uiAllocated(0), uiNumAllocatedBlocks(0), uiFree(0), uiNumFreeBlocks(0), uiCompacted(0),
	uiBatchDeallocateTlsSlot(TlsAlloc())
{
	Lock.uiLock = 0;
	size_t uiReserve = auiSize > 0x10000 ? (auiSize + 0xFFFF) & ~size_t(0xFFFF) : 0x20000;
	size_t uiCommit = auiSize > 0x10000 && auiPrecommit > 0x10000 ? ((auiPrecommit + 0xFFFF) & ~size_t(0xFFFF)) - 0x10000 : 0x10000;
	pAllocBase = VirtualAlloc(nullptr, uiReserve, MEM_RESERVE, PAGE_READWRITE);
	pAllocEndMin = pAllocEnd = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(pAllocBase) + uiCommit);
	VirtualAlloc(pAllocBase, uiCommit, MEM_COMMIT, PAGE_READWRITE);
	pStoreEnd = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(pAllocBase) + uiReserve);
	pStoreBlockMin = reinterpret_cast<StoreBlock*>(reinterpret_cast<uintptr_t>(pStoreEnd) - 0x10000);
	pNextStoreBlock = reinterpret_cast<StoreBlock*>(reinterpret_cast<uintptr_t>(pStoreEnd) - 16);
	VirtualAlloc(pStoreBlockMin, 0x10000, MEM_COMMIT, PAGE_READWRITE);
	auto* pBlock = static_cast<FreeBlock*>(pAllocBase);
	pBlock->uiSize = uiCommit;
	pBlock->pLeft = pBlock->pRight = pBlock;
	pCurrentFree = pBlock;
	pLastBlock = pBlock;
	uiNumFreeBlocks = 1;
	uiFree = uiCommit;
	std::memset(pSmallFreeA, 0, sizeof(pSmallFreeA));
}

Store::~Store()
{
	VirtualFree(pAllocBase, 0, MEM_RELEASE);
	TlsFree(uiBatchDeallocateTlsSlot);
}

void Store::DeallocateBlock(IMemoryTracker*, bool, StoreBlock* apStoreBlock)
{
	if (!apStoreBlock)
		return;
	auto* pBlock = reinterpret_cast<FreeBlock*>(static_cast<char*>(apStoreBlock->pAddress) - 16);
	size_t uiSize = pBlock->uiSize & 0xFFFFFFFC;
	pBlock->uiSize = uiSize;
	uiFree += uiSize;
	InsertFreeBlock(pBlock, uiSize);
	uiAllocated -= uiSize;
	--uiNumAllocatedBlocks;
	MergeFrom(pBlock);
	TryDecommit();
	apStoreBlock->uiAccessFlags = 0xFFFFFFFF;
	apStoreBlock->pNext = pFreeStoreBlockList;
	pFreeStoreBlockList = apStoreBlock;
	uiCompacted = 0;
}

void Store::DeallocatePinned(IMemoryTracker* apTracker, bool abTrack, void* apBlock)
{
	if (!apBlock)
		return;
	Lock.Lock();
	CurrentThread = GetCurrentThreadId();
	auto* pStoreBlock = *(reinterpret_cast<StoreBlock**>(apBlock) - 1);
	InterlockedDecrement(reinterpret_cast<volatile LONG*>(&pStoreBlock->uiAccessFlags));
	DeallocateBlock(apTracker, abTrack, pStoreBlock);
	CurrentThread = 0;
	Lock.Unlock();
}

bool Store::AllocateAlign(IMemoryTracker*, bool, HandleType& arResult, size_t auiSize, size_t auiAlignment, MoveCallback* apCallback, bool abMustSucceed, bool abAllowExtend)
{
	if (!auiSize)
	{
		arResult.pStoreBlock = nullptr;
		return true;
	}
	Lock.Lock();
	CurrentThread = GetCurrentThreadId();
	StoreBlock* pHandle = NewStoreBlock();
	bool bSuccess = false;
	if (pHandle)
	{
		uint32_t uiAlignment = uint32_t(auiAlignment);
		size_t uiSize = (auiSize + (apCallback ? 31 : 23)) & ~size_t(7);
		// The alignment mask is zero-extended from a 32-bit register.
		uintptr_t uiMask = uint32_t(-uiAlignment);
		uintptr_t uiPad = uint32_t(uiAlignment - 1);
		FreeBlock* pFree = nullptr;
		uintptr_t uiHeader = 0;
		uintptr_t uiEnd = 0;
		size_t uiPrefix = 0;
		size_t uiTrailer = 0;
		size_t uiBlockSize = 0;
		auto FindBlock = [&](FreeBlock* apHead)
		{
			pFree = GetBestScoredBlockFromList(apHead, uiSize, uiAlignment, uiPad, uiMask, uiHeader, uiEnd, uiTrailer, uiPrefix, uiBlockSize);
		};
		if (uiFree >= uiSize)
		{
			uint32_t uiIndex = uint32_t((uiSize >> 3) - 1);
			uint32_t uiLimit = uiIndex < 66 ? uiIndex : 66;
			auto* pLowest = static_cast<FreeBlock*>(pLastBlock);
			for (uint32_t i = 1; i < uiLimit; ++i)
				pLowest = MergeBlockList(pSmallFreeA[i], pLowest);
			for (uint32_t i = uiIndex; i < 66 && !pFree; ++i)
				FindBlock(pSmallFreeA[i]);
			if (!pFree)
				FindBlock(pCurrentFree);
			if (pFree)
				RemoveFreeBlock(pFree, pFree->uiSize);
		}
		if (!pFree && abMustSucceed && abAllowExtend)
		{
			auto* pBlock = static_cast<FreeBlock*>(pLastBlock);
			bool bFree = pBlock && !(pBlock->uiSize & 1);
			if (!bFree)
				pBlock = static_cast<FreeBlock*>(pAllocEnd);
			uintptr_t uiNewEnd = (reinterpret_cast<uintptr_t>(pBlock) + uiAlignment + uiSize + 65575) & ~uintptr_t(0xFFFF);
			size_t uiExtra = uiNewEnd - reinterpret_cast<uintptr_t>(pAllocEnd);
			if (uiNewEnd <= (reinterpret_cast<uintptr_t>(pStoreBlockMin) & ~uintptr_t(0xFFFF)) && VirtualAlloc(pAllocEnd, uiExtra, MEM_COMMIT, PAGE_READWRITE))
			{
				pFree = pBlock;
				if (bFree)
					RemoveFreeBlock(pBlock, pBlock->uiSize);
				else
					pLastBlock = pBlock;
				pAllocEnd = reinterpret_cast<void*>(uiNewEnd);
				pBlock->uiSize = uiNewEnd - reinterpret_cast<uintptr_t>(pBlock);
				uiFree += uiExtra;
				uiEnd = uiNewEnd;
				uiHeader = ((reinterpret_cast<uintptr_t>(pBlock) + uiPad + 16) & uiMask) - 16;
				uiPrefix = uiHeader - reinterpret_cast<uintptr_t>(pBlock);
				if (uiPrefix - 1 <= 23)
				{
					uiHeader += uiAlignment;
					uiPrefix += uiAlignment;
				}
			}
		}
		if (pFree)
		{
			uintptr_t uiAfter = uiHeader + uiSize;
			size_t uiSuffix = uiEnd - uiAfter;
			if (uiPrefix)
			{
				pFree->uiSize = uiPrefix;
				InsertFreeBlock(pFree, uiPrefix);
			}
			FreeBlock* pSuffix = nullptr;
			if (uiSuffix < 24)
				uiSize = uiEnd - uiHeader;
			else
			{
				pSuffix = reinterpret_cast<FreeBlock*>(uiAfter);
				pSuffix->uiSize = uiSuffix;
				InsertFreeBlock(pSuffix, uiSuffix);
				if (reinterpret_cast<uintptr_t>(pSuffix) > reinterpret_cast<uintptr_t>(pLastBlock))
					pLastBlock = pSuffix;
			}
			uiFree -= uiSize;
			uiAllocated += uiSize;
			++uiNumAllocatedBlocks;
			if (apCallback)
			{
				*reinterpret_cast<MoveCallback**>(uiHeader + uiSize - 8) = apCallback;
				uiSize |= 2;
			}
			*reinterpret_cast<size_t*>(uiHeader) = uiSize | 1;
			*reinterpret_cast<StoreBlock**>(uiHeader + 8) = pHandle;
			if (uiHeader > reinterpret_cast<uintptr_t>(pLastBlock))
				pLastBlock = reinterpret_cast<BlockHeader*>(uiHeader);
			pHandle->pAddress = reinterpret_cast<void*>(uiHeader + 16);
			pHandle->uiAccessFlags = uint32_t(uiAlignment - 1) << 23;
			arResult.pStoreBlock = pHandle;
			if (pSuffix)
			{
				MergeFrom(pSuffix);
				TryDecommit();
			}
			bSuccess = true;
		}
		else
		{
			pHandle->uiAccessFlags = 0xFFFFFFFF;
			pHandle->pNext = pFreeStoreBlockList;
			pFreeStoreBlockList = pHandle;
		}
	}
	CurrentThread = 0;
	Lock.Unlock();
	return bSuccess;
}

NoopMoveCallback NoopMoveCallback::instance;
MoveCallback::~MoveCallback() = default;
void NoopMoveCallback::operator()(void*, const void*, size_t) {}

void CompactingStore::ExecuteMove(void* apDest, void* apSrc, size_t auiSizeFlags)
{
	size_t uiSize = uint32_t(auiSizeFlags) & 0xFFFFFFFC;
	if (auiSizeFlags & 2)
	{
		uiSize -= 24;
		auto* pCallback = *reinterpret_cast<MoveCallback**>(static_cast<char*>(apSrc) + uiSize);
		(*pCallback)(apDest, apSrc, uiSize);
		*reinterpret_cast<MoveCallback**>(static_cast<char*>(apDest) + uiSize) = pCallback;
	}
	else
		memmove_s(apDest, uiSize - 16, apSrc, uiSize - 16);
}

size_t Store::PushFree(AllocatedBlock*& arpGap, bool abAllowAlignPad)
{
	FreeBlock* pGap = reinterpret_cast<FreeBlock*>(arpGap);
	size_t uiGapSize = pGap->uiSize;
	size_t uiNextFlags = 0;
	for (unsigned int uiPass = 0; uiPass <= unsigned(abAllowAlignPad); ++uiPass)
	{
		auto* pNext = reinterpret_cast<AllocatedBlock*>(reinterpret_cast<char*>(pGap) + uiGapSize);
		while (reinterpret_cast<uintptr_t>(pNext) < reinterpret_cast<uintptr_t>(pAllocEnd))
		{
			uiNextFlags = pNext->uiSize;
			if (!(uiNextFlags & 1))
				break;
			StoreBlock* pOwner = pNext->pOwner;
			uint32_t uiFlags = pOwner->uiAccessFlags & 0x7F800000;
			uint32_t uiAlignPad = uiFlags >> 23;
			uint32_t uiAlignment = uiAlignPad + 1;
			uintptr_t uiDest = ((reinterpret_cast<uintptr_t>(pGap) + uiAlignPad + 16) & uint32_t(-uiAlignment)) - 16;
			size_t uiPrefix = uiDest - reinterpret_cast<uintptr_t>(pGap);
			size_t uiRemaining = uiGapSize;
			if (!uiPass && uiPrefix)
				break;
			if (uiPass && uiPrefix)
			{
				if (uiPrefix < 24)
				{
					uiPrefix += uiAlignment;
					uiDest += uiAlignment;
				}
				uiRemaining -= uiPrefix;
				if (uiRemaining > uiGapSize || uiRemaining < 24)
					break;
			}
			if (uint32_t(InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&pOwner->uiAccessFlags), LONG(uiFlags | 0x80000000), LONG(uiFlags))) != uiFlags)
				break;
			RemoveFreeBlock(pGap, uiGapSize);
			FreeBlock* pPrefix = nullptr;
			if (uiPrefix)
			{
				pGap->uiSize = uiPrefix;
				InsertFreeBlock(pGap, uiPrefix);
				pPrefix = pGap;
				uiGapSize = uiRemaining;
			}
			_mm_mfence();
			auto* pDest = reinterpret_cast<AllocatedBlock*>(uiDest);
			ExecuteMove(pDest + 1, pNext + 1, uiNextFlags);
			pDest->uiSize = uiNextFlags;
			pDest->pOwner = pOwner;
			pOwner->pAddress = pDest + 1;
			pGap = reinterpret_cast<FreeBlock*>(uiDest + (uint32_t(uiNextFlags) & 0xFFFFFFFC));
			pGap->uiSize = uiGapSize;
			InsertFreeBlock(pGap, uiGapSize);
			_mm_mfence();
			pOwner->uiAccessFlags = uiFlags;
			if (pLastBlock == pNext)
				pLastBlock = pGap;
			if (uiPrefix > uiGapSize)
			{
				pGap = pPrefix;
				break;
			}
			pNext = reinterpret_cast<AllocatedBlock*>(reinterpret_cast<char*>(pGap) + uiGapSize);
			uiNextFlags = 0;
		}
		if (!(uiNextFlags & 1))
			break;
	}
	arpGap = reinterpret_cast<AllocatedBlock*>(pGap);
	return uiNextFlags;
}

size_t Store::UpdateFree(const FreeBlock*& arpGap)
{
	FreeBlock* pGap = const_cast<FreeBlock*>(arpGap);
	FreeBlock* pCandidate = nullptr;
	size_t uiCandidateFlags = 0;
	uint32_t uiGapSize = uint32_t(pGap->uiSize) & 0xFFFFFFFC;
	auto* pScan = reinterpret_cast<AllocatedBlock*>(reinterpret_cast<char*>(pGap) + uiGapSize);
	pScan = reinterpret_cast<AllocatedBlock*>(reinterpret_cast<char*>(pScan) + (uint32_t(pScan->uiSize) & 0xFFFFFFFC));
	while (reinterpret_cast<uintptr_t>(pScan) < reinterpret_cast<uintptr_t>(pAllocEnd))
	{
		size_t uiFlagsSize = pScan->uiSize;
		uint32_t uiSize = uint32_t(uiFlagsSize) & 0xFFFFFFFC;
		if (uiFlagsSize & 1)
		{
			StoreBlock* pOwner = pScan->pOwner;
			uint32_t uiFlags = pOwner->uiAccessFlags & 0x7F800000;
			uint32_t uiAlignPad = uiFlags >> 23;
			uintptr_t uiDest = (uint32_t(reinterpret_cast<uintptr_t>(pGap) + 16 + uiAlignPad) & ~uiAlignPad) - uintptr_t(16);
			uint32_t uiRemaining = uiGapSize - uiSize;
			if (uiDest == reinterpret_cast<uintptr_t>(pGap) && uiRemaining <= uiGapSize && uiRemaining - 1 > 22 &&
				uint32_t(InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&pOwner->uiAccessFlags), LONG(uiFlags | 0x80000000), LONG(uiFlags))) == uiFlags)
			{
				RemoveFreeBlock(pGap, uiGapSize);
				_mm_mfence();
				auto* pDest = reinterpret_cast<AllocatedBlock*>(uiDest);
				ExecuteMove(pDest + 1, pScan + 1, uiFlagsSize);
				pDest->uiSize = uiFlagsSize;
				pDest->pOwner = pOwner;
				pOwner->pAddress = pDest + 1;
				auto* pSourceFree = reinterpret_cast<FreeBlock*>(pScan);
				pSourceFree->uiSize = uiSize;
				InsertFreeBlock(pSourceFree, uiSize);
				_mm_mfence();
				pOwner->uiAccessFlags = uiFlags;
				pGap = nullptr;
				if (uiRemaining)
				{
					pGap = reinterpret_cast<FreeBlock*>(uiDest + uiSize);
					pGap->uiSize = uiRemaining;
					InsertFreeBlock(pGap, uiRemaining);
					AllocatedBlock* pPush = reinterpret_cast<AllocatedBlock*>(pGap);
					size_t uiPushedFlags = PushFree(pPush, false);
					if (pPush != reinterpret_cast<AllocatedBlock*>(pGap))
					{
						pCandidate = reinterpret_cast<FreeBlock*>(pPush);
						uiCandidateFlags = uiPushedFlags;
						pGap = nullptr;
					}
				}
				uiGapSize = uiRemaining;
				if (reinterpret_cast<uintptr_t>(pSourceFree) < reinterpret_cast<uintptr_t>(pCandidate))
				{
					pCandidate = pSourceFree;
					uiCandidateFlags = uiSize;
				}
			}
		}
		else if (!pCandidate)
		{
			pCandidate = reinterpret_cast<FreeBlock*>(pScan);
			uiCandidateFlags = uiFlagsSize;
		}
		pScan = reinterpret_cast<AllocatedBlock*>(reinterpret_cast<char*>(pScan) + uiSize);
		if (!pGap)
			break;
	}
	if (!uiGapSize && !pCandidate)
	{
		while (reinterpret_cast<uintptr_t>(pScan) < reinterpret_cast<uintptr_t>(pAllocEnd))
		{
			size_t uiFlagsSize = pScan->uiSize;
			if (!(uiFlagsSize & 1))
			{
				pCandidate = reinterpret_cast<FreeBlock*>(pScan);
				uiCandidateFlags = uiFlagsSize;
				break;
			}
			pScan = reinterpret_cast<AllocatedBlock*>(reinterpret_cast<char*>(pScan) + (uint32_t(uiFlagsSize) & 0xFFFFFFFC));
		}
	}
	arpGap = pCandidate;
	return uiCandidateFlags;
}

volatile uint32_t CompactingStore::uiWantFree = 0;

size_t Store::RequestCompact(size_t auiSize)
{
	size_t uiResult = 0;
	if (!uiCompacted && uiNumFreeBlocks > 1)
	{
		FreeBlock* pGap = reinterpret_cast<FreeBlock*>(pLastBlock);
		for (FreeBlock*& rpList : pSmallFreeA)
			pGap = MergeBlockList(rpList, pGap);
		pGap = MergeBlockList(pCurrentFree, pGap);
		if (pGap != pLastBlock)
		{
			AllocatedBlock* pPush = reinterpret_cast<AllocatedBlock*>(pGap);
			size_t uiFlags = PushFree(pPush, true);
			pGap = reinterpret_cast<FreeBlock*>(pPush);
			do
			{
				if (uiWantFree)
					uiFlags = 0;
				else if (uiFlags)
				{
					if (uiFlags & 1)
					{
						const FreeBlock* pUpdate = pGap;
						uiFlags = UpdateFree(pUpdate);
						pGap = const_cast<FreeBlock*>(pUpdate);
					}
					else
					{
						size_t uiMerged = MergeFrom(pGap);
						if (uiMerged >= auiSize)
						{
							uiResult = uiMerged;
							uiFlags = 0;
						}
						else
						{
							pPush = reinterpret_cast<AllocatedBlock*>(pGap);
							uiFlags = PushFree(pPush, true);
							pGap = reinterpret_cast<FreeBlock*>(pPush);
						}
					}
				}
			} while (pGap && uiFlags);
		}
		++uiCompacted;
	}
	return uiResult;
}

void Store::RequestDecommit(bool abAlwaysCompact)
{
	if (!Lock.TryLock())
		return;
	CurrentThread = GetCurrentThreadId();
	bool bDecommit = uiFree >= 0x10000 && reinterpret_cast<uintptr_t>(pAllocEnd) > reinterpret_cast<uintptr_t>(pAllocEndMin);
	if (bDecommit || abAlwaysCompact)
		RequestCompact(reinterpret_cast<uintptr_t>(pStoreBlockMin) - reinterpret_cast<uintptr_t>(pAllocBase));
	if (bDecommit)
		TryDecommit();
	CurrentThread = 0;
	Lock.Unlock();
}

void Store::RequestFullCompact()
{
	Lock.Lock();
	CurrentThread = GetCurrentThreadId();
	RequestCompact(reinterpret_cast<uintptr_t>(pStoreBlockMin) - reinterpret_cast<uintptr_t>(pAllocBase));
	CurrentThread = 0;
	Lock.Unlock();
}

void Store::RequestCompactForSize(size_t auiSize)
{
	if (uiFree < auiSize + 16 || !Lock.TryLock())
		return;
	CurrentThread = GetCurrentThreadId();
	RequestCompact(auiSize + 16);
	CurrentThread = 0;
	Lock.Unlock();
}

void Store::RequestStepMerge()
{
	if (!Lock.TryLock())
		return;
	CurrentThread = GetCurrentThreadId();
	FreeBlock* pBlock = pNextMerge ? pNextMerge : pCurrentFree;
	if (pBlock)
	{
		MergeFrom(pBlock);
		pBlock = pBlock->pRight;
	}
	pNextMerge = pBlock;
	TryDecommit();
	CurrentThread = 0;
	Lock.Unlock();
}

void* Store::AllocateAlignPinned(IMemoryTracker* apTracker, bool abTrack, size_t auiSize, size_t auiAlignment)
{
	HandleType Result;
	if (!AllocateAlign(apTracker, abTrack, Result, auiSize, uint32_t(auiAlignment), nullptr, true, true))
		return nullptr;
	RequestFullCompact();
	return Accessor::BeginStoreBlockAccess(Result.pStoreBlock);
}

BatchDeallocateOperation::BatchDeallocateOperation() : pStore(nullptr), pTrack(nullptr), bTrack(false) {}

BatchDeallocateOperation::~BatchDeallocateOperation()
{
	if (pStore)
		pStore->EndBatchDeallocate(*this);
}

void Store::Deallocate(IMemoryTracker* apTracker, bool abTrack, HandleType& arHandle)
{
	auto* pBatch = static_cast<BatchDeallocateOperation*>(TlsGetValue(uiBatchDeallocateTlsSlot));
	StoreBlock* pHandle = arHandle.pStoreBlock;
	if (pBatch)
	{
		if (pHandle)
		{
			void* pAddress = Accessor::BeginStoreBlockAccess(pHandle);
			size_t uiSize = (static_cast<BlockHeader*>(pAddress) - 2)->uiSize;
			if (uiSize & 2)
				*reinterpret_cast<MoveCallback**>(static_cast<char*>(pAddress) + (uint32_t(uiSize) & 0xFFFFFFFC) - 24) = &NoopMoveCallback::instance;
			pBatch->HandleArena.Add(arHandle);
			arHandle.pStoreBlock = nullptr;
			Accessor::EndStoreBlockAccess(pHandle);
		}
	}
	else
	{
		InterlockedIncrement(reinterpret_cast<volatile LONG*>(&uiWantFree));
		Lock.Lock();
		CurrentThread = GetCurrentThreadId();
		arHandle.pStoreBlock = nullptr;
		DeallocateBlock(apTracker, abTrack, pHandle);
		CurrentThread = 0;
		InterlockedDecrement(reinterpret_cast<volatile LONG*>(&uiWantFree));
		Lock.Unlock();
	}
}

void Store::ProcessBatchDeallocate(BatchDeallocateOperation& arBatch)
{
	if (!arBatch.HandleArena.QSize())
		return;
	InterlockedIncrement(reinterpret_cast<volatile LONG*>(&uiWantFree));
	Lock.Lock();
	auto it = arBatch.HandleArena.Begin();
	auto End = arBatch.HandleArena.End();
	IMemoryTracker* pTrack = arBatch.pTrack;
	bool bTrack = arBatch.bTrack;
	CurrentThread = GetCurrentThreadId();
	while (it != End)
	{
		DeallocateBlock(pTrack, bTrack, it->pStoreBlock);
		++it;
	}
	CurrentThread = 0;
	InterlockedDecrement(reinterpret_cast<volatile LONG*>(&uiWantFree));
	Lock.Unlock();
}

void Store::FlushBatchDeallocates()
{
	auto* pBatch = static_cast<BatchDeallocateOperation*>(TlsGetValue(uiBatchDeallocateTlsSlot));
	if (pBatch)
	{
		ProcessBatchDeallocate(*pBatch);
		pBatch->HandleArena.Clear(false);
	}
}

void Store::EndBatchDeallocate(BatchDeallocateOperation& arBatch)
{
	ProcessBatchDeallocate(arBatch);
	TlsSetValue(uiBatchDeallocateTlsSlot, nullptr);
}

bool Store::QBlockIsValid(const BlockHeader* apBlock) const
{
	size_t uiFlagsSize = apBlock->uiSize;
	uintptr_t uiBlock = reinterpret_cast<uintptr_t>(apBlock);
	uintptr_t uiBegin = reinterpret_cast<uintptr_t>(pAllocBase);
	uintptr_t uiEnd = reinterpret_cast<uintptr_t>(pAllocEnd);
	if (uiBlock < uiBegin || uiBlock > uiEnd)
		return false;
	if (uiFlagsSize & 1)
	{
		StoreBlock* pOwner = static_cast<const AllocatedBlock*>(apBlock)->pOwner;
		uintptr_t uiOwner = reinterpret_cast<uintptr_t>(pOwner);
		uintptr_t uiOwnerMin = reinterpret_cast<uintptr_t>(pStoreBlockMin);
		if (((uiOwner - uiOwnerMin) & 15) || uiOwner < uiOwnerMin || uiOwner >= reinterpret_cast<uintptr_t>(pStoreEnd) ||
			reinterpret_cast<uintptr_t>(pOwner->pAddress) - 16 != uiBlock)
			return false;
	}
	else
	{
		if ((uiFlagsSize & 3) || uiFlagsSize < 24 || uiFlagsSize > uiFree)
			return false;
		auto* pFree = static_cast<const FreeBlock*>(apBlock);
		uintptr_t uiLeft = reinterpret_cast<uintptr_t>(pFree->pLeft);
		uintptr_t uiRight = reinterpret_cast<uintptr_t>(pFree->pRight);
		if (uiRight < uiBegin || uiRight > uiEnd || uiLeft < uiBegin || uiLeft > uiEnd ||
			pFree->pRight->pLeft != pFree || pFree->pLeft->pRight != pFree || (pFree->pLeft->uiSize & 1) || (pFree->pRight->uiSize & 1))
			return false;
	}
	return uiBlock + (uiFlagsSize & 0xFFFFFFFC) <= uiEnd;
}

bool Store::QNextBlockIsValid(const BlockHeader* apBlock) const
{
	auto* pNext = reinterpret_cast<const BlockHeader*>(reinterpret_cast<const char*>(apBlock) + (uint32_t(apBlock->uiSize) & 0xFFFFFFFC));
	return pNext == pAllocEnd || QBlockIsValid(pNext);
}

void Store::ValidateBlocks() const
{
	auto* pBlock = static_cast<const BlockHeader*>(pAllocBase);
	const BlockHeader* pPrevious = nullptr;
	while (reinterpret_cast<uintptr_t>(pBlock) < reinterpret_cast<uintptr_t>(pAllocEnd))
	{
		size_t uiFlagsSize = pBlock->uiSize;
		if (!QBlockIsValid(pBlock))
			BSprintf("invalid block 0x%8x, prev = 0x%8x\n", pBlock, pPrevious);
		pPrevious = pBlock;
		pBlock = reinterpret_cast<const BlockHeader*>(reinterpret_cast<const char*>(pBlock) + (uint32_t(uiFlagsSize) & 0xFFFFFFFC));
	}
}

HandleType::~HandleType() { pStoreBlock = nullptr; }

FreeBlock* Store::GetBestScoredBlockFromList(FreeBlock* apList, size_t auiSize, size_t auiAlignment, size_t auiAlignPadMask, size_t auiAlignMask, size_t& aruiResultAddr, size_t& aruiEndAddr, size_t& aruiTrailerAddr, size_t& aruiLeaderSize, size_t& aruiBlockToUseSize)
{
	FreeBlock* pResult = nullptr;
	if (apList)
	{
		size_t uiResult = 0;
		size_t uiEnd = 0;
		size_t uiTrailer = 0;
		size_t uiLeader = 0;
		size_t uiSize = 0;
		FreeBlock* pBlock = apList;
		do
		{
			size_t uiBegin = reinterpret_cast<uintptr_t>(pBlock);
			size_t uiAddress = ((uiBegin + auiAlignPadMask + 16) & auiAlignMask) - 16;
			size_t uiPrefix = uiAddress - uiBegin;
			if (uiPrefix - 1 <= 23)
			{
				uiAddress += auiAlignment;
				uiPrefix += auiAlignment;
			}
			size_t uiBlockEnd = uiBegin + pBlock->uiSize;
			size_t uiAfter = uiAddress + auiSize;
			if (uiAfter <= uiBlockEnd && uiBlockEnd + uiPrefix - uiAfter != SIZE_MAX)
			{
				pResult = pBlock;
				uiSize = pBlock->uiSize;
				uiLeader = uiPrefix;
				uiResult = uiAddress;
				uiEnd = uiBlockEnd;
				uiTrailer = uiAfter;
			}
			pBlock = pBlock->pRight;
		} while (pBlock != apList);
		aruiBlockToUseSize = uiSize;
		aruiLeaderSize = uiLeader;
		aruiResultAddr = uiResult;
		aruiEndAddr = uiEnd;
		aruiTrailerAddr = uiTrailer;
	}
	return pResult;
}

void CompactingStore::Store::BeginBatchDeallocate(IMemoryTracker* apTracker, bool abTrack, BatchDeallocateOperation& arBatch)
{
	arBatch.pStore = this;
	arBatch.pTrack = apTracker;
	arBatch.bTrack = abTrack;
	TlsSetValue(uiBatchDeallocateTlsSlot, &arBatch);
}
