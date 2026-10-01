#pragma once

template<class T>
BSScrapArray<T>::BSScrapArray(unsigned int auiReserveSize) : BSTArray<T, BSScrapArrayAllocator>(auiReserveSize)
{}

template<class T>
BSScrapArray<T>::BSScrapArray(unsigned int auiReserveSize, unsigned int auiInitialSize) : BSTArray<T, BSScrapArrayAllocator>(auiReserveSize, auiInitialSize)
{}

template<class T>
template<class Allocator>
BSScrapArray<T>::BSScrapArray(const BSTArray<T, Allocator>& arCopy) : BSTArray<T, BSScrapArrayAllocator>(arCopy)
{}
