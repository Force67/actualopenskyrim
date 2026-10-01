#pragma once

#include <cstddef>

class ScrapHeap;

class BSScrapArrayAllocator
{
public:
	BSScrapArrayAllocator() : pScrapHeap(nullptr), pBuffer(nullptr), uiAllocSize(0) {}
	bool Allocate(unsigned int auiMinNewSize, unsigned int auiElemSize);
	bool Reallocate(unsigned int aMinNewSizeInItems, unsigned int aFrontCopyCount, unsigned int aShiftCount, unsigned int aBackCopyCount, unsigned int aElemSize);
	void Deallocate();
	void* QBuffer() const { return pBuffer; }
	unsigned int QAllocSize() const { return uiAllocSize; }

protected:
	~BSScrapArrayAllocator() = default;

public:
	ScrapHeap* pScrapHeap;
	void* pBuffer;
	unsigned int uiAllocSize;
	char cPadding[4];
};
static_assert(sizeof(BSScrapArrayAllocator) == 0x18);
static_assert(offsetof(BSScrapArrayAllocator, pBuffer) == 0x8);
static_assert(offsetof(BSScrapArrayAllocator, uiAllocSize) == 0x10);

#include "BSCore/BSTArray.h"

template<class T>
class BSScrapArray : public BSTArray<T, BSScrapArrayAllocator>
{
public:
	BSScrapArray() = default;
	explicit BSScrapArray(unsigned int auiReserveSize);
	BSScrapArray(unsigned int auiReserveSize, unsigned int auiInitialSize);
	template<class Allocator> explicit BSScrapArray(const BSTArray<T, Allocator>& arCopy);
};
static_assert(sizeof(BSScrapArray<void*>) == 0x20);
static_assert(offsetof(BSScrapArray<void*>, iSize) == 0x18);

#include "BSCore/BSScrapArray.inl"
