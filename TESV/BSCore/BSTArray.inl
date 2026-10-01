#pragma once

#include <new>
#include <utility>

template<class T, class Allocator>
BSTArray<T, Allocator>::BSTArray(unsigned int aiReserveSize)
{
	InitialReserve(aiReserveSize);
}

template<class T, class Allocator>
BSTArray<T, Allocator>::BSTArray(unsigned int aiReserveSize, unsigned int aiInitialSize)
{
	InitialSetup(aiReserveSize, aiInitialSize);
}

template<class T, class Allocator>
bool BSTArray<T, Allocator>::InitialSetup(unsigned int aiReserveSize, unsigned int aiInitialSize)
{
	iSize = 0;
	bool bResult = InitialReserve(aiReserveSize > aiInitialSize ? aiReserveSize : aiInitialSize);
	if (bResult && aiInitialSize)
	{
		ConstructDefaultItems(0, aiInitialSize);
		iSize = aiInitialSize;
	}
	return bResult;
}

template<class T, class Allocator>
BSTArray<T, Allocator>::~BSTArray()
{
	Clear(true);
}

template<class T, class Allocator>
bool BSTArray<T, Allocator>::InitialReserve(unsigned int aiReserveSize)
{
	BSTArrayAllocatorFunctor<Allocator> Functor(this);
	return BSTArrayBase::InitialReserve(Functor, aiReserveSize, sizeof(T));
}

template<class T, class Allocator>
bool BSTArray<T, Allocator>::SetAllocSize(unsigned int aiNewAllocSize)
{
	BSTArrayAllocatorFunctor<Allocator> Functor(this);
	return BSTArrayBase::SetAllocSize(Functor, QAllocSize(), aiNewAllocSize, sizeof(T));
}

template<class T, class Allocator>
unsigned int BSTArray<T, Allocator>::AddUninitialized()
{
	BSTArrayAllocatorFunctor<Allocator> Functor(this);
	return BSTArrayBase::AddUninitialized(Functor, QAllocSize(), sizeof(T));
}

template<class T, class Allocator>
bool BSTArray<T, Allocator>::InsertUninitialized(unsigned int aiIndex)
{
	BSTArrayAllocatorFunctor<Allocator> Functor(this);
	return BSTArrayBase::InsertUninitialized(Functor, QBuffer(), QAllocSize(), aiIndex, sizeof(T));
}

template<class T, class Allocator>
unsigned int BSTArray<T, Allocator>::Add(const T& arValue)
{
	unsigned int uiIndex = AddUninitialized();
	if (uiIndex != ~0u)
		new (QBuffer() + uiIndex) T(arValue);
	return uiIndex;
}

template<class T, class Allocator>
template<class... Args>
unsigned int BSTArray<T, Allocator>::Add(Args&&... arArgs)
{
	unsigned int uiIndex = AddUninitialized();
	if (uiIndex != ~0u)
		new (QBuffer() + uiIndex) T(std::forward<Args>(arArgs)...);
	return uiIndex;
}

template<class T, class Allocator>
unsigned int BSTArray<T, Allocator>::AddN(const T& arValue, unsigned int aiHowMany)
{
	unsigned int uiIndex = iSize;
	if (!SetAllocSize(uiIndex + aiHowMany))
		return ~0u;
	T* pItem = QBuffer() + iSize;
	for (unsigned int ui = 0; ui < aiHowMany; ++ui)
		new (pItem + ui) T(arValue);
	iSize += aiHowMany;
	return uiIndex;
}

template<class T, class Allocator>
bool BSTArray<T, Allocator>::Insert(unsigned int aiIndex, const T& arValue)
{
	if (!InsertUninitialized(aiIndex))
		return false;
	new (QBuffer() + aiIndex) T(arValue);
	return true;
}

template<class T, class Allocator>
void BSTArray<T, Allocator>::DestructItems(unsigned int aiIndex, unsigned int aiHowMany)
{
	T* pItem = QBuffer() + aiIndex;
	for (unsigned int ui = 0; ui < aiHowMany; ++ui)
		pItem[ui].~T();
}

template<class T, class Allocator>
void BSTArray<T, Allocator>::RemoveFast(unsigned int aiIndex, unsigned int aiHowMany)
{
	DestructItems(aiIndex, aiHowMany);
	unsigned int uiCopyCount = iSize - aiIndex - aiHowMany;
	if (aiHowMany < uiCopyCount)
		uiCopyCount = aiHowMany;
	BSTArrayBase::MoveItems(QBuffer(), aiIndex, iSize - uiCopyCount, uiCopyCount, sizeof(T));
	iSize -= aiHowMany;
}

template<class T, class Allocator>
void BSTArray<T, Allocator>::Clear(bool abFreeMemory)
{
	if (QBuffer())
	{
		DestructItems(0, iSize);
		if (abFreeMemory)
			Allocator::Deallocate();
		iSize = 0;
	}
}

template<class T, class Allocator>
void BSTArray<T, Allocator>::ConstructDefaultItems(unsigned int aiIndex, unsigned int aiHowMany)
{
	T* pItem = QBuffer() + aiIndex;
	for (unsigned int ui = 0; ui < aiHowMany; ++ui)
		new (pItem + ui) T;
}

template<class T, class Allocator>
bool BSTArray<T, Allocator>::SetSize(unsigned int aiNewSize)
{
	bool bResult = true;
	if (!aiNewSize)
		Clear(false);
	else if (aiNewSize != iSize)
	{
		if (aiNewSize <= QAllocSize())
		{
			if (aiNewSize < iSize)
				DestructItems(aiNewSize, iSize - aiNewSize);
			else
				ConstructDefaultItems(iSize, aiNewSize - iSize);
		}
		else
		{
			bResult = QAllocSize() ? Allocator::Reallocate(aiNewSize, iSize, 0, 0, sizeof(T)) : Allocator::Allocate(aiNewSize, sizeof(T));
			if (bResult)
				ConstructDefaultItems(iSize, aiNewSize - iSize);
		}
	}
	if (bResult)
		iSize = aiNewSize;
	return bResult;
}

template<class T, class Allocator>
BSTArray<T, Allocator>::BSTArray(const BSTArray& arCopy) : BSTArray(arCopy.QSize())
{
	for (auto Iterator = arCopy.Begin(), EndIterator = arCopy.End(); Iterator != EndIterator; ++Iterator)
		Add(*Iterator);
}

template<class T, class Allocator>
template<class OtherAllocator>
BSTArray<T, Allocator>::BSTArray(const BSTArray<T, OtherAllocator>& arCopy) : BSTArray(arCopy.QSize())
{
	for (auto Iterator = arCopy.Begin(), EndIterator = arCopy.End(); Iterator != EndIterator; ++Iterator)
		Add(*Iterator);
}

template<class T, class Allocator>
void BSTArray<T, Allocator>::MoveItems(unsigned int aiTo, unsigned int aiFrom, unsigned int aiHowMany)
{
	BSTArrayBase::MoveItems(QBuffer(), aiTo, aiFrom, aiHowMany, sizeof(T));
}

template<class T, class Allocator>
void BSTArray<T, Allocator>::Remove(unsigned int aiIndex, unsigned int aiHowMany)
{
	if (aiHowMany == iSize)
		Clear(false);
	else
	{
		DestructItems(aiIndex, aiHowMany);
		MoveItems(aiIndex, aiIndex + aiHowMany, iSize - aiIndex - aiHowMany);
		iSize -= aiHowMany;
	}
}

template<class T, class Allocator>
void BSTArray<T, Allocator>::RemoveLast()
{
	DestructItems(iSize - 1, 1);
	--iSize;
}

template<class T, class Allocator>
BSTArrayIterator<T> BSTArray<T, Allocator>::RemoveAt(BSTArrayIterator<T> aWhere)
{
	unsigned int uiIndex = static_cast<unsigned int>(aWhere.pCurrent - QBuffer());
	Remove(uiIndex);
	return iSize ? BSTArrayIterator<T>(QBuffer() + uiIndex) : End();
}

template<class T, class Allocator>
void BSTArray<T, Allocator>::Swap(unsigned int aiIndex1, unsigned int aiIndex2)
{
	BSTArrayInternal::SwapItems<T, sizeof(T)>::Swap(QBuffer() + aiIndex1, QBuffer() + aiIndex2);
}

template<class T, class Allocator>
template<class Functor>
BSContainer::ForEachResult BSTArray<T, Allocator>::ForEach(Functor& arFunctor)
{
	for (auto Iterator = Begin(), EndIterator = End(); Iterator != EndIterator; ++Iterator)
	{
		auto Result = arFunctor(*Iterator);
		if (Result != BSContainer::kContinue)
			return Result;
	}
	return BSContainer::kContinue;
}

template<class T, class Allocator>
template<class Functor>
BSContainer::ForEachResult BSTArray<T, Allocator>::ForEach(Functor& arFunctor) const
{
	for (auto Iterator = Begin(), EndIterator = End(); Iterator != EndIterator; ++Iterator)
	{
		auto Result = arFunctor(*Iterator);
		if (Result != BSContainer::kContinue)
			return Result;
	}
	return BSContainer::kContinue;
}
