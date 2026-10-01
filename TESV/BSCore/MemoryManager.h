#pragma once

#include "BSCore/MemoryDefs.h"
#include "BSCore/ScrapHeap.h"
#include "BSCore/BSTSingleton.h"

#include <cstddef>
#include <cstdint>

class BSSmallBlockAllocator;
class IMemoryHeap;
struct HeapStats;
struct MemoryStats;
struct MemoryPoolStats;
class IMemoryTracker;
template <class Event> class BSTEventSink;

class IMemoryManagerFile
{
public:
	virtual ~IMemoryManagerFile();
	virtual int Read(void* apBuffer, size_t auiSize) = 0;
	virtual int Write(const void* apBuffer, size_t auiSize) = 0;
	virtual int Size() = 0;
	virtual void Seek(int aiOffset) = 0;
};
static_assert(sizeof(IMemoryManagerFile) == 8);

class IMemoryManagerFileFactory : public BSTSingletonExplicit<IMemoryManagerFileFactory>
{
public:
	virtual ~IMemoryManagerFileFactory();
	virtual bool Create(const char* apName, IMemoryManagerFile*& arpFile) = 0;
	virtual bool OpenSingletonFile(const char* apName, IMemoryManagerFile*& arpFile) = 0;
	virtual bool CloseSingletonFile(IMemoryManagerFile*& arpFile) = 0;
};
static_assert(sizeof(IMemoryManagerFileFactory) == 8);

namespace CompactingStore
{
	class Store;
	struct HandleType;
	class MoveCallback;
	class BatchDeallocateOperation;
}

namespace MemoryManagement
{
	struct PMPEvent
	{
		enum State : int32_t
		{
			Started = 0,
			Finished = 1,
		};

		State eState;
	};

	class PMPEventSource;
	void RegisterSink(BSTEventSink<PMPEvent>* apSink);
	void UnregisterSink(BSTEventSink<PMPEvent>* apSink);
}

// Routes allocations to heaps by memory context, with a small block allocator
// for small ones and a scrap heap per thread.
class MemoryManager
{
public:
	struct AutoScrapBuffer
	{
		AutoScrapBuffer();
		AutoScrapBuffer(size_t auiSize, size_t auiAlignment);
		~AutoScrapBuffer();
		void* QPtr() const { return pPtr; }
		void Swap(AutoScrapBuffer& arRhs)
		{
			void* pTemp = pPtr;
			pPtr = arRhs.pPtr;
			arRhs.pPtr = pTemp;
		}
		void* pPtr;
	};
	static_assert(sizeof(AutoScrapBuffer) == 8);

	struct ThreadScrapHeap
	{
		ScrapHeap Heap;
		ThreadScrapHeap* pNext;
	};
	static_assert(sizeof(ThreadScrapHeap) == 0x98);
	static_assert(offsetof(ThreadScrapHeap, pNext) == 0x90);

	~MemoryManager();

	static MemoryManager& Instance()
	{
		alignas(MemoryManager) static char cbuffer[sizeof(MemoryManager)];
		static unsigned int uiinitFence;
		if (uiinitFence != 2)
			UpdateInitState(reinterpret_cast<MemoryManager*>(cbuffer), &uiinitFence);
		return *reinterpret_cast<MemoryManager*>(cbuffer);
	}

	void* Allocate(size_t aSize) { return Allocate(aSize, 0, false); }
	void* AllocateAligned(size_t aSize, unsigned int auiAlignment) { return Allocate(aSize, auiAlignment, true); }
	void* Reallocate(void* apOld, size_t aSize) { return Reallocate(apOld, aSize, 0, false); }
	void* ReallocateAligned(void* apOld, size_t aSize, unsigned int auiAlignment) { return Reallocate(apOld, aSize, auiAlignment, true); }
	void Deallocate(void* apMem) { Deallocate(apMem, false); }
	void DeallocateAligned(void* apMem) { Deallocate(apMem, true); }

	void* Allocate(size_t aSize, unsigned int auiAlignment, bool abAlignmentRequired);
	void* Reallocate(void* apOld, size_t aSize, unsigned int auiAlignment, bool abAlignmentRequired);
	void Deallocate(void* apMem, bool abAlignmentRequired);

	bool AllocateCompactable(CompactingStore::HandleType& arResult, size_t auiSize, size_t auiAlignment, CompactingStore::MoveCallback* apCallback, bool abMustSucceed, bool abCompactBeforeExtend);
	void* AllocateCompactablePinned(size_t auiSize, size_t auiAlignment);
	void DeallocateCompactable(CompactingStore::HandleType& arHandle);
	void BeginBatchDeallocateCompactable(CompactingStore::BatchDeallocateOperation& arBatch);
	void DeallocateCompactablePinned(void* apPtr);
	void CleanPools();
	void CleanCompactingStore(bool abAlwaysCompact);
	void CompactCompactingStore();
	void StepCompactingStoreMerge();
	unsigned int QMainThreadMemoryProblemPassSignal() { return iMainThreadMemoryProblemPassSignal; }
	unsigned int ClearMainThreadMemoryProblemPassSignal() { return iMainThreadMemoryProblemPassSignal = 0; }
	void SetExternalHavokAllocator(IMemoryHeap* apAllocator);
	IMemoryHeap* QExternalHavokAllocator() const;
	void GetExternalHavokAllocatorStats(MemoryStats* apStats) const;
	IMemoryHeap* GetHeapByIndex(unsigned int auiIndex) const;
	IMemoryHeap* GetHeapForContext(MEM_CONTEXT aeContext) const;
	bool GetHeapStats(unsigned int auiIndex, bool abFullBlockInfo, HeapStats* apStats) const;
	bool GetPhysicalHeapStats(unsigned int auiIndex, bool abFullBlockInfo, HeapStats* apStats) const;
	bool GetDefaultHeapStats(unsigned int auiIndex, bool abFullBlockInfo, HeapStats* apStats) const;
	bool GetCompactingStoreHeapStats(bool abFullBlockInfo, HeapStats* apStats) const;
	bool GetCompactingStoreMemoryStats(MemoryStats* apStats) const;
	unsigned int QTotalPools();
	unsigned int GetMemoryInThreadStacks();
	static IMemoryTracker** QTrackerPtr();
	bool QPoolExists(unsigned int auiPoolIndex) const;
	bool GetPoolStats(unsigned int auiPoolIndex, MemoryPoolStats* apStats) const;
	bool GetPoolStats(unsigned int auiPoolIndex, MemoryPoolStats* apStats, unsigned int& aruiMaxConsecutiveFailedAllocCount) const;
	bool GetPoolContextInfo(unsigned int auiPoolIndex, unsigned int* apInfoDest) const;

	size_t Size(const void* apMem) const;
	ScrapHeap* GetThreadScrapHeap();
	IMemoryHeap* GetHeapForPointer(const void* apMem) const;
	IMemoryHeap* GetHeapForPhysicalPointer(const void* apMem) const;
	unsigned int ProcessMemoryProblem(IMemoryHeap* apHeap, int aiMemoryPass, bool* apbAllowSystemAllocs);

protected:
	MemoryManager();
	void Initialize();
	void CreatePoolStore(unsigned int auiAddressRangeSize, unsigned int auiInitialCommit);
	void CreateCompactingStore(size_t auiSize, unsigned int auiInitialCommit);

	static void UpdateInitState(MemoryManager* apInstanceBuffer, unsigned int* apuiInitFence);

	// Game specific setup of heaps and pools.
	void SpecifyMemoryLayout();
	void SpecifyPools();
	void RegisterMemoryManager();
	unsigned int ProcessMemoryProblemImpl(IMemoryHeap* apHeap, int aiMemoryPass, bool* apbAllowSystemAllocs);

public:
	bool bInitialized;
	uint16_t iNumHeaps;
	uint16_t iNumPhysicalHeaps;
	IMemoryHeap** ppHeaps;
	bool* pAllowOtherContextAllocs;
	IMemoryHeap* pHeapsByContextA[MEM_CONTEXT_COUNT];
	ThreadScrapHeap* pThreadScrapHeap;
	IMemoryHeap** ppPhysicalHeaps;
	IMemoryHeap* pBigAllocHeap;
	IMemoryHeap* pEmergencyHeap;
	BSSmallBlockAllocator* pSmallBlockAllocator;
	CompactingStore::Store* pCompactingStore;
	IMemoryHeap* pExternalHavokAllocator;
	bool bSpecialHeaps;
	bool bAllowPoolUse;
	size_t iSysAllocBytes;
	size_t iMallocBytes;
	unsigned int iAlignmentForPools;
	unsigned int iMainThreadMemoryProblemPassSignal;
	size_t iFailedAllocationSize;
	unsigned int iNumMemoryProblemPassesRun;
	size_t iTimeOfLastMemoryProblemPass;

private:
	// Per thread state of the memory manager.
	static thread_local bool bAllowCleanCompactingStoreST;
	static thread_local unsigned int uiThreadInitState;
	static thread_local bool bThreadAllocationPass;
	static thread_local unsigned int uiThreadMemoryProblemDepth;
	static thread_local unsigned int uiThreadMemoryProblemPass;
	static thread_local ThreadScrapHeap* pThreadScrapHeapTLS;
	alignas(8) static thread_local unsigned char aThreadScrapHeapBuffer[sizeof(ThreadScrapHeap)];
};
static_assert(sizeof(MemoryManager) == 0x480);
static_assert(offsetof(MemoryManager, ppHeaps) == 0x8);
static_assert(offsetof(MemoryManager, pHeapsByContextA) == 0x18);
static_assert(offsetof(MemoryManager, pThreadScrapHeap) == 0x410);
static_assert(offsetof(MemoryManager, ppPhysicalHeaps) == 0x418);
static_assert(offsetof(MemoryManager, pSmallBlockAllocator) == 0x430);
static_assert(offsetof(MemoryManager, pCompactingStore) == 0x438);
static_assert(offsetof(MemoryManager, bAllowPoolUse) == 0x449);
static_assert(offsetof(MemoryManager, iAlignmentForPools) == 0x460);
static_assert(offsetof(MemoryManager, iMainThreadMemoryProblemPassSignal) == 0x464);
static_assert(offsetof(MemoryManager, iFailedAllocationSize) == 0x468);
