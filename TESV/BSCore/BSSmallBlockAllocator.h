#pragma once

#include "BSCore/IMemoryStore.h"

// Pooled allocator for allocations of up to 0x200 bytes.
class BSSmallBlockAllocator : public IMemoryStore
{
public:
	bool QBlockInStore(const void* apBlock) const;
};
