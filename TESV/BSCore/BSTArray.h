#pragma once

#include "BSCore/BSCoreTypes.h"

#include <cstddef>

namespace BSTArrayInternal
{
	void SwapElements(void* apBuffer1, void* apBuffer2, void* apBufferTemp, unsigned int aiElemSize);

	template<class T, unsigned int Size>
	struct SwapItems
	{
		static void Swap(T* apItem1, T* apItem2)
		{
			char cBuffer[Size];
			SwapElements(apItem1, apItem2, cBuffer, Size);
		}
	};
}

class BSTArrayBase
{
public:
	struct IAllocatorFunctor
	{
		virtual bool Allocate(unsigned int auiSize, unsigned int auiElemSize) const = 0;
		virtual bool Reallocate(unsigned int auiSize, unsigned int auiFrontCopyCount, unsigned int auiShiftCount, unsigned int auiBackCopyCount, unsigned int auiElemSize) const = 0;
		virtual void Deallocate() const = 0;
		virtual ~IAllocatorFunctor() = default;
	};

	BSTArrayBase();
	void MoveItems(void* apBuffer, unsigned int auiTo, unsigned int auiFrom, unsigned int auiCount, unsigned int auiElemSize);
	bool InitialReserve(const IAllocatorFunctor& arFunctor, unsigned int auiReserveSize, unsigned int auiElemSize);
	unsigned int AddUninitialized(const IAllocatorFunctor& arFunctor, unsigned int auiAllocSize, unsigned int auiElemSize);
	bool InsertUninitialized(const IAllocatorFunctor& arFunctor, void* apBuffer, unsigned int auiAllocSize, unsigned int auiIndex, unsigned int auiElemSize);
	bool SetAllocSize(const IAllocatorFunctor& arFunctor, unsigned int auiAllocSize, unsigned int auiNewAllocSize, unsigned int auiElemSize);

protected:
	~BSTArrayBase();

public:
	unsigned int iSize;
	unsigned int QSize() const { return iSize; }
	bool QEmpty() const { return iSize == 0; }
};
static_assert(sizeof(BSTArrayBase) == 4);
static_assert(sizeof(BSTArrayBase::IAllocatorFunctor) == 8);

class BSTArrayHeapAllocator
{
public:
	BSTArrayHeapAllocator();
	bool Allocate(unsigned int aiMinNewSize, unsigned int aiElemSize);
	bool Reallocate(unsigned int aiMinNewSizeInItems, unsigned int aiFrontCopyCount, unsigned int aiShiftCount, unsigned int aiBackCopyCount, unsigned int aiElemSize);
	void Deallocate();
	void* QBuffer() const { return pBuffer; }
	unsigned int QAllocSize() const { return iAllocSize; }

protected:
	~BSTArrayHeapAllocator();

public:
	void* pBuffer;
	unsigned int iAllocSize;
	char cPadding[4];
};
static_assert(sizeof(BSTArrayHeapAllocator) == 0x10);
static_assert(offsetof(BSTArrayHeapAllocator, iAllocSize) == 0x8);

template<class Allocator>
class BSTArrayAllocatorFunctor : public BSTArrayBase::IAllocatorFunctor
{
public:
	explicit BSTArrayAllocatorFunctor(Allocator* apAllocator) : pAllocator(apAllocator) {}
	bool Allocate(unsigned int auiSize, unsigned int auiElemSize) const override
	{
		return pAllocator->Allocate(auiSize, auiElemSize);
	}
	bool Reallocate(unsigned int auiSize, unsigned int auiFrontCopyCount, unsigned int auiShiftCount, unsigned int auiBackCopyCount, unsigned int auiElemSize) const override
	{
		return pAllocator->Reallocate(auiSize, auiFrontCopyCount, auiShiftCount, auiBackCopyCount, auiElemSize);
	}
	void Deallocate() const override { pAllocator->Deallocate(); }
	~BSTArrayAllocatorFunctor() override = default;

	Allocator* pAllocator;
};
static_assert(sizeof(BSTArrayAllocatorFunctor<BSTArrayHeapAllocator>) == 0x10);

template<class T>
class BSTArrayConstIterator
{
public:
	explicit BSTArrayConstIterator(const T* apCurrent = nullptr) : pCurrent(apCurrent) {}
	BSTArrayConstIterator(const BSTArrayConstIterator& arOther) : pCurrent(arOther.pCurrent) {}
	~BSTArrayConstIterator() {}
	const T& operator*() const { return *pCurrent; }
	const T* operator->() const { return pCurrent; }
	BSTArrayConstIterator& operator++() { ++pCurrent; return *this; }
	BSTArrayConstIterator& operator--() { --pCurrent; return *this; }
	bool operator==(const BSTArrayConstIterator& arOther) const { return pCurrent == arOther.pCurrent; }
	bool operator!=(const BSTArrayConstIterator& arOther) const { return pCurrent != arOther.pCurrent; }

	const T* pCurrent;
};

template<class T>
class BSTArrayIterator : public BSTArrayConstIterator<T>
{
public:
	explicit BSTArrayIterator(T* apCurrent = nullptr) : BSTArrayConstIterator<T>(apCurrent) {}
	T& operator*() const { return *const_cast<T*>(this->pCurrent); }
	T* operator->() const { return const_cast<T*>(this->pCurrent); }
	BSTArrayIterator& operator++() { ++this->pCurrent; return *this; }
	BSTArrayIterator& operator--() { --this->pCurrent; return *this; }
};
static_assert(sizeof(BSTArrayIterator<void*>) == 8);
static_assert(sizeof(BSTArrayConstIterator<void*>) == 8);

template<class T, class Allocator = BSTArrayHeapAllocator>
class BSTArray : public Allocator, public BSTArrayBase
{
public:
	BSTArray() = default;
	explicit BSTArray(unsigned int aiReserveSize);
	BSTArray(unsigned int aiReserveSize, unsigned int aiInitialSize);
	BSTArray(const BSTArray& arCopy);
	template<class OtherAllocator> explicit BSTArray(const BSTArray<T, OtherAllocator>& arCopy);
	~BSTArray();

	T* QBuffer() { return static_cast<T*>(Allocator::QBuffer()); }
	const T* QBuffer() const { return static_cast<const T*>(Allocator::QBuffer()); }
	unsigned int QAllocSize() const { return Allocator::QAllocSize(); }
	T& operator[](unsigned int aiIndex) { return QBuffer()[aiIndex]; }
	const T& operator[](unsigned int aiIndex) const { return QBuffer()[aiIndex]; }
	T& GetAt(unsigned int aiIndex) { return (*this)[aiIndex]; }
	const T& GetAt(unsigned int aiIndex) const { return (*this)[aiIndex]; }
	void SetAt(unsigned int aiIndex, const T& arValue) { (*this)[aiIndex] = arValue; }
	T& GetFirst() { return GetAt(0); }
	const T& GetFirst() const { return GetAt(0); }
	T& GetLast() { return GetAt(iSize - 1); }
	const T& GetLast() const { return GetAt(iSize - 1); }
	void SetFirst(const T& arValue) { SetAt(0, arValue); }
	void SetLast(const T& arValue) { SetAt(iSize - 1, arValue); }
	BSTArrayIterator<T> Begin() { return BSTArrayIterator<T>(iSize ? QBuffer() : nullptr); }
	BSTArrayConstIterator<T> Begin() const { return BSTArrayConstIterator<T>(iSize ? QBuffer() : nullptr); }
	BSTArrayIterator<T> End() { return BSTArrayIterator<T>(iSize ? QBuffer() + iSize : nullptr); }
	BSTArrayConstIterator<T> End() const { return BSTArrayConstIterator<T>(iSize ? QBuffer() + iSize : nullptr); }
	BSTArrayIterator<T> RBegin() { return BSTArrayIterator<T>(iSize ? QBuffer() + (iSize - 1) : nullptr); }
	BSTArrayConstIterator<T> RBegin() const { return BSTArrayConstIterator<T>(iSize ? QBuffer() + (iSize - 1) : nullptr); }
	BSTArrayIterator<T> REnd() { return BSTArrayIterator<T>(iSize ? QBuffer() - 1 : nullptr); }
	BSTArrayConstIterator<T> REnd() const { return BSTArrayConstIterator<T>(iSize ? QBuffer() - 1 : nullptr); }
	unsigned int Add(const T& arValue);
	unsigned int AddN(const T& arValue, unsigned int aiHowMany);
	template<class... Args> unsigned int Add(Args&&... arArgs);
	bool Insert(unsigned int aiIndex, const T& arValue);
	void RemoveFast(unsigned int aiIndex, unsigned int aiHowMany = 1);
	void Remove(unsigned int aiIndex, unsigned int aiHowMany = 1);
	void RemoveLast();
	BSTArrayIterator<T> RemoveAt(BSTArrayIterator<T> aWhere);
	void Swap(unsigned int aiIndex1, unsigned int aiIndex2);
	template<class Functor> BSContainer::ForEachResult ForEach(Functor& arFunctor);
	template<class Functor> BSContainer::ForEachResult ForEach(Functor& arFunctor) const;
	void Clear(bool abFreeMemory = true);
	bool SetAllocSize(unsigned int aiNewAllocSize);
	bool SetSize(unsigned int aiNewSize);

protected:
	unsigned int AddUninitialized();
	bool InsertUninitialized(unsigned int aiIndex);
	bool InitialReserve(unsigned int aiReserveSize);
	bool InitialSetup(unsigned int aiReserveSize, unsigned int aiInitialSize);
	void MoveItems(unsigned int aiTo, unsigned int aiFrom, unsigned int aiHowMany);
	void DestructItems(unsigned int aiIndex, unsigned int aiHowMany);
	void ConstructDefaultItems(unsigned int aiIndex, unsigned int aiHowMany);
};
static_assert(sizeof(BSTArray<void*>) == 0x18);
static_assert(offsetof(BSTArray<void*>, iSize) == 0x10);

#include "BSCore/BSTArray.inl"
