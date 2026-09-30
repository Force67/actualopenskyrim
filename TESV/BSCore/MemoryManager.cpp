#include "BSCore/MemoryManager.h"

#include "BSCore/BSSafeSleep.h"
#include "BSCore/BSSmallBlockAllocator.h"
#include "BSCore/BSTEvent.h"
#include "BSCore/BSTSingleton.h"
#include "BSCore/BSThread.h"
#include "BSCore/CompactingStore.h"
#include "BSCore/IMemoryHeap.h"
#include "BSCore/MemoryContextTracker.h"

#include <cstdlib>
#include <cstring>
#include <emmintrin.h>
#include <new>
#include <windows.h>

namespace MemoryManagement
{
	class PMPEventSource : public BSTEventSource<PMPEvent>
	{
	};
}

thread_local MEM_CONTEXT MemoryContextTracker::eThreadContext;

thread_local unsigned int MemoryManager::uiThreadInitState;
thread_local bool MemoryManager::bThreadAllocationPass;
thread_local unsigned int MemoryManager::uiThreadMemoryProblemDepth;
thread_local unsigned int MemoryManager::uiThreadMemoryProblemPass;
thread_local MemoryManager::ThreadScrapHeap* MemoryManager::pThreadScrapHeapTLS;
alignas(8) thread_local unsigned char MemoryManager::aThreadScrapHeapBuffer[sizeof(ThreadScrapHeap)];

namespace
{
	constexpr size_t SMALL_BLOCK_MAX_SIZE = 0x200;
	constexpr size_t THREAD_SCRAP_HEAP_MIN_COMMIT = 0x20000;

	// Allocations of zero bytes return one of two addresses in here, alternating,
	// so consecutive ones differ.
	alignas(16) char zeroReturns[32];
	void* pZeroAddress;

	// Never set by the game; a debugger switch that bypasses the heaps.
	bool bUseSystemAllocator;

	void* SystemAllocate(size_t aSize, unsigned int auiAlignment, bool abAlignmentRequired)
	{
		return abAlignmentRequired ? _aligned_malloc(aSize, auiAlignment) : malloc(aSize);
	}

	void SystemDeallocate(void* apMem, bool abAlignmentRequired)
	{
		if (abAlignmentRequired)
			_aligned_free(apMem);
		else
			free(apMem);
	}

	void* NextZeroAddress()
	{
		void* pResult = pZeroAddress;
		pZeroAddress = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(pZeroAddress) ^ 0x10);
		return pResult;
	}

	// True for either zero size address. Like the original, a match with the
	// current address leaves pZeroAddress flipped.
	bool IsZeroAddress(const void* apMem)
	{
		void* const pCurrent = pZeroAddress;
		void* const pOther = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(pCurrent) ^ 0x10);
		pZeroAddress = pOther;
		if (apMem == pCurrent)
			return true;
		pZeroAddress = pCurrent;
		return apMem == pOther;
	}

	bool InPinnedStore(const CompactingStore::Store* apStore, const void* apMem)
	{
		return apStore && apMem >= apStore->pAllocBase && apMem <= apStore->pAllocEnd;
	}
}

void MemoryManager::UpdateInitState(MemoryManager* apInstanceBuffer, unsigned int* apuiInitFence)
{
	if (uiThreadInitState)
		return;

	if (InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(apuiInitFence), 1, 0))
	{
		BSSafeSleep kSleep;
		while (*reinterpret_cast<volatile unsigned int*>(apuiInitFence) == 1)
			kSleep.Wait();
		return;
	}

	uiThreadInitState = 1;
	if (apInstanceBuffer)
		new (apInstanceBuffer) MemoryManager();
	_mm_mfence();
	*apuiInitFence = 2;
}

MemoryManager::MemoryManager() :
	bInitialized(false),
	iNumHeaps(0),
	iNumPhysicalHeaps(0),
	ppHeaps(nullptr),
	pAllowOtherContextAllocs(nullptr),
	pThreadScrapHeap(nullptr),
	ppPhysicalHeaps(nullptr),
	pBigAllocHeap(nullptr),
	pEmergencyHeap(nullptr),
	pSmallBlockAllocator(nullptr),
	pCompactingStore(nullptr),
	pExternalHavokAllocator(nullptr),
	bSpecialHeaps(true),
	bAllowPoolUse(true),
	iSysAllocBytes(0),
	iMallocBytes(0),
	iAlignmentForPools(4),
	iMainThreadMemoryProblemPassSignal(0),
	iFailedAllocationSize(0),
	iNumMemoryProblemPassesRun(0),
	iTimeOfLastMemoryProblemPass(0)
{
	AutoMemContext kContext(MC_CORE_SYSTEM);

	MEMORYSTATUSEX kStatus;
	kStatus.dwLength = sizeof(kStatus);
	if (GlobalMemoryStatusEx(&kStatus) && kStatus.ullAvailPageFile < 0x20000000)
	{
		MessageBoxA(nullptr, "Not enough memory to run application.", "Memory Error", MB_ICONERROR);
		exit(0);
	}

	SpecifyMemoryLayout();
	bInitialized = true;
	SpecifyPools();
	pZeroAddress = zeroReturns;
	RegisterMemoryManager();
	BSThreadEvent::InitSDM();
}

void* MemoryManager::Allocate(size_t aSize, unsigned int auiAlignment, bool abAlignmentRequired)
{
	const MEM_CONTEXT eContext = MemoryContextTracker::GetMemContext();
	IMemoryHeap* const pDefaultHeap = pHeapsByContextA[MC_CORE_UNKNOWN];
	IMemoryHeap* const pContextHeap = pHeapsByContextA[eContext];
	if (bUseSystemAllocator || !pDefaultHeap)
		return SystemAllocate(aSize, auiAlignment, abAlignmentRequired);
	if (!aSize)
		return NextZeroAddress();

	IMemoryHeap* const pHeap = pContextHeap ? pContextHeap : pDefaultHeap;
	int iMemoryPass = 0;
	bool bAllowSystemAllocs = false;
	void* pResult = nullptr;
	bThreadAllocationPass = false;
	do
	{
		if (bAllowPoolUse && pHeap->ShouldTrySmallBlockPools(aSize, eContext) && aSize <= SMALL_BLOCK_MAX_SIZE)
		{
			if (pSmallBlockAllocator)
			{
				if ((pResult = pSmallBlockAllocator->TryAllocate(aSize, auiAlignment)))
					return pResult;
				if ((pResult = pHeap->TryAllocate(aSize, auiAlignment)))
					return pResult;
				pResult = pSmallBlockAllocator->AllocateAlign(aSize, auiAlignment);
			}
		}
		if (pResult)
			break;

		if ((pResult = pHeap->AllocateAlign(aSize, auiAlignment)))
			return pResult;
		if (pHeap != pDefaultHeap && (pResult = pDefaultHeap->AllocateAlign(aSize, auiAlignment)))
			return pResult;

		iFailedAllocationSize = aSize;
		iMemoryPass = ProcessMemoryProblem(pHeap, iMemoryPass, &bAllowSystemAllocs);
		if (bAllowSystemAllocs)
			pResult = SystemAllocate(aSize, auiAlignment, abAlignmentRequired);
		if (!bAllowPoolUse && aSize <= SMALL_BLOCK_MAX_SIZE && pSmallBlockAllocator)
			pResult = pSmallBlockAllocator->AllocateAlign(aSize, auiAlignment);
	} while (!pResult);
	return pResult;
}

void* MemoryManager::Reallocate(void* apOld, size_t aSize, unsigned int auiAlignment, bool abAlignmentRequired)
{
	if (!aSize)
		return nullptr;

	void* const pNew = Allocate(aSize, auiAlignment, abAlignmentRequired);
	if (!apOld)
		return pNew;

	if (pNew)
	{
		size_t uiOldSize = 0;
		if (pSmallBlockAllocator && pSmallBlockAllocator->QBlockInStore(apOld))
			uiOldSize = pSmallBlockAllocator->Size(apOld);
		else if (InPinnedStore(pCompactingStore, apOld))
			uiOldSize = pCompactingStore->QSizePinned(apOld);
		else if (IMemoryHeap* pHeap = GetHeapForPointer(apOld))
			uiOldSize = pHeap->Size(apOld);
		else if (IMemoryHeap* pPhysicalHeap = GetHeapForPhysicalPointer(apOld))
			uiOldSize = pPhysicalHeap->Size(apOld);
		else if (!IsZeroAddress(apOld))
			uiOldSize = _msize(apOld);

		const size_t uiCopy = uiOldSize < aSize ? uiOldSize : aSize;
		if (uiCopy)
			memcpy_s(pNew, aSize, apOld, uiCopy);
	}
	Deallocate(apOld, abAlignmentRequired);
	return pNew;
}

void MemoryManager::Deallocate(void* apMem, bool abAlignmentRequired)
{
	if (!apMem)
		return;
	if (!bInitialized)
	{
		SystemDeallocate(apMem, abAlignmentRequired);
		return;
	}
	if (InPinnedStore(pCompactingStore, apMem))
	{
		pCompactingStore->DeallocatePinned(nullptr, false, apMem);
		return;
	}
	if (!pHeapsByContextA[MC_CORE_UNKNOWN])
	{
		SystemDeallocate(apMem, abAlignmentRequired);
		return;
	}
	if (pSmallBlockAllocator && pSmallBlockAllocator->QBlockInStore(apMem))
	{
		pSmallBlockAllocator->DeallocateAlign(apMem);
		return;
	}
	if (IMemoryHeap* pHeap = GetHeapForPointer(apMem))
	{
		pHeap->DeallocateAlign(apMem);
		return;
	}
	if (!IsZeroAddress(apMem))
		SystemDeallocate(apMem, abAlignmentRequired);
}

size_t MemoryManager::Size(const void* apMem) const
{
	if (pSmallBlockAllocator && pSmallBlockAllocator->QBlockInStore(apMem))
		return pSmallBlockAllocator->Size(apMem);
	if (InPinnedStore(pCompactingStore, apMem))
		return pCompactingStore->QSizePinned(apMem);
	if (IMemoryHeap* pHeap = GetHeapForPointer(apMem))
		return pHeap->Size(apMem);
	if (IMemoryHeap* pPhysicalHeap = GetHeapForPhysicalPointer(apMem))
		return pPhysicalHeap->Size(apMem);
	if (IsZeroAddress(apMem))
		return 0;
	return _msize(const_cast<void*>(apMem));
}

ScrapHeap* MemoryManager::GetThreadScrapHeap()
{
	if (!pThreadScrapHeapTLS)
	{
		AutoMemContext kContext(MC_CORE_SYSTEM);
		auto* pScrapHeap = reinterpret_cast<ThreadScrapHeap*>(aThreadScrapHeapBuffer);
		new (&pScrapHeap->Heap) ScrapHeap(ScrapHeap::QMaxMemory(), THREAD_SCRAP_HEAP_MIN_COMMIT);
		pScrapHeap->pNext = nullptr;
		pThreadScrapHeapTLS = pScrapHeap;

		ThreadScrapHeap* pHead = pThreadScrapHeap;
		ThreadScrapHeap* pSeen;
		do
		{
			pSeen = pHead;
			pThreadScrapHeapTLS->pNext = pHead;
			pHead = static_cast<ThreadScrapHeap*>(InterlockedCompareExchangePointer(
				reinterpret_cast<void* volatile*>(&pThreadScrapHeap), pThreadScrapHeapTLS, pHead));
		} while (pHead != pSeen);
	}
	return &pThreadScrapHeapTLS->Heap;
}

IMemoryHeap* MemoryManager::GetHeapForPointer(const void* apMem) const
{
	IMemoryHeap* pFound = nullptr;
	for (uint16_t i = 0; i < iNumHeaps && !pFound; ++i)
	{
		if (ppHeaps[i]->PointerInHeap(apMem))
			pFound = ppHeaps[i];
	}
	return pFound;
}

IMemoryHeap* MemoryManager::GetHeapForPhysicalPointer(const void* apMem) const
{
	IMemoryHeap* pFound = nullptr;
	for (uint16_t i = 0; i < iNumPhysicalHeaps && !pFound; ++i)
	{
		if (ppPhysicalHeaps[i]->PointerInHeap(apMem))
			pFound = ppPhysicalHeaps[i];
	}
	return pFound;
}

unsigned int MemoryManager::ProcessMemoryProblem(IMemoryHeap* apHeap, int aiMemoryPass, bool* apbAllowSystemAllocs)
{
	using MemoryManagement::PMPEvent;
	using MemoryManagement::PMPEventSource;

	const PMPEvent kStarted{ PMPEvent::Started };
	BSTSingletonImplicit<PMPEventSource>::QInstance()->Notify(kStarted);

	if (++uiThreadMemoryProblemDepth != 1)
		aiMemoryPass = static_cast<int>(uiThreadMemoryProblemPass);
	const unsigned int uiPass = ProcessMemoryProblemImpl(apHeap, aiMemoryPass, apbAllowSystemAllocs);
	--uiThreadMemoryProblemDepth;
	uiThreadMemoryProblemPass = uiPass;

	const PMPEvent kFinished{ PMPEvent::Finished };
	BSTSingletonImplicit<PMPEventSource>::QInstance()->Notify(kFinished);
	return uiThreadMemoryProblemPass;
}
