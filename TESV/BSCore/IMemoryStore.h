#pragma once

#include <cstddef>
#include <cstdint>

struct MemoryStats
{
	const char* pName;
	size_t uiUsedSize;
	size_t uiCommittedSize;
	size_t uiReservedSize;
	uint32_t uiOverhead;
	size_t uiFreeSize;
};
static_assert(sizeof(MemoryStats) == 0x30);

class IMemoryStoreBase
{
public:
	virtual ~IMemoryStoreBase();

	virtual size_t Size(const void* apBlock) const = 0;
	virtual void GetMemoryStats(MemoryStats* apStats) = 0;

private:
	virtual bool ContainsBlockImpl(const void* apBlock) const = 0;

public:
	bool ContainsBlock(const void* apBlock) const { return ContainsBlockImpl(apBlock); }
};

class IMemoryStore : public IMemoryStoreBase
{
public:
	~IMemoryStore() override;

	void* AllocateAlign(size_t auiSize, uint32_t auiAlignment) { return AllocateAlignImpl(auiSize, auiAlignment); }
	void DeallocateAlign(void*& arpBlock) { DeallocateAlignImpl(arpBlock); }
	void* TryAllocate(size_t auiSize, uint32_t auiAlignment) { return TryAllocateImpl(auiSize, auiAlignment); }

private:
	virtual void* AllocateAlignImpl(size_t auiSize, uint32_t auiAlignment) = 0;
	virtual void DeallocateAlignImpl(void*& arpBlock) = 0;
	virtual void* TryAllocateImpl(size_t auiSize, uint32_t auiAlignment);
};
