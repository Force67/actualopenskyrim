#pragma once

#include "BSCore/BSNonReentrantSpinLock.h"
#include "BSCore/CompactingStoreCommon.h"
#include "BSCore/BSTObjectArena.h"
#include "BSCore/IMemoryHeap.h"

class IMemoryTracker;

namespace CompactingStore
{
	class Store;
	class alignas(64) BatchDeallocateOperation
	{
	public:
		BatchDeallocateOperation();
		~BatchDeallocateOperation();
		BSTObjectArena<HandleType, BSTObjectArenaScrapAlloc, 32> HandleArena;
		Store* pStore;
		IMemoryTracker* pTrack;
		bool bTrack;
	};
	static_assert(sizeof(BatchDeallocateOperation) == 0x80);
	static_assert(offsetof(BatchDeallocateOperation, pStore) == 0x40);
	extern volatile uint32_t uiWantFree;
	class Store : public IMemoryStoreBase
	{
	public:
		Store(size_t auiSize, size_t auiPrecommit);
		~Store() override;
		bool AllocateAlign(IMemoryTracker* apTracker, bool abTrack, HandleType& arResult, size_t auiSize, size_t auiAlignment, MoveCallback* apCallback, bool abMustSucceed, bool abAllowExtend);
		void Deallocate(IMemoryTracker* apTracker, bool abTrack, HandleType& arHandle);
		void BeginBatchDeallocate(IMemoryTracker* apTracker, bool abTrack, BatchDeallocateOperation& arBatch);
		void FlushBatchDeallocates();
		bool QAddressInStore(const void* apPtr) const { return apPtr >= pAllocBase && apPtr <= pAllocEnd; }
		bool IsHandleLocked(const HandleType& arHandle) const { return arHandle.pStoreBlock && (arHandle.pStoreBlock->uiAccessFlags & 0x7FFF); }
		size_t Size(const void* apBlock) const override;
		void GetMemoryStats(MemoryStats* apStats) override;
		void GetHeapStats(HeapStats* apStats, bool abFullBlockInfo);
		void RequestDecommit(bool abAlwaysCompact);
		void RequestFullCompact();
		void RequestCompactForSize(size_t auiSize);
		void RequestStepMerge();
		void* AllocateAlignPinned(IMemoryTracker* apTracker, bool abTrack, size_t auiSize, size_t auiAlignment);
		void DeallocatePinned(IMemoryTracker* apTracker, bool abTrack, void* apBlock);
		size_t QSizePinned(const void* apBlock) const;
		StoreBlock* NewStoreBlock();
		FreeBlock* AddPages(size_t auiAmount);
		void TryDecommit();
		void DeallocateBlock(IMemoryTracker* apTracker, bool abTrack, StoreBlock* apStoreBlock);
		void RemoveFreeBlock(FreeBlock* apBlock, size_t auiSize);
		void InsertFreeBlock(FreeBlock* apBlock, size_t auiSize);
		size_t QListIndexForSize(size_t auiSize) const;
		size_t MergeFrom(FreeBlock* apBlock);
		FreeBlock* MergeBlockList(FreeBlock*& arpList, FreeBlock* apLowest);

		BSNonReentrantSpinLock Lock;
		void* pAllocBase;
		void* pAllocEndMin;
		void* pAllocEnd;
		void* pStoreEnd;
		BlockHeader* pLastBlock;
		FreeBlock* pSmallFreeA[66];
		FreeBlock* pCurrentFree;
		FreeBlock* pNextMerge;
		StoreBlock* pStoreBlockMin;
		StoreBlock* pNextStoreBlock;
		StoreBlock* pFreeStoreBlockList;
		uint32_t CurrentThread;
		size_t uiAllocated;
		uint32_t uiNumAllocatedBlocks;
		size_t uiFree;
		uint32_t uiNumFreeBlocks;
		uint32_t uiCompacted;
		uint32_t uiBatchDeallocateTlsSlot;

	private:
		FreeBlock* GetBestScoredBlockFromList(FreeBlock* apList, size_t auiSize, size_t auiAlignment, size_t auiAlignPadMask, size_t auiAlignMask, size_t& aruiResultAddr, size_t& aruiEndAddr, size_t& aruiTrailerAddr, size_t& aruiLeaderSize, size_t& aruiBlockToUseSize);
		bool QBlockIsValid(const BlockHeader* apBlock) const;
		bool QNextBlockIsValid(const BlockHeader* apBlock) const;
		void ValidateBlocks() const;
		friend class BatchDeallocateOperation;
		void ProcessBatchDeallocate(BatchDeallocateOperation& arBatch);
		void EndBatchDeallocate(BatchDeallocateOperation& arBatch);
		size_t RequestCompact(size_t auiSize);
		size_t UpdateFree(const FreeBlock*& arpGap);
		size_t PushFree(AllocatedBlock*& arpGap, bool abAllowAlignPad);
		bool ContainsBlockImpl(const void* apBlock) const override;
	};
	static_assert(sizeof(Store) == 0x2A0);
	static_assert(offsetof(Store, pAllocBase) == 0x10);
	static_assert(offsetof(Store, pCurrentFree) == 0x248);
	static_assert(offsetof(Store, pStoreBlockMin) == 0x258);
	static_assert(offsetof(Store, CurrentThread) == 0x270);
	static_assert(offsetof(Store, uiFree) == 0x288);
}
