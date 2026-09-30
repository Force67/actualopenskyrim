#pragma once

#include "BSCore/BSNonReentrantSpinLock.h"
#include "BSCore/IMemoryStore.h"

class IMemoryTracker;

namespace CompactingStore
{
	// Only the members the memory manager reads are declared so far.
	class Store : public IMemoryStoreBase
	{
	public:
		void DeallocatePinned(IMemoryTracker* apTracker, bool abTrack, void* apBlock);
		size_t QSizePinned(const void* apBlock) const;

		BSNonReentrantSpinLock Lock;
		void* pAllocBase;
		void* pAllocEndMin;
		void* pAllocEnd;
	};
}
