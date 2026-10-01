#include "BSCore/IMemoryStore.h"

IMemoryStoreBase::IMemoryStoreBase() = default;
IMemoryStoreBase::~IMemoryStoreBase() = default;

IMemoryStore::IMemoryStore() = default;
IMemoryStore::~IMemoryStore() = default;

void* IMemoryStore::TryAllocateImpl(size_t, uint32_t)
{
	return nullptr;
}
