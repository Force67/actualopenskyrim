#include "BSCore/IMemoryHeap.h"

IMemoryHeap::~IMemoryHeap() = default;

bool IMemoryHeap::ContainsBlockImpl(const void* apBlock) const
{
	return PointerInHeap(apBlock);
}

// The heap picks its own alignment; the requested one is not passed on.
void* IMemoryHeap::AllocateAlignImpl(size_t auiSize, uint32_t)
{
	return Allocate(auiSize, 0);
}

void IMemoryHeap::DeallocateAlignImpl(void*& arpBlock)
{
	Deallocate(arpBlock, 0);
}
