#pragma once

#include "BSCore/IMemoryStore.h"
#include <windows.h>

class BSSmallBlockAllocator;

struct MemoryPoolStats
{
	const char* pDesc;
	uint32_t iOverhead;
	uint32_t iBlockSize;
	uint32_t iReservedMem;
	uint32_t iReservedPages;
	uint32_t iCommittedMem;
	uint32_t iCommittedPages;
	uint32_t iHighCommittedPages;
	uint32_t iFreeBlocks;
	uint32_t iUsedBlocks;
};
static_assert(sizeof(MemoryPoolStats) == 0x30);

namespace BSSmallBlockAllocatorUtil
{
	struct FreeBlock
	{
		FreeBlock* pNext;
	};

	struct BlockPage
	{
		BlockPage* pLeft;
		BlockPage* pRight;
		FreeBlock* pBlocks;
		uint16_t TotalElem;
		uint16_t FreeElem;
		uint16_t ElemSize;
		uint16_t Check;
	};
	static_assert(sizeof(BlockPage) == 0x20);
	static_assert(offsetof(BlockPage, ElemSize) == 0x1C);

	struct Pool
	{
		BlockPage* pPageList = nullptr;
		BlockPage* pCurrAlloc = nullptr;
		uint32_t TotalFreeBlocks = 0;
		uint32_t TotalAllocatedBlocks = 0;
		uint32_t TotalBytes = 0;
		uint32_t ElementSize = 0;

		void RemovePage(BlockPage* apPage);
		void InsertEmptyPage(BlockPage* apPage);
		void InsertNonEmptyPage(BlockPage* apPage);
		void AddPageExternal(BlockPage* apPage);
		void AddPage(BlockPage* apPage, FreeBlock* apBuffer);
		FreeBlock* AllocateFromPage(BlockPage* apPage);
		bool DeallocateToPage(BlockPage* apPage, FreeBlock* apPtr);
	};
	static_assert(sizeof(Pool) == 0x20);

	class UserPoolBase
	{
	public:
		UserPoolBase(BSSmallBlockAllocator* apOwner, size_t aElemSize, uint32_t aInitialReserve);
		virtual ~UserPoolBase();
		virtual void Lock() {}
		virtual void Unlock() {}
		FreeBlock* BaseAllocate();
		void BaseDeallocate(FreeBlock* apPtr);

		Pool MyPool;
		BSSmallBlockAllocator* pOwner;
		uint32_t InitialReserve;
		uint32_t BatchModeEnableCount;
	};
	static_assert(sizeof(UserPoolBase) == 0x38);
}

class BSSmallBlockAllocator : public IMemoryStore
{
public:
	struct Pool : BSSmallBlockAllocatorUtil::Pool
	{
		Pool();
		~Pool();
		CRITICAL_SECTION Lock;
	};
	static_assert(sizeof(Pool) == 0x48);

	struct MegaBlockPage
	{
		char Mem[0x1FE000];
		BSSmallBlockAllocatorUtil::BlockPage BlockPageA[255];
		MegaBlockPage* pLeft;
		MegaBlockPage* pRight;
		BSSmallBlockAllocatorUtil::BlockPage* pFreeBlockPages;
		uint16_t FreeBlockPages;
		uint16_t NextBlockPageAlloc;
		bool bDecommitted;

		BSSmallBlockAllocatorUtil::BlockPage* AddrToBlockPage(const void* apBlock)
		{
			return &BlockPageA[(reinterpret_cast<intptr_t>(apBlock) - reinterpret_cast<intptr_t>(this)) >> 13];
		}
	};
	static_assert(sizeof(MegaBlockPage) == 0x200000);
	static_assert(offsetof(MegaBlockPage, pLeft) == 0x1FFFE0);
	static_assert(offsetof(MegaBlockPage, FreeBlockPages) == 0x1FFFF8);

	BSSmallBlockAllocator(uint32_t aAddressRangeSize, uint32_t aInitialCommit);
	~BSSmallBlockAllocator() override;
	size_t Size(const void* apBlock) const override;
	void GetMemoryStats(MemoryStats* apStats) override;
	bool GetPoolStats(MemoryPoolStats* apStats, uint32_t auiPoolIndex);
	bool GetPoolContextInfo(uint32_t* apInfoDest, uint32_t auiPoolIndex);
	uint32_t QTotalPools() const;
	bool QBlockInStore(const void* apBlock) const;
	void RequestDecommit();
	void* BaseAllocate(size_t auiSize, uint32_t auiAlignment, bool abAddPages);
	BSSmallBlockAllocatorUtil::FreeBlock* AllocateFromPool(BSSmallBlockAllocatorUtil::Pool* apPool, bool abAddPages);
	void RegisterUserPool(BSSmallBlockAllocatorUtil::UserPoolBase* apPool, uint32_t aInitialReserve);
	void DeallocateToUserPool(BSSmallBlockAllocatorUtil::UserPoolBase* apPool, BSSmallBlockAllocatorUtil::FreeBlock* apPtr);
	MegaBlockPage* AddrToMegaBlockPage(const void* apBlock) const;
	void PopUnusedBlockPages();
	BSSmallBlockAllocatorUtil::BlockPage* AllocBlockPage(BSSmallBlockAllocatorUtil::FreeBlock*& arpOutBuffer, bool abAddPages);
	void FreeBlockPage(BSSmallBlockAllocatorUtil::BlockPage* apPage, MegaBlockPage* apOwner, bool abDecommit);

	Pool PoolA[64];
	CRITICAL_SECTION Lock;
	uint32_t AddressSpaceSize;
	char* pAllocBase;
	char* pBlockPageCommitMin;
	char* pBlockPageCommit;
	MegaBlockPage* pMegaBlockPageList;
	MegaBlockPage* pMegaBlockCurrAlloc;
	uint32_t TotalFreeBlockPages;
	bool bAllowDecommits;

private:
	bool ContainsBlockImpl(const void* apBlock) const override;
	void* AllocateAlignImpl(size_t auiSize, uint32_t auiAlignment) override;
	void DeallocateAlignImpl(void*& arpBlock) override;
	void* TryAllocateImpl(size_t auiSize, uint32_t auiAlignment) override;
};
static_assert(sizeof(BSSmallBlockAllocator) == 0x1268);
static_assert(offsetof(BSSmallBlockAllocator, PoolA) == 0x8);
static_assert(offsetof(BSSmallBlockAllocator, Lock) == 0x1208);
static_assert(offsetof(BSSmallBlockAllocator, pAllocBase) == 0x1238);
static_assert(offsetof(BSSmallBlockAllocator, TotalFreeBlockPages) == 0x1260);

inline void BSSmallBlockAllocatorUtil::Pool::RemovePage(BlockPage* apPage)
{
	if (apPage->pRight == apPage)
		pPageList = nullptr;
	else
	{
		if (pPageList == apPage)
			pPageList = apPage->pRight;
		apPage->pRight->pLeft = apPage->pLeft;
		apPage->pLeft->pRight = apPage->pRight;
	}
	if (pCurrAlloc == apPage)
		pCurrAlloc = nullptr;
	TotalFreeBlocks -= apPage->FreeElem;
	TotalAllocatedBlocks += uint32_t(apPage->FreeElem) - apPage->TotalElem;
	TotalBytes -= 0x2000;
	apPage->pLeft = apPage->pRight = nullptr;
}

inline void BSSmallBlockAllocatorUtil::Pool::InsertEmptyPage(BlockPage* apPage)
{
	TotalFreeBlocks += apPage->FreeElem;
	TotalAllocatedBlocks += uint32_t(apPage->TotalElem) - apPage->FreeElem;
	TotalBytes += 0x2000;
	if (pPageList)
	{
		apPage->pRight = pPageList;
		apPage->pLeft = pPageList->pLeft;
		pPageList->pLeft->pRight = apPage;
		pPageList->pLeft = apPage;
	}
	else
	{
		apPage->pLeft = apPage->pRight = apPage;
		pPageList = apPage;
	}
}

inline void BSSmallBlockAllocatorUtil::Pool::InsertNonEmptyPage(BlockPage* apPage)
{
	InsertEmptyPage(apPage);
	pPageList = apPage;
	if (!pCurrAlloc)
		pCurrAlloc = apPage;
}
