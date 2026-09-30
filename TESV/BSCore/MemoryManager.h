#pragma once

#include "BSCore/MemoryDefs.h"
#include "BSCore/ScrapHeap.h"

#include <cstddef>
#include <cstdint>

class BSSmallBlockAllocator;
class IMemoryHeap;

namespace CompactingStore
{
	class Store;
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
}

// Routes allocations to heaps by memory context, with a small block allocator
// for small ones and a scrap heap per thread.
class MemoryManager
{
public:
	struct ThreadScrapHeap
	{
		ScrapHeap Heap;
		ThreadScrapHeap* pNext;
		unsigned int OwningThread;
	};
	static_assert(sizeof(ThreadScrapHeap) == 0xA0);

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

	size_t Size(const void* apMem) const;
	ScrapHeap* GetThreadScrapHeap();
	IMemoryHeap* GetHeapForPointer(const void* apMem) const;
	IMemoryHeap* GetHeapForPhysicalPointer(const void* apMem) const;
	unsigned int ProcessMemoryProblem(IMemoryHeap* apHeap, int aiMemoryPass, bool* apbAllowSystemAllocs);

protected:
	MemoryManager();

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
	static thread_local unsigned int uiThreadInitState;
	static thread_local bool bThreadAllocationPass;
	static thread_local unsigned int uiThreadMemoryProblemDepth;
	static thread_local unsigned int uiThreadMemoryProblemPass;
	static thread_local ThreadScrapHeap* pThreadScrapHeapTLS;
	alignas(8) static thread_local unsigned char aThreadScrapHeapBuffer[sizeof(ThreadScrapHeap)];
};
static_assert(sizeof(MemoryManager) == 0x480);
static_assert(offsetof(MemoryManager, pThreadScrapHeap) == 0x410);
static_assert(offsetof(MemoryManager, pSmallBlockAllocator) == 0x430);
static_assert(offsetof(MemoryManager, bAllowPoolUse) == 0x449);
static_assert(offsetof(MemoryManager, iAlignmentForPools) == 0x460);
static_assert(offsetof(MemoryManager, iFailedAllocationSize) == 0x468);
