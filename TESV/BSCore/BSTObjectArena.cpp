#include "BSCore/BSTObjectArena.h"
#include "BSCore/CompactingStoreCommon.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/ScrapHeap.h"

void* BSTObjectArenaHeapAlloc::Allocate(size_t auiSize) const
{
	return MemoryManager::Instance().Allocate(auiSize, 0, false);
}

void BSTObjectArenaHeapAlloc::Deallocate(void* apBlock) const
{
	MemoryManager::Instance().Deallocate(apBlock, false);
}

BSTObjectArenaScrapAllocBase::BSTObjectArenaScrapAllocBase() : pScrapHeap(MemoryManager::Instance().GetThreadScrapHeap()) {}
BSTObjectArenaScrapAllocBase::BSTObjectArenaScrapAllocBase(ScrapHeap* apScrapHeap) : pScrapHeap(apScrapHeap) {}
BSTObjectArenaScrapAllocBase::~BSTObjectArenaScrapAllocBase() = default;

void* BSTObjectArenaScrapAlloc::Allocate(unsigned int auiSize) const
{
	return pScrapHeap->Allocate(auiSize, 8);
}

void BSTObjectArenaScrapAlloc::Deallocate(void* apBlock) const
{
	pScrapHeap->Deallocate(apBlock);
}

template class BSTObjectArena<CompactingStore::HandleType, BSTObjectArenaScrapAlloc, 32>;
