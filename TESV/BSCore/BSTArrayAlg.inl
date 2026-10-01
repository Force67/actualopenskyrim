#pragma once

template<class T, class Functor, class Allocator, class Value>
bool RemoveFirstEqual(BSTArray<T, Allocator>& arArray, const Value& arValue, const Functor& arFunctor)
{
	unsigned int uiIndex = Find(arArray, arValue, 0, arFunctor);
	if (uiIndex == ~0u)
		return false;
	arArray.Remove(uiIndex);
	return true;
}

template<class T, class Allocator, class Value>
bool RemoveFirstEqual(BSTArray<T, Allocator>& arArray, const Value& arValue)
{
	return RemoveFirstEqual(arArray, arValue, ArrayEqualFunctor<T, Value>{});
}

template<class T, class Functor, class Allocator, class OtherAllocator>
bool CompareArrays(const BSTArray<T, Allocator>& arArray, const BSTArray<T, OtherAllocator>& arOtherArray, const Functor& arFunctor)
{
	unsigned int uiSize = arArray.QSize();
	if (arOtherArray.QSize() != uiSize)
		return false;
	for (unsigned int ui = 0; ui < uiSize; ++ui)
		if (!arFunctor(arArray[ui], arOtherArray[ui]))
			return false;
	return true;
}

template<class T, class Allocator, class OtherAllocator>
bool CompareArrays(const BSTArray<T, Allocator>& arArray, const BSTArray<T, OtherAllocator>& arOtherArray)
{
	return CompareArrays(arArray, arOtherArray, ArrayEqualFunctor<T, T>{});
}

template<class T, class Functor, class Allocator, class Value>
bool IsInArray(const BSTArray<T, Allocator>& arArray, const Value& arValue, const Functor& arFunctor)
{
	return Find(arArray, arValue, 0, arFunctor) != ~0u;
}

template<class T, class Allocator, class Value>
bool IsInArray(const BSTArray<T, Allocator>& arArray, const Value& arValue)
{
	return IsInArray(arArray, arValue, ArrayEqualFunctor<T, Value>{});
}

template<class T, class Functor, class Allocator>
bool IsInSortedArray(const BSTArray<T, Allocator>& arArray, const T& arValue, const Functor& arFunctor)
{
	return SortedFind(arArray, arValue, arFunctor) != ~0u;
}

template<class T, class Functor, class Allocator, class Value>
unsigned int FindFirstGreaterEqualSorted(const BSTArray<T, Allocator>& arArray, const Value& arValue, const Functor& arFunctor, bool& arbFoundExact)
{
	arbFoundExact = false;
	unsigned int uiSize = arArray.QSize();
	unsigned int uiLow = 0;
	if (!uiSize)
		return uiLow;
	int iHigh = static_cast<int>(uiSize - 1);
	while (static_cast<int>(uiLow) <= iHigh)
	{
		unsigned int uiMiddle = uiLow + (static_cast<int>(static_cast<unsigned int>(iHigh) - uiLow) >> 1);
		int iResult = arFunctor(arValue, arArray[uiMiddle]);
		if (iResult > 0)
			uiLow = uiMiddle + 1;
		else if (iResult < 0)
			iHigh = static_cast<int>(uiMiddle - 1);
		else
		{
			arbFoundExact = true;
			return uiMiddle;
		}
	}
	return uiLow;
}

template<class T, class Functor, class Allocator, class Value>
unsigned int FindFirstGreaterEqualSorted(const BSTArray<T, Allocator>& arArray, const Value& arValue, const Functor& arFunctor)
{
	bool bFoundExact;
	return FindFirstGreaterEqualSorted(arArray, arValue, arFunctor, bFoundExact);
}

template<class T, class Functor, class Allocator, class Value>
unsigned int SortedFind(const BSTArray<T, Allocator>& arArray, const Value& arValue, const Functor& arFunctor)
{
	bool bFoundExact;
	unsigned int uiIndex = FindFirstGreaterEqualSorted(arArray, arValue, arFunctor, bFoundExact);
	return bFoundExact ? uiIndex : ~0u;
}

template<class T, class Functor, class Allocator, class Value>
unsigned int FindFirstGreaterThanSorted(const BSTArray<T, Allocator>& arArray, const Value& arValue, const Functor& arFunctor)
{
	unsigned int uiSize = arArray.QSize();
	if (!uiSize)
		return ~0u;
	unsigned int uiLow = 0;
	unsigned int uiHigh = uiSize - 1;
	while (uiLow != uiHigh)
	{
		unsigned int uiMiddle = uiLow + ((uiHigh - uiLow) >> 1);
		if (arFunctor(arValue, arArray[uiMiddle]) >= 0)
			uiLow = uiMiddle + 1;
		else
			uiHigh = uiMiddle > uiLow ? uiMiddle - 1 : uiLow;
	}
	if (arFunctor(arValue, arArray[uiLow]) < 0)
		return uiLow;
	return uiLow == arArray.QSize() - 1 ? ~0u : uiLow + 1;
}

template<class T, class Functor, class Allocator, class Value>
unsigned int Find(const BSTArray<T, Allocator>& arArray, const Value& arValue, unsigned int aiStartIndex, const Functor& arFunctor)
{
	unsigned int uiSize = arArray.QSize();
	for (unsigned int ui = aiStartIndex; ui < uiSize; ++ui)
		if (arFunctor(arArray[ui], arValue))
			return ui;
	return ~0u;
}

template<class T, class Allocator, class Value>
unsigned int Find(const BSTArray<T, Allocator>& arArray, const Value& arValue)
{
	return Find(arArray, arValue, 0, ArrayEqualFunctor<T, Value>{});
}

template<class T, class Allocator>
void Reverse(BSTArray<T, Allocator>& arArray)
{
	unsigned int uiSize = arArray.QSize();
	if (uiSize > 1)
	{
		unsigned int uiFirst = 0;
		unsigned int uiLast = uiSize - 1;
		while (uiFirst < uiLast)
			arArray.Swap(uiFirst++, uiLast--);
	}
}

template<class T, class Functor, class Allocator, class Value>
unsigned int ReverseFind(const BSTArray<T, Allocator>& arArray, const Value& arValue, const Functor& arFunctor)
{
	unsigned int uiSize = arArray.QSize();
	while (uiSize)
	{
		unsigned int uiIndex = --uiSize;
		if (arFunctor(arArray[uiIndex], arValue))
			return uiIndex;
	}
	return ~0u;
}

template<class T, class Functor, class Allocator>
unsigned int ArrayPartitionRecursive(BSTArray<T, Allocator>& arArray, const Functor& arFunctor, unsigned int aiLowIndex, unsigned int aiHighIndex)
{
	unsigned int uiPivot = aiLowIndex;
	const T& Pivot = arArray[uiPivot];
	while (aiLowIndex < aiHighIndex)
	{
		while (aiLowIndex < aiHighIndex && arFunctor(Pivot, arArray[aiHighIndex]) < 0)
			--aiHighIndex;
		while (aiLowIndex < aiHighIndex && arFunctor(Pivot, arArray[aiLowIndex]) >= 0)
			++aiLowIndex;
		if (aiLowIndex < aiHighIndex)
			arArray.Swap(aiLowIndex, aiHighIndex);
	}
	arArray.Swap(uiPivot, aiHighIndex);
	return aiHighIndex;
}

template<class T, class Functor, class Allocator>
void ArrayQuickSortRecursive(BSTArray<T, Allocator>& arArray, const Functor& arFunctor, unsigned int aiLowIndex, unsigned int aiHighIndex)
{
	while (aiLowIndex < aiHighIndex)
	{
		unsigned int uiPivot = ArrayPartitionRecursive(arArray, arFunctor, aiLowIndex, aiHighIndex);
		if (uiPivot)
			ArrayQuickSortRecursive(arArray, arFunctor, aiLowIndex, uiPivot - 1);
		if (uiPivot >= arArray.QSize() - 1)
			break;
		aiLowIndex = uiPivot + 1;
	}
}

template<class T, class Functor, class Allocator>
void QuickSort(BSTArray<T, Allocator>& arArray, const Functor& arFunctor)
{
	unsigned int uiSize = arArray.QSize();
	if (uiSize > 1)
		ArrayQuickSortRecursive(arArray, arFunctor, 0, uiSize - 1);
}
