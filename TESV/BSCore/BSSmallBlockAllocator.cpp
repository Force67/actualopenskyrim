#include "BSCore/BSSmallBlockAllocator.h"
#include <cstring>

using namespace BSSmallBlockAllocatorUtil;

void BSSmallBlockAllocatorUtil::Pool::AddPageExternal(BlockPage* apPage)
{
	apPage->pLeft = apPage->pRight = nullptr;
	FreeBlock* pFree = nullptr;
	char* pBuffer = reinterpret_cast<char*>(apPage) - 8160;
	uint16_t uiCount = 0;
	for (uint32_t uiRemaining = 8160; uiRemaining >= ElementSize; uiRemaining -= ElementSize)
	{
		auto* pBlock = reinterpret_cast<FreeBlock*>(pBuffer);
		pBlock->pNext = pFree;
		pFree = pBlock;
		pBuffer += ElementSize;
		++uiCount;
	}
	apPage->TotalElem = apPage->FreeElem = uiCount;
	apPage->pBlocks = pFree;
	InsertNonEmptyPage(apPage);
}

void BSSmallBlockAllocatorUtil::Pool::AddPage(BlockPage* apPage, FreeBlock* apBuffer)
{
	apPage->ElemSize = uint16_t(ElementSize);
	apPage->Check = 0xDEAF;
	apPage->pLeft = apPage->pRight = nullptr;
	FreeBlock* pFree = nullptr;
	uint16_t uiCount = 0;
	for (uint32_t uiRemaining = 0x2000; uiRemaining >= ElementSize; uiRemaining -= ElementSize)
	{
		apBuffer->pNext = pFree;
		pFree = apBuffer;
		apBuffer = reinterpret_cast<FreeBlock*>(reinterpret_cast<char*>(apBuffer) + ElementSize);
		++uiCount;
	}
	apPage->TotalElem = apPage->FreeElem = uiCount;
	apPage->pBlocks = pFree;
	InsertNonEmptyPage(apPage);
}

FreeBlock* BSSmallBlockAllocatorUtil::Pool::AllocateFromPage(BlockPage* apPage)
{
	FreeBlock* pBlock = apPage->pBlocks;
	apPage->pBlocks = pBlock->pNext;
	--apPage->FreeElem;
	--TotalFreeBlocks;
	++TotalAllocatedBlocks;
	if (apPage->FreeElem)
		pCurrAlloc = apPage;
	else
	{
		RemovePage(apPage);
		InsertEmptyPage(apPage);
	}
	return pBlock;
}

bool BSSmallBlockAllocatorUtil::Pool::DeallocateToPage(BlockPage* apPage, FreeBlock* apPtr)
{
	apPtr->pNext = apPage->pBlocks;
	apPage->pBlocks = apPtr;
	++apPage->FreeElem;
	--TotalAllocatedBlocks;
	++TotalFreeBlocks;
	if (apPage->FreeElem != 1 || apPage == pPageList)
		return apPage->FreeElem == apPage->TotalElem;
	RemovePage(apPage);
	InsertNonEmptyPage(apPage);
	return false;
}

UserPoolBase::UserPoolBase(BSSmallBlockAllocator* apOwner, size_t aElemSize, uint32_t aInitialReserve) :
	pOwner(apOwner), InitialReserve(aInitialReserve)
{
	MyPool.ElementSize = (uint32_t(aElemSize) + 7) & ~7u;
	if (pOwner)
		pOwner->RegisterUserPool(this, aInitialReserve);
}

UserPoolBase::~UserPoolBase() = default;
FreeBlock* UserPoolBase::BaseAllocate() { return pOwner->AllocateFromPool(&MyPool, true); }
void UserPoolBase::BaseDeallocate(FreeBlock* apPtr) { pOwner->DeallocateToUserPool(this, apPtr); }

BSSmallBlockAllocator::Pool::Pool() { InitializeCriticalSection(&Lock); }
BSSmallBlockAllocator::Pool::~Pool() { DeleteCriticalSection(&Lock); }

BSSmallBlockAllocator::BSSmallBlockAllocator(uint32_t aAddressRangeSize, uint32_t aInitialCommit) :
	AddressSpaceSize(aAddressRangeSize), pAllocBase(nullptr), pBlockPageCommitMin(nullptr),
	pBlockPageCommit(nullptr), pMegaBlockPageList(nullptr), pMegaBlockCurrAlloc(nullptr),
	TotalFreeBlockPages(0), bAllowDecommits(true)
{
	InitializeCriticalSection(&Lock);
	size_t uiReserve = aAddressRangeSize < 0x200000 ? 0x200000 : (size_t(aAddressRangeSize) + 0x1FFFFF) & ~size_t(0x1FFFFF);
	size_t uiCommit = aInitialCommit < 0x200000 ? 0x200000 : (size_t(aInitialCommit) + 0x1FFFFF) & ~size_t(0x1FFFFF);
	pAllocBase = static_cast<char*>(VirtualAlloc(nullptr, uiReserve, MEM_RESERVE, PAGE_READWRITE));
	if (pAllocBase && VirtualAlloc(pAllocBase, uiCommit, MEM_COMMIT, PAGE_READWRITE))
	{
		pBlockPageCommitMin = pBlockPageCommit = pAllocBase + uiCommit;
		for (char* pAddress = pAllocBase; pAddress < pBlockPageCommit; pAddress += 0x200000)
		{
			auto* pPage = reinterpret_cast<MegaBlockPage*>(pAddress);
			pPage->pFreeBlockPages = nullptr;
			pPage->FreeBlockPages = 255;
			pPage->NextBlockPageAlloc = 0;
			pPage->bDecommitted = false;
			if (pMegaBlockPageList)
			{
				pPage->pRight = pMegaBlockPageList;
				pPage->pLeft = pMegaBlockPageList->pLeft;
				pMegaBlockPageList->pLeft->pRight = pPage;
				pMegaBlockPageList->pLeft = pPage;
			}
			else
			{
				pPage->pLeft = pPage->pRight = pPage;
				pMegaBlockPageList = pPage;
			}
			TotalFreeBlockPages += 255;
		}
		pMegaBlockCurrAlloc = pMegaBlockPageList;
	}
	for (uint32_t i = 0; i < 64; ++i)
		PoolA[i].ElementSize = 8 * (i + 1);
}

BSSmallBlockAllocator::~BSSmallBlockAllocator()
{
	if (pAllocBase)
		VirtualFree(pAllocBase, 0, MEM_RELEASE);
	DeleteCriticalSection(&Lock);
}

BSSmallBlockAllocator::MegaBlockPage* BSSmallBlockAllocator::AddrToMegaBlockPage(const void* apBlock) const
{
	return reinterpret_cast<MegaBlockPage*>(reinterpret_cast<uintptr_t>(pAllocBase) +
		((reinterpret_cast<uintptr_t>(apBlock) - reinterpret_cast<uintptr_t>(pAllocBase)) & ~uintptr_t(0x1FFFFF)));
}

size_t BSSmallBlockAllocator::Size(const void* apBlock) const
{
	return AddrToMegaBlockPage(apBlock)->AddrToBlockPage(apBlock)->ElemSize;
}

bool BSSmallBlockAllocator::QBlockInStore(const void* apBlock) const
{
	return reinterpret_cast<uintptr_t>(apBlock) >= reinterpret_cast<uintptr_t>(pAllocBase) &&
		reinterpret_cast<uintptr_t>(apBlock) < reinterpret_cast<uintptr_t>(pBlockPageCommit);
}

bool BSSmallBlockAllocator::ContainsBlockImpl(const void* apBlock) const { return QBlockInStore(apBlock); }
uint32_t BSSmallBlockAllocator::QTotalPools() const { return 64; }
bool BSSmallBlockAllocator::GetPoolContextInfo(uint32_t*, uint32_t) { return false; }

void BSSmallBlockAllocator::GetMemoryStats(MemoryStats* apStats)
{
	std::memset(apStats, 0, sizeof(*apStats));
	apStats->pName = "Small Object Allocator";
	apStats->uiCommittedSize = reinterpret_cast<uintptr_t>(pBlockPageCommit) - reinterpret_cast<uintptr_t>(pAllocBase);
	apStats->uiReservedSize = AddressSpaceSize;
	// The overhead write includes the padding after uiOverhead.
	const size_t uiOverhead = 32 * (apStats->uiCommittedSize >> 13) + sizeof(*this);
	std::memcpy(&apStats->uiOverhead, &uiOverhead, sizeof(uiOverhead));
	for (uint32_t i = 0; i < 64; ++i)
	{
		apStats->uiFreeSize += uint32_t(PoolA[i].TotalFreeBlocks * (8 * (i + 1)));
		apStats->uiUsedSize += uint32_t(PoolA[i].TotalAllocatedBlocks * (8 * (i + 1)));
	}
}

bool BSSmallBlockAllocator::GetPoolStats(MemoryPoolStats* apStats, uint32_t auiPoolIndex)
{
	if (auiPoolIndex >= 64)
		return false;
	Pool& rPool = PoolA[auiPoolIndex];
	EnterCriticalSection(&rPool.Lock);
	std::memset(apStats, 0, sizeof(*apStats));
	apStats->pDesc = "Small Block Pool";
	apStats->iOverhead = 32 * (rPool.TotalBytes >> 13) + sizeof(Pool);
	apStats->iBlockSize = 8 * auiPoolIndex;
	apStats->iReservedMem = apStats->iCommittedMem = rPool.TotalBytes;
	apStats->iReservedPages = apStats->iCommittedPages = rPool.TotalBytes >> 16;
	apStats->iFreeBlocks = rPool.TotalFreeBlocks;
	apStats->iUsedBlocks = rPool.TotalAllocatedBlocks;
	LeaveCriticalSection(&rPool.Lock);
	return true;
}

void BSSmallBlockAllocator::RequestDecommit()
{
	EnterCriticalSection(&Lock);
	PopUnusedBlockPages();
	LeaveCriticalSection(&Lock);
}

void* BSSmallBlockAllocator::AllocateAlignImpl(size_t auiSize, uint32_t auiAlignment) { return BaseAllocate(auiSize, auiAlignment, true); }
void* BSSmallBlockAllocator::TryAllocateImpl(size_t auiSize, uint32_t auiAlignment) { return BaseAllocate(auiSize, auiAlignment, false); }

void* BSSmallBlockAllocator::BaseAllocate(size_t auiSize, uint32_t auiAlignment, bool abAddPages)
{
	size_t uiAlignment = auiAlignment > 8 ? auiAlignment : 8;
	size_t uiSize = (auiSize + uiAlignment - 1) & -uiAlignment;
	if (uiSize <= 8)
		uiSize = 8;
	if (uiSize > 512)
		return nullptr;
	Pool& rPool = PoolA[(uiSize >> 3) - 1];
	EnterCriticalSection(&rPool.Lock);
	void* pBlock = AllocateFromPool(&rPool, abAddPages);
	LeaveCriticalSection(&rPool.Lock);
	return pBlock;
}

FreeBlock* BSSmallBlockAllocator::AllocateFromPool(BSSmallBlockAllocatorUtil::Pool* apPool, bool abAddPages)
{
	BlockPage* pPage;
	if (apPool->TotalFreeBlocks)
	{
		pPage = apPool->pCurrAlloc;
		if (!pPage || !pPage->FreeElem)
			pPage = apPool->pPageList;
	}
	else
	{
		FreeBlock* pBuffer = nullptr;
		EnterCriticalSection(&Lock);
		pPage = AllocBlockPage(pBuffer, abAddPages);
		LeaveCriticalSection(&Lock);
		if (!pPage)
			return nullptr;
		apPool->AddPage(pPage, pBuffer);
	}
	return pPage ? apPool->AllocateFromPage(pPage) : nullptr;
}

void BSSmallBlockAllocator::DeallocateAlignImpl(void*& arpBlock)
{
	MegaBlockPage* pOwner = AddrToMegaBlockPage(arpBlock);
	BlockPage* pPage = pOwner->AddrToBlockPage(arpBlock);
	Pool& rPool = PoolA[(pPage->ElemSize >> 3) - 1];
	EnterCriticalSection(&rPool.Lock);
	bool bEmpty = rPool.DeallocateToPage(pPage, static_cast<FreeBlock*>(arpBlock));
	if (bEmpty)
		rPool.RemovePage(pPage);
	LeaveCriticalSection(&rPool.Lock);
	if (bEmpty)
	{
		EnterCriticalSection(&Lock);
		FreeBlockPage(pPage, pOwner, bAllowDecommits);
		PopUnusedBlockPages();
		LeaveCriticalSection(&Lock);
	}
	arpBlock = nullptr;
}

void BSSmallBlockAllocator::RegisterUserPool(UserPoolBase* apPool, uint32_t aInitialReserve)
{
	EnterCriticalSection(&Lock);
	FreeBlock* pBuffer = nullptr;
	while (apPool->MyPool.TotalFreeBlocks < aInitialReserve)
	{
		BlockPage* pPage = AllocBlockPage(pBuffer, true);
		if (!pPage)
			break;
		apPool->MyPool.AddPage(pPage, pBuffer);
	}
	LeaveCriticalSection(&Lock);
}

void BSSmallBlockAllocator::DeallocateToUserPool(UserPoolBase* apPool, FreeBlock* apPtr)
{
	MegaBlockPage* pOwner = AddrToMegaBlockPage(apPtr);
	BlockPage* pPage = pOwner->AddrToBlockPage(apPtr);
	if (apPool->MyPool.DeallocateToPage(pPage, apPtr))
	{
		uint32_t uiBlocks = apPool->MyPool.TotalFreeBlocks + apPool->MyPool.TotalAllocatedBlocks;
		if (uiBlocks > pPage->FreeElem && uiBlocks - pPage->FreeElem >= apPool->InitialReserve)
		{
			apPool->MyPool.RemovePage(pPage);
			EnterCriticalSection(&Lock);
			FreeBlockPage(pPage, pOwner, apPool->BatchModeEnableCount == 0);
			PopUnusedBlockPages();
			LeaveCriticalSection(&Lock);
		}
	}
}

void BSSmallBlockAllocator::PopUnusedBlockPages()
{
	char* pEnd = pBlockPageCommit;
	while (reinterpret_cast<uintptr_t>(pEnd) > reinterpret_cast<uintptr_t>(pBlockPageCommitMin))
	{
		auto* pPage = reinterpret_cast<MegaBlockPage*>(pEnd - 0x200000);
		if (pPage->FreeBlockPages != 255)
			break;
		if (pPage == pMegaBlockCurrAlloc)
			pMegaBlockCurrAlloc = nullptr;
		if (pPage->pRight == pPage)
			pMegaBlockPageList = nullptr;
		else
		{
			if (pPage == pMegaBlockPageList)
				pMegaBlockPageList = pPage->pRight;
			pPage->pRight->pLeft = pPage->pLeft;
			pPage->pLeft->pRight = pPage->pRight;
		}
		TotalFreeBlockPages -= 255;
		pEnd -= 0x200000;
	}
	if (bAllowDecommits && reinterpret_cast<uintptr_t>(pBlockPageCommit) - reinterpret_cast<uintptr_t>(pEnd) >= 0x200000)
	{
		VirtualFree(pEnd, size_t(pBlockPageCommit - pEnd), MEM_DECOMMIT);
		pBlockPageCommit = pEnd;
	}
}

BlockPage* BSSmallBlockAllocator::AllocBlockPage(FreeBlock*& arpOutBuffer, bool abAddPages)
{
	MegaBlockPage* pPage = pMegaBlockCurrAlloc;
	if (TotalFreeBlockPages)
	{
		if (!pPage)
		{
			pPage = pMegaBlockPageList;
			for (MegaBlockPage* pNext = pMegaBlockPageList->pRight; pNext != pMegaBlockPageList; pNext = pNext->pRight)
			{
				if (!pNext->FreeBlockPages)
					break;
				if (pNext->FreeBlockPages < pPage->FreeBlockPages ||
					(pNext->FreeBlockPages == pPage->FreeBlockPages && pNext < pPage))
					pPage = pNext;
			}
		}
	}
	else
	{
		if (!abAddPages || reinterpret_cast<uintptr_t>(pBlockPageCommit) + 0x200000 > reinterpret_cast<uintptr_t>(pAllocBase) + AddressSpaceSize ||
			!VirtualAlloc(pBlockPageCommit, 0x200000, MEM_COMMIT, PAGE_READWRITE))
			return nullptr;
		pPage = reinterpret_cast<MegaBlockPage*>(pBlockPageCommit);
		pBlockPageCommit += 0x200000;
		if (pMegaBlockPageList)
		{
			pPage->pRight = pMegaBlockPageList;
			pPage->pLeft = pMegaBlockPageList->pLeft;
			pMegaBlockPageList->pLeft->pRight = pPage;
			pMegaBlockPageList->pLeft = pPage;
		}
		else
			pPage->pLeft = pPage->pRight = pPage;
		pMegaBlockPageList = pPage;
		pPage->FreeBlockPages = 255;
		pPage->NextBlockPageAlloc = 0;
		pPage->pFreeBlockPages = nullptr;
		pPage->bDecommitted = false;
		TotalFreeBlockPages += 255;
	}
	if (!pPage)
		return nullptr;
	BlockPage* pBlock = pPage->pFreeBlockPages;
	if (pBlock)
	{
		pPage->pFreeBlockPages = pBlock->pRight;
		arpOutBuffer = reinterpret_cast<FreeBlock*>(pPage->Mem + (pBlock - pPage->BlockPageA) * 0x2000);
	}
	else
	{
		uint16_t uiIndex = pPage->NextBlockPageAlloc++;
		arpOutBuffer = reinterpret_cast<FreeBlock*>(pPage->Mem + size_t(uiIndex) * 0x2000);
		pBlock = &pPage->BlockPageA[uiIndex];
		pBlock->Check = 0xFEED;
	}
	if (pPage->FreeBlockPages-- == 1)
	{
		if (!pMegaBlockPageList)
		{
			pPage->pLeft = pPage->pRight = pPage;
			pMegaBlockPageList = pPage;
		}
		else if (pPage->pRight == pPage)
			pMegaBlockCurrAlloc = nullptr;
		else
		{
			if (pPage == pMegaBlockCurrAlloc)
				pMegaBlockCurrAlloc = nullptr;
			if (pPage == pMegaBlockPageList)
				pMegaBlockPageList = pPage->pRight;
			pPage->pRight->pLeft = pPage->pLeft;
			pPage->pLeft->pRight = pPage->pRight;
			pPage->pRight = pMegaBlockPageList;
			pPage->pLeft = pMegaBlockPageList->pLeft;
			pMegaBlockPageList->pLeft->pRight = pPage;
			pMegaBlockPageList->pLeft = pPage;
		}
	}
	else
		pMegaBlockCurrAlloc = pPage;
	--TotalFreeBlockPages;
	return pBlock;
}

void BSSmallBlockAllocator::FreeBlockPage(BlockPage* apPage, MegaBlockPage* apOwner, bool)
{
	apPage->pRight = apOwner->pFreeBlockPages;
	++apOwner->FreeBlockPages;
	apOwner->pFreeBlockPages = apPage;
	apPage->Check = 0xFEED;
	if (apOwner->FreeBlockPages == 1)
	{
		if (apOwner->pRight != apOwner && apOwner != pMegaBlockPageList)
		{
			apOwner->pRight->pLeft = apOwner->pLeft;
			apOwner->pLeft->pRight = apOwner->pRight;
			apOwner->pRight = pMegaBlockPageList;
			apOwner->pLeft = pMegaBlockPageList->pLeft;
			pMegaBlockPageList->pLeft->pRight = apOwner;
			pMegaBlockPageList->pLeft = apOwner;
			pMegaBlockPageList = apOwner;
		}
		if (!pMegaBlockCurrAlloc)
			pMegaBlockCurrAlloc = apOwner;
	}
	++TotalFreeBlockPages;
}
