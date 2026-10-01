#pragma once

#include "BSCore/BSTArray.h"

template<class T, class Value>
struct ArrayEqualFunctor
{
	bool operator()(const T& arOne, const Value& arTwo) const { return arOne == arTwo; }
};

template<class T, class Functor, class Allocator, class Value>
unsigned int Find(const BSTArray<T, Allocator>& arArray, const Value& arValue, unsigned int aiStartIndex, const Functor& arFunctor);

template<class T, class Allocator, class Value>
unsigned int Find(const BSTArray<T, Allocator>& arArray, const Value& arValue);

template<class T, class Functor, class Allocator, class Value>
unsigned int ReverseFind(const BSTArray<T, Allocator>& arArray, const Value& arValue, const Functor& arFunctor);

template<class T, class Functor, class Allocator, class OtherAllocator>
bool CompareArrays(const BSTArray<T, Allocator>& arArray, const BSTArray<T, OtherAllocator>& arOtherArray, const Functor& arFunctor);

template<class T, class Allocator, class OtherAllocator>
bool CompareArrays(const BSTArray<T, Allocator>& arArray, const BSTArray<T, OtherAllocator>& arOtherArray);

template<class T, class Functor, class Allocator, class Value>
bool IsInArray(const BSTArray<T, Allocator>& arArray, const Value& arValue, const Functor& arFunctor);

template<class T, class Allocator, class Value>
bool IsInArray(const BSTArray<T, Allocator>& arArray, const Value& arValue);

template<class T, class Functor, class Allocator>
bool IsInSortedArray(const BSTArray<T, Allocator>& arArray, const T& arValue, const Functor& arFunctor);

template<class T, class Functor, class Allocator, class Value>
bool RemoveFirstEqual(BSTArray<T, Allocator>& arArray, const Value& arValue, const Functor& arFunctor);

template<class T, class Allocator, class Value>
bool RemoveFirstEqual(BSTArray<T, Allocator>& arArray, const Value& arValue);

template<class T, class Allocator>
void Reverse(BSTArray<T, Allocator>& arArray);

template<class T, class Functor, class Allocator, class Value>
unsigned int FindFirstGreaterEqualSorted(const BSTArray<T, Allocator>& arArray, const Value& arValue, const Functor& arFunctor, bool& arbFoundExact);

template<class T, class Functor, class Allocator, class Value>
unsigned int FindFirstGreaterEqualSorted(const BSTArray<T, Allocator>& arArray, const Value& arValue, const Functor& arFunctor);

template<class T, class Functor, class Allocator, class Value>
unsigned int SortedFind(const BSTArray<T, Allocator>& arArray, const Value& arValue, const Functor& arFunctor);

template<class T, class Functor, class Allocator, class Value>
unsigned int FindFirstGreaterThanSorted(const BSTArray<T, Allocator>& arArray, const Value& arValue, const Functor& arFunctor);

template<class T, class Functor, class Allocator>
unsigned int ArrayPartitionRecursive(BSTArray<T, Allocator>& arArray, const Functor& arFunctor, unsigned int aiLowIndex, unsigned int aiHighIndex);

template<class T, class Functor, class Allocator>
void ArrayQuickSortRecursive(BSTArray<T, Allocator>& arArray, const Functor& arFunctor, unsigned int aiLowIndex, unsigned int aiHighIndex);

template<class T, class Functor, class Allocator>
void QuickSort(BSTArray<T, Allocator>& arArray, const Functor& arFunctor);

#include "BSCore/BSTArrayAlg.inl"
